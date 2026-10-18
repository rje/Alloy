// SPDX-License-Identifier: MIT
#include "MeshExport.h"
#include <lib3mf_implicit.hpp>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Pln.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>
#include <numbers>

void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir temporary;check(temporary.isValid(),"temporary directory");
        const QDir output(argc>1?QString::fromLocal8Bit(argv[1]):temporary.path());check(output.exists(),"output directory");
        EvaluatedBody first;first.bodyId="box";first.name="Named box";first.shape=BRepPrimAPI_MakeBox(10,20,30).Shape();
        gp_Trsf placement;placement.SetRotation(gp_Ax1(gp_Pnt(),gp_Dir(0,0,1)),std::numbers::pi/2);placement.SetTranslationPart(gp_Vec(40,-10,7));
        EvaluatedBody second=first;second.bodyId="rotated";second.shape=BRepBuilderAPI_Transform(first.shape,placement,true).Shape();
        const std::vector<EvaluatedBody> bodies{first,second};const std::vector<gp_Trsf> placements{gp_Trsf(),placement};
        const auto firstBefore=GeometryKernel::summarize(first.shape),secondBefore=GeometryKernel::summarize(second.shape);
        const auto file=output.filePath("two-objects.3mf");
        const auto summary=MeshExport::write3mf(bodies,placements,file,.1,10);
        check(summary.value("triangles").toInt()==24 && summary.value("objectCount").toInt()==2,"two complete box objects");
        const auto wrapper=Lib3MF::CWrapper::loadLibrary();const auto model=wrapper->CreateModel();const auto reader=model->QueryReader("3mf");reader->ReadFromFile(file.toStdString());
        check(reader->GetWarningCount()==0,"3MF read warnings");check(model->GetUnit()==Lib3MF::eModelUnit::MilliMeter,"embedded millimeters");
        const auto meshes=model->GetMeshObjects();check(meshes->Count()==2,"separate mesh resource count");
        while(meshes->MoveNext()) {
            const auto mesh=meshes->GetCurrentMeshObject();check(mesh->IsManifoldAndOriented(),"manifold readback");check(mesh->GetTriangleCount()==12,"box readback triangles");
            for(quint32 i=0;i<mesh->GetVertexCount();++i) {
                const auto point=mesh->GetVertex(i);check(point.m_Coordinates[0]>=-1e-5 && point.m_Coordinates[0]<=10.00001,"local x before build transform");
                check(point.m_Coordinates[1]>=-1e-5 && point.m_Coordinates[1]<=20.00001,"local y before build transform");
            }
        }
        const auto items=model->GetBuildItems();check(items->Count()==2,"build item count");
        check(items->MoveNext() && items->MoveNext(),"second item");const auto transform=items->GetCurrent()->GetObjectTransform();
        check(std::abs(transform.m_Fields[0][1]-1)<1e-6 && std::abs(transform.m_Fields[1][0]+1)<1e-6,"row-vector rotation layout");
        check(transform.m_Fields[3][0]==40 && transform.m_Fields[3][1]==-10 && transform.m_Fields[3][2]==7,"build translation");
        check(GeometryKernel::summarize(first.shape)==firstBefore && GeometryKernel::summarize(second.shape)==secondBefore,"source shapes unchanged");
        MeshExport::write3mf({first},{gp_Trsf()},output.filePath("one-object.3mf"),.1,10);
        int curve=0;
        for(const auto& shape:std::vector<TopoDS_Shape>{BRepPrimAPI_MakeSphere(5).Shape(),BRepPrimAPI_MakeCylinder(5,10).Shape(),BRepPrimAPI_MakeTorus(10,2).Shape()}) {
            EvaluatedBody body;body.bodyId="curved";body.shape=shape;
            const auto result=MeshExport::write3mf({body},{gp_Trsf()},output.filePath(QString("curve-%1.3mf").arg(curve++)),.05,10);
            check(result.value("triangles").toInt()>12,"curved closed mesh");
        }
        const auto protectedPath=output.filePath("protected.3mf");QFile sentinel(protectedPath);check(sentinel.open(QIODevice::WriteOnly),"open sentinel");sentinel.write("preserve");sentinel.close();
        const auto filesBefore=output.entryList(QDir::Files|QDir::Hidden,QDir::Name);
        for(int failure=0;failure<4;++failure) {
            bool refused=false;
            try {
                if(failure==0)MeshExport::write3mf({first},{},protectedPath,.1,10);
                if(failure==1)MeshExport::write3mf({first},{gp_Trsf()},protectedPath,.1,10,[]{return false;});
                if(failure==2)MeshExport::write3mf({first},{gp_Trsf()},protectedPath,.1,10,[]()->bool{throw std::runtime_error("callback failed");});
                if(failure==3){EvaluatedBody sheet;sheet.bodyId="sheet";sheet.shape=BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(),gp_Dir(0,0,1)),-2,2,-2,2).Face();MeshExport::write3mf({first,sheet},placements,protectedPath,.1,10);}
            } catch(const GeometryKernel::EvaluationError& error) {
                const auto code=error.diagnostics().value("code").toString();refused=code==(failure==0?"invalid_transform":failure==1?"export_cancelled":"solid_required");
            } catch(const std::runtime_error& error){refused=failure==2 && QString::fromUtf8(error.what())=="callback failed";}
            check(refused,"expected structured refusal or callback exception");check(sentinel.open(QIODevice::ReadOnly),"read sentinel");check(sentinel.readAll()=="preserve","preserved destination");sentinel.close();
            check(output.entryList(QDir::Files|QDir::Hidden,QDir::Name)==filesBefore,"no leaked staging files");
        }
        std::cout<<QJsonDocument(summary).toJson(QJsonDocument::Compact).constData()<<"\nCore 3MF objects, placements, curved seams and atomic publication passed\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
