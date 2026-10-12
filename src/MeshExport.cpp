#include "MeshExport.h"
#include <BRepBuilderAPI_Copy.hxx>
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
    if(units!="mm" && units!="in")reject("invalid_units","STL output units must be mm or in");
    if(!std::isfinite(surfaceDeviationMm) || surfaceDeviationMm<0.001 || surfaceDeviationMm>10
        || !std::isfinite(normalDeviationDeg) || normalDeviationDeg<1 || normalDeviationDeg>45)
        reject("invalid_mesh_settings","Surface deviation must be 0.001–10 mm and normal deviation 1–45 degrees");
    const QFileInfo destination(path);
    if(!destination.isAbsolute() || destination.fileName().isEmpty() || !destination.dir().exists())
        reject("invalid_path","Choose an absolute STL path in an existing directory");
    const auto summary=GeometryKernel::summarize(body.shape);
    if(!summary.value("valid").toBool() || !summary.value("closed").toBool()
        || summary.value("topology").toObject().value("solids").toInt()!=1
        || summary.value("volumeMm3").toDouble()<=0)
        reject("solid_required","This export supports one valid closed solid");

    // Copy both topology and geometry, excluding existing display triangulation.
    // Meshing always uses canonical mm before scaling the detached mesh to output units.
    BRepBuilderAPI_Copy copy(body.shape,true,false);
    auto shape=copy.Shape();
    BRepMesh_IncrementalMesh mesher(shape,surfaceDeviationMm,false,
        normalDeviationDeg*std::numbers::pi/180.0,false);
    if(!mesher.IsDone())reject("mesh_failed","Export tessellation did not complete");
    qint64 triangles=0;int faces=0;
    for(TopExp_Explorer it(shape,TopAbs_FACE);it.More();it.Next()) {
        TopLoc_Location location;
        const auto mesh=BRep_Tool::Triangulation(TopoDS::Face(it.Current()),location);
        if(mesh.IsNull() || mesh->NbTriangles()==0)reject("mesh_failed","A solid face has no export triangles");
        triangles+=mesh->NbTriangles();++faces;
    }
    if(!faces || !triangles)reject("mesh_failed","Export tessellation is empty");
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
    return {{"path",destination.absoluteFilePath()},{"format",binary ? "stlBinary" : "stlAscii"},
        {"bodyId",body.bodyId},{"units",units},{"unitsEmbedded",false},
        {"surfaceDeviationMm",surfaceDeviationMm},{"normalDeviationDeg",normalDeviationDeg},
        {"triangles",triangles},{"faces",faces},{"bytes",input.size()},
        {"mesherStatus",mesher.GetStatusFlags()},{"authoredChanged",false}};
}
