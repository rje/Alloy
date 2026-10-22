// SPDX-License-Identifier: MIT
#include "MeshImport.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <iostream>
#include <cmath>
static void check(bool x,const char* why){if(!x)throw std::runtime_error(why);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 QTemporaryDir dir;auto path=dir.filePath("original.obj");auto write=[&](QByteArray b){QFile f(path);check(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"fixture write");};
 auto read=[&](QByteArray b,QString unit="mm"){write(b);auto m=MeshImport::readObj(path,unit);check(MeshImport::validate(m).isEmpty(),"retained validation");return m;};
 QByteArray quad="v 1 2 3\nv 5 2 3\nv 5 6 3\nv 1 6 3\nf 1 2 3 4\n";
 auto m=read(quad);check(m.value("triangles").toArray()==QJsonArray{QJsonArray{3,0,1},QJsonArray{1,2,3}},"deterministic quad ears");check(m.value("verticesMm").toArray()[0].toArray()==QJsonArray{1,2,3},"identity axes");
 auto inch=read(quad,"in");check(inch.value("verticesMm").toArray()[0].toArray()==QJsonArray{1*25.4,2*25.4,3*25.4},"inch exact scaling");
 QByteArray concave="v 0 0 0\nv 4 0 0\nv 4 4 0\nv 2 1 0\nv 0 4 0\nf 1 2 3 4 5\n";auto c=read(concave);check(c.value("triangles").toArray()==QJsonArray{QJsonArray{1,2,3},QJsonArray{0,1,3},QJsonArray{0,3,4}},"deterministic concave ears");
 auto reverse=read(QByteArray(concave).replace("f 1 2 3 4 5","f -1 -2 -3 -4 -5"));double area=0;auto pv=reverse.value("verticesMm").toArray();for(const auto& v:reverse.value("triangles").toArray()){auto t=v.toArray(),a=pv[t[0].toInt()].toArray(),b=pv[t[1].toInt()].toArray(),d=pv[t[2].toInt()].toArray();double cross=(b[0].toDouble()-a[0].toDouble())*(d[1].toDouble()-a[1].toDouble())-(b[1].toDouble()-a[1].toDouble())*(d[0].toDouble()-a[0].toDouble());check(cross<0,"reverse winding");area+=cross/2;}check(area==-10,"concave area");
 QByteArray grouped="o first\ng shell one\nv 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\nvt 0\nvt 0 1 0\nvn 0 0 1\nmtllib /definitely/nonexistent/sidecar.mtl\nusemtl original\ns 1\nl 1/1 2/2\np -1\nf 1 3 2\nf 1/1 2/2 4/1\nf 1//1 4//1 3//1\nf -3/1/1 -2/2/1 -1/1/1\no second\ng second\nv 10 0 0\nv 11 0 0\nv 10 1 0\nv 10 0 1\nf 5 7 6\nf 5 6 8\nf 5 8 7\nf 6 7 8\n";auto g=read(grouped);check(g.value("triangles").toArray().size()==8 && g.value("diagnostics").toObject().value("sourceLabels").toArray().size()==4,"groups one mesh");
 auto refused=[&](QByteArray b,int expectedLine=0){write(b);bool rejected=false;try{MeshImport::readObj(path,"mm");}catch(const MeshImport::Error& e){check(e.details.value("format")=="obj" && e.details.value("filename")=="original.obj" && !e.details.value("remedy").toString().isEmpty(),"structured error");if(expectedLine)check(e.details.value("line")==expectedLine,"original source line");rejected=true;}check(rejected,"bad OBJ accepted");};
 for(const auto& bad:{QByteArray{},QByteArray("# only\n"),QByteArray("v 0 0 0\np 1\n"),quad+"f 1 2 99\n",QByteArray(quad).replace("f 1 2 3 4","f 1/1 2/1 3/1"),QByteArray(quad).replace("f 1 2 3 4","f 1//1 2//1 3//1"),QByteArray(quad).replace("f 1 2 3 4","f 1 3 2 4"),QByteArray(quad).replace("v 5 6 3","v 5 6 4"),QByteArray(quad).replace("f 1 2 3 4","f 1 2 2 4"),quad+"surf 1\n",quad+"v nan 0 0\n",quad+"v 1 2 3 1\n",quad+"f 0 1 2\n",quad+"f 9223372036854775808 1 2\n",quad+"f 1/ 2 3\n",quad+"s nonsense\n",quad+"f 1 2 3 \\\n",quad+QByteArray("\xff",1)})refused(bad);
 refused(quad+"f 1 2 99\n",6);
 refused(quad+QByteArray("\xff",1),6);refused(quad+QByteArray("\0",1),6);refused(quad+QByteArray("\xe2\x82",2),6);
 auto split=read("v 0 0 0\nv 1 0 0\nv 0 1 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3\nf 4 5 6\n");auto quality=split.value("summary").toObject().value("quality").toObject();check(split.value("verticesMm").toArray().size()==6 && split.value("triangles").toArray()==QJsonArray{QJsonArray{0,1,2},QJsonArray{3,4,5}} && quality.value("boundaryEdges")==4 && quality.value("connectedComponents")==1,"geometric quality without authored merge");
 // Numeric gates straddle the documented policy; no source-repair tolerance.
 read("v 0 0 0\nv 1 0 0\nv 1 1 0.00000001\nv 0 1 0\nf 1 2 3 4\n");refused("v 0 0 0\nv 1 0 0\nv 1 1 0.000001\nv 0 1 0\nf 1 2 3 4\n");
 read("v 0 0 0\nv 1 0 0\nv 0 0.000000000002 0\nf 1 2 3\n");refused("v 0 0 0\nv 1 0 0\nv 0 0.0000000000005 0\nf 1 2 3\n");
 auto polygon=[&](int n){QByteArray b;for(int i=0;i<n;++i)b+="v "+QByteArray::number(std::cos(i*6.283185307179586/n),'g',17)+" "+QByteArray::number(std::sin(i*6.283185307179586/n),'g',17)+" 0\n";b+="f";for(int i=1;i<=n;++i)b+=" "+QByteArray::number(i);return b+"\n";};check(read(polygon(256)).value("triangles").toArray().size()==254,"256 corner boundary");refused(polygon(257));refused(quad+QByteArray("o ")+QByteArray(257,'x')+"\n");refused(QByteArray(64*1024*1024+1,'#'));
 QByteArray cap="v 0 0 0\nv 1 0 0\nv 0 1 0\n";for(int i=0;i<200000;++i)cap+="f 1 2 3\n";check(read(cap).value("triangles").toArray().size()==200000,"200000 triangle boundary");refused(cap+"f 1 2 3\n");
 // Source-independent retained document and complete-file atomic candidate.
 CadDocument d;write(grouped);auto candidate=CadDocument::objImportCandidate(d.authored(),{{"path",path},{"units","mm"}});check(d.commit(candidate,"Import OBJ mesh").isEmpty(),"commit");auto authored=d.authored();QFile::remove(path);d.history().undo();check(d.authored().value("meshes").toArray().isEmpty(),"undo");d.history().redo();check(d.authored()==authored,"redo identity");auto saved=dir.filePath("retained.alloy");check(d.save(saved).isEmpty(),"save");CadDocument reopened;check(reopened.open(saved).isEmpty()&&reopened.authored()==authored,"source independent reopen");write(quad+"f 1 2 99\n");try{CadDocument::objImportCandidate(d.authored(),{{"path",path},{"units","mm"}});throw std::runtime_error("atomic candidate accepted");}catch(const MeshImport::Error&){}check(d.authored()==authored,"invalid mutation");
 std::cout<<"strict OBJ coordinates/ears/winding/limits/refusals/history/reopen passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
