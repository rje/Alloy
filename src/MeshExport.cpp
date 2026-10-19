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
#include <array>
#include <map>
#include <limits>
#include <lib3mf_implicit.hpp>
#include <Standard_Failure.hxx>

namespace {
[[noreturn]] void reject(const QString& code,const QString& message) {
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
}

QJsonObject MeshExport::writeStl(const EvaluatedBody& body,const QString& path,
    const QString& units,bool binary,double surfaceDeviationMm,double normalDeviationDeg,const std::function<bool()>& approvePublication) {
    return writeStl(std::vector<EvaluatedBody>{body},path,units,binary,surfaceDeviationMm,normalDeviationDeg,approvePublication);
}

QJsonObject MeshExport::writeStl(const std::vector<EvaluatedBody>& bodies,const QString& path,
    const QString& units,bool binary,double surfaceDeviationMm,double normalDeviationDeg,const std::function<bool()>& approvePublication) {
    if(units!="mm" && units!="in")reject("invalid_units","STL output units must be mm or in");
    if(!std::isfinite(surfaceDeviationMm) || surfaceDeviationMm<0.001 || surfaceDeviationMm>10
        || !std::isfinite(normalDeviationDeg) || normalDeviationDeg<1 || normalDeviationDeg>45)
        reject("invalid_mesh_settings","Surface deviation must be 0.001–10 mm and normal deviation 1–45 degrees");
    const QFileInfo destination(path);
    if(!destination.isAbsolute() || destination.fileName().isEmpty() || !destination.dir().exists())
        reject("invalid_path","Choose an absolute STL path in an existing directory");
    if(bodies.empty())reject("solid_required","Select at least one valid closed solid");
    QSet<QString> distinct;QJsonArray bodyIds,instances;
    for(const auto& body:bodies) {
        const auto key=body.occurrenceId+":"+body.bodyId;
        if(body.bodyId.isEmpty() || distinct.contains(key))reject("invalid_body_selection","Select distinct body instances");
        distinct.insert(key);bodyIds.append(body.bodyId);
        instances.append(QJsonObject{{"occurrenceId",body.occurrenceId},{"definitionId",body.definitionId},{"bodyId",body.bodyId},{"boundsMm",body.summary["boundsMm"]}});
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
    if(approvePublication && !approvePublication())reject("export_cancelled","STL publication cancelled; destination preserved");
    if(!output.commit())reject("export_failed",output.errorString());
    QJsonObject result{{"path",destination.absoluteFilePath()},{"format",binary ? "stlBinary" : "stlAscii"},
        {"bodyIds",bodyIds},{"instances",instances},{"bodyCount",int(bodies.size())},{"units",units},{"unitsEmbedded",false},
        {"surfaceDeviationMm",surfaceDeviationMm},{"normalDeviationDeg",normalDeviationDeg},
        {"triangles",triangles},{"collapsedTrianglesRemoved",collapsedTriangles},{"serializedCollapsedTrianglesRemoved",serializedCollapsed},{"faces",faces},{"bytes",outputBytes},
        {"mesherStatus",mesherStatus},{"authoredChanged",false}};
    if(bodies.size()==1)result["bodyId"]=bodies.front().bodyId;
    return result;
}

namespace {
struct CoreMesh {
    std::vector<Lib3MF::sPosition> vertices;
    std::vector<Lib3MF::sTriangle> triangles;
    qint64 collapsed=0;
    int faces=0,status=0;
};

CoreMesh coreMesh(const EvaluatedBody& body,const gp_Trsf& placement,double deviation,double angle) {
    const auto summary=GeometryKernel::summarize(body.shape);
    if(!summary.value("valid").toBool() || !summary.value("closed").toBool()
        || summary.value("topology").toObject().value("solids").toInt()!=1
        || summary.value("volumeMm3").toDouble()<=0)
        reject("solid_required","Each selected export body must be one valid closed solid: "+body.bodyId);
    BRepBuilderAPI_Copy copy(body.shape,true,false);auto detached=copy.Shape();
    BRepMesh_IncrementalMesh mesher(detached,deviation,false,angle*std::numbers::pi/180.0,false);
    if(!mesher.IsDone())reject("mesh_failed","Indexed export tessellation did not complete");
    CoreMesh result;result.status=mesher.GetStatusFlags();
    // A fixed sub-micron weld joins shared BRep face seams, including analytic
    // poles. It never grows with mesh deviation; final indexed manifold checks
    // reject any hole or inconsistent seam instead of healing source geometry.
    constexpr double weldMm=1e-6;
    using Cell=std::array<qint64,3>;
    std::map<Cell,std::vector<quint32>> cells;
    const auto inverse=placement.Inverted();
    const auto vertex=[&](gp_Pnt point) {
        point.Transform(inverse);
        Lib3MF::sPosition encoded{{float(point.X()),float(point.Y()),float(point.Z())}};
        Cell cell;
        for(int axis=0;axis<3;++axis) {
            const double value=encoded.m_Coordinates[axis];
            if(!std::isfinite(value) || std::abs(value/weldMm)>double(std::numeric_limits<qint64>::max())-4)
                reject("mesh_failed","Indexed mesh vertex exceeds supported finite coordinate range");
            cell[axis]=qint64(std::floor(value/weldMm));
        }
        for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z) {
            const auto found=cells.find(Cell{cell[0]+x,cell[1]+y,cell[2]+z});
            if(found==cells.end())continue;
            for(const auto index:found->second) {
                const auto& other=result.vertices[index];double distance2=0;
                for(int axis=0;axis<3;++axis)distance2+=std::pow(double(encoded.m_Coordinates[axis])-other.m_Coordinates[axis],2);
                if(distance2<=weldMm*weldMm)return index;
            }
        }
        if(result.vertices.size()>=std::numeric_limits<quint32>::max())reject("mesh_failed","Indexed mesh vertex count exceeds Core index range");
        const auto index=quint32(result.vertices.size());result.vertices.push_back(encoded);cells[cell].push_back(index);return index;
    };
    for(TopExp_Explorer it(detached,TopAbs_FACE);it.More();it.Next()) {
        const auto face=TopoDS::Face(it.Current());TopLoc_Location location;
        const auto mesh=BRep_Tool::Triangulation(face,location);
        if(mesh.IsNull() || mesh->NbTriangles()==0)reject("mesh_failed","A selected solid face has no export triangles");
        ++result.faces;std::vector<quint32> indices(mesh->NbNodes()+1);
        for(int node=1;node<=mesh->NbNodes();++node)indices[node]=vertex(mesh->Node(node).Transformed(location.Transformation()));
        for(int index=1;index<=mesh->NbTriangles();++index) {
            int a,b,c;mesh->Triangle(index).Get(a,b,c);
            if(face.Orientation()==TopAbs_REVERSED)std::swap(b,c);
            Lib3MF::sTriangle triangle{{indices[a],indices[b],indices[c]}};
            const auto p=[&](quint32 i){const auto& v=result.vertices[i];return gp_Pnt(v.m_Coordinates[0],v.m_Coordinates[1],v.m_Coordinates[2]);};
            const auto cross=gp_Vec(p(triangle.m_Indices[0]),p(triangle.m_Indices[1])).Crossed(gp_Vec(p(triangle.m_Indices[0]),p(triangle.m_Indices[2])));
            if(triangle.m_Indices[0]==triangle.m_Indices[1] || triangle.m_Indices[1]==triangle.m_Indices[2]
                || triangle.m_Indices[0]==triangle.m_Indices[2] || cross.SquareMagnitude()==0) {++result.collapsed;continue;}
            result.triangles.push_back(triangle);
        }
    }
    if(result.triangles.empty())reject("mesh_failed","Indexed export tessellation is empty");
    struct Edge {int count=0,balance=0;};std::map<std::pair<quint32,quint32>,Edge> edges;
    // Signed volume relative to a nearby origin avoids cancellation for bodies
    // placed far from the document origin. Cavity shell orientation is retained.
    const auto& origin=result.vertices.front();double volume6=0;
    for(const auto& triangle:result.triangles) {
        gp_XYZ points[3];
        for(int i=0;i<3;++i) {
            const auto a=triangle.m_Indices[i],b=triangle.m_Indices[(i+1)%3];
            auto& edge=edges[{std::min(a,b),std::max(a,b)}];++edge.count;edge.balance+=a<b?1:-1;
            const auto& p=result.vertices[a];points[i]=gp_XYZ(double(p.m_Coordinates[0])-origin.m_Coordinates[0],double(p.m_Coordinates[1])-origin.m_Coordinates[1],double(p.m_Coordinates[2])-origin.m_Coordinates[2]);
        }
        volume6+=points[0].Dot(points[1].Crossed(points[2]));
    }
    for(const auto& [key,edge]:edges)if(edge.count!=2 || edge.balance!=0)
        reject("mesh_failed","Indexed mesh tessellation is not a consistently oriented watertight indexed mesh: "+body.bodyId);
    if(!std::isfinite(volume6) || volume6==0)reject("mesh_failed","Indexed export mesh has no finite enclosed volume");
    if(volume6<0)for(auto& triangle:result.triangles)std::swap(triangle.m_Indices[1],triangle.m_Indices[2]);
    return result;
}
}

QJsonObject MeshExport::write3mf(const std::vector<EvaluatedBody>& bodies,
    const std::vector<gp_Trsf>& buildTransforms,const QString& path,double surfaceDeviationMm,
    double normalDeviationDeg,const std::function<bool()>& approvePublication) {
    if(!std::isfinite(surfaceDeviationMm) || surfaceDeviationMm<0.001 || surfaceDeviationMm>10
        || !std::isfinite(normalDeviationDeg) || normalDeviationDeg<1 || normalDeviationDeg>45)
        reject("invalid_mesh_settings","Surface deviation must be 0.001–10 mm and normal deviation 1–45 degrees");
    const QFileInfo destination(path);
    if(!destination.isAbsolute() || destination.fileName().isEmpty() || !destination.dir().exists())
        reject("invalid_path","Choose an absolute 3MF path in an existing directory");
    if(bodies.empty())reject("solid_required","Select at least one valid closed solid");
    if(buildTransforms.size()!=bodies.size())reject("invalid_transform","Each 3MF body requires exactly one build transform");
    QSet<QString> distinct;QJsonArray bodyIds,instances;std::vector<Lib3MF_uint8> bytes;
    qint64 triangles=0,vertices=0,collapsed=0;int faces=0,mesherStatus=0;
    try {
        const auto wrapper=Lib3MF::CWrapper::loadLibrary();const auto model=wrapper->CreateModel();
        model->SetUnit(Lib3MF::eModelUnit::MilliMeter);
        for(size_t index=0;index<bodies.size();++index) {
            const auto& body=bodies[index];const auto& placement=buildTransforms[index];
            const auto key=body.occurrenceId+":"+body.bodyId;
            if(body.bodyId.isEmpty() || distinct.contains(key))reject("invalid_body_selection","Select distinct body instances");
            distinct.insert(key);bodyIds.append(body.bodyId);
            // Occurrence placements are rigid, proper transforms. Reflections or
            // scale would change local winding/physical units and are rejected.
            if(placement.IsNegative() || std::abs(placement.ScaleFactor()-1)>1e-10)
                reject("invalid_transform","3MF build placements must be proper rigid transforms");
            Lib3MF::sTransform transform{};QJsonArray transformJson;
            // OCCT uses column vectors; Core 3MF stores row-vector 4x3 fields.
            for(int row=0;row<4;++row)for(int column=0;column<3;++column) {
                const float value=float(placement.Value(column+1,row+1));
                if(!std::isfinite(value))reject("invalid_transform","3MF build transform must be finite");
                transform.m_Fields[row][column]=value;transformJson.append(double(value));
            }
            const auto mesh=coreMesh(body,placement,surfaceDeviationMm,normalDeviationDeg);
            auto object=model->AddMeshObject();object->SetName((body.name.isEmpty()?body.bodyId:body.name).toStdString());
            object->SetGeometry(mesh.vertices,mesh.triangles);
            if(!object->IsManifoldAndOriented())reject("mesh_failed","3MF library rejected mesh orientation or manifold edges");
            model->AddBuildItem(std::static_pointer_cast<Lib3MF::CObject>(object),transform);
            triangles+=qint64(mesh.triangles.size());vertices+=qint64(mesh.vertices.size());collapsed+=mesh.collapsed;
            faces+=mesh.faces;mesherStatus|=mesh.status;
            instances.append(QJsonObject{{"occurrenceId",body.occurrenceId},{"definitionId",body.definitionId},{"bodyId",body.bodyId},
                {"boundsMm",body.summary.value("boundsMm")},{"buildTransform",transformJson},
                {"triangles",qint64(mesh.triangles.size())},{"vertices",qint64(mesh.vertices.size())}});
        }
        model->QueryWriter("3mf")->WriteToBuffer(bytes);
    } catch(const Lib3MF::ELib3MFException& error) {
        reject("export_failed","3MF writer: "+QString::fromUtf8(error.what()));
    } catch(const Standard_Failure& error) {
        reject("mesh_failed","3MF tessellation: "+QString::fromUtf8(error.GetMessageString()));
    }
    if(bytes.empty())reject("export_failed","3MF writer produced no output");
    QSaveFile output(path);output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly))reject("export_failed",output.errorString());
    if(output.write(reinterpret_cast<const char*>(bytes.data()),qint64(bytes.size()))!=qint64(bytes.size()))
        reject("export_failed",output.errorString());
    if(approvePublication && !approvePublication())reject("export_cancelled","3MF publication cancelled; destination preserved");
    if(!output.commit())reject("export_failed",output.errorString());
    QJsonObject result{{"path",destination.absoluteFilePath()},{"format","3mf"},{"bodyIds",bodyIds},{"instances",instances},
        {"bodyCount",int(bodies.size())},{"objectCount",int(bodies.size())},{"buildItemCount",int(bodies.size())},
        {"units","mm"},{"unitsEmbedded",true},{"surfaceDeviationMm",surfaceDeviationMm},{"normalDeviationDeg",normalDeviationDeg},
        {"triangles",triangles},{"vertices",vertices},{"collapsedTrianglesRemoved",collapsed},{"faces",faces},
        {"bytes",qint64(bytes.size())},{"mesherStatus",mesherStatus},{"authoredChanged",false}};
    if(bodies.size()==1)result["bodyId"]=bodies.front().bodyId;
    return result;
}

QJsonObject MeshExport::writeObj(const std::vector<EvaluatedBody>& bodies,const QString& path,
    const QString& units,double surfaceDeviationMm,double normalDeviationDeg,
    const std::function<bool()>& approvePublication) {
    if(units!="mm" && units!="in")reject("invalid_units","OBJ output units must be mm or in");
    if(!std::isfinite(surfaceDeviationMm) || surfaceDeviationMm<0.001 || surfaceDeviationMm>10
        || !std::isfinite(normalDeviationDeg) || normalDeviationDeg<1 || normalDeviationDeg>45)
        reject("invalid_mesh_settings","Surface deviation must be 0.001–10 mm and normal deviation 1–45 degrees");
    const QFileInfo destination(path);
    if(!destination.isAbsolute() || destination.fileName().isEmpty() || !destination.dir().exists())
        reject("invalid_path","Choose an absolute OBJ path in an existing directory");
    if(bodies.empty())reject("solid_required","Select at least one valid closed solid");
    QSaveFile output(path);output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly))reject("export_failed",output.errorString());
    const auto write=[&](const QByteArray& bytes){if(output.write(bytes)!=bytes.size())reject("export_failed",output.errorString());};
    write("# Alloy triangle mesh\n# Coordinates: "+units.toLatin1()+"; world axes, Z up\n");
    QSet<QString> distinct;QJsonArray bodyIds,instances;
    qint64 triangles=0,vertices=0,collapsed=0;int faces=0,status=0;
    const double scale=units=="in" ? 1.0/25.4 : 1.0;
    try {
        for(size_t index=0;index<bodies.size();++index) {
            const auto& body=bodies[index];const auto key=body.occurrenceId+":"+body.bodyId;
            if(body.bodyId.isEmpty() || distinct.contains(key))reject("invalid_body_selection","Select distinct body instances");
            distinct.insert(key);bodyIds.append(body.bodyId);
            // Reuse the validated indexed mesh and fixed sub-micron seam weld.
            // Identity placement retains world coordinates; groups never weld
            // across separate bodies. Generated ASCII identifiers cannot inject
            // OBJ records through user-authored names.
            const auto mesh=coreMesh(body,gp_Trsf(),surfaceDeviationMm,normalDeviationDeg);
            const auto group=QByteArray("body_")+QByteArray::number(index+1);
            write("g "+group+"\n");
            for(const auto& vertex:mesh.vertices) {
                QByteArray line="v";
                for(int axis=0;axis<3;++axis)line+=' '+QByteArray::number(double(vertex.m_Coordinates[axis])*scale,'g',17);
                write(line+'\n');
            }
            for(const auto& triangle:mesh.triangles) {
                QByteArray line="f";
                for(int axis=0;axis<3;++axis)line+=' '+QByteArray::number(vertices+triangle.m_Indices[axis]+1);
                write(line+'\n');
            }
            instances.append(QJsonObject{{"occurrenceId",body.occurrenceId},{"definitionId",body.definitionId},{"bodyId",body.bodyId},
                {"group",QString::fromLatin1(group)},{"boundsMm",body.summary.value("boundsMm")},
                {"triangles",qint64(mesh.triangles.size())},{"vertices",qint64(mesh.vertices.size())}});
            vertices+=qint64(mesh.vertices.size());triangles+=qint64(mesh.triangles.size());collapsed+=mesh.collapsed;
            faces+=mesh.faces;status|=mesh.status;
        }
    } catch(const Standard_Failure& error){reject("mesh_failed","OBJ tessellation: "+QString::fromUtf8(error.GetMessageString()));}
    const auto bytes=output.size();
    if(approvePublication && !approvePublication())reject("export_cancelled","OBJ publication cancelled; destination preserved");
    if(!output.commit())reject("export_failed",output.errorString());
    QJsonObject result{{"path",destination.absoluteFilePath()},{"format","obj"},{"units",units},{"unitsEmbedded",false},
        {"axisConvention","worldZUp"},{"materials",false},{"groupCount",int(bodies.size())},{"bodyCount",int(bodies.size())},
        {"bodyIds",bodyIds},{"instances",instances},{"surfaceDeviationMm",surfaceDeviationMm},{"normalDeviationDeg",normalDeviationDeg},
        {"vertices",vertices},{"triangles",triangles},{"faces",faces},{"mesherStatus",status},
        {"collapsedTrianglesRemoved",collapsed},{"bytes",bytes},{"authoredChanged",false}};
    if(bodies.size()==1)result["bodyId"]=bodies.front().bodyId;
    return result;
}
