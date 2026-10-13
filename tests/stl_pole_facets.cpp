// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "MeshExport.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    if(argc!=3)throw std::runtime_error("Expected output directory and native design");
    QDir output(QString::fromLocal8Bit(argv[1]));if(!output.exists())throw std::runtime_error("Output directory must exist");
    EvaluatedBody body;body.bodyId="pole-fixture";
    CadDocument document;
    if(argc==3){const auto error=document.open(QString::fromLocal8Bit(argv[2]));if(!error.isEmpty())throw std::runtime_error(error.toStdString());if(document.bodies().size()!=1)throw std::runtime_error("Expected one die body");body=document.bodies()[0];}
    else throw std::runtime_error("Expected a native pole fixture");
    QJsonObject metadata;
    for(const auto& units:QStringList{"mm","in"}) {
        const auto path=output.filePath("pole-"+units+".stl");
        const auto result=MeshExport::writeStl(body,path,units,true,0.05,8);metadata[units]=result;
        QFile file(path);if(!file.open(QIODevice::ReadOnly) || file.size()!=result["bytes"].toInteger())throw std::runtime_error("Returned binary byte count differs from file");
    }
    metadata["ascii"]=MeshExport::writeStl(body,output.filePath("pole-mm-ascii.stl"),"mm",false,0.05,8);
    metadata["brep"]=GeometryKernel::summarize(body.shape);
    QFile report(output.filePath("metadata.json"));if(!report.open(QIODevice::WriteOnly))throw std::runtime_error("Metadata output failed");report.write(QJsonDocument(metadata).toJson());
    std::cout<<QJsonDocument(metadata).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
