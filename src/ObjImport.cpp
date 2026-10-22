// SPDX-License-Identifier: MIT
#include "MeshImport.h"
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QUuid>
#include <array>
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
namespace MeshImport {
QJsonObject analyzeGeometry(const QJsonArray&,const QJsonArray&);
namespace {
using P=std::array<double,3>; using V=std::array<double,2>;
constexpr qint64 byteCap=64*1024*1024, triangleCap=200000, entryCap=600000, workCap=512*1024*1024;
constexpr int cornerCap=256,labelCap=1024,labelBytes=256;
struct Parser {
 QString path; int line=0,face=0; qint64 bytes=0,expected=0,vt=0,vn=0; std::vector<P> points; QJsonArray triangles,labels,ignoredNames; QJsonObject ignored;
 [[noreturn]] void fail(QString reason)const{throw Error({{"code","mesh_import_failed"},{"format","obj"},{"filename",QFileInfo(path).fileName()},{"message",reason},{"line",line},{"face",face},{"remedy","Use finite v/vt/vn and valid already-declared indices with simple planar nondegenerate faces (3–256 corners); reduce to 64 MiB/200000 triangles/600000 entries and choose explicit mm or in units."}});}
 void capacity()const {if(bytes>byteCap || expected>triangleCap || points.size()>entryCap || vt>entryCap || vn>entryCap || bytes+expected*2048+(points.size()+vt+vn)*256>workCap)fail("OBJ import resource limit exceeded (64 MiB, 200000 triangles, 600000 entries/table, estimated 512 MiB)");}
 double number(const QString& s)const {static const QRegularExpression numeric("^[+-]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?$");bool ok=false;double x=s.toDouble(&ok);if(!numeric.match(s).hasMatch() || !ok || !std::isfinite(x))fail("Invalid finite OBJ number");return x;}
 int index(const QString& s,qint64 count)const{static const QRegularExpression integer("^[+-]?[0-9]+$");bool ok=false;const qint64 n=s.toLongLong(&ok);if(!integer.match(s).hasMatch() || !ok || n==0 || n>count || n< -count)fail("OBJ index is zero, malformed, forward or out of range");return int(n>0?n-1:count+n);}
 int corner(const QString& token,bool lineElement=false)const{const auto a=token.split('/');if(a.size()>3 || a[0].isEmpty() || (lineElement && a.size()>2))fail("Malformed OBJ corner");int p=index(a[0],points.size());if(a.size()==2){if(a[1].isEmpty())fail("Missing texture index");index(a[1],vt);}if(a.size()==3){if(!a[1].isEmpty())index(a[1],vt);if(a[2].isEmpty())fail("Missing normal index");index(a[2],vn);}return p;}
 void name(QJsonArray& out,QString kind,QString text){if(text.toUtf8().size()>labelBytes || text.contains(QChar::Null))fail("OBJ label exceeds 256 UTF-8 bytes or contains NUL");for(const auto& v:out){auto x=v.toObject();if(x.value("kind")==kind && x.value("name")==text)return;}if(out.size()>=labelCap)fail("OBJ label count exceeds 1024");out.append(QJsonObject{{"kind",kind},{"name",text},{"line",line}});}
 void polygon(const std::vector<int>& ids){
  const int n=int(ids.size());P lo=points[ids[0]],hi=lo,normal{};const P origin=lo;
  for(int i=0;i<n;++i){const auto a=points[ids[i]],b=points[ids[(i+1)%n]];for(int k=0;k<3;++k){lo[k]=std::min(lo[k],a[k]);hi[k]=std::max(hi[k],a[k]);normal[k]+=(a[(k+1)%3]-origin[(k+1)%3])*(b[(k+2)%3]-origin[(k+2)%3])-(a[(k+2)%3]-origin[(k+2)%3])*(b[(k+1)%3]-origin[(k+1)%3]);}}
  double extent=std::max({hi[0]-lo[0],hi[1]-lo[1],hi[2]-lo[2]});double linear=std::max(1e-12,1e-12*extent), area=std::max(1e-24,1e-12*extent*extent),planar=std::max(1e-9,1e-8*extent);
  const double length=std::hypot(normal[0],normal[1],normal[2]);if(!std::isfinite(length) || length<=area)fail("Degenerate or self-intersecting polygon has no stable area");
  int drop=int(std::max_element(normal.begin(),normal.end(),[](double a,double b){return std::abs(a)<std::abs(b);})-normal.begin());std::vector<V> p;
  for(int id:ids){const auto a=points[id];double distance=0;for(int k=0;k<3;++k)distance+=(a[k]-origin[k])*normal[k]/length;if(std::abs(distance)>planar)fail("Nonplanar polygon exceeds max(1e-9 mm, 1e-8 × face extent)");p.push_back({a[(drop+1)%3]-origin[(drop+1)%3],a[(drop+2)%3]-origin[(drop+2)%3]});}
  auto cross=[&](int a,int b,int c){return (p[b][0]-p[a][0])*(p[c][1]-p[a][1])-(p[b][1]-p[a][1])*(p[c][0]-p[a][0]);};
  for(int i=0;i<n;++i)for(int j=i+1;j<n;++j)if(std::hypot(p[i][0]-p[j][0],p[i][1]-p[j][1])<=linear)fail("Repeated polygon corner");
  auto on=[&](int a,int b,int c){return std::abs(cross(a,b,c))<=area && p[c][0]>=std::min(p[a][0],p[b][0])-linear && p[c][0]<=std::max(p[a][0],p[b][0])+linear && p[c][1]>=std::min(p[a][1],p[b][1])-linear && p[c][1]<=std::max(p[a][1],p[b][1])+linear;};
  for(int i=0;i<n;++i){int j=(i+1)%n;if(std::abs(cross((i+n-1)%n,i,j))<=area)fail("Collinear polygon corner is unsupported");for(int k=i+1;k<n;++k){int l=(k+1)%n;if(j==k || l==i)continue;double a=cross(i,j,k),b=cross(i,j,l),c=cross(k,l,i),d=cross(k,l,j);if(((a>area && b< -area)||(a< -area && b>area)) && ((c>area && d< -area)||(c< -area && d>area)) || on(i,j,k)||on(i,j,l)||on(k,l,i)||on(k,l,j))fail("Self-intersecting polygon");}}
  double sum=0;for(int i=0;i<n;++i)sum+=p[i][0]*p[(i+1)%n][1]-p[(i+1)%n][0]*p[i][1];if(std::abs(sum)<=area)fail("Zero polygon area");const double sign=sum>0?1:-1;
  std::vector<int> active(n);std::iota(active.begin(),active.end(),0);
  while(active.size()>3){bool found=false;for(int pos=0;pos<int(active.size());++pos){int a=active[(pos+active.size()-1)%active.size()],b=active[pos],c=active[(pos+1)%active.size()];if(sign*cross(a,b,c)<=area)continue;bool blocked=false;for(int x:active)if(x!=a && x!=b && x!=c && sign*cross(a,b,x)>=-area && sign*cross(b,c,x)>=-area && sign*cross(c,a,x)>=-area){blocked=true;break;}if(blocked)continue;triangles.append(QJsonArray{ids[a],ids[b],ids[c]});active.erase(active.begin()+pos);found=true;break;}if(!found)fail("Polygon cannot be triangulated safely");}
  if(sign*cross(active[0],active[1],active[2])<=area)fail("Degenerate final polygon triangle");triangles.append(QJsonArray{ids[active[0]],ids[active[1]],ids[active[2]]});
 }
};
}
QJsonObject readObj(const QString& path,const QString& units,const Progress& progress)try{
 Parser p;p.path=path;if(units!="mm" && units!="in")p.fail("OBJ requires explicit source units mm or in");const double scale=units=="in"?25.4:1.;
 QFile file(path);if(!QFileInfo(path).isAbsolute() || !file.open(QIODevice::ReadOnly))p.fail("Choose a readable absolute OBJ path");p.bytes=file.size();p.capacity();const auto bytes=file.read(byteCap+1);p.bytes=bytes.size();p.capacity();QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);const QString text=decoder(bytes);if(decoder.hasError() || text.contains(QChar::Null)){
  qsizetype at=0;p.line=0;while(at<bytes.size()){++p.line;auto end=bytes.indexOf('\n',at);auto length=(end<0?bytes.size():end)-at;QByteArrayView row(bytes.constData()+at,length);QStringDecoder lineDecoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);const QString decoded=lineDecoder(row);if(lineDecoder.hasError() || decoded.contains(QChar::Null))break;at=end<0?bytes.size():end+1;}p.fail("OBJ must be valid UTF-8 text without NUL");
 }
 qsizetype offset=0;while(offset<text.size()){const auto end=text.indexOf('\n',offset);const auto length=(end<0?text.size():end)-offset;auto row=QStringView(text).mid(offset,length);offset=end<0?text.size():end+1;++p.line;if(length>65536)p.fail("OBJ record exceeds 65536 UTF-16 code units");if(progress && p.line%1024==0)progress("preflight",p.line,0);QString line=row.toString();line=line.left(line.indexOf('#')<0?line.size():line.indexOf('#')).trimmed();if(line.isEmpty())continue;if(line.contains('\\'))p.fail("OBJ continuation/backslash syntax is unsupported");const auto t=line.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);const auto key=t[0];
  if(key=="v" || key=="vt" || key=="vn"){int count=t.size()-1;if((key=="vt" && (count<1 || count>3)) || (key!="vt" && count!=3))p.fail("Wrong coordinate count for "+key);P a{};for(int i=0;i<count;++i){double x=p.number(t[i+1]);if(key=="v"){x*=scale;if(!std::isfinite(x)||std::abs(x)>1e9)p.fail("OBJ coordinate exceeds 1e9 mm");a[i]=x;}}if(key=="v")p.points.push_back(a);else {if(key=="vt")++p.vt;else ++p.vn;p.ignored[key]=p.ignored.value(key).toInt()+1;}p.capacity();
  }else if(key=="f"){++p.face;if(t.size()<4 || t.size()>cornerCap+1)p.fail("OBJ face requires 3–256 corners");p.expected+=t.size()-3;p.capacity();std::vector<int> ids;for(int i=1;i<t.size();++i)ids.push_back(p.corner(t[i]));p.polygon(ids);
  }else if(key=="o" || key=="g"){p.name(p.labels,key,t.mid(1).join(' '));
  }else if(key=="mtllib" || key=="usemtl" || key=="s"){if(t.size()<2 || (key=="s" && (t.size()!=2 || (t[1]!="off" && t[1]!="on" && !QRegularExpression("^[0-9]+$").match(t[1]).hasMatch()))))p.fail("Malformed ignored OBJ record "+key);p.ignored[key]=p.ignored.value(key).toInt()+1;p.name(p.ignoredNames,key,t.mid(1).join(' '));
  }else if(key=="l" || key=="p"){if(t.size()<(key=="l"?3:2))p.fail("Malformed ignored OBJ geometry "+key);for(int i=1;i<t.size();++i){if(key=="p")p.index(t[i],p.points.size());else p.corner(t[i],true);}p.ignored[key]=p.ignored.value(key).toInt()+1;
  }else p.fail("Unsupported OBJ directive: "+key);
 }
 if(p.triangles.isEmpty())p.fail("OBJ has no polygon faces");QJsonArray vertices;for(const auto& a:p.points)vertices.append(QJsonArray{a[0],a[1],a[2]});
 QJsonObject mesh{{"schema","alloy-mesh/1"},{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"kind","mesh"},{"name",QFileInfo(path).fileName()},
 {"source",QJsonObject{{"format","obj"},{"encoding","utf-8"},{"name",QFileInfo(path).fileName()},{"sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())},{"units",units},{"scaleToMm",scale},{"bytes",bytes.size()}}},
 {"placement",QJsonArray{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}},{"verticesMm",vertices},{"triangles",p.triangles},
 {"diagnostics",QJsonObject{{"decoder","Alloy strict OBJ/1; direct owned triangulation"},{"sourceFaces",p.face},{"ignoredRecords",p.ignored},{"ignoredNames",p.ignoredNames},{"sourceLabels",p.labels},{"sidecarsRead",false}}}};
 mesh["summary"]=analyzeGeometry(vertices,p.triangles);if(progress)progress("prepared",1,1);return mesh;
}catch(const std::bad_alloc&){Parser p;p.path=path;p.fail("OBJ allocation failed; reduce the source size");}
}
