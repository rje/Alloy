// SPDX-License-Identifier: MIT
#include "MeshImport.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(argc==2){const auto mesh=MeshImport::readStl(QString::fromLocal8Bit(argv[1]),"mm");if(!MeshImport::validate(mesh).isEmpty())throw std::runtime_error("validation");std::cout<<QJsonDocument(MeshImport::summary(mesh)).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;}
    QTemporaryDir dir;const auto path=dir.filePath("input.stl");
    const QByteArray valid="solid original\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid original\n";
    auto write=[&](QByteArray bytes){QFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size())throw std::runtime_error("fixture write");};
    write(valid);const auto mesh=MeshImport::readStl(path,"in");if(!MeshImport::validate(mesh).isEmpty() || mesh.value("summary").toObject().value("boundsMm").toObject().value("max").toArray()!=QJsonArray{25.4,25.4,0})throw std::runtime_error("inch bounds");
    auto corrupt=mesh;auto triangles=mesh.value("triangles").toArray();triangles[0]=QJsonArray{0,1,999};corrupt["triangles"]=triangles;if(MeshImport::validate(corrupt).isEmpty())throw std::runtime_error("invalid index accepted");
    for(const auto& bytes:{QByteArray{},valid.left(valid.indexOf("endfacet")),QByteArray(valid).replace("vertex 1 0 0","vertex nan 0 0"),QByteArray(valid).replace("endloop","endloop extra"),valid+"trailing\n",QByteArray(64*1024*1024+1,'x')}){write(bytes);bool refused=false;try{MeshImport::readStl(path,"mm");}catch(const MeshImport::Error& e){refused=e.details.value("filename")=="input.stl" && !e.details.value("remedy").toString().isEmpty();}if(!refused)throw std::runtime_error("malformed input accepted");}
    write(valid);bool refused=false;try{MeshImport::readStl(path,"");}catch(const MeshImport::Error&){refused=true;}if(!refused)throw std::runtime_error("implicit units accepted");
    std::cout<<"retained STL adapter checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
