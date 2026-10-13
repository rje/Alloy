// SPDX-License-Identifier: MIT
#include "MeshExport.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <gp_Pln.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        if(argc!=2)throw std::runtime_error("Expected output directory");
        QDir output(QString::fromLocal8Bit(argv[1]));
        EvaluatedBody first;first.bodyId="first";first.shape=BRepPrimAPI_MakeBox(gp_Pnt(-5,2,3),20,10,15).Shape();
        EvaluatedBody second;second.bodyId="second";second.shape=BRepPrimAPI_MakeBox(gp_Pnt(40,-5,7),12,12,12).Shape();
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
        std::cout<<"Two placed body exports and atomic invalid-body refusal passed\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
