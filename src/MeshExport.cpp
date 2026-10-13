#include "MeshExport.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QJsonArray>
#include <QSet>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <StlAPI_Writer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QtEndian>
#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace {
[[noreturn]] void reject(const QString& code,const QString& message) {
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
}

QJsonObject MeshExport::writeStl(const EvaluatedBody& body,const QString& path,
    const QString& units,bool binary,double surfaceDeviationMm,double normalDeviationDeg) {
    return writeStl(std::vector<EvaluatedBody>{body},path,units,binary,surfaceDeviationMm,normalDeviationDeg);
}

QJsonObject MeshExport::writeStl(const std::vector<EvaluatedBody>& bodies,const QString& path,
    const QString& units,bool binary,double surfaceDeviationMm,double normalDeviationDeg) {
    if(units!="mm" && units!="in")reject("invalid_units","STL output units must be mm or in");
    if(!std::isfinite(surfaceDeviationMm) || surfaceDeviationMm<0.001 || surfaceDeviationMm>10
        || !std::isfinite(normalDeviationDeg) || normalDeviationDeg<1 || normalDeviationDeg>45)
        reject("invalid_mesh_settings","Surface deviation must be 0.001–10 mm and normal deviation 1–45 degrees");
    const QFileInfo destination(path);
    if(!destination.isAbsolute() || destination.fileName().isEmpty() || !destination.dir().exists())
        reject("invalid_path","Choose an absolute STL path in an existing directory");
    if(bodies.empty())reject("solid_required","Select at least one valid closed solid");
    QSet<QString> distinct;QJsonArray bodyIds;
    for(const auto& body:bodies) {
        if(body.bodyId.isEmpty() || distinct.contains(body.bodyId))reject("invalid_body_selection","Select distinct stable body IDs");
        distinct.insert(body.bodyId);bodyIds.append(body.bodyId);
        const auto summary=GeometryKernel::summarize(body.shape);
        if(!summary.value("valid").toBool() || !summary.value("closed").toBool()
            || summary.value("topology").toObject().value("solids").toInt()!=1
            || summary.value("volumeMm3").toDouble()<=0)
            reject("solid_required","Each selected export body must be one valid closed solid: "+body.bodyId);
    }

    // Each solid keeps its world placement. No Boolean union or topology healing:
    // separate material bodies remain separate triangle components in the STL.
    BRep_Builder builder;TopoDS_Compound compound;builder.MakeCompound(compound);
    qint64 triangles=0, collapsedTriangles=0;int faces=0;int mesherStatus=0;
    for(const auto& body:bodies) {
        // Detach geometry and discard display triangulation before canonical-mm meshing.
        BRepBuilderAPI_Copy copy(body.shape,true,false);auto detached=copy.Shape();
        BRepMesh_IncrementalMesh mesher(detached,surfaceDeviationMm,false,
            normalDeviationDeg*std::numbers::pi/180.0,false);
        if(!mesher.IsDone())reject("mesh_failed","Export tessellation did not complete");
        mesherStatus|=mesher.GetStatusFlags();
        for(TopExp_Explorer it(detached,TopAbs_FACE);it.More();it.Next()) {
            TopLoc_Location location;
            const auto mesh=BRep_Tool::Triangulation(TopoDS::Face(it.Current()),location);
            if(mesh.IsNull() || mesh->NbTriangles()==0)reject("mesh_failed","A selected solid face has no export triangles");
            // Binary STL stores float coordinates. Tiny pole triangles can
            // collapse only at that precision, so discard them on the detached
            // export mesh before asking OCCT to write normals and facets.
            if(binary) {
                std::vector<Poly_Triangle> retained;
                for(int index=1;index<=mesh->NbTriangles();++index) {
                    const auto triangle=mesh->Triangle(index);int a,b,c;triangle.Get(a,b,c);
                    const auto outputPoint=[&](int node) {
                        auto point=mesh->Node(node).Transformed(location.Transformation());
                        const double scale=units=="in" ? 1.0/25.4 : 1.0;
                        return gp_Pnt(float(point.X()*scale),float(point.Y()*scale),float(point.Z()*scale));
                    };
                    const auto pa=outputPoint(a),pb=outputPoint(b),pc=outputPoint(c);
                    if(gp_Vec(pa,pb).Crossed(gp_Vec(pa,pc)).SquareMagnitude()==0){++collapsedTriangles;continue;}
                    retained.push_back(triangle);
                }
                if(retained.empty())reject("mesh_failed","A face collapses at binary STL coordinate precision");
                if(int(retained.size())!=mesh->NbTriangles()) {
                    mesh->ResizeTriangles(int(retained.size()),false);
                    for(size_t index=0;index<retained.size();++index)mesh->SetTriangle(int(index)+1,retained[index]);
                }
            }
            triangles+=mesh->NbTriangles();++faces;
        }
        builder.Add(compound,detached);
    }
    if(!faces || !triangles)reject("mesh_failed","Export tessellation is empty");
    TopoDS_Shape shape=compound;
    if(units=="in") {
        gp_Trsf scale;scale.SetScale(gp_Pnt(0,0,0),1.0/25.4);
        shape=BRepBuilderAPI_Transform(shape,scale,true,true).Shape();
    }

    // The OCCT writer takes a filename. Keep its partial output private, then
    // atomically replace the requested destination only after successful writing.
    QTemporaryFile staging(destination.dir().filePath(".alloy-stl-XXXXXX"));
    if(!staging.open())reject("export_failed",staging.errorString());
    const auto stagingPath=staging.fileName();staging.close();
    StlAPI_Writer writer;writer.ASCIIMode()=!binary;
    if(!writer.Write(shape,stagingPath.toUtf8().constData()))reject("export_failed","STL writer failed");
    QFile input(stagingPath);
    if(!input.open(QIODevice::ReadOnly) || input.size()==0)reject("export_failed","STL writer produced no readable output");
    // Validate actual serialized binary facets as well as the detached mesh.
    // The prior cross-product-only check can miss identical pole vertices due
    // to floating-point cancellation; no area tolerance or CAD healing is used.
    quint32 serializedTriangles=0;quint32 writtenTriangles=0;qint64 serializedCollapsed=0;
    QByteArray binaryHeader;
    if(binary) {
        binaryHeader=input.read(84);
        if(binaryHeader.size()!=84)reject("export_failed","Binary STL writer produced a truncated header");
        serializedTriangles=qFromLittleEndian<quint32>(binaryHeader.constData()+80);
        if(serializedTriangles==0 || input.size()!=84+qint64(serializedTriangles)*50)
            reject("export_failed","Binary STL writer produced an inconsistent facet count");
    }
    QSaveFile output(path);output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly))reject("export_failed",output.errorString());
    if(binary) {
        if(output.write(binaryHeader)!=84)reject("export_failed",output.errorString());
        quint32 readTriangles=0;
        while(readTriangles<serializedTriangles) {
            const auto records=std::min<quint32>(serializedTriangles-readTriangles,20000);
            const auto chunk=input.read(qint64(records)*50);
            if(chunk.size()!=qint64(records)*50)reject("export_failed","Binary STL facet read failed");
            QByteArray retained;retained.reserve(chunk.size());
            for(quint32 index=0;index<records;++index) {
                const auto* facet=chunk.constData()+qint64(index)*50;
                double vertices[3][3];
                for(int point=0;point<3;++point)for(int axis=0;axis<3;++axis) {
                    const auto bits=qFromLittleEndian<quint32>(facet+12+(point*3+axis)*4);
                    const auto coordinate=std::bit_cast<float>(bits);
                    if(!std::isfinite(coordinate))reject("export_failed","Binary STL contains a nonfinite vertex");
                    vertices[point][axis]=coordinate;
                }
                const gp_Pnt a(vertices[0][0],vertices[0][1],vertices[0][2]),
                    b(vertices[1][0],vertices[1][1],vertices[1][2]),
                    c(vertices[2][0],vertices[2][1],vertices[2][2]);
                const auto sameVertex=[&](int first,int second) {
                    return vertices[first][0]==vertices[second][0] && vertices[first][1]==vertices[second][1]
                        && vertices[first][2]==vertices[second][2];
                };
                // Check identical serialized vertices directly. A fused
                // multiply-add cross product of equal vectors can retain a
                // tiny rounding residue instead of returning exact zero.
                const auto cross=gp_Vec(a,b).Crossed(gp_Vec(a,c));
                if(sameVertex(0,1) || sameVertex(1,2) || sameVertex(0,2)
                    || cross.SquareMagnitude()==0) {++serializedCollapsed;continue;}
                // OCCT computed its normal before float32 vertex rounding.
                // Derive the binary normal from the actual serialized winding,
                // preserving all 38 coordinate/attribute bytes without healing.
                const double length=cross.Magnitude();char normal[12];
                const double components[3]{cross.X()/length,cross.Y()/length,cross.Z()/length};
                for(int axis=0;axis<3;++axis)
                    qToLittleEndian(std::bit_cast<quint32>(static_cast<float>(components[axis])),normal+axis*4);
                retained.append(normal,12);retained.append(facet+12,38);++writtenTriangles;
            }
            if(output.write(retained)!=retained.size())reject("export_failed",output.errorString());
            readTriangles+=records;
        }
        if(writtenTriangles==0)reject("mesh_failed","All facets collapse in serialized binary STL");
        char count[4];qToLittleEndian(writtenTriangles,count);
        if(!output.seek(80) || output.write(count,4)!=4)reject("export_failed",output.errorString());
        triangles=writtenTriangles;collapsedTriangles+=serializedCollapsed;
    } else {
        // Parse only the OCCT writer's staged ASCII records. Preserve each
        // retained facet's original text and normal; remove exact collapses
        // at the printed coordinate precision without adding a tolerance.
        const auto header=input.readLine();
        if(!header.trimmed().startsWith("solid"))reject("export_failed","ASCII STL writer produced an invalid header");
        QByteArray retained=header;qint64 asciiTriangles=0;bool ended=false;
        const auto flush=[&]{
            if(output.write(retained)!=retained.size())reject("export_failed",output.errorString());
            retained.clear();
        };
        while(!input.atEnd()) {
            auto facet=input.readLine();const auto first=facet.trimmed();
            if(first.startsWith("endsolid")) {retained.append(facet);ended=true;break;}
            if(!first.startsWith("facet normal "))reject("export_failed","ASCII STL writer produced an invalid facet");
            const auto outer=input.readLine();facet.append(outer);
            if(outer.trimmed()!="outer loop")reject("export_failed","ASCII STL facet is missing its loop");
            double vertices[3][3];
            for(int point=0;point<3;++point) {
                const auto line=input.readLine();facet.append(line);const auto fields=line.simplified().split(' ');
                if(fields.size()!=4 || fields[0]!="vertex")reject("export_failed","ASCII STL facet requires three vertices");
                for(int axis=0;axis<3;++axis) {
                    bool ok=false;const auto coordinate=fields[axis+1].toDouble(&ok);
                    if(!ok || !std::isfinite(coordinate))reject("export_failed","ASCII STL contains an invalid vertex");
                    vertices[point][axis]=coordinate;
                }
            }
            const auto endLoop=input.readLine(),endFacet=input.readLine();facet.append(endLoop);facet.append(endFacet);
            if(endLoop.trimmed()!="endloop" || endFacet.trimmed()!="endfacet")reject("export_failed","ASCII STL writer produced a truncated facet");
            const auto sameVertex=[&](int a,int b){return vertices[a][0]==vertices[b][0] && vertices[a][1]==vertices[b][1] && vertices[a][2]==vertices[b][2];};
            const gp_Pnt a(vertices[0][0],vertices[0][1],vertices[0][2]),
                b(vertices[1][0],vertices[1][1],vertices[1][2]),c(vertices[2][0],vertices[2][1],vertices[2][2]);
            if(sameVertex(0,1) || sameVertex(1,2) || sameVertex(0,2)
                || gp_Vec(a,b).Crossed(gp_Vec(a,c)).SquareMagnitude()==0) {++serializedCollapsed;continue;}
            retained.append(facet);++asciiTriangles;
            if(retained.size()>=1024*1024)flush();
        }
        if(!ended || !input.atEnd() || input.error()!=QFileDevice::NoError)reject("export_failed","ASCII STL writer produced an incomplete solid");
        if(asciiTriangles==0)reject("mesh_failed","All facets collapse in serialized ASCII STL");
        flush();triangles=asciiTriangles;collapsedTriangles+=serializedCollapsed;
    }
    const auto outputBytes=output.size();
    if(!output.commit())reject("export_failed",output.errorString());
    QJsonObject result{{"path",destination.absoluteFilePath()},{"format",binary ? "stlBinary" : "stlAscii"},
        {"bodyIds",bodyIds},{"bodyCount",int(bodies.size())},{"units",units},{"unitsEmbedded",false},
        {"surfaceDeviationMm",surfaceDeviationMm},{"normalDeviationDeg",normalDeviationDeg},
        {"triangles",triangles},{"collapsedTrianglesRemoved",collapsedTriangles},{"serializedCollapsedTrianglesRemoved",serializedCollapsed},{"faces",faces},{"bytes",outputBytes},
        {"mesherStatus",mesherStatus},{"authoredChanged",false}};
    if(bodies.size()==1)result["bodyId"]=bodies.front().bodyId;
    return result;
}
