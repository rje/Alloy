// SPDX-License-Identifier: MIT
#include "MeshExport.h"
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <sstream>
#include <iostream>
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 if(argc!=2)throw std::runtime_error("Expected output directory");const QDir out(QString::fromLocal8Bit(argv[1]));
 // An independently authored skinny cap near z80: its double-precision normal
 // changes after float32 vertex serialization, while no vertex collapses.
 const gp_Pnt a(-.32688385248184204,.1438046097755432,79.76462074168276),
     b(-.35843726992607117,.1591918170452118,79.74126152150083),
     c(-.24812832474708557,.10321561992168427,79.82327990921588);
 BRepBuilderAPI_MakePolygon boundary;boundary.Add(a);boundary.Add(b);boundary.Add(c);boundary.Close();
 const auto face=BRepBuilderAPI_MakeFace(boundary.Wire()).Face();
 auto direction=gp_Vec(a,b).Crossed(gp_Vec(a,c));direction*=.01/direction.Magnitude();
 EvaluatedBody body;body.bodyId="quantized-normal-prism";body.shape=BRepPrimAPI_MakePrism(face,direction).Shape();
 std::ostringstream before;BRepTools::Write(body.shape,before);QJsonObject report;
 for(const auto& units:QStringList{"mm","in"})report[units]=MeshExport::writeStl(body,out.filePath(units+".stl"),units,true,.05,8);
 std::ostringstream after;BRepTools::Write(body.shape,after);if(before.str()!=after.str())throw std::runtime_error("Export altered the original BRep");
 QFile file(out.filePath("metadata.json"));if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot write metadata");file.write(QJsonDocument(report).toJson());return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
