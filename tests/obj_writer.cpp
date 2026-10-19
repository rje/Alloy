// SPDX-License-Identifier: MIT
#include "MeshExport.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Pln.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QByteArray read(const QString& path){QFile file(path);check(file.open(QIODevice::ReadOnly),"read output");return file.readAll();}
int main(int argc,char** argv){
 QCoreApplication app(argc,argv);
 try {
  QTemporaryDir dir;check(dir.isValid(),"temporary directory");
  EvaluatedBody body;body.bodyId="box";body.name="unsafe\ng injected";body.shape=BRepPrimAPI_MakeBox(10,20,30).Shape();
  const auto before=GeometryKernel::summarize(body.shape);const auto path=dir.filePath("model.obj");
  const auto result=MeshExport::writeObj({body},path,"mm",.1,10);
  check(result.value("triangles").toInt()==12 && result.value("vertices").toInt()==8,"indexed box");
  const auto original=read(path);MeshExport::writeObj({body},path,"mm",.1,10);check(read(path)==original,"deterministic bytes");
  check(!original.contains("injected") && !original.contains("mtllib") && !original.contains("usemtl"),"generated safe groups without materials");
  check(GeometryKernel::summarize(body.shape)==before,"source shape unchanged");
  EvaluatedBody curved;curved.bodyId="sphere";curved.shape=BRepPrimAPI_MakeSphere(5).Shape();
  check(MeshExport::writeObj({curved},dir.filePath("sphere.obj"),"in",.05,10).value("triangles").toInt()>12,"closed curved seams");
  const auto files=QDir(dir.path()).entryList(QDir::Files|QDir::Hidden);
  for(int variant=0;variant<6;++variant){
   bool refused=false;
   try {
    if(variant==0)MeshExport::writeObj({body},path,"cm",.1,10);
    if(variant==1)MeshExport::writeObj({body},path,"mm",0,10);
    if(variant==2)MeshExport::writeObj({body,body},path,"mm",.1,10);
    if(variant==3)MeshExport::writeObj({body},path,"mm",.1,10,[]{return false;});
    if(variant==4){EvaluatedBody sheet;sheet.bodyId="sheet";sheet.shape=BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(),gp_Dir(0,0,1)),-2,2,-2,2).Face();MeshExport::writeObj({body,sheet},path,"mm",.1,10);}
    if(variant==5)MeshExport::writeObj({},path,"mm",.1,10);
   }catch(const GeometryKernel::EvaluationError& error){const QStringList codes{"invalid_units","invalid_mesh_settings","invalid_body_selection","export_cancelled","solid_required","solid_required"};refused=error.diagnostics().value("code")==codes[variant];}
   check(refused,"structured refusal");check(read(path)==original,"preserved destination");
   check(QDir(dir.path()).entryList(QDir::Files|QDir::Hidden)==files,"no staging leaks");
  }
  std::cout<<"OBJ indexed mesh, deterministic bytes, curved seams and atomic refusal passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
