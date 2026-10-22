// SPDX-License-Identifier: MIT
#include "MeshImport.h"
#include <RWStl.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QUuid>
#include <QSet>
#include <QRegularExpression>
#include <QtEndian>
#include <array>
#include <map>
#include <numeric>
#include <cmath>
#include <cstring>
#include <sstream>
#include <limits>
namespace MeshImport {
namespace {
constexpr qint64 maxBytes=64*1024*1024, maxTriangles=200000, maxWorkset=512*1024*1024;
using Point=std::array<double,3>;
using Triangle=std::array<int,3>;
[[noreturn]] void fail(const QString& path,const QString& reason,int facet=0,int line=0) {
    throw Error({{"code","mesh_import_failed"},{"format","stl"},{"filename",QFileInfo(path).fileName()},
        {"message",reason},{"facet",facet},{"line",line},{"remedy","Choose a complete finite STL; reduce to at most 200000 facets and 64 MiB, then retry with explicit mm or in units."}});
}
void capacity(const QString& path,qint64 bytes,qint64 count) {
    const qint64 estimate=bytes+count*2048;
    if(bytes>maxBytes || count>maxTriangles || estimate>maxWorkset)
        fail(path,QString("Import limit exceeded: %1 bytes, %2 facets, estimated %3 working bytes; limits %4 / %5 / %6").arg(bytes).arg(count).arg(estimate).arg(maxBytes).arg(maxTriangles).arg(maxWorkset));
}
QJsonArray json(const Point& p){return {p[0],p[1],p[2]};}
QJsonObject analyze(const QJsonArray& vertices,const QJsonArray& triangles) {
    Point lo{INFINITY,INFINITY,INFINITY},hi{-INFINITY,-INFINITY,-INFINITY};
    std::vector<Point> points;for(const auto& v:vertices){const auto a=v.toArray();Point p{a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};points.push_back(p);for(int k=0;k<3;++k){lo[k]=std::min(lo[k],p[k]);hi[k]=std::max(hi[k],p[k]);}}
    // Source OBJ positions stay authored independently; quality connectivity is
    // geometric, so exact-coordinate duplicates share only analysis identities.
    std::map<Point,int> coordinateIds;std::vector<int> geometricIds;geometricIds.reserve(points.size());
    for(const auto& p:points){auto [it,inserted]=coordinateIds.emplace(p,coordinateIds.size());geometricIds.push_back(it->second);}
    struct Edge{int count=0,direction=0,first=-1;};std::map<std::pair<int,int>,Edge> edges;
    std::vector<int> parent(triangles.size());std::iota(parent.begin(),parent.end(),0);
    auto root=[&](int x){while(parent[x]!=x){parent[x]=parent[parent[x]];x=parent[x];}return x;};
    std::vector<bool> usable(triangles.size(),true);
    int degenerate=0;
    for(int n=0;n<triangles.size();++n){const auto a=triangles[n].toArray();Triangle t{a[0].toInt(),a[1].toInt(),a[2].toInt()};const auto p=points[t[0]],q=points[t[1]],r=points[t[2]];
        Point u{},v{};for(int k=0;k<3;++k){u[k]=q[k]-p[k];v[k]=r[k]-p[k];}
        if((u[1]*v[2]-u[2]*v[1])==0 && (u[2]*v[0]-u[0]*v[2])==0 && (u[0]*v[1]-u[1]*v[0])==0){++degenerate;usable[n]=false;continue;}
        for(int k=0;k<3;++k){int x=geometricIds[t[k]],y=geometricIds[t[(k+1)%3]];if(x==y)continue;auto& e=edges[std::minmax(x,y)];if(e.first>=0)parent[root(n)]=root(e.first);else e.first=n;++e.count;e.direction+=x<y?1:-1;}}
    int boundary=0,nonmanifold=0,orientation=0,components=0;
    for(const auto& [key,e]:edges){if(e.count==1)++boundary;if(e.count>2)++nonmanifold;if(e.count==2 && e.direction!=0)++orientation;}
    for(int n=0;n<triangles.size();++n)if(usable[n] && root(n)==n)++components;
    return {{"triangleCount",triangles.size()},{"vertexCount",vertices.size()},{"boundsMm",QJsonObject{{"min",json(lo)},{"max",json(hi)}}},
        {"quality",QJsonObject{{"boundaryEdges",boundary},{"nonManifoldEdges",nonmanifold},{"inconsistentOrientationEdges",orientation},{"degenerateFacets",degenerate},{"connectedComponents",components},{"repairApplied",false}}}};
}
}
QJsonObject analyzeGeometry(const QJsonArray& v,const QJsonArray& t){return analyze(v,t);}
Error::Error(QJsonObject d):std::runtime_error(d.value("message").toString().toStdString()),details(std::move(d)){}
QJsonObject readStl(const QString& path,const QString& units,const Progress& progress) try {
    if(units!="mm" && units!="in")fail(path,"STL requires explicit source units mm or in");
    if(progress)progress("reading",0,0);
    QFile file(path);if(!QFileInfo(path).isAbsolute() || !file.open(QIODevice::ReadOnly))fail(path,"Choose a readable absolute STL path");
    capacity(path,file.size(),0);const auto bytes=file.read(maxBytes+1);capacity(path,bytes.size(),0);if(bytes.isEmpty())fail(path,"Empty STL has no facets");
    std::vector<std::array<Point,3>> facets;int color=0;QString encoding;
    auto add=[&](const std::array<Point,3>& f){if(progress && facets.size()%1024==0)progress("preflight",facets.size(),0);capacity(path,bytes.size(),facets.size()+1);facets.push_back(f);};
    auto finite=[&](double v,int facet,int line=0,bool coordinate=true){if(!std::isfinite(v) || (coordinate && std::abs(v)>1e9/(units=="in"?25.4:1.0)))fail(path,"Nonfinite or out-of-range coordinate/normal",facet,line);return v;};
    quint32 count=0;if(bytes.size()>=84)count=qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData()+80));
    const bool exactBinary=bytes.size()>=84 && quint64(bytes.size())==84ull+50ull*count;
    if(exactBinary || !bytes.trimmed().startsWith("solid")) {
        encoding="binary";if(bytes.size()<84)fail(path,"Truncated binary STL header");capacity(path,bytes.size(),count);
        if(!exactBinary)fail(path,QString("Binary STL count/length mismatch: declared %1 facets, %2 bytes").arg(count).arg(bytes.size()));
        for(quint32 n=0;n<count;++n){std::array<Point,3> f{};const char* row=bytes.constData()+84+50*n;
            for(int k=0;k<12;++k){const auto bits=qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(row+4*k));float value;std::memcpy(&value,&bits,4);finite(value,n+1,0,k>=3);if(k>=3)f[(k-3)/3][(k-3)%3]=value;}
            if(qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(row+48))!=0)++color;add(f);}
    } else {
        encoding="ascii";std::istringstream input(bytes.toStdString());std::string line;int number=0;
        auto next=[&](){while(std::getline(input,line)){++number;if(line.find_first_not_of(" \t\r")!=std::string::npos)return true;}return false;};
        auto row=[&](const std::string& prefix,int values,Point* result=nullptr){std::istringstream s(line);std::istringstream words(prefix);std::string expected,got;while(words>>expected)if(!(s>>got)||got!=expected)fail(path,"Malformed ASCII STL: expected "+QString::fromStdString(prefix),facets.size()+1,number);for(int k=0;k<values;++k){double value;if(!(s>>value))fail(path,"Invalid ASCII number",facets.size()+1,number);finite(value,facets.size()+1,number,result!=nullptr);if(result)(*result)[k]=value;}if(s>>got)fail(path,"Unexpected ASCII token",facets.size()+1,number);};
        if(!next())fail(path,"Empty ASCII STL");{std::istringstream s(line);std::string token;s>>token;if(token!="solid")fail(path,"Expected ASCII solid header",0,number);}
        bool ended=false;while(next()){std::istringstream s(line);std::string token;s>>token;if(token=="endsolid"){ended=true;break;}row("facet normal",3);if(!next())fail(path,"Truncated outer loop");row("outer loop",0);std::array<Point,3> f{};for(int k=0;k<3;++k){if(!next())fail(path,"Truncated vertex");row("vertex",3,&f[k]);}if(!next())fail(path,"Truncated endloop");row("endloop",0);if(!next())fail(path,"Truncated endfacet");row("endfacet",0);add(f);}
        if(!ended || next())fail(path,"ASCII STL requires one complete solid and no trailing records",0,number);
    }
    if(facets.empty())fail(path,"STL has no facets");
    // Decode the exact validated bytes, avoiding source-file replacement races.
    QTemporaryFile staged;if(!staged.open() || staged.write(bytes)!=bytes.size() || !staged.flush())fail(path,"Cannot stage validated STL");
    if(progress)progress("decoding",0,0);
    const auto decoded=RWStl::ReadFile(staged.fileName().toUtf8().constData());
    if(decoded.IsNull() || decoded->NbTriangles()==0)fail(path,"OCCT found no usable triangles");
    // The decoder is admitted only if each received output triangle has the
    // original oriented coordinates. This also catches unexpected reader loss.
    if(progress)progress("checking decoder",0,0);
    using OrientedFacet=std::array<Point,3>;
    auto canonical=[](OrientedFacet f){auto best=f;for(int k=1;k<3;++k){OrientedFacet rotated{f[k],f[(k+1)%3],f[(k+2)%3]};best=std::min(best,rotated);}return best;};
    std::map<OrientedFacet,int> received;
    for(const auto& facet:facets){if(progress && received.size()%1024==0)progress("checking decoder",received.size(),facets.size());++received[canonical(facet)];}
    for(int n=1;n<=decoded->NbTriangles();++n){int a,b,c;decoded->Triangle(n).Get(a,b,c);OrientedFacet f{};int k=0;for(int node:{a,b,c}){const auto& p=decoded->Node(node);f[k++]={p.X(),p.Y(),p.Z()};}auto it=received.find(canonical(f));if(it==received.end() || it->second==0)fail(path,"OCCT decoder changed facet coordinates or winding");--it->second;}
    for(const auto& [facet,count]:received)if(count>0){const auto& a=facet[0];const auto& b=facet[1];const auto& c=facet[2];if(a!=b && b!=c && a!=c)fail(path,"OCCT decoder omitted a non-repeated-vertex facet");}
    // Exact-coordinate merging matches the decoder. Retain raw facet order and
    // degenerate facets that OCCT omits, rather than silently losing source data.
    QJsonArray vertices,triangles;std::map<Point,int> index;const double scale=units=="in"?25.4:1.0;
    if(progress)progress("retaining coordinates",0,0);
    for(const auto& facet:facets){if(progress && triangles.size()%1024==0)progress("retaining coordinates",triangles.size(),facets.size());QJsonArray triangle;for(auto p:facet){for(auto& x:p)x*=scale;auto [it,inserted]=index.emplace(p,index.size());if(inserted)vertices.append(json(p));triangle.append(it->second);}triangles.append(triangle);}
    QJsonObject mesh{{"schema","alloy-mesh/1"},{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"kind","mesh"},{"name",QFileInfo(path).fileName()},
        {"source",QJsonObject{{"format","stl"},{"encoding",encoding},{"name",QFileInfo(path).fileName()},{"sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())},{"units",units},{"scaleToMm",scale},{"bytes",bytes.size()}}},
        {"placement",QJsonArray{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}},{"verticesMm",vertices},{"triangles",triangles},
        {"diagnostics",QJsonObject{{"decoder","OCCT RWStl 7.9.3"},{"sourceNormalsIgnored",true},{"decodedTriangles",decoded->NbTriangles()},{"sourceFacets",int(facets.size())},{"ignoredAttributeFacets",color},{"vertexPolicy","exact-coordinate merge; source facet order/winding and degenerates retained"}}}};
    if(progress)progress("quality analysis",0,0);
    mesh["summary"]=analyze(vertices,triangles);if(progress)progress("prepared",1,1);return mesh;
} catch(const Standard_Failure& error){fail(path,"OCCT STL decode failed: "+QString::fromUtf8(error.GetMessageString()));}
  catch(const std::bad_alloc&){fail(path,"STL allocation failed; reduce the source triangle count and retry");}
QJsonObject withUnits(const QJsonObject& base,const QString& units) {
    if(units!="mm" && units!="in")throw std::invalid_argument("Choose mm or in source units");
    auto mesh=base;const double scale=units=="in"?25.4:1.;QJsonArray vertices;
    for(const auto& row:base.value("verticesMm").toArray()) {QJsonArray point;for(const auto& value:row.toArray()) {
        const double x=value.toDouble()*scale;if(!std::isfinite(x) || std::abs(x)>1e9)throw std::invalid_argument("Scaled mesh exceeds coordinate limit; choose mm or reduce source coordinates");point.append(x);
    }vertices.append(point);}
    auto source=mesh.value("source").toObject();source["units"]=units;source["scaleToMm"]=scale;mesh["source"]=source;
    mesh["verticesMm"]=vertices;mesh["summary"]=analyze(vertices,mesh.value("triangles").toArray());return mesh;
}
QString validate(const QJsonObject& mesh) {
    const auto source=mesh.value("source").toObject();const auto vertices=mesh.value("verticesMm").toArray(),triangles=mesh.value("triangles").toArray();
    const auto diagnostics=mesh.value("diagnostics").toObject();
    static const QRegularExpression hash("^[0-9a-f]{64}$");
    if((mesh.size()!=10+(mesh.contains("visible")?1:0)) || (mesh.contains("visible") && !mesh.value("visible").isBool()) || source.size()!=7 || diagnostics.size()!=6 || mesh.value("schema")!="alloy-mesh/1" || mesh.value("kind")!="mesh" || QUuid(mesh.value("id").toString()).isNull() || mesh.value("name").toString().isEmpty()
        || (source.value("format")!="stl" && source.value("format")!="obj") || (source.value("units")!="mm" && source.value("units")!="in") || source.value("scaleToMm").toDouble()!=(source.value("units")=="in"?25.4:1.0)
        || !hash.match(source.value("sha256").toString()).hasMatch()
        || (source.value("format")=="stl" ? (source.value("encoding")!="ascii" && source.value("encoding")!="binary") : source.value("encoding")!="utf-8")
        || source.value("name")!=mesh.value("name") || !source.value("bytes").isDouble() || source.value("bytes").toDouble()<1 || source.value("bytes").toDouble()>maxBytes
        || (source.value("format")=="stl" && (diagnostics.value("sourceFacets").toInt(-1)!=triangles.size() || diagnostics.value("decodedTriangles").toInt(-1)<1
        || diagnostics.value("decodedTriangles").toInt()>triangles.size() || diagnostics.value("ignoredAttributeFacets").toInt(-1)<0 || diagnostics.value("ignoredAttributeFacets").toInt()>triangles.size()))
        || (source.value("format")=="obj" && (diagnostics.value("decoder")!="Alloy strict OBJ/1; direct owned triangulation" || diagnostics.value("sourceFaces").toInt(-1)<1 || diagnostics.value("sourceFaces").toInt()>triangles.size() || !diagnostics.value("sidecarsRead").isBool() || diagnostics.value("sidecarsRead").toBool() || !diagnostics.value("ignoredRecords").isObject() || !diagnostics.value("sourceLabels").isArray() || !diagnostics.value("ignoredNames").isArray())) || vertices.isEmpty() || triangles.isEmpty() || triangles.size()>maxTriangles || vertices.size()>3*maxTriangles
        || mesh.value("placement").toArray()!=QJsonArray{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1})return "Invalid retained mesh metadata or limits";
    if(source.value("format")=="obj") {
        const auto faces=diagnostics.value("sourceFaces");
        if(!faces.isDouble() || faces.toDouble()!=faces.toInt())return "Invalid OBJ source face count";
        for(const auto& field:{"sourceLabels","ignoredNames"}) {
            const auto labels=diagnostics.value(field).toArray();if(labels.size()>1024)return "Too many OBJ labels";
            QSet<QString> seen;
            for(const auto& value:labels){const auto label=value.toObject();const auto kind=label.value("kind").toString(),name=label.value("name").toString();const auto line=label.value("line");
                if(label.size()!=3 || !label.value("name").isString() || name.toUtf8().size()>256 || name.contains(QChar::Null) || !line.isDouble() || line.toDouble()!=line.toInt() || line.toInt()<1
                    || (QString(field)=="sourceLabels" ? (kind!="o" && kind!="g") : (kind!="mtllib" && kind!="usemtl" && kind!="s")) || seen.contains(kind+"\n"+name))return "Invalid OBJ source label";
                seen.insert(kind+"\n"+name);
            }
        }
        const auto ignored=diagnostics.value("ignoredRecords").toObject();
        for(auto it=ignored.begin();it!=ignored.end();++it)if(!QStringList{"vt","vn","mtllib","usemtl","s","l","p"}.contains(it.key()) || !it.value().isDouble() || it.value().toDouble()!=it.value().toInt(-1) || it.value().toInt(-1)<1)return "Invalid OBJ ignored record count";
    }
    for(const auto& v:vertices){const auto a=v.toArray();if(a.size()!=3)return "Mesh vertex requires three coordinates";for(const auto& x:a)if(!x.isDouble() || !std::isfinite(x.toDouble()) || std::abs(x.toDouble())>1e9)return "Invalid mesh coordinate";}
    for(const auto& t:triangles){const auto a=t.toArray();if(a.size()!=3)return "Mesh facet requires three indices";for(const auto& x:a)if(!x.isDouble() || x.toDouble()!=x.toInt(-1) || x.toInt(-1)<0 || x.toInt()>=vertices.size())return "Invalid mesh triangle index";}
    if(mesh.value("summary").toObject()!=analyze(vertices,triangles))return "Mesh summary does not match retained geometry";
    return {};
}
QJsonObject summary(const QJsonObject& mesh){auto result=mesh.value("summary").toObject();result["id"]=mesh.value("id");result["kind"]="mesh";result["visible"]=mesh.value("visible").toBool(true);result["name"]=mesh.value("name");result["source"]=mesh.value("source");result["diagnostics"]=mesh.value("diagnostics");return result;}
}
