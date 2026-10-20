// SPDX-License-Identifier: MIT
#include "MeshExport.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <gp_Pln.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        if(argc!=2)throw std::runtime_error("Expected output directory");
        QDir output(QString::fromLocal8Bit(argv[1]));
        EvaluatedBody first;first.bodyId="first";first.name="Part / CON";first.shape=BRepPrimAPI_MakeBox(gp_Pnt(-5,2,3),20,10,15).Shape();
        EvaluatedBody second;second.bodyId="second";second.name="part : con";second.shape=BRepPrimAPI_MakeBox(gp_Pnt(40,-5,7),12,12,12).Shape();
        const std::vector<EvaluatedBody> bodies{first,second};
        auto binary=MeshExport::writeStl(bodies,output.filePath("bodies-mm.stl"),"mm",true,0.1,10);
        if(binary["bodyCount"]!=2 || binary["triangles"]!=24)throw std::runtime_error("Expected both complete box meshes");
        MeshExport::writeStl(bodies,output.filePath("bodies-in.stl"),"in",false,0.1,10);
        const auto single=MeshExport::writeStl(first,output.filePath("single.stl"),"mm",true,0.1,10);
        if(single["bodyId"]!="first")throw std::runtime_error("Legacy bodyId response changed");
        const auto protectedPath=output.filePath("preserved.stl");QFile sentinel(protectedPath);
        if(!sentinel.open(QIODevice::WriteOnly) || sentinel.write("preserve")!=8)throw std::runtime_error("Sentinel creation failed");sentinel.close();
        EvaluatedBody invalid;invalid.bodyId="invalid";invalid.shape=BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(0,0,0),gp_Dir(0,0,1)),-2,2,-2,2).Face();
        bool refused=false;try {MeshExport::writeStl(std::vector<EvaluatedBody>{first,invalid},protectedPath,"mm",true,0.1,10);}
        catch(const GeometryKernel::EvaluationError& error){refused=error.diagnostics()["code"]=="solid_required";}
        if(!refused || !sentinel.open(QIODevice::ReadOnly) || sentinel.readAll()!="preserve")throw std::runtime_error("Invalid second body must preserve destination");
        sentinel.close();
        const auto filesBefore=output.entryList(QDir::Files|QDir::Hidden,QDir::Name);
        for(const bool throws:{false,true}){
            bool cancelled=false;
            try {MeshExport::writeStl(first,protectedPath,"mm",true,.1,10,[&]()->bool {
                if(throws)throw std::runtime_error("publication callback error");return false;
            });}
            catch(const GeometryKernel::EvaluationError& error){cancelled=error.diagnostics().value("code")=="export_cancelled";}
            catch(const std::runtime_error& error){cancelled=QString::fromUtf8(error.what())=="publication callback error";}
            if(!cancelled || !sentinel.open(QIODevice::ReadOnly) || sentinel.readAll()!="preserve")throw std::runtime_error("Denied/throwing publication changed destination");sentinel.close();
            if(output.entryList(QDir::Files|QDir::Hidden,QDir::Name)!=filesBefore)throw std::runtime_error("Publication failure leaked staging files");
        }
        gp_Trsf placed;placed.SetTranslation(gp_Vec(40,-5,7));
        const std::vector<gp_Trsf> placements{gp_Trsf{},placed};QJsonObject report;
        for(const auto& format:QStringList{"binary","ascii","3mf"}){
            const auto target=output.filePath("batch-"+format);
            const auto plan=MeshExport::batchPlan({second,first},target,format);
            if(plan!=MeshExport::batchPlan(bodies,target,format))throw std::runtime_error("Names depend on selection ordering");
            report[format]=MeshExport::writeBatch(bodies,placements,target,format,"mm",.1,10,[]{return true;});
            const auto files=report.value(format).toObject().value("files").toArray();
            if(files.size()!=2 || files.first().toObject().value("bodyId")!="first")throw std::runtime_error("Ordered batch mapping incorrect");
            bool refusedExisting=false;try{MeshExport::writeBatch(bodies,placements,target,format,"mm",.1,10,[]{return true;});}
            catch(const GeometryKernel::EvaluationError& e){refusedExisting=e.diagnostics().value("code")=="destination_exists";}
            if(!refusedExisting)throw std::runtime_error("Existing folder was not refused");
        }
        const auto entries=output.entryList(QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot,QDir::Name);
        for(int failure=0;failure<4;++failure){
            const auto target=output.filePath("failed-batch");bool refusedBatch=false;
            invalid.bodyId="zzz-invalid";
            try{MeshExport::writeBatch(failure==0 ? std::vector<EvaluatedBody>{first,invalid} : bodies,placements,target,"binary","mm",.1,10,
                [&]{if(failure==2)QDir().mkdir(target);return failure!=1;},[&]{return failure==3;});}
            catch(const GeometryKernel::EvaluationError&){refusedBatch=true;}
            if(!refusedBatch)throw std::runtime_error("Failed batch reported success");
            if(failure==2){if(!QDir(target).isEmpty())throw std::runtime_error("Racing destination was modified");QDir().rmdir(target);}
            if(QFileInfo::exists(target) || output.entryList(QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot,QDir::Name)!=entries)
                throw std::runtime_error("Failure leaked final or private staging output");
        }
        for(const auto& target:QStringList{protectedPath,output.filePath("dangling")}){
            if(target.endsWith("dangling") && !QFile::link(output.filePath("missing-target"),target))throw std::runtime_error("Cannot create symlink fixture");
            bool refusedExisting=false;try{MeshExport::writeBatch(bodies,placements,target,"binary","mm",.1,10,[]{return true;});}
            catch(const GeometryKernel::EvaluationError& e){refusedExisting=e.diagnostics().value("code")=="destination_exists";}
            if(!refusedExisting)throw std::runtime_error("Existing file/symlink was not refused");
        }
        QFile metadata(output.filePath("batch.json"));if(!metadata.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot write batch metadata");metadata.write(QJsonDocument(report).toJson());
        std::cout<<"Two placed body exports and atomic invalid-body refusal passed\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
