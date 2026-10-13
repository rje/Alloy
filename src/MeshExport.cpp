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
    qint64 triangles=0;int faces=0;int mesherStatus=0;
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
    QSaveFile output(path);output.setDirectWriteFallback(false);
    if(!output.open(QIODevice::WriteOnly))reject("export_failed",output.errorString());
    while(!input.atEnd()) {
        const auto chunk=input.read(1024*1024);
        if(chunk.isEmpty() && input.error()!=QFileDevice::NoError)reject("export_failed",input.errorString());
        if(output.write(chunk)!=chunk.size())reject("export_failed",output.errorString());
    }
    if(!output.commit())reject("export_failed",output.errorString());
    QJsonObject result{{"path",destination.absoluteFilePath()},{"format",binary ? "stlBinary" : "stlAscii"},
        {"bodyIds",bodyIds},{"bodyCount",int(bodies.size())},{"units",units},{"unitsEmbedded",false},
        {"surfaceDeviationMm",surfaceDeviationMm},{"normalDeviationDeg",normalDeviationDeg},
        {"triangles",triangles},{"faces",faces},{"bytes",input.size()},
        {"mesherStatus",mesherStatus},{"authoredChanged",false}};
    if(bodies.size()==1)result["bodyId"]=bodies.front().bodyId;
    return result;
}
