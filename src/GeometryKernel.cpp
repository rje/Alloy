#include "Scale.h"
#include "Align.h"
#include "BodyCopy.h"
#include "PlanarFaces.h"
#include "PrimitiveTopology.h"
#include "SolidFillet.h"
#include "SolidChamfer.h"
#include "SolidShell.h"
#include "SolidFaceOffset.h"
#include "SolidDraft.h"
#include "ClosedBlendShell.h"
#include "SolidSplit.h"
#include "SolidSphere.h"
#include "SolidHole.h"
#include "SolidThreadGeometry.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepTools.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include "SolidLoft.h"
#include "SurfaceExtrude.h"
#include "SurfaceRevolve.h"
#include "SurfaceOffset.h"
#include "SurfaceRuled.h"
#include "SurfaceSweep.h"
#include "SurfacePatch.h"
#include "BoundaryFill.h"
#include "SurfaceTrim.h"
#include "SolidThicken.h"
#include "SolidRevolve.h"
#include "SolidSweep.h"
#include "FeaturePattern.h"
#include "JoinRootEdges.h"
#include "SolidMirror.h"
#include "CircularPattern.h"
#include "RectangularBodyPattern.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include "GeometryKernel.h"
#include "LocalReference.h"
#include "SolidFillet.h"
#include "SketchGeometry.h"
#include "RoundedLoopOffset.h"
#include "StepExchange.h"
#include <BRepBndLib.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <array>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepOffsetAPI_DraftAngle.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <gp_Pnt2d.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Vec.hxx>
#include <gp_Circ.hxx>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QCryptographicHash>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace {
class ValidatedBoolean final : public BRepAlgoAPI_BooleanOperation {
public:
    QJsonObject simplificationStatus;
    void simplifyKeepingValid() {
        // Cleanup may invalidate an otherwise valid Boolean. Test it on an
        // isolated copy first; vetoing cleanup preserves raw topology and its
        // original history, including unchanged retained boundary identities.
        const auto rawShape=Shape();
        const bool rawValid=!rawShape.IsNull() && BRepCheck_Analyzer(rawShape).IsValid();
        simplificationStatus["rawValidBefore"]=rawValid;
        if(rawValid) {
            BRepBuilderAPI_Copy copier(rawShape,true,false);
            // Match OCCT 7.9 SimplifyResult's unifier configuration, including
            // spline concatenation. This is a validity veto, not equivalence proof.
            ShapeUpgrade_UnifySameDomain preflight(copier.Shape(),true,true,true);
            preflight.SetLinearTolerance(myFuzzyValue);
            preflight.SetAngularTolerance(1e-12);
            preflight.SetSafeInputMode(myNonDestructive);
            preflight.AllowInternalEdges(false);
            preflight.Build();
            const bool preflightValid=!preflight.Shape().IsNull()
                && BRepCheck_Analyzer(preflight.Shape()).IsValid();
            simplificationStatus["preflightValid"]=preflightValid;
            simplificationStatus["cleanupVetoed"]=!preflightValid;
            if(!preflightValid)return;
        }
        // Preserve the ordinary successful cleanup and its native history.
        // If its outcome differs from the preflight, final validity still refuses it.
        SimplifyResult(true,true,1e-12);
    }
};
[[noreturn]] void reject(const QString& code,const QString& message) {throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
int count(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind) {
    TopTools_IndexedMapOfShape map; TopExp::MapShapes(shape,kind,map); return map.Extent();
}
void importedTopology(const TopoDS_Shape& shape,const QString& featureId,const QString& bodyId,
    const QString& occurrenceId,std::vector<EvaluatedSupportFace>& faces,std::vector<EvaluatedSolidEdge>& edges) {
    // The embedded source hash fixes this XDE topology snapshot. IDs include the
    // full occurrence identity, so two references to one definition never alias.
    auto id=[&](const char* kind,int index){return QUuid::createUuidV5(QUuid(featureId),
        QByteArray("step-xde/1:")+occurrenceId.toUtf8()+":"+kind+":"+QByteArray::number(index))
        .toString(QUuid::WithoutBraces);};
    auto xyz=[](const gp_Pnt& p){return QJsonArray{p.X(),p.Y(),p.Z()};};
    auto direction=[](const gp_Dir& d){return QJsonArray{d.X(),d.Y(),d.Z()};};
    TopTools_IndexedMapOfShape allEdges;TopExp::MapShapes(shape,TopAbs_EDGE,allEdges);
    for(int i=1;i<=allEdges.Extent();++i) {
        const auto edge=TopoDS::Edge(allEdges(i));
        const BRepAdaptor_Curve curve(edge);
        QJsonObject provenance{{"role","imported.edge"},{"occurrenceId",occurrenceId},{"sourceEdgeIndex",i}};
        if(curve.GetType()==GeomAbs_Line){provenance["startMm"]=xyz(curve.Value(curve.FirstParameter()));provenance["endMm"]=xyz(curve.Value(curve.LastParameter()));}
        edges.push_back({id("edge",i),edge,provenance});
    }
    TopTools_IndexedMapOfShape allFaces;TopExp::MapShapes(shape,TopAbs_FACE,allFaces);
    for(int i=1;i<=allFaces.Extent();++i) {
        const auto face=TopoDS::Face(allFaces(i));const BRepAdaptor_Surface surface(face);
        if(surface.GetType()!=GeomAbs_Plane)continue;
        const auto plane=surface.Plane();auto normal=plane.Axis().Direction();
        if(face.Orientation()==TopAbs_REVERSED)normal.Reverse();
        auto u=plane.XAxis().Direction();auto v=gp_Dir(gp_Vec(normal).Crossed(gp_Vec(u)));
        EvaluatedSupportFace support{id("face",i),bodyId,featureId,"imported.planar",face,
            QJsonObject{{"origin",xyz(plane.Location())},{"uAxis",direction(u)},
                {"vAxis",direction(v)},{"normal",direction(normal)}},{},
            QJsonObject{{"occurrenceId",occurrenceId},{"sourceFaceIndex",i}}};
        TopTools_IndexedMapOfShape boundary;TopExp::MapShapes(face,TopAbs_EDGE,boundary);
        for(int j=1;j<=boundary.Extent();++j) {
            const auto edge=TopoDS::Edge(boundary(j));const auto index=allEdges.FindIndex(edge);
            if(index<1)reject("step_topology","Imported face edge is outside its leaf solid");
            const auto& named=edges.at(static_cast<size_t>(index-1));
            support.boundaryEdges.push_back({named.edgeId,
                BRepAdaptor_Curve(edge).GetType()==GeomAbs_Line?"line":"curve",edge,named.provenance});
        }
        faces.push_back(support);
    }
}
double length(const QJsonObject& parameters, const QString& key, bool placement=false) {
    const auto value=parameters.value(key);
    if (!value.isDouble() || !std::isfinite(value.toDouble()) || std::abs(value.toDouble())>100000
        || (!placement && value.toDouble()<0.001)) throw std::invalid_argument(("Unsupported length: "+key).toStdString());
    return value.toDouble();
}
}

GeometryKernel::EvaluationError::EvaluationError(const QJsonObject& diagnostics)
    : std::runtime_error(diagnostics["message"].toString().toStdString()), diagnostics_(diagnostics) {}

TopoDS_Shape GeometryKernel::primitive(const QString& type, const QJsonObject& p) {
    const gp_Pnt origin(length(p,"xMm",true),length(p,"yMm",true),length(p,"zMm",true));
    TopoDS_Shape shape;
    if (type=="box") shape=BRepPrimAPI_MakeBox(origin,length(p,"widthMm"),length(p,"depthMm"),length(p,"heightMm")).Shape();
    else if (type=="cylinder") shape=BRepPrimAPI_MakeCylinder(gp_Ax2(origin,gp::DZ()),length(p,"radiusMm"),length(p,"heightMm")).Shape();
    else if (type=="cone") {
        const double base=length(p,"baseDiameterMm")/2,top=length(p,"topDiameterMm",true)/2;
        if(top<0)throw std::invalid_argument("Top diameter must be nonnegative");
        const double height=length(p,"heightMm");
        if(base==top)shape=BRepPrimAPI_MakeCylinder(gp_Ax2(origin,gp::DZ()),base,height).Shape();
        else shape=BRepPrimAPI_MakeCone(gp_Ax2(origin,gp::DZ()),base,top,height).Shape();
    } else if (type=="torus") {
        const double major=length(p,"majorRadiusMm"),minor=length(p,"minorRadiusMm");
        if(major<=minor)throw std::invalid_argument("Major radius must exceed minor radius");
        shape=BRepPrimAPI_MakeTorus(gp_Ax2(origin,gp::DZ()),major,minor).Shape();
    } else throw std::invalid_argument("Unsupported primitive type");
    if (shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_SOLID)!=1)
        throw std::runtime_error("Kernel did not produce a valid solid");
    return shape;
}
EvaluatedSupportFace GeometryKernel::resolveSketchSupport(const EvaluatedDocument& evaluation,const QJsonObject& support) {
    auto reject=[&](const QString& code,const QString& message){auto diagnostic=support;diagnostic["code"]=code;diagnostic["message"]=message;throw EvaluationError(diagnostic);};
    if(support.size()!=3)reject("invalid_support","Support requires producerFeatureId, bodyId and faceId");
    for(const auto& field:{"producerFeatureId","bodyId","faceId"})if(!support[field].isString() || QUuid(support[field].toString()).isNull())reject("invalid_support","Support requires stable UUID references");
    const auto body=std::find_if(evaluation.bodies.begin(),evaluation.bodies.end(),[&](const auto& value){return value.bodyId==support["bodyId"].toString();});
    if(body==evaluation.bodies.end())reject("support_unavailable","The exact support producer/body is unavailable or consumed");
    const EvaluatedSupportFace* found=nullptr;
    for(const auto& face:body->supportFaces)if(face.faceId==support["faceId"].toString() && face.producerFeatureId==support["producerFeatureId"].toString()){if(found)reject("ambiguous_support","More than one face matches the support reference");found=&face;}
    if(!found)reject("support_unavailable","The exact named planar support has no unique surviving face");

    int matches=0;for(TopExp_Explorer face(body->shape,TopAbs_FACE);face.More();face.Next())if(face.Current().IsSame(found->face))++matches;
    if(matches!=1 || found->face.IsNull() || !BRepCheck_Analyzer(found->face).IsValid()
        || BRepAdaptor_Surface(found->face).GetType()!=GeomAbs_Plane)reject("support_unavailable","The retained support is not one real planar producer face");
    return *found;
}

QJsonObject GeometryKernel::sketchFrame(const EvaluatedDocument& evaluation,const QJsonObject& feature) {
    const auto parameters=feature["parameters"].toObject();
    const auto support=resolveSketchSupport(evaluation,parameters["support"].toObject());
    auto frame=support.frame;
    if(!parameters.contains("originConvention"))return frame;
    if(parameters["originConvention"]!="projectedOwnerV1")
        throw EvaluationError({{"code","invalid_sketch_origin"},{"message","Unknown face sketch origin convention"}});
    for(const auto& body:evaluation.bodies)if(body.bodyId==support.bodyId && !body.occurrenceId.isEmpty())
        throw EvaluationError({{"code","unsupported_sketch_origin_context"},{"message","Imported occurrence faces need an explicit local modeling context"}});
    // The admitted modeling context has an identity owner occurrence. Its
    // definition-space origin is world zero; retain the exact face axes.
    const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
    double distance=0;
    for(int i=0;i<3;++i)distance+=origin[i].toDouble()*normal[i].toDouble();
    QJsonArray projected;
    for(int i=0;i<3;++i)projected.append(distance*normal[i].toDouble());
    frame["origin"]=projected;
    return frame;
}

QJsonObject GeometryKernel::resolvePlaneSource(const EvaluatedDocument& evaluation,const QJsonObject& source) {
    if(source.size()!=1)throw EvaluationError({{"code","invalid_plane_source"},{"message","Plane requires exactly one source"}});
    if(source.contains("plane"))return SketchGeometry::frame(source["plane"].toString());
    if(source.contains("planeId")) {
        for(const auto& plane:evaluation.constructionPlanes)if(plane.featureId==source["planeId"].toString())return plane.frame;
    } else if(source.contains("sketchId")) {
        for(const auto& sketch:evaluation.sketches)if(sketch.featureId==source["sketchId"].toString())return sketch.resolvedFrame;
    } else if(source.contains("support"))return resolveSketchSupport(evaluation,source["support"].toObject()).frame;
    throw EvaluationError({{"code","plane_source_unavailable"},{"message","The retained plane source is unavailable"},{"source",source}});
}

QJsonObject GeometryKernel::pathEndpointPlaneFrame(const EvaluatedDocument& evaluation,
    const QJsonObject& path,const QJsonArray& authoredFeatures) {
    const auto sketchId=path["sketchId"].toString(),entityId=path["entityId"].toString();
    const int endpoint=path["endpoint"].toInt(-1);
    if(endpoint!=0 && endpoint!=1)reject("invalid_path_endpoint","Choose Start or End of the retained spline");
    const EvaluatedSketch* evaluated=nullptr;
    for(const auto& sketch:evaluation.sketches)if(sketch.featureId==sketchId){evaluated=&sketch;break;}
    if(!evaluated)reject("path_unavailable","The retained source sketch is unavailable at this history position");
    QJsonObject authored,entity;
    for(const auto& row:authoredFeatures)if(row.toObject()["id"]==sketchId){authored=row.toObject();break;}
    if(authored["type"]!="sketch")reject("path_unavailable","The retained path sketch no longer exists");
    const auto parameters=authored["parameters"].toObject();
    for(const auto& row:parameters["entities"].toArray())if(row.toObject()["id"]==entityId){entity=row.toObject();break;}
    if(entity["type"]!="fitSpline")reject("unsupported_path","Select one retained fit-point spline");
    const auto current=evaluated->entityEdges.value(entityId);
    if(current.IsNull())reject("path_unavailable","The exact evaluated spline edge is unavailable");
    const auto pointIds=entity["pointIds"].toArray();
    if(pointIds.size()<2)reject("path_unavailable","The retained spline endpoints are unavailable");
    QMap<QString,gp_Pnt> solvedPoints;
    for(const auto& row:evaluated->summary["worldPoints"].toArray()){
        const auto point=row.toObject();const auto xyz=point["positionMm"].toArray();
        if(xyz.size()==3)solvedPoints[point["id"].toString()]=gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
    }
    const auto startId=pointIds.first().toString(),endId=pointIds.last().toString();
    if(!solvedPoints.contains(startId) || !solvedPoints.contains(endId))
        reject("path_unavailable","The evaluated spline fit endpoints are unavailable");
    const auto frame=evaluated->resolvedFrame;
    try {
        const BRepAdaptor_Curve curve(current);
        if(curve.GetType()!=GeomAbs_BSplineCurve)reject("unsupported_path","The retained path must evaluate as a spline");
        const auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
        const bool forward=first.Distance(solvedPoints[startId])<1e-6 &&
            last.Distance(solvedPoints[endId])<1e-6;
        const bool reversed=last.Distance(solvedPoints[startId])<1e-6 &&
            first.Distance(solvedPoints[endId])<1e-6;
        if(forward==reversed)reject("path_unavailable","Evaluated spline endpoints do not uniquely match retained fit-point order");
        const double parameter=(endpoint==0)==forward ? curve.FirstParameter() : curve.LastParameter();
        gp_Pnt origin;gp_Vec tangent;curve.D1(parameter,origin,tangent);
        if(reversed)tangent.Reverse();
        if(!std::isfinite(tangent.Magnitude()) || tangent.Magnitude()<1e-9)
            reject("degenerate_path_tangent","The selected spline endpoint has no usable tangent");
        tangent.Normalize();
        const auto axis=frame["normal"].toArray();
        if(axis.size()!=3)reject("invalid_path_frame","The source sketch normal is unavailable");
        gp_Vec u(axis[0].toDouble(),axis[1].toDouble(),axis[2].toDouble());
        u-=tangent.Multiplied(u.Dot(tangent));
        if(!std::isfinite(u.Magnitude()) || u.Magnitude()<1e-9)
            reject("invalid_path_frame","The source sketch normal is parallel to the endpoint tangent");
        u.Normalize();const gp_Vec v=tangent.Crossed(u);
        return {{"origin",QJsonArray{origin.X(),origin.Y(),origin.Z()}},
            {"uAxis",QJsonArray{u.X(),u.Y(),u.Z()}},
            {"vAxis",QJsonArray{v.X(),v.Y(),v.Z()}},
            {"normal",QJsonArray{tangent.X(),tangent.Y(),tangent.Z()}}};
    }catch(const EvaluationError&){throw;}
    catch(const std::exception& error){reject("path_unavailable",QString("Unable to evaluate retained spline: %1").arg(error.what()));}
}

QJsonObject GeometryKernel::tangentPointPlaneFrame(const EvaluatedDocument& evaluation,const QJsonObject& tangent) {
    const auto reference=tangent["face"].toObject(),point=tangent["point"].toObject();
    const EvaluatedBody* body=nullptr;
    for(const auto& candidate:evaluation.bodies)if(candidate.bodyId==reference["bodyId"].toString())body=&candidate;
    if(!body || point["bodyId"]!=reference["bodyId"])reject("tangent_support_unavailable","Face and endpoint must belong to the same current body");
    const EvaluatedSupportFace* face=nullptr;
    for(const auto& candidate:body->supportFaces)if(candidate.faceId==reference["faceId"].toString()
        && candidate.producerFeatureId==reference["producerFeatureId"].toString()){
        if(face)reject("tangent_support_ambiguous","The retained face is ambiguous");face=&candidate;}
    const EvaluatedSolidEdge* edge=nullptr;
    for(const auto& candidate:body->solidEdges)if(candidate.edgeId==point["edgeId"].toString()){
        if(edge)reject("tangent_point_ambiguous","The retained edge is ambiguous");edge=&candidate;}
    if(!face || !edge || face->face.IsNull() || edge->edge.IsNull())
        reject("tangent_support_unavailable","The retained face or edge endpoint is unavailable");
    int faceMatches=0,edgeMatches=0;
    for(TopExp_Explorer f(body->shape,TopAbs_FACE);f.More();f.Next())if(f.Current().IsSame(face->face))++faceMatches;
    TopTools_IndexedMapOfShape currentEdges;TopExp::MapShapes(body->shape,TopAbs_EDGE,currentEdges);
    if(currentEdges.Contains(edge->edge))edgeMatches=1;
    if(faceMatches!=1 || edgeMatches!=1)reject("tangent_support_unavailable","Retained references are not exact current body topology");
    TopoDS_Vertex first,last;TopExp::Vertices(edge->edge,first,last,true);
    if(first.IsNull() || last.IsNull() || first.IsSame(last))reject("tangent_point_unavailable","Select an edge with two distinct endpoints");
    const auto start=edge->provenance["startMm"].toArray(),end=edge->provenance["endMm"].toArray();
    if(start.size()!=3 || end.size()!=3 || (point["endpoint"]!=0 && point["endpoint"]!=1))
        reject("tangent_point_unavailable","The retained edge has no oriented endpoint metadata");
    if(edge->provenance.value("role")=="roundBoundary"){
        const auto key=point["endpoint"].toInt()==0 ? "startWallFaceId" : "endWallFaceId";
        if(edge->provenance.value(key).toString().isEmpty())
            reject("tangent_point_unavailable","The round boundary endpoint is an unstable interior split vertex");
    }
    const gp_Pnt expectedStart(start[0].toDouble(),start[1].toDouble(),start[2].toDouble()),
        expectedEnd(end[0].toDouble(),end[1].toDouble(),end[2].toDouble());
    const auto a=BRep_Tool::Pnt(first),b=BRep_Tool::Pnt(last);
    const bool forward=a.Distance(expectedStart)<1e-6 && b.Distance(expectedEnd)<1e-6;
    const bool reversed=b.Distance(expectedStart)<1e-6 && a.Distance(expectedEnd)<1e-6;
    if(forward==reversed)reject("tangent_point_ambiguous","Current edge vertices do not uniquely match the retained oriented endpoints");
    const auto origin=point["endpoint"].toInt()==0?expectedStart:expectedEnd;
    TopLoc_Location location;auto surface=BRep_Tool::Surface(face->face,location);
    const auto local=origin.Transformed(location.Transformation().Inverted());
    GeomAPI_ProjectPointOnSurf projection(local,surface);
    if(projection.NbPoints()<1 || projection.LowerDistance()>1e-5)
        reject("tangent_point_off_face","The retained endpoint must lie on the selected face");
    double uParameter,vParameter;projection.LowerDistanceParameters(uParameter,vParameter);
    BRepClass_FaceClassifier classifier(face->face,gp_Pnt2d(uParameter,vParameter),1e-6);
    if(classifier.State()!=TopAbs_IN && classifier.State()!=TopAbs_ON)
        reject("tangent_point_off_face","The retained endpoint is outside the selected face boundary");
    gp_Pnt evaluated;gp_Vec du,dv;surface->D1(uParameter,vParameter,evaluated,du,dv);
    du.Transform(location.Transformation());dv.Transform(location.Transformation());
    auto normal=du.Crossed(dv);
    if(normal.Magnitude()<1e-10)reject("tangent_normal_unavailable","Surface normal is singular at this endpoint");
    normal.Normalize();if(face->face.Orientation()==TopAbs_REVERSED)normal.Reverse();
    gp_Vec u(1,0,0);u-=normal.Multiplied(u.Dot(normal));
    if(u.Magnitude()<1e-7){u=gp_Vec(0,1,0);u-=normal.Multiplied(u.Dot(normal));}
    u.Normalize();const auto v=normal.Crossed(u);
    return {{"origin",QJsonArray{origin.X(),origin.Y(),origin.Z()}},
        {"uAxis",QJsonArray{u.X(),u.Y(),u.Z()}},{"vAxis",QJsonArray{v.X(),v.Y(),v.Z()}},
        {"normal",QJsonArray{normal.X(),normal.Y(),normal.Z()}}};
}

QJsonObject GeometryKernel::offsetPlaneFrame(const QJsonObject& frame,double offsetMm) {
    auto result=frame;auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
    if(origin.size()!=3 || normal.size()!=3 || !std::isfinite(offsetMm))throw EvaluationError({{"code","invalid_plane_frame"},{"message","Plane source frame is invalid"}});
    for(int axis=0;axis<3;++axis)origin[axis]=origin[axis].toDouble()+offsetMm*normal[axis].toDouble();
    result["origin"]=origin;return result;
}

QJsonObject GeometryKernel::midplaneFrame(const EvaluatedDocument& evaluation,const QJsonArray& faces) {
    if(faces.size()!=2 || faces[0]==faces[1])
        throw EvaluationError({{"code","invalid_midplane_faces"},{"message","Midplane requires two distinct planar faces"}});
    const auto first=resolveSketchSupport(evaluation,faces[0].toObject());
    const auto second=resolveSketchSupport(evaluation,faces[1].toObject());
    const auto a=first.frame["origin"].toArray(),b=second.frame["origin"].toArray();
    const auto na=first.frame["normal"].toArray(),nb=second.frame["normal"].toArray();
    if(a.size()!=3 || b.size()!=3 || na.size()!=3 || nb.size()!=3)
        throw EvaluationError({{"code","invalid_midplane_frames"},{"message","Midplane face frames are unavailable"}});
    double dot=0,separation=0;
    for(int axis=0;axis<3;++axis){dot+=na[axis].toDouble()*nb[axis].toDouble();
        separation+=(b[axis].toDouble()-a[axis].toDouble())*na[axis].toDouble();}
    if(std::abs(std::abs(dot)-1)>1e-6)
        throw EvaluationError({{"code","nonparallel_midplane_faces"},{"message","Midplane faces must be parallel"}});
    if(std::abs(separation)<1e-6)
        throw EvaluationError({{"code","coincident_midplane_faces"},{"message","Midplane faces must have separate support planes"}});
    auto frame=first.frame;auto origin=a;
    for(int axis=0;axis<3;++axis)origin[axis]=a[axis].toDouble()+0.5*separation*na[axis].toDouble();
    frame["origin"]=origin;
    // Presentation bounds fit the selected face pair; the canonical sketch
    // frame origin remains the midpoint of the first retained face frame.
    const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
    if(u.size()!=3 || v.size()!=3)
        throw EvaluationError({{"code","invalid_midplane_frames"},{"message","Midplane face axes are unavailable"}});
    double uMin=std::numeric_limits<double>::infinity(),uMax=-uMin,vMin=uMin,vMax=-uMin;
    for(const auto& face:{first.face,second.face})for(TopExp_Explorer vertex(face,TopAbs_VERTEX);vertex.More();vertex.Next()){
        const auto point=BRep_Tool::Pnt(TopoDS::Vertex(vertex.Current()));double localU=0,localV=0;
        const double xyz[3]={point.X(),point.Y(),point.Z()};
        for(int axis=0;axis<3;++axis){localU+=(xyz[axis]-origin[axis].toDouble())*u[axis].toDouble();
            localV+=(xyz[axis]-origin[axis].toDouble())*v[axis].toDouble();}
        uMin=std::min(uMin,localU);uMax=std::max(uMax,localU);
        vMin=std::min(vMin,localV);vMax=std::max(vMax,localV);
    }
    if(std::isfinite(uMin) && uMax>uMin && vMax>vMin){
        const double padU=std::max(3.0,(uMax-uMin)*0.08),padV=std::max(3.0,(vMax-vMin)*0.08);
        frame["displayBoundsMm"]=QJsonArray{uMin-padU,uMax+padU,vMin-padV,vMax+padV};
    }
    return frame;
}

QJsonArray GeometryKernel::supportEdgeDescriptors(const EvaluatedSupportFace& support) {
    QJsonArray result;
    for(const auto& edge:support.boundaryEdges) {
        auto descriptor=edge.provenance;descriptor["edgeId"]=edge.edgeId;descriptor["type"]=edge.type;
        descriptor["supported"]=edge.type=="line";result.append(descriptor);
    }
    return result;
}
QJsonObject GeometryKernel::projectionContext(const EvaluatedDocument& evaluation,const QJsonObject& feature) {
    const auto parameters=feature["parameters"].toObject();QJsonArray lines,circles,unsupported,points;
    const auto plane=parameters["plane"].toString();
    if(plane!="face" && plane!="construction")return {{"lines",lines},{"circles",circles},{"unsupportedEdges",unsupported},{"points",points}};
    EvaluatedSupportFace cap;
    QJsonObject frame;
    if(plane=="face") {
        cap=resolveSketchSupport(evaluation,parameters["support"].toObject());frame=sketchFrame(evaluation,feature);
        for(const auto& edge:cap.boundaryEdges)if(edge.type!="line")unsupported.append(edge.edgeId);
    } else {
        const auto planeId=parameters["planeId"].toString();
        for(const auto& candidate:evaluation.constructionPlanes)if(candidate.featureId==planeId)frame=candidate.frame;
        if(frame.isEmpty())reject("plane_unavailable","Retained construction plane is unavailable for Project");
    }
    // Face-action identity is retained independently from its surviving edges.
    // Refuse stale membership rather than silently rebinding a source face.
    for(const auto& row:parameters.value("faceProjectionGroups").toArray()) {
        const auto group=row.toObject();const auto source=group.value("source").toObject();
        const auto face=resolveSketchSupport(evaluation,source);
        for(const auto& id:group.value("projectionIds").toArray()) {
            QString edgeId;
            for(const auto& key:{"projections","circleProjections","pointProjections"})for(const auto& projection:parameters.value(key).toArray())
                if(projection.toObject().value("id")==id)edgeId=projection.toObject().value("source").toObject().value("edgeId").toString();
            if(std::none_of(face.boundaryEdges.begin(),face.boundaryEdges.end(),[&](const auto& edge){return edge.edgeId==edgeId;}))
                reject("projection_unavailable","Retained face Project member is no longer a boundary of its source face");
        }
    }
    const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
    auto local=[&](const QJsonArray& point,const QJsonArray& axis){double result=0;for(int i=0;i<3;++i)result+=(point[i].toDouble()-origin[i].toDouble())*axis[i].toDouble();return result;};
    QMap<QString,QJsonObject> centers;
    for(const auto& edge:cap.boundaryEdges) {
        if(edge.type=="line")continue;
        BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
        const auto circle=curve.Circle();const auto center=circle.Location();
        const auto lineage=edge.provenance;
        const auto sketchId=lineage["sourceSketchId"].toString(),entityId=lineage["sourceEntityId"].toString();
        // Generated blend arcs are real face boundaries, but they have no authored
        // sketch circle whose center can be retained as a sketch reference.
        if(sketchId.isEmpty() && entityId.isEmpty())continue;
        if(sketchId.isEmpty() || entityId.isEmpty())reject("ambiguous_projection","Circular support boundary has incomplete source identity");
        const auto key=sketchId+":"+entityId
            +(lineage.contains("occurrenceId") ? ":"+lineage["occurrenceId"].toString() : QString{});
        auto source=parameters["support"].toObject();source["sourceSketchId"]=lineage["sourceSketchId"];source["sourceEntityId"]=lineage["sourceEntityId"];source["role"]="curveCenter";
        if(lineage.contains("occurrenceId"))source["occurrenceId"]=lineage["occurrenceId"];
        const QJsonArray world{center.X(),center.Y(),center.Z()};
        const double cu=local(world,u),cv=local(world,v);
        double normalDistance=0;const auto normal=frame["normal"].toArray();for(int i=0;i<3;++i)normalDistance+=(world[i].toDouble()-origin[i].toDouble())*normal[i].toDouble();
        if(std::abs(normalDistance)>1e-6)reject("ambiguous_projection","Circular cap center is outside the resolved support plane");
        if(centers.contains(key)) {
            const auto prior=centers[key];
            if(std::hypot(prior["uMm"].toDouble()-cu,prior["vMm"].toDouble()-cv)>1e-6 || std::abs(prior["radiusMm"].toDouble()-circle.Radius())>1e-6)
                reject("ambiguous_projection","Circular descendants disagree on their retained source center");
            continue;
        }
        const auto id=SketchGeometry::curveCenterPointId(feature["id"].toString(),source);
        centers[key]={{"id",id},{"source",source},{"uMm",cu},{"vMm",cv},{"radiusMm",circle.Radius()},{"retained",false}};
    }
    for(const auto& value:parameters["pointReferences"].toArray()) {
        const auto reference=value.toObject();bool found=false;
        for(auto it=centers.begin();it!=centers.end();++it)if(it.value()["id"]==reference["id"] && it.value()["source"]==reference["source"]){auto point=it.value();point["retained"]=true;it.value()=point;found=true;break;}
        if(!found)throw EvaluationError({{"code","projection_unavailable"},{"pointReferenceId",reference["id"]},{"source",reference["source"]},{"message","Retained circular support center is no longer uniquely available"}});
    }
    for(auto point:centers){point.remove("radiusMm");points.append(point);}
    auto edgeFor=[&](const QJsonObject& source)->EvaluatedSupportEdge {
        if(source.size()==4) {
            if(plane!="face")reject("projection_unavailable","Legacy support boundary requires its original face sketch");
            for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(source[key]!=parameters["support"].toObject()[key])
                reject("projection_unavailable","Legacy support projection no longer belongs to its sketch face");
            const auto found=std::find_if(cap.boundaryEdges.begin(),cap.boundaryEdges.end(),[&](const auto& edge){return edge.edgeId==source["edgeId"].toString();});
            if(found==cap.boundaryEdges.end())reject("projection_unavailable","Retained support boundary edge is no longer available");
            return *found;
        }
        const auto body=std::find_if(evaluation.bodies.begin(),evaluation.bodies.end(),[&](const auto& b){return b.bodyId==source["bodyId"].toString();});
        if(body==evaluation.bodies.end())reject("projection_unavailable","Projected source body is no longer active");
        if(source.contains("occurrenceId") && source["occurrenceId"]!=body->occurrenceId)
            reject("projection_unavailable","Imported projected edge requires its current occurrence identity");
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(body->shape,TopAbs_EDGE,present);
        const EvaluatedSupportEdge* found=nullptr;
        for(const auto& face:body->supportFaces)for(const auto& edge:face.boundaryEdges)if(edge.edgeId==source["edgeId"].toString()) {
            if(edge.edge.IsNull() || !present.Contains(edge.edge))continue;
            if(found && !found->edge.IsSame(edge.edge))reject("ambiguous_projection","Projected edge has multiple current descendants");
            found=&edge;
        }
        if(!found)reject("projection_unavailable","Projected source edge is no longer a real supported boundary");
        return *found;
    };
    const auto normal=frame["normal"].toArray();
    for(const auto& collection:{QString("projections"),QString("pointProjections")})for(const auto& item:parameters[collection].toArray()) {
        const auto projection=item.toObject(),source=projection["source"].toObject();
        const auto edge=edgeFor(source);
        if(edge.type!="line")reject("unsupported_projection","Linked Project accepts straight model edges only");
        const auto a=edge.provenance["startMm"].toArray(),b=edge.provenance["endMm"].toArray();
        if(a.size()!=3 || b.size()!=3)reject("projection_unavailable","Straight source edge lacks current endpoints");
        double normalSpan=0,worldSpan=0;
        for(int i=0;i<3;++i){const double delta=b[i].toDouble()-a[i].toDouble();normalSpan+=delta*normal[i].toDouble();worldSpan+=delta*delta;}
        const double u1=local(a,u),v1=local(a,v),u2=local(b,u),v2=local(b,v);
        const double projectedSpan=std::hypot(u2-u1,v2-v1);
        if(worldSpan<1e-12 || (projectedSpan>1e-6 && std::abs(normalSpan)>1e-6))
            reject("unsupported_projection","This bounded Project supports edges parallel or perpendicular to the sketch plane");
        const auto id=projection["id"].toString();
        if(collection=="projections") {
            if(projectedSpan<=1e-6)reject("projection_dimension_changed","Linked line source now projects to a point");
            lines.append(QJsonObject{{"id",id},{"startPointId",SketchGeometry::projectionPointId(id,false)},{"endPointId",SketchGeometry::projectionPointId(id,true)},
                {"u1Mm",u1},{"v1Mm",v1},{"u2Mm",u2},{"v2Mm",v2},{"source",source}});
        } else {
            if(projectedSpan>1e-6)reject("projection_dimension_changed","Linked point source now projects to a line");
            points.append(QJsonObject{{"id",id},{"source",source},{"uMm",u1},{"vMm",v1},{"retained",true},{"kind","projectedEdgePoint"}});
        }
    }
    for(const auto& item:parameters["circleProjections"].toArray()) {
        const auto projection=item.toObject(),source=projection["source"].toObject();
        const auto edge=edgeFor(source);
        if(edge.edge.IsNull())reject("projection_unavailable","Circular source edge is unavailable");
        const BRepAdaptor_Curve curve(edge.edge);
        if(curve.GetType()!=GeomAbs_Circle)reject("unsupported_projection","Linked circular Project requires a true circular model edge");
        if(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::numbers::pi)>1e-6)
            reject("unsupported_projection","Partial circular arcs cannot be projected as full circles");
        const auto circle=curve.Circle();const auto axis=circle.Axis().Direction();
        const double parallel=std::abs(axis.X()*normal[0].toDouble()+axis.Y()*normal[1].toDouble()+axis.Z()*normal[2].toDouble());
        if(parallel<1-1e-6)reject("unsupported_projection","Circular source edge must be parallel to the sketch plane");
        const auto center=circle.Location();const QJsonArray world{center.X(),center.Y(),center.Z()};
        circles.append(QJsonObject{{"id",projection["id"]},{"source",source},{"centerPointId",SketchGeometry::projectionPointId(projection["id"].toString(),false)},
            {"uMm",local(world,u)},{"vMm",local(world,v)},{"radiusMm",circle.Radius()}});
    }
    return {{"lines",lines},{"circles",circles},{"unsupportedEdges",unsupported},{"points",points}};
}

std::vector<EvaluatedProfile> GeometryKernel::closedRectOffsetUnionCells(const EvaluatedSketch& sketch,
    const QJsonObject& sketchFeature,const QJsonArray& groupIds) {
    const auto fail=[](const char* message)->void{throw std::invalid_argument(message);};
    if(sketch.featureId!=sketchFeature.value("id").toString() || groupIds.isEmpty())
        fail("Offset union requires one current source sketch and retained groups");
    const auto groups=sketchFeature.value("parameters").toObject().value("closedRectOffsets").toArray();
    QSet<QString> requested;
    TopoDS_Shape intended;
    auto area=[](const TopoDS_Shape& shape){
        if(shape.IsNull())return 0.;
        GProp_GProps properties;BRepGProp::SurfaceProperties(shape,properties);
        return properties.Mass();
    };
    const auto sameArea=[](double a,double b){return std::abs(a-b)<=std::max(1e-6,std::max(a,b)*1e-9);};
    for(const auto& value:groupIds) {
        const auto id=value.toString();
        if(id.isEmpty() || requested.contains(id))fail("Offset union group identities must be distinct");
        requested.insert(id);
        QJsonObject group;
        for(const auto& entry:groups)if(entry.toObject().value("id")==id) {
            if(!group.isEmpty())fail("Offset union group identity is ambiguous");
            group=entry.toObject();
        }
        if(group.isEmpty() || group.value("entityIds").toArray().size()!=4)
            fail("Offset union group is unavailable");
        BRepBuilderAPI_MakeWire wire;
        for(const auto& edgeId:group.value("entityIds").toArray()) {
            const auto current=sketch.entityEdges.constFind(edgeId.toString());
            if(current==sketch.entityEdges.cend() || current.value().IsNull())
                fail("Offset union source edge is unavailable at this history stage");
            wire.Add(current.value());
            if(!wire.IsDone())fail("Offset union source edges do not make a current loop");
        }
        if(!BRep_Tool::IsClosed(wire.Wire()))fail("Offset union source loop is open");
        BRepBuilderAPI_MakeFace face(wire.Wire());
        if(!face.IsDone() || !BRepCheck_Analyzer(face.Face()).IsValid() || area(face.Face())<=1e-8)
            fail("Offset union source loop is not a valid planar face");
        if(intended.IsNull())intended=face.Face();
        else {
            BRepAlgoAPI_Fuse fused(intended,face.Face());
            if(!fused.IsDone() || fused.HasErrors())fail("Offset union source loops cannot be fused");
            fused.SimplifyResult(false,true);intended=fused.Shape();
        }
    }
    if(intended.IsNull() || !BRepCheck_Analyzer(intended).IsValid() || count(intended,TopAbs_FACE)!=1)
        fail("Offset union must form one connected planar face");
    const double intendedArea=area(intended);
    std::vector<EvaluatedProfile> selected;
    for(const auto& profile:sketch.profiles) {
        BRepAlgoAPI_Common common(profile.face,intended);
        if(!common.IsDone() || common.HasErrors())fail("Offset union cell coverage cannot be resolved");
        const double covered=area(common.Shape()),cellArea=area(profile.face);
        if(covered<=1e-8)continue;
        if(!sameArea(covered,cellArea))fail("Offset union partly covers a sketch region; repair the sketch");
        selected.push_back(profile);
    }
    if(selected.empty())fail("Offset union contains no current sketch regions");
    const auto united=unionProfiles(selected);
    BRepAlgoAPI_Common checked(united.face,intended);
    if(!checked.IsDone() || checked.HasErrors() || !sameArea(area(checked.Shape()),intendedArea)
        || !sameArea(area(checked.Shape()),area(united.face)))
        fail("Offset union differs from the current selected sketch regions");
    return selected;
}

EvaluatedProfile GeometryKernel::unionProfiles(const std::vector<EvaluatedProfile>& profiles) {
    if(profiles.empty())throw std::invalid_argument("Select at least one current profile");
    if(profiles.size()==1)return profiles.front();
    const auto& first=profiles.front();TopoDS_Shape shape=first.face;
    QStringList profileIds,entityIds;double expectedArea=0;
    auto lineage=first.boundaryEdges;
    QSet<QString> distinct;
    for(const auto& profile:profiles){
        if(profile.sketchId!=first.sketchId || profile.plane!=first.plane || profile.face.IsNull()
            || !BRepCheck_Analyzer(profile.face).IsValid() || distinct.contains(profile.profileId))
            throw std::invalid_argument("Profile union requires distinct valid regions from one sketch");
        distinct.insert(profile.profileId);profileIds.append(profile.profileId);expectedArea+=profile.summary["areaMm2"].toDouble();
        for(const auto& id:profile.summary["sourceEntityIds"].toArray())entityIds.append(id.toString());
    }
    // All regions are already derived on the same plane. Fuse before making a
    // prism so shared partitions cannot survive as separate feature bodies.
    for(size_t i=1;i<profiles.size();++i){BRepAlgoAPI_Fuse fused(shape,profiles[i].face);
        if(!fused.IsDone() || fused.HasErrors())throw std::runtime_error("OCCT could not unite the selected planar regions");
        // Merge coplanar cells while retaining split edge contacts. Unifying
        // collinear edges here would erase their authored endpoint ancestry.
        fused.SimplifyResult(false,true);shape=fused.Shape();
        lineage.insert(lineage.end(),profiles[i].boundaryEdges.begin(),profiles[i].boundaryEdges.end());
        std::vector<EvaluatedBoundaryEdge> next;
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(shape,TopAbs_EDGE,present);
        for(const auto& source:lineage) {
            TopTools_IndexedMapOfShape descendants;
            if(present.Contains(source.edge))descendants.Add(source.edge);
            for(const auto& changed:fused.Modified(source.edge))if(changed.ShapeType()==TopAbs_EDGE && present.Contains(changed))descendants.Add(changed);
            for(int j=1;j<=descendants.Extent();++j){auto mapped=source;mapped.edge=TopoDS::Edge(descendants(j));
                if(source.type=="line") {
                    TopoDS_Vertex a,b,c,d;TopExp::Vertices(source.edge,a,b,true);TopExp::Vertices(mapped.edge,c,d,true);
                    const auto pa=BRep_Tool::Pnt(a),pb=BRep_Tool::Pnt(b),pc=BRep_Tool::Pnt(c),pd=BRep_Tool::Pnt(d);
                    if(pa.Distance(pc)<=1e-6 && pb.Distance(pd)<=1e-6){}
                    else if(pa.Distance(pd)<=1e-6 && pb.Distance(pc)<=1e-6)std::swap(mapped.startContact,mapped.endContact);
                    else reject("ambiguous_projection","A straight source edge was split or merged; endpoint lineage requires explicit repair");
                }
                const bool duplicate=std::any_of(next.begin(),next.end(),[&](const auto& prior){return prior.edge.IsSame(mapped.edge) && prior.sourceEntityId==mapped.sourceEntityId && prior.sourceSketchId==mapped.sourceSketchId;});
                if(!duplicate)next.push_back(mapped);}
        }
        lineage=std::move(next);
    }
    // A closed circle's parameter seam can split one retained boundary into
    // two edges, although the seam has no authored contact. Remove only those
    // artificial vertices before prism generation. Keep every real contact and
    // every noncircular vertex, so straight boundary ancestry is unchanged.
    TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(shape,TopAbs_VERTEX,vertices);
    ShapeUpgrade_UnifySameDomain unify(shape,true,false,false);
    bool hasCircularSeam=false;
    for(int i=1;i<=vertices.Extent();++i) {
        TopTools_IndexedMapOfShape incident;
        QString sketchId,entityId;bool seam=true;
        for(const auto& boundary:lineage) {
            TopoDS_Vertex a,b;TopExp::Vertices(boundary.edge,a,b,true);
            if(!a.IsSame(vertices(i)) && !b.IsSame(vertices(i)))continue;
            incident.Add(boundary.edge);
            seam &= boundary.type=="circle"
                && (!a.IsSame(vertices(i)) || boundary.startContact.isEmpty())
                && (!b.IsSame(vertices(i)) || boundary.endContact.isEmpty());
            if(sketchId.isEmpty()){sketchId=boundary.sourceSketchId;entityId=boundary.sourceEntityId;}
            else seam &= sketchId==boundary.sourceSketchId && entityId==boundary.sourceEntityId;
        }
        seam &= incident.Extent()==2;
        if(seam)hasCircularSeam=true;else unify.KeepShape(vertices(i));
    }
    if(hasCircularSeam) {
        unify.Build();const auto unified=unify.Shape();
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(unified,TopAbs_EDGE,present);
        std::vector<EvaluatedBoundaryEdge> canonical;
        for(const auto& source:lineage) {
            TopTools_IndexedMapOfShape descendants;
            if(present.Contains(source.edge))descendants.Add(source.edge);
            for(const auto& changed:unify.History()->Modified(source.edge))
                if(changed.ShapeType()==TopAbs_EDGE && present.Contains(changed))descendants.Add(changed);
            if(descendants.Extent()!=1)throw std::runtime_error("Circular seam has no unique boundary history");
            auto mapped=source;mapped.edge=TopoDS::Edge(descendants(1));
            auto prior=std::find_if(canonical.begin(),canonical.end(),[&](const auto& edge){return edge.edge.IsSame(mapped.edge);});
            if(prior!=canonical.end()) {
                if(prior->sourceSketchId!=source.sourceSketchId || prior->sourceEntityId!=source.sourceEntityId)
                    throw std::runtime_error("Circular seam merged distinct boundary sources");
                continue;
            }
            // History chooses the edge; exact shared vertices retain its two
            // oriented contacts. The removed parameter seam contributes none.
            if(!mapped.edge.IsSame(source.edge)) {
                mapped.startContact.clear();mapped.endContact.clear();
                TopoDS_Vertex a,b;TopExp::Vertices(mapped.edge,a,b,true);
                bool foundA=false,foundB=false;
                const auto contact=[&](const TopoDS_Vertex& vertex,QString& value,bool& found,
                    const TopoDS_Vertex& original,const QString& retained) {
                    bool same=vertex.IsSame(original);
                    for(const auto& changed:unify.History()->Modified(original))same |= vertex.IsSame(changed);
                    if(!same)return;
                    if(found && value!=retained)throw std::runtime_error("Ambiguous circular seam endpoint lineage");
                    value=retained;found=true;
                };
                for(const auto& ancestor:lineage) {
                    if(ancestor.sourceSketchId!=source.sourceSketchId || ancestor.sourceEntityId!=source.sourceEntityId)continue;
                    TopoDS_Vertex c,d;TopExp::Vertices(ancestor.edge,c,d,true);
                    contact(a,mapped.startContact,foundA,c,ancestor.startContact);
                    contact(a,mapped.startContact,foundA,d,ancestor.endContact);
                    contact(b,mapped.endContact,foundB,c,ancestor.startContact);
                    contact(b,mapped.endContact,foundB,d,ancestor.endContact);
                }
                if(!foundA || !foundB)throw std::runtime_error("Circular seam lost retained endpoint lineage");
            }
            canonical.push_back(mapped);
        }
        shape=unified;lineage=std::move(canonical);
    }
    if(shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_FACE)!=1)
        throw std::invalid_argument("Selected regions must form one connected planar face");
    TopExp_Explorer face(shape,TopAbs_FACE);const auto united=TopoDS::Face(face.Current());
    GProp_GProps area;BRepGProp::SurfaceProperties(united,area);
    if(!std::isfinite(area.Mass()) || std::abs(area.Mass()-expectedArea)>std::max(1e-6,expectedArea*1e-9))
        throw std::runtime_error("Selected region union area does not match its disjoint planar regions");
    profileIds.sort();entityIds.removeDuplicates();entityIds.sort();QJsonArray refs,sources;
    for(const auto& id:profileIds)refs.append(id);for(const auto& id:entityIds)sources.append(id);
    const auto id=QUuid::createUuidV5(QUuid(first.sketchId),QByteArray("alloy-profile-union/1\n")+profileIds.join('\n').toUtf8()).toString(QUuid::WithoutBraces);
    QJsonObject summary{{"profileId",id},{"profileIds",refs},{"sketchId",first.sketchId},{"plane",first.plane},
        {"frame",first.summary["frame"]},{"valid",true},{"areaUnit","mm2"},{"areaMm2",area.Mass()},{"sourceEntityIds",sources},{"type","profile_union"}};
    return {id,first.sketchId,first.plane,united,summary,lineage};
}

double GeometryKernel::extrusionStart(const QJsonObject& p) {
    if(p.value("direction")=="symmetric")return -extrusionSpan(p)/2;
    if(p.value("direction")=="twoSides")return -p.value("negativeDistanceMm").toDouble();
    return p.value("startOffsetMm").toDouble();
}
EvaluatedProfile GeometryKernel::thinLineProfile(const EvaluatedSketch& sketch,const QString& entityId,double thickness,const QString& location) {
    const auto reject=[](const QString& message){throw EvaluationError({{"code","invalid_thin_profile"},{"message",message}});};
    if(!std::isfinite(thickness) || thickness<0.001 || thickness>100000 || (location!="side1" && location!="side2" && location!="center"))reject("Enter a positive Wall Thickness and supported Wall Location");
    QJsonObject entity;
    for(const auto& row:sketch.summary.value("entities").toArray())if(row.toObject().value("id")==entityId)entity=row.toObject();
    if(entity.value("type")!="line" || entity.value("construction").toBool() || !sketch.entityEdges.contains(entityId))reject("Thin Extrude requires one nonconstruction open straight Line");
    for(const auto& profile:sketch.profiles)if(profile.summary.value("sourceEntityIds").toArray().contains(entityId))reject("Thin Extrude requires an open Line, outside a closed profile");
    QMap<QString,gp_Pnt> points;
    for(const auto& row:sketch.summary.value("worldPoints").toArray()){const auto point=row.toObject();const auto xyz=point.value("positionMm").toArray();if(xyz.size()==3)points.insert(point.value("id").toString(),gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble()));}
    const auto aId=entity.value("startPointId").toString(),bId=entity.value("endPointId").toString();
    if(!points.contains(aId) || !points.contains(bId))reject("Thin Line endpoints are unavailable");
    const auto a=points.value(aId),b=points.value(bId);gp_Vec tangent(a,b);
    const auto n=sketch.resolvedFrame.value("normal").toArray();if(n.size()!=3 || tangent.Magnitude()<1e-6)reject("Thin Line is collapsed or has no plane");
    gp_Vec side=gp_Vec(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()).Crossed(tangent);if(side.Magnitude()<1e-8)reject("Thin Line has a degenerate plane");side.Normalize();
    // Directed side 1 is sketch normal cross tangent; +X in XY therefore means +Y.
    const double low=location=="side1" ? 0 : location=="side2" ? -thickness : -thickness/2;
    const double high=low+thickness;
    const std::array<gp_Pnt,4> corners{a.Translated(side*low),b.Translated(side*low),b.Translated(side*high),a.Translated(side*high)};
    BRepBuilderAPI_MakeWire wire;for(int i=0;i<4;++i)wire.Add(BRepBuilderAPI_MakeEdge(corners[i],corners[(i+1)%4]).Edge());
    if(!wire.IsDone())reject("Thin Line strip could not be closed");BRepBuilderAPI_MakeFace face(wire.Wire());
    if(!face.IsDone() || !BRepCheck_Analyzer(face.Face()).IsValid())reject("Thin Line strip is invalid");
    // Derived strip and boundaries are disposable. Only the authored Line ID persists.
    EvaluatedProfile result;result.profileId=entityId;result.sketchId=sketch.featureId;result.plane=sketch.plane;result.face=face.Face();
    result.summary={{"frame",sketch.resolvedFrame},{"areaMm2",tangent.Magnitude()*thickness},{"sourceEntityIds",QJsonArray{entityId}}};return result;
}
double GeometryKernel::extrusionSpan(const QJsonObject& p) {
    const double d=p.value("distanceMm").toDouble();
    if(p.value("direction")=="symmetric")return p.value("measurement")=="wholeLength" ? d : 2*d;
    if(p.value("direction")=="twoSides")return d+p.value("negativeDistanceMm").toDouble();
    return d;
}
double GeometryKernel::extrusionDistance(const QJsonObject& p,const EvaluatedProfile& profile,const EvaluatedDocument& prior) {
    if(p.value("extent")=="all") {
        const auto targets=p.value("targetBodyIds").toArray();
        const auto reject=[](const QString& message){throw EvaluationError({{"code","invalid_extrude_extent"},{"message",message}});};
        if(p.value("operation")!="cut" || targets.size()!=1)reject("All extent requires one retained Cut target");
        const auto target=std::find_if(prior.bodies.begin(),prior.bodies.end(),[&](const auto& body){return body.bodyId==targets.first().toString();});
        if(target==prior.bodies.end())reject("All extent target is unavailable at this history stage");
        Bnd_Box bounds;BRepBndLib::AddOptimal(target->shape,bounds,false,false);
        if(bounds.IsVoid() || bounds.IsWhole())reject("All extent target has no finite bounds");
        double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
        const auto frame=profile.summary.value("frame").toObject();
        const auto normal=frame.value("normal").toArray(),origin=frame.value("origin").toArray();
        if(normal.size()!=3 || origin.size()!=3)reject("All extent requires a resolved profile frame");
        const double nx=normal[0].toDouble(),ny=normal[1].toDouble(),nz=normal[2].toDouble();
        const double base=nx*origin[0].toDouble()+ny*origin[1].toDouble()+nz*origin[2].toDouble()+p.value("startOffsetMm").toDouble();
        double low=std::numeric_limits<double>::infinity(),high=-low;
        for(double x:{x0,x1})for(double y:{y0,y1})for(double z:{z0,z1}){
            const double d=nx*x+ny*y+nz*z-base;low=std::min(low,d);high=std::max(high,d);
        }
        const bool flip=p.value("flip").toBool();
        const double reach=flip ? -low : high;
        if(!std::isfinite(reach) || reach<=0)reject("All extent target lies behind the selected direction");
        const double distance=std::copysign(reach+std::max(0.1,(high-low)*1e-5),flip ? -1.0 : 1.0);
        if(std::abs(distance)>100000)reject("All extent exceeds the supported extrusion length");
        return distance;
    }
    if(p.value("extent")!="toObject")return extrusionSpan(p);
    const auto target=resolveSketchSupport(prior,p.value("endSupport").toObject());
    const auto frame=profile.summary.value("frame").toObject();
    const auto n=frame.value("normal").toArray(),origin=frame.value("origin").toArray();
    const auto tn=target.frame.value("normal").toArray(),to=target.frame.value("origin").toArray();
    const auto reject=[](const QString& message){throw EvaluationError({{"code","invalid_extrude_extent"},{"message",message}});};
    if(n.size()!=3 || origin.size()!=3 || tn.size()!=3 || to.size()!=3)reject("To Object requires resolved planar frames");
    double alignment=0,distance=0;
    for(int i=0;i<3;++i){alignment+=n[i].toDouble()*tn[i].toDouble();distance+=(to[i].toDouble()-origin[i].toDouble())*n[i].toDouble();}
    if(std::abs(std::abs(alignment)-1)>1e-7)reject("To Object currently requires a face perpendicular to extrusion direction");
    distance-=p.value("startOffsetMm").toDouble();
    if(std::abs(distance)<0.001)reject("To Object target coincides with the extrusion start");
    // Positive clearance stops short of the target in either direction.
    const double resolved=distance-std::copysign(1.0,distance)*p.value("endOffsetMm").toDouble();
    if(!std::isfinite(resolved) || resolved*distance<=0 || std::abs(resolved)<0.001 || std::abs(resolved)>100000)
        reject("End offset crosses the extrusion start or exceeds the supported extent");
    return resolved;
}

TopoDS_Shape GeometryKernel::extrusion(const EvaluatedProfile& sourceProfile, double distanceMm, EvaluatedSupportFace* endCap, std::vector<EvaluatedSolidEdge>* edges, const QString& generatorFeatureId, std::vector<EvaluatedSupportFace>* planarFaces, double startOffsetMm, TopoDS_Face* baseFace) {
    if (!std::isfinite(distanceMm) || std::abs(distanceMm) < 0.001 || std::abs(distanceMm) > 100000)
        throw std::invalid_argument("Extrusion distance must be signed 0.001..100000 mm; zero is unsupported");
    if(!std::isfinite(startOffsetMm) || std::abs(startOffsetMm)>100000)
        throw std::invalid_argument("Extrusion start offset must be signed and within 100000 mm");
    auto profile=sourceProfile;
    if(startOffsetMm!=0){
        auto frame=profile.summary["frame"].toObject();auto origin=frame["origin"].toArray();const auto normal=frame["normal"].toArray();
        if(origin.size()!=3 || normal.size()!=3)throw std::invalid_argument("Extrusion profile has no resolved frame");
        const gp_Vec shift(normal[0].toDouble()*startOffsetMm,normal[1].toDouble()*startOffsetMm,normal[2].toDouble()*startOffsetMm);
        gp_Trsf translation;translation.SetTranslation(shift);
        BRepBuilderAPI_Transform moved(profile.face,translation,true);
        if(!moved.IsDone() || moved.Shape().IsNull() || moved.Shape().ShapeType()!=TopAbs_FACE)throw std::runtime_error("OCCT could not shift the extrusion start face");
        profile.face=TopoDS::Face(moved.Shape());
        for(auto& boundary:profile.boundaryEdges){
            const auto edge=moved.ModifiedShape(boundary.edge);
            if(edge.IsNull() || edge.ShapeType()!=TopAbs_EDGE)throw std::runtime_error("Offset extrusion start lost a source boundary edge");
            boundary.edge=TopoDS::Edge(edge);
        }
        for(int i=0;i<3;++i)origin[i]=origin[i].toDouble()+normal[i].toDouble()*startOffsetMm;
        frame["origin"]=origin;profile.summary["frame"]=frame;
    }
    if (profile.face.IsNull() || !BRepCheck_Analyzer(profile.face).IsValid())
        throw std::invalid_argument("Extrusion requires a current valid closed profile face");
    const auto sourceFrame=profile.summary["frame"].toObject();
    const auto normal = sourceFrame["normal"].toArray();
    if(normal.size()!=3)throw std::invalid_argument("Extrusion profile has no resolved frame");
    BRepPrimAPI_MakePrism prism(profile.face,gp_Vec(normal[0].toDouble()*distanceMm,
        normal[1].toDouble()*distanceMm,normal[2].toDouble()*distanceMm),true,true);
    if (!prism.IsDone()) throw std::runtime_error("OCCT could not build the profile extrusion");
    const auto shape = prism.Shape();
    if(baseFace){const auto base=prism.FirstShape();
        if(base.IsNull() || base.ShapeType()!=TopAbs_FACE)
            throw std::runtime_error("Prism has no exact base face");
        TopTools_IndexedMapOfShape currentFaces;TopExp::MapShapes(shape,TopAbs_FACE,currentFaces);
        if(!currentFaces.Contains(base))throw std::runtime_error("Prism base face is not current");
        *baseFace=TopoDS::Face(base);}
    if(edges) *edges=SolidFillet::prismEdges(prism,profile,generatorFeatureId);
    if(planarFaces)*planarFaces=PlanarFaces::prismSides(prism,profile,generatorFeatureId,distanceMm);
    if(endCap) {
        const auto cap=prism.LastShape();
        if(cap.IsNull() || cap.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(cap)).GetType()!=GeomAbs_Plane)
            throw std::runtime_error("Prism has no unique planar generated end cap");
        int matches=0;for(TopExp_Explorer face(shape,TopAbs_FACE);face.More();face.Next())if(face.Current().IsSame(cap))++matches;
        if(matches!=1)throw std::runtime_error("Generated end cap is not unique on the prism result");
        auto transported=sourceFrame;auto origin=sourceFrame["origin"].toArray();
        for(int i=0;i<3;++i)origin[i]=origin[i].toDouble()+normal[i].toDouble()*distanceMm;
        transported["origin"]=origin;
        const gp_Pnt expected(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble());
        const gp_Dir direction(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
        const auto realPlane=BRepAdaptor_Surface(TopoDS::Face(cap)).Plane();
        if(realPlane.Distance(expected)>1e-6 || std::abs(realPlane.Axis().Direction().Dot(direction))<1-1e-9)
            throw std::runtime_error("Generated end cap does not match the transported source frame");
        endCap->role="extrude.endCap";endCap->face=TopoDS::Face(cap);endCap->frame=transported;
        TopTools_IndexedMapOfShape capEdges;TopExp::MapShapes(cap,TopAbs_EDGE,capEdges);
        QSet<QString> identities;
        for(const auto& boundary:profile.boundaryEdges) {
            const auto transportedEdge=prism.LastShape(boundary.edge);
            if(transportedEdge.IsNull() || transportedEdge.ShapeType()!=TopAbs_EDGE || !capEdges.Contains(transportedEdge))
                reject("projection_lineage_unavailable","A source boundary has no unique actual prism cap edge");
            for(const auto& other:profile.boundaryEdges)if(other.sourceEntityId!=boundary.sourceEntityId && other.edge.IsSame(boundary.edge))
                reject("ambiguous_projection","A cap edge combines more than one authored source");
            QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
            const auto signature=boundary.sourceEntityId+"\n"+contacts.join("\n");
            const auto edgeId=QUuid::createUuidV5(QUuid(boundary.sourceSketchId),QByteArray("alloy-cap-edge/1\n")+signature.toUtf8()).toString(QUuid::WithoutBraces);
            if(boundary.type=="line" && identities.contains(edgeId))reject("ambiguous_projection","One authored boundary maps to multiple cap edges");identities.insert(edgeId);
            QJsonObject provenance{{"sourceSketchId",boundary.sourceSketchId},{"sourceEntityId",boundary.sourceEntityId},
                {"startContact",contacts[0]},{"endContact",contacts[1]}};
            TopoDS_Vertex start,end;TopExp::Vertices(TopoDS::Edge(transportedEdge),start,end,true);
            auto a=BRep_Tool::Pnt(start),b=BRep_Tool::Pnt(end);
            if(boundary.type=="line") {
                TopoDS_Vertex originalStart,originalEnd;TopExp::Vertices(boundary.edge,originalStart,originalEnd,true);
                const auto shift=gp_Vec(normal[0].toDouble()*distanceMm,normal[1].toDouble()*distanceMm,normal[2].toDouble()*distanceMm);
                const auto expectedStart=BRep_Tool::Pnt(originalStart).Translated(shift),expectedEnd=BRep_Tool::Pnt(originalEnd).Translated(shift);
                if(a.Distance(expectedStart)<=1e-6 && b.Distance(expectedEnd)<=1e-6){}
                else if(b.Distance(expectedStart)<=1e-6 && a.Distance(expectedEnd)<=1e-6)std::swap(a,b);
                else reject("ambiguous_projection","Prism endpoint lineage does not match the source boundary");
            }
            if(boundary.startContact>boundary.endContact)std::swap(a,b);
            provenance["startMm"]=QJsonArray{a.X(),a.Y(),a.Z()};provenance["endMm"]=QJsonArray{b.X(),b.Y(),b.Z()};
            endCap->boundaryEdges.push_back({edgeId,boundary.type,TopoDS::Edge(transportedEdge),provenance});
        }

    }
    if (shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_SOLID) != 1)
        throw std::runtime_error("Extrusion did not produce one valid solid");
    const auto summary = summarize(shape);
    const double expected = profile.summary["areaMm2"].toDouble()*std::abs(distanceMm);
    if (!summary["closed"].toBool() || !summary["volumeMm3"].isDouble() || !std::isfinite(expected)
        || expected <= 0 || std::abs(summary["volumeMm3"].toDouble()-expected) > std::max(1e-6,expected*1e-9))
        throw std::runtime_error("Extrusion volume does not match profile area times distance");
    return shape;
}
TopoDS_Shape GeometryKernel::taperedExtrusion(const EvaluatedProfile& profile, double distanceMm,
    double angleDeg, EvaluatedSupportFace* endCap) {
    if(!std::isfinite(angleDeg) || std::abs(angleDeg)>=90 || !std::isfinite(distanceMm) || distanceMm<=0)
        reject("invalid_extrude_taper","Taper requires a positive distance and a finite angle strictly between -90 and 90 degrees");
    if(angleDeg==0)return extrusion(profile,distanceMm,endCap);
    if(profile.face.IsNull() || count(profile.face,TopAbs_WIRE)!=1)
        reject("unsupported_extrude_taper","Taper requires one closed polygon without holes");
    for(TopExp_Explorer e(profile.face,TopAbs_EDGE);e.More();e.Next())
        if(BRepAdaptor_Curve(TopoDS::Edge(e.Current())).GetType()!=GeomAbs_Line)
            reject("unsupported_extrude_taper","Taper currently requires straight polygon edges");
    EvaluatedSupportFace cap;
    // The disposable prism owns its topology: draft cannot mutate retained input geometry.
    const auto prism=extrusion(profile,distanceMm,&cap);
    const auto frame=profile.summary.value("frame").toObject();
    const auto origin=frame.value("origin").toArray(),normal=frame.value("normal").toArray();
    if(origin.size()!=3 || normal.size()!=3)reject("invalid_extrude_taper","Taper requires a resolved profile frame");
    const gp_Dir direction(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble());
    const gp_Pln neutral(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()),direction);
    try {
        BRepOffsetAPI_DraftAngle draft(prism);
        for(TopExp_Explorer f(prism,TopAbs_FACE);f.More();f.Next()) {
            const auto face=TopoDS::Face(f.Current());
            const BRepAdaptor_Surface surface(face);
            if(surface.GetType()!=GeomAbs_Plane)reject("unsupported_extrude_taper","Taper requires planar polygon side faces");
            if(std::abs(surface.Plane().Axis().Direction().Dot(direction))>1e-8)continue;
            // OCCT positive removes material on the positive side of the neutral plane.
            draft.Add(face,direction,-angleDeg*std::numbers::pi/180,neutral);
            if(!draft.AddDone())reject("extrude_taper_failed","OCCT refused a tapered side face");
        }
        draft.Build();
        if(!draft.IsDone())reject("extrude_taper_failed","OCCT could not build the tapered solid");
        const auto shape=draft.Shape();
        if(shape.IsNull() || !BRepCheck_Analyzer(shape).IsValid() || count(shape,TopAbs_SOLID)!=1
            || count(shape,TopAbs_FACE)!=count(prism,TopAbs_FACE))
            reject("extrude_taper_failed","Taper collapsed or changed the polygon topology");
        const auto summary=summarize(shape);
        if(!summary.value("closed").toBool() || summary.value("volumeMm3").toDouble()<=0)
            reject("extrude_taper_failed","Taper did not produce a closed positive-volume solid");
        const auto modifiedCap=draft.ModifiedShape(cap.face);
        TopTools_IndexedMapOfShape faces;TopExp::MapShapes(shape,TopAbs_FACE,faces);
        if(modifiedCap.IsNull() || modifiedCap.ShapeType()!=TopAbs_FACE || !faces.Contains(modifiedCap))
            reject("extrude_taper_failed","Taper lost its retained far cap");
        cap.face=TopoDS::Face(modifiedCap);
        const BRepAdaptor_Surface capSurface(cap.face);
        const auto capOrigin=cap.frame.value("origin").toArray();
        if(capSurface.GetType()!=GeomAbs_Plane || capSurface.Plane().Distance(gp_Pnt(capOrigin[0].toDouble(),capOrigin[1].toDouble(),capOrigin[2].toDouble()))>1e-6
            || std::abs(capSurface.Plane().Axis().Direction().Dot(direction))<1-1e-9)
            reject("extrude_taper_failed","Taper moved its retained far plane");
        TopTools_IndexedMapOfShape capEdges;TopExp::MapShapes(cap.face,TopAbs_EDGE,capEdges);
        for(auto& edge:cap.boundaryEdges) {
            const auto mapped=draft.ModifiedShape(edge.edge);
            if(mapped.IsNull() || mapped.ShapeType()!=TopAbs_EDGE || !capEdges.Contains(mapped))
                reject("extrude_taper_failed","Taper lost a retained far-cap boundary");
            // Map the actual vertices too; never infer contact identity from new coordinates.
            TopoDS_Vertex a,b;TopExp::Vertices(edge.edge,a,b,true);
            const auto oldStart=edge.provenance.value("startMm").toArray();
            const gp_Pnt start(oldStart[0].toDouble(),oldStart[1].toDouble(),oldStart[2].toDouble());
            const bool reverse=BRep_Tool::Pnt(b).Distance(start)<BRep_Tool::Pnt(a).Distance(start);
            const auto mappedA=draft.ModifiedShape(a),mappedB=draft.ModifiedShape(b);
            if(mappedA.IsNull() || mappedB.IsNull() || mappedA.ShapeType()!=TopAbs_VERTEX || mappedB.ShapeType()!=TopAbs_VERTEX)
                reject("extrude_taper_failed","Taper lost a retained far-cap endpoint");
            edge.edge=TopoDS::Edge(mapped);a=TopoDS::Vertex(mappedA);b=TopoDS::Vertex(mappedB);
            TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(edge.edge,TopAbs_VERTEX,vertices);
            if(!vertices.Contains(a) || !vertices.Contains(b))reject("extrude_taper_failed","Taper endpoint is not on its retained edge");
            if(reverse)std::swap(a,b);
            const auto pa=BRep_Tool::Pnt(a),pb=BRep_Tool::Pnt(b);
            edge.provenance["startMm"]=QJsonArray{pa.X(),pa.Y(),pa.Z()};
            edge.provenance["endMm"]=QJsonArray{pb.X(),pb.Y(),pb.Z()};
        }
        if(endCap)*endCap=std::move(cap);
        return shape;
    } catch(const Standard_Failure&) {
        reject("extrude_taper_failed","OCCT could not construct the requested taper");
    }
}
gp_Trsf GeometryKernel::moveTransform(const QJsonObject& parameters) {
    gp_Trsf result;const gp_Vec pivot(parameters["pivotXMm"].toDouble(),parameters["pivotYMm"].toDouble(),parameters["pivotZMm"].toDouble());result.SetTranslation(pivot);
    const char* angles[]={"xDeg","yDeg","zDeg"};const gp_Dir axes[]={gp_Dir(1,0,0),gp_Dir(0,1,0),gp_Dir(0,0,1)};
    for(const auto& item:parameters["steps"].toArray()){const auto step=item.toObject();gp_Trsf rotation;
        for(int i=0;i<3;++i){gp_Trsf r;r.SetRotation(gp_Ax1(gp_Pnt(0,0,0),axes[i]),step[angles[i]].toDouble()*std::acos(-1.0)/180);rotation=r*rotation;}
        gp_Trsf translation;translation.SetTranslation(gp_Vec(step["xMm"].toDouble(),step["yMm"].toDouble(),step["zMm"].toDouble()));result=result*translation*rotation;
    }
    gp_Trsf undoPivot;undoPivot.SetTranslation(-pivot);return result*undoPivot;
}
TopoDS_Shape GeometryKernel::booleanOperation(const QString& operation, const TopoDS_Shape& target,
    const std::vector<TopoDS_Shape>& tools, std::vector<EvaluatedSupportFace>* retainedFaces, std::vector<EvaluatedSolidEdge>* retainedEdges,
    const std::vector<EvaluatedSolidEdge>* cutToolEdges,const QString& cutFeatureId,
    const std::vector<EvaluatedSupportFace>* cutToolFaces,
    const std::vector<EvaluatedJoinToolBase>* joinToolBases) {
    auto reject = [](const QString& code, const QString& message) {
        throw EvaluationError({{"code",code},{"message",message}});
    };
    BOPAlgo_Operation kind;
    if (operation == "join") kind = BOPAlgo_FUSE;
    else if (operation == "cut") kind = BOPAlgo_CUT;
    else if (operation == "intersect") kind = BOPAlgo_COMMON;
    else throw std::invalid_argument("Unsupported Boolean operation");
    if (target.IsNull() || tools.empty()) throw std::invalid_argument("Boolean operation needs a target and at least one tool");
    auto validInput = [](const TopoDS_Shape& shape) {
        if (shape.IsNull() || count(shape,TopAbs_SOLID) != 1) return false;
        const auto summary = summarize(shape);
        return summary["valid"].toBool() && summary["closed"].toBool() && summary["volumeMm3"].isDouble();
    };
    if (!validInput(target)) reject("boolean_input_invalid","Boolean target must be one valid closed solid");
    TopTools_ListOfShape objects, operands;objects.Append(target);
    for (const auto& tool : tools) {
        if (!validInput(tool)) reject("boolean_input_invalid","Every Boolean tool must be one valid closed solid");
        operands.Append(tool);
    }
    if (tools.size() > 1) {
        // Common with several separate tool arguments can preserve their
        // partition boundaries as adjacent solids. Unite the tool group first
        // so Intersect means overlap with their union, with no split artifacts.
        TopTools_ListOfShape groupObjects,groupTools;groupObjects.Append(tools.front());
        for (size_t i = 1; i < tools.size(); ++i) groupTools.Append(tools[i]);
        ValidatedBoolean group;
        group.SetArguments(groupObjects);group.SetTools(groupTools);group.SetOperation(BOPAlgo_FUSE);
        group.SetNonDestructive(true);group.SetRunParallel(false);group.SetFuzzyValue(0.0);group.Build();
        if (!group.IsDone() || group.HasErrors()) reject("boolean_kernel_failed","OCCT could not unite the Boolean tool group");
        group.simplifyKeepingValid();
        if (group.Shape().IsNull() || !BRepCheck_Analyzer(group.Shape()).IsValid())
            reject("boolean_input_invalid","Boolean tool group did not produce valid topology");
        operands.Clear();operands.Append(group.Shape());
    }
    ValidatedBoolean builder;
    builder.SetArguments(objects);builder.SetTools(operands);builder.SetOperation(kind);
    // Candidate and undo snapshots share input shapes. Never let the algorithm
    // modify their topology/tolerances while evaluating a disposable candidate.
    builder.SetNonDestructive(true);builder.SetRunParallel(false);builder.SetFuzzyValue(0.0);
    builder.Build();
    if (!builder.IsDone() || builder.HasErrors()) reject("boolean_kernel_failed","OCCT could not complete the Boolean operation");
    builder.simplifyKeepingValid();
    if(qEnvironmentVariableIsSet("ALLOY_BOOLEAN_DIAGNOSTICS"))
        qInfo().noquote()<<"Boolean simplification"<<QJsonDocument(builder.simplificationStatus).toJson(QJsonDocument::Compact);
    const auto shape = builder.Shape();
    if (shape.IsNull() || count(shape,TopAbs_SOLID) == 0)
        reject("boolean_empty_result","No solid result remains. Check body overlap or change Operation; the target was preserved");
    if (count(shape,TopAbs_SOLID) != 1)
        reject("boolean_multiple_solids","This Combine slice requires one connected solid; disconnected or split results are unsupported");
    // Refuse stray faces/edges rather than silently dropping lower-dimensional
    // pieces from an otherwise positive-volume result.
    TopExp_Explorer solids(shape,TopAbs_SOLID);
    const auto solid = solids.Current();
    for (const auto kind : {TopAbs_FACE,TopAbs_EDGE,TopAbs_VERTEX})
        if (count(shape,kind) != count(solid,kind))
            reject("boolean_mixed_result","Boolean result contains geometry outside its single solid");
    const auto summary = summarize(shape);
    if (!summary["valid"].toBool() || !summary["closed"].toBool() || !summary["volumeMm3"].isDouble())
        throw EvaluationError({{"code","boolean_result_invalid"},{"message","Boolean operation did not produce a valid closed positive-volume solid"},
            {"simplification",builder.simplificationStatus}});
    std::vector<EvaluatedSupportFace> priorFaces;
    if(retainedFaces) {
        priorFaces=*retainedFaces;
        const auto& sources=priorFaces;
        *retainedFaces=PlanarFaces::transfer(builder,sources,shape);
        if(operation=="cut" && cutToolEdges)PlanarFaces::retainLinearCutBoundaries(builder,sources,*cutToolEdges,*retainedFaces,cutFeatureId);
        if(operation=="cut" && cutToolEdges && cutToolFaces)
            PlanarFaces::retainLinearCutSides(builder,*cutToolFaces,*cutToolEdges,shape,*retainedFaces,cutFeatureId);
    }
    if(retainedEdges) {
        *retainedEdges=SolidFillet::transferEdges(builder,*retainedEdges,shape);
        if(operation=="cut" && retainedFaces && cutToolEdges && !cutFeatureId.isEmpty()) {
            auto rims=PlanarFaces::solidLinearCutRims(*retainedFaces,*cutToolEdges,shape,cutFeatureId);
            retainedEdges->insert(retainedEdges->end(),rims.begin(),rims.end());
        }
        if(operation=="join" && retainedFaces && cutToolEdges && joinToolBases && !cutFeatureId.isEmpty()){
            const auto roots=JoinRootEdges::publish(builder,priorFaces,*joinToolBases,*cutToolEdges,
                shape,*retainedFaces,cutFeatureId);
            for(const auto& root:roots)if(std::none_of(retainedEdges->begin(),retainedEdges->end(),
                [&](const auto& item){return !item.edge.IsNull() && item.edge.IsSame(root.edge);}))
                retainedEdges->push_back(root);
        }
    }
    return shape;
}
QJsonObject GeometryKernel::summarize(const TopoDS_Shape& shape) {
    if (shape.IsNull()) throw std::invalid_argument("Cannot summarize a null shape");
    const bool valid=BRepCheck_Analyzer(shape).IsValid();
    TopTools_IndexedMapOfShape shells; TopExp::MapShapes(shape,TopAbs_SHELL,shells);
    bool closed=shells.Extent()>0;
    for(int i=1;i<=shells.Extent();++i) closed=closed && BRep_Tool::IsClosed(shells(i));
    const int solids=count(shape,TopAbs_SOLID);
    Bnd_Box bounds; BRepBndLib::AddOptimal(shape,bounds,false,false);
    double x0,y0,z0,x1,y1,z1; bounds.Get(x0,y0,z0,x1,y1,z1);
    GProp_GProps area; BRepGProp::SurfaceProperties(shape,area);
    QJsonValue volume(QJsonValue::Null);
    if (valid && closed && solids>0) {
        bool exactOffset=false;
        for(TopExp_Explorer edge(shape,TopAbs_EDGE);edge.More() && !exactOffset;edge.Next()){
            double first,last;
            const auto curve=BRep_Tool::Curve(TopoDS::Edge(edge.Current()),first,last);
            exactOffset=!Handle(Geom_OffsetCurve)::DownCast(curve).IsNull();
        }
        GProp_GProps properties;
        if(exactOffset)BRepGProp::VolumePropertiesGK(shape,properties,1e-9,true);
        else BRepGProp::VolumeProperties(shape,properties,true);
        if (std::isfinite(properties.Mass()) && properties.Mass()>0) volume=properties.Mass();
    }
    return {{"kernel","OCCT"},{"version",ALLOY_OCCT_VERSION},{"valid",valid},{"closed",closed},
        {"type",solids>0 ? "solid" : "surface"},{"lengthUnit","mm"},{"areaUnit","mm2"},{"volumeUnit","mm3"},
        {"volumeMm3",volume},{"areaMm2",area.Mass()},
        {"topology",QJsonObject{{"solids",solids},{"shells",shells.Extent()},
            {"faces",count(shape,TopAbs_FACE)},{"edges",count(shape,TopAbs_EDGE)},{"vertices",count(shape,TopAbs_VERTEX)}}},
        {"boundsMm",QJsonObject{{"min",QJsonArray{x0,y0,z0}},{"max",QJsonArray{x1,y1,z1}},
            {"size",QJsonArray{x1-x0,y1-y0,z1-z0}}}}};
}
std::vector<EvaluatedBody> GeometryKernel::evaluate(const QJsonObject& authored) {
    return evaluateDocument(authored).bodies;
}
EvaluatedDocument GeometryKernel::evaluateDocument(const QJsonObject& authored) {
    return evaluateDocument(authored,EvaluatedDocument{});
}
EvaluatedDocument GeometryKernel::evaluateDocument(const QJsonObject& authored,const EvaluatedDocument& prefix) {
    auto evaluated=prefix;
    evaluated.recompute=prefix.featureOrder.empty() ? "full_ordered" : "exact_prefix_append";
    const auto features=authored["features"].toArray();
    if(prefix.featureOrder.size()>static_cast<size_t>(features.size()))
        reject("invalid_evaluation_prefix","Evaluated prefix exceeds authored history");
    for(size_t i=0;i<prefix.featureOrder.size();++i)
        if(prefix.featureOrder[i]!=features[static_cast<int>(i)].toObject()["id"].toString())
            reject("invalid_evaluation_prefix","Evaluated prefix order does not match authored history");
    // First suppression slice: independent Boxes with retained planar Offset Face
    // chains. Refuse other families rather than guessing their dependency graph.
    bool suppressionActive=false;
    for(const auto& row:features)suppressionActive|=row.toObject().value("suppressed").toBool();
    if(suppressionActive && !authored.value("joints").toArray().isEmpty())
        reject("suppression_unsupported","Suppression does not yet support assembly joints");
    QMap<QString,QString> lastBodyFeature;
    evaluated.featureStates={};
    for(const auto& row:features) {
        const auto f=row.toObject();const auto id=f.value("id").toString(),type=f.value("type").toString();
        if(f.contains("suppressed") && (!f.value("suppressed").isBool() || (type!="box" && type!="faceOffset")))
            throw EvaluationError({{"code","suppression_unsupported"},{"message","Suppression requires a boolean on a supported Box or Offset Face feature"},{"featureId",id},{"featureType",type}});
        if(suppressionActive && type!="box" && type!="faceOffset")
            throw EvaluationError({{"code","suppression_unsupported"},{"message","Suppression currently supports only Box and Offset Face histories"},{"featureId",id},{"featureType",type}});
        const auto parameters=f.value("parameters").toObject();QString dependency;
        if(type=="faceOffset") {
            const auto prior=lastBodyFeature.value(parameters.value("targetBodyId").toString());
            const auto producer=parameters.value("face").toObject().value("producerFeatureId").toString();
            for(const auto& ref:{prior,producer})if(evaluated.featureStates.value(ref).toObject().value("effectiveSuppressed").toBool()) {dependency=ref;break;}
        }
        const bool explicitSuppressed=f.value("suppressed").toBool();
        evaluated.featureStates[id]=QJsonObject{{"explicitSuppressed",explicitSuppressed},{"effectiveSuppressed",explicitSuppressed || !dependency.isEmpty()},{"dependencyFeatureId",dependency}};
        if(type=="box")lastBodyFeature[f.value("bodyId").toString()]=id;
        else if(type=="faceOffset")lastBodyFeature[parameters.value("targetBodyId").toString()]=id;
    }
    QMap<QString,size_t> priorSketches;
    for(size_t i=0;i<evaluated.sketches.size();++i)priorSketches[evaluated.sketches[i].featureId]=i;
    for (int featureIndex=static_cast<int>(prefix.featureOrder.size());featureIndex<features.size();++featureIndex) {
        const auto entry=features[featureIndex];
        const auto feature = entry.toObject();
        const auto id = feature["id"].toString(), type = feature["type"].toString();
        const auto parameters = feature["parameters"].toObject();
        QJsonObject context{{"featureId",id},{"featureType",type}};
        try {
            if(evaluated.featureStates.value(id).toObject().value("effectiveSuppressed").toBool()) {
                evaluated.featureOrder.push_back(id);continue;
            }
            if(type=="assemblyPin") {
                // Pin constrains occurrence presentation. Definition geometry and
                // its authored history prefix remain unchanged.
                const auto occurrenceId=parameters["occurrenceId"].toString();
                bool current=false;
                for(const auto& value:authored.value("occurrences").toArray())
                    if(value.toObject()["id"]==occurrenceId)current=true;
                if(!current)reject("pin_occurrence_unavailable","Pin occurrence is unavailable");
            } else if(type=="constructionPlane") {
                const auto source=parameters["source"].toObject();
                auto frame=source.contains("faces") ? midplaneFrame(evaluated,source["faces"].toArray())
                    : source.contains("tangent") ? tangentPointPlaneFrame(evaluated,source["tangent"].toObject())
                    : source.contains("path") ? pathEndpointPlaneFrame(evaluated,source["path"].toObject(),features)
                    : offsetPlaneFrame(resolvePlaneSource(evaluated,source),parameters["offsetMm"].toDouble());
                const auto bounds=frame.take("displayBoundsMm").toArray();
                evaluated.constructionPlanes.push_back({id,feature["name"].toString(),feature["visible"].toBool(),frame,{},bounds});
            } else if (type == "sketch") {
                priorSketches[id] = evaluated.sketches.size();
                QJsonObject resolvedFrame;
                if(parameters["plane"]=="face") {
                    try {resolvedFrame=sketchFrame(evaluated,feature);}
                    catch(const EvaluationError& error){auto diagnostic=context;const auto details=error.diagnostics();for(auto it=details.begin();it!=details.end();++it)diagnostic[it.key()]=it.value();throw EvaluationError(diagnostic);}
                }
                if(parameters["plane"]=="construction") {
                    for(const auto& plane:evaluated.constructionPlanes)if(plane.featureId==parameters["planeId"].toString())resolvedFrame=plane.frame;
                    if(resolvedFrame.isEmpty())throw EvaluationError({{"code","plane_unavailable"},{"message","Sketch construction plane is unavailable"},{"featureId",id}});
                }
                if(parameters.contains("loopOffsets")) {
                    const auto support=resolveSketchSupport(evaluated,parameters.value("support").toObject());
                    try {evaluated.sketches.push_back(RoundedLoopOffset::evaluate(feature,support,resolvedFrame));}
                    catch(const EvaluationError& error) {
                        auto diagnostic=error.diagnostics();
                        if(diagnostic.value("code")=="ambiguous_inset_lineage"
                            || (diagnostic.value("code")=="offset_lineage_unavailable" && parameters.value("loopOffsets").toArray().size()==1
                                && parameters.value("loopOffsets").toArray().first().toObject().value("kind")=="planarInsetV1")) {
                            // The loop evaluator owns the error; this boundary owns
                            // the authoritative sketch and already-resolved support.
                            diagnostic["featureId"]=id;diagnostic["featureType"]=type;
                            diagnostic["bodyId"]=support.bodyId;diagnostic["producerFeatureId"]=support.producerFeatureId;
                            diagnostic["faceId"]=support.faceId;throw EvaluationError(diagnostic);
                        }
                        throw;
                    }
                } else evaluated.sketches.push_back(SketchGeometry::evaluateFeature(feature,resolvedFrame,projectionContext(evaluated,feature)));
            } else if(type=="bodyCopy") {
                evaluated.bodies.push_back(BodyCopy::evaluate(feature));
            } else if (type == "stepImport" || type == "stepAssemblyImport") {
                const auto failStep=[&](const QString& code,const QString& message) {
                    auto details=context;details["code"]=code;details["message"]=message;
                    throw EvaluationError(details);
                };
                const auto source=parameters["source"].toObject();
                const auto encoded=source["bytesBase64"].toString().toLatin1();
                if (encoded.size()>44739244)
                    failStep("step_source_size","Embedded STEP source exceeds 32 MiB");
                const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
                if (!decoded || decoded.decoded.isEmpty() || decoded.decoded.size()>StepExchange::maximumSourceBytes
                    || decoded.decoded.size()!=source["byteCount"].toInteger()
                    || decoded.decoded.toBase64()!=encoded)
                    failStep("step_source_corrupt","Embedded STEP source encoding or byte count is invalid");
                const auto digest=QCryptographicHash::hash(decoded.decoded,QCryptographicHash::Sha256).toHex();
                if (QString::fromLatin1(digest)!=source["sha256"].toString())
                    failStep("step_source_hash","Embedded STEP source hash does not match its bytes");
                try {
                    auto append=[&](const TopoDS_Shape& shape,const QString& bodyId,const QString& name,
                                    const QJsonObject& diagnostics,const QString& occurrenceId={},const QString& definitionId={}) {
                        auto summary=summarize(shape);
                        if (!summary["valid"].toBool() || !summary["closed"].toBool()
                            || summary["topology"].toObject()["solids"].toInt()!=1
                            || !summary["volumeMm3"].isDouble()
                            || !std::isfinite(summary["volumeMm3"].toDouble())
                            || summary["volumeMm3"].toDouble()<=0)
                            failStep("step_invalid_solid","Imported STEP leaf must be a valid closed solid with positive volume");
                        summary["sourceFormat"]="step";summary["sourcePolicy"]="embedded";
                        summary["sourceSha256"]=source["sha256"];
                        summary["sourceOriginalName"]=source["originalName"];
                        summary["sourceLengthUnits"]=diagnostics["fileLengthUnits"];
                        summary["translator"]=diagnostics["translator"];
                        std::vector<EvaluatedSupportFace> faces;std::vector<EvaluatedSolidEdge> edges;
                        if(!occurrenceId.isEmpty()) {
                            importedTopology(shape,id,bodyId,occurrenceId,faces,edges);
                            summary["supportFaces"]=PlanarFaces::descriptors(faces);
                        }
                        evaluated.bodies.push_back({id,bodyId,name,feature["visible"].toBool(),shape,summary,faces,edges,
                            feature["ownerId"].toString(),occurrenceId,definitionId});
                    };
                    if(type=="stepImport") {
                        const auto imported=StepExchange::read(decoded.decoded);
                        append(imported.shape,feature["bodyId"].toString(),feature["name"].toString(),imported.diagnostics);
                    } else {
                        const auto assembly=StepExchange::readAssembly(decoded.decoded,id);
                        if(assembly.graph!=parameters["assembly"].toObject())
                            failStep("step_assembly_snapshot","Embedded STEP hierarchy differs from its authored snapshot");
                        const auto linked=parameters.value("reference").toObject();
                        const auto outer=linked.isEmpty() ? gp_Trsf{} : LocalReference::transform(linked["outerPlacement"].toObject());
                        for(const auto& leaf:assembly.leaves){
                            append(leaf.shape,leaf.bodyId,leaf.name,assembly.diagnostics,leaf.occurrenceId,leaf.definitionId);
                            if(!linked.isEmpty()){
                                auto& body=evaluated.bodies.back();body=LocalReference::placed(body,outer);
                                body.summary["outerOccurrenceId"]=LocalReference::outerOccurrenceId(id);
                            }
                        }
                    }
                } catch(const StepExchange::Error& error) {
                    const auto details=error.diagnostics();
                    for(auto it=details.begin();it!=details.end();++it)context[it.key()]=it.value();
                    throw EvaluationError(context);
                }
            } else if(type=="fillet") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Fillet target must be active earlier in history"}});
                for(const auto& value:parameters["edgeIds"].toArray())for(const auto& edge:target->solidEdges)
                    if(edge.edgeId==value.toString() && !edge.edge.IsNull())evaluated.finishFeatureInputs[id].push_back(edge);
                SolidFillet::evaluate(*target,parameters,id);
            } else if(type=="chamfer") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Chamfer target must be active earlier in history"}});
                for(const auto& value:parameters["edgeIds"].toArray())for(const auto& edge:target->solidEdges)
                    if(edge.edgeId==value.toString() && !edge.edge.IsNull())evaluated.finishFeatureInputs[id].push_back(edge);
                SolidChamfer::evaluate(*target,parameters,id);
            } else if(type=="rectangularPattern") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                QJsonObject seed;for(const auto& value:authored["features"].toArray())if(value.toObject()["id"]==parameters["sourceFeatureId"]){seed=value.toObject();break;}
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"message","Pattern target must be active earlier in history"}});
                try {FeaturePattern::evaluate(*target,feature,seed,evaluated);}
                catch(const EvaluationError& error) {
                    auto details=error.diagnostics();details["featureId"]=id;details["featureType"]="rectangularPattern";
                    details["sourceFeatureId"]=parameters["sourceFeatureId"];
                    details["targetBodyId"]=parameters["targetBodyId"];
                    throw EvaluationError(details);
                }
            } else if(type=="featureMirror") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())reject("body_unavailable","Feature Mirror target must be active earlier in history");
                const auto firstId=parameters["sourceFeatureIds"].toArray().first().toString();
                QJsonObject firstSource;for(const auto& value:features)if(value.toObject()["id"]==firstId){firstSource=value.toObject();break;}
                if(firstSource["parameters"].toObject()["operation"]=="join")
                    FeaturePattern::mirrorSnapGroup(*target,feature,features,evaluated);
                else FeaturePattern::mirrorCuts(*target,feature,evaluated);
            } else if(type=="sweep") {
                QJsonObject source;for(const auto& entry:authored["features"].toArray())if(entry.toObject()["id"]==parameters["pathSketchId"])source=entry.toObject();
                evaluated.bodies.push_back(SolidSweep::evaluate(feature,evaluated,source));
            } else if(type=="revolve") {
                QJsonObject source;for(const auto& entry:authored["features"].toArray())if(entry.toObject()["id"]==parameters["sketchId"])source=entry.toObject();
                evaluated.bodies.push_back(SolidRevolve::evaluate(feature,evaluated,source));
            } else if(type=="loft") {
                evaluated.bodies.push_back(SolidLoft::evaluate(feature,evaluated));
            } else if(type=="surfacePatch") {
                evaluated.bodies.push_back(SurfacePatch::evaluate(feature,evaluated));
            } else if(type=="surfaceSweep") {
                evaluated.bodies.push_back(SurfaceSweep::evaluate(feature,evaluated));
            } else if(type=="surfaceRuled") {
                evaluated.bodies.push_back(SurfaceRuled::evaluate(feature,evaluated));
            } else if(type=="surfaceOffset") {
                evaluated.bodies.push_back(SurfaceOffset::evaluate(feature,evaluated));
            } else if(type=="surfaceRevolve") {
                evaluated.bodies.push_back(SurfaceRevolve::evaluate(feature,evaluated));
            } else if(type=="surfaceExtrude") {
                const auto results=SurfaceExtrude::evaluateResults(feature,evaluated);
                evaluated.bodies.insert(evaluated.bodies.end(),results.begin(),results.end());
            } else if(type=="boundaryFill") {
                auto solid=BoundaryFill::evaluate(feature,evaluated);
                if(parameters["removeTools"].toBool()){
                    QSet<QString> consumed;for(const auto& ref:parameters["tools"].toArray())consumed.insert(ref.toObject()["bodyId"].toString());
                    std::erase_if(evaluated.bodies,[&](const auto& b){return consumed.contains(b.bodyId);});
                }
                evaluated.bodies.push_back(std::move(solid));
            } else if(type=="surfaceTrim") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),
                    [&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())reject("trim_target_unavailable","Trim target must be active earlier in history");
                SurfaceTrim::evaluate(*target,feature,evaluated);
            } else if(type=="removeBody") {
                const auto bodyId=parameters["bodyId"].toString();
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),
                    [&](const auto& body){return body.bodyId==bodyId;});
                if(target==evaluated.bodies.end())reject("remove_body_unavailable","Remove target must be active earlier in history");
                if(!target->occurrenceId.isEmpty() || target->summary["type"]!="surface"
                    || target->summary["topology"].toObject()["solids"].toInt()!=0)
                    reject("remove_body_unsupported","This Remove slice accepts one local surface body");
                evaluated.bodies.erase(target);
            } else if(type=="thicken") {
                const auto ref=parameters["sourceFace"].toObject();
                const auto source=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==ref["bodyId"].toString();});
                if(source==evaluated.bodies.end())reject("surface_unavailable","Thicken source must be an earlier retained sheet");
                auto solid=SolidThicken::evaluate(feature,*source);
                evaluated.bodies.push_back(std::move(solid));
            } else if(type=="sphere") {
                SolidSphere::evaluate(evaluated,feature);
            } else if(type=="draft") {
                SolidDraft::evaluate(evaluated,feature);
            } else if(type=="faceOffset") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())reject("face_offset_body_unavailable","Offset Face requires an active earlier solid body");
                SolidFaceOffset::evaluate(*target,parameters,id);
            } else if(type=="thread") {
                const auto support=parameters["support"].toObject();
                context["support"]=support;context["targetBodyId"]=support["bodyId"];
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& b){return b.bodyId==support["bodyId"].toString();});
                if(target==evaluated.bodies.end())reject("thread_body_unavailable","Thread body must be active earlier in history");
                const EvaluatedSupportFace* selected=nullptr;
                for(const auto& face:target->supportFaces)if(face.faceId==support["faceId"].toString() && face.producerFeatureId==support["producerFeatureId"].toString()) {
                    if(selected)reject("thread_face_ambiguous","Thread face has multiple descendants");selected=&face;
                }
                if(!selected || selected->face.IsNull())reject("thread_face_unavailable","The exact retained cylindrical face is unavailable");
                int matches=0;for(TopExp_Explorer it(target->shape,TopAbs_FACE);it.More();it.Next())if(it.Current().IsSame(selected->face))++matches;
                if(matches!=1)reject("thread_face_unavailable","Thread reference is not one current body face");
                const auto inputSummary=summarize(target->shape);
                if(!inputSummary["valid"].toBool() || !inputSummary["closed"].toBool() || inputSummary["topology"].toObject()["solids"].toInt()!=1 || !target->occurrenceId.isEmpty())
                    reject("thread_source_unsupported","Thread requires one valid local solid");
                BRepAdaptor_Surface surface(selected->face);
                if(surface.GetType()!=GeomAbs_Cylinder)reject("thread_face_unsupported","Thread requires an analytic cylindrical face");
                const auto cylinder=surface.Cylinder();
                if(std::abs(2*cylinder.Radius()-parameters["nominalDiameterMm"].toDouble())>1e-5)
                    reject("thread_diameter_mismatch","Selected cylinder diameter must match the nominal designation");
                double u0,u1,v0,v1;BRepTools::UVBounds(selected->face,u0,u1,v0,v1);
                int wires=0;for(TopExp_Explorer it(selected->face,TopAbs_WIRE);it.More();it.Next())++wires;
                GProp_GProps area;BRepGProp::SurfaceProperties(selected->face,area);
                const double span=v1-v0;
                if(!std::isfinite(span) || span<0.001 || std::abs((u1-u0)-2*std::numbers::pi)>1e-6 || wires!=1
                    || std::abs(area.Mass()-2*std::numbers::pi*cylinder.Radius()*span)>1e-5)
                    reject("thread_face_trimmed","Thread requires one complete finite cylindrical face without openings");
                gp_Pnt sample;gp_Vec du,dv;surface.D1((u0+u1)/2,(v0+v1)/2,sample,du,dv);
                auto normal=du.Crossed(dv);if(selected->face.Orientation()==TopAbs_REVERSED)normal.Reverse();
                const auto axial=cylinder.Axis().Direction();const gp_Vec relative(cylinder.Location(),sample);
                const auto radial=relative-gp_Vec(axial)*relative.Dot(gp_Vec(axial));
                if(normal.Dot(radial)<=0)reject("thread_internal_unsupported","Only an external cylindrical face is supported");
                SolidThreadGeometry::Parameters settings;
                settings.axis=gp_Ax2(cylinder.Location().Translated(gp_Vec(axial)*v0),axial,cylinder.XAxis().Direction());
                settings.majorRadius=cylinder.Radius();settings.pitch=parameters["pitchMm"].toDouble();
                settings.start=parameters["startOffsetMm"].toDouble();settings.length=parameters["fullLength"].toBool()?span:parameters["lengthMm"].toDouble();
                settings.rightHand=parameters["direction"]=="right";
                if(settings.start<0 || settings.length<0.001 || settings.start+settings.length>span+1e-6 || settings.length/settings.pitch>100)
                    reject("thread_interval_invalid","Thread interval exceeds the selected face or 100-turn limit");
                std::vector<EvaluatedSupportFace> faces;
                TopoDS_Shape threaded;
                try {threaded=SolidThreadGeometry::cutExternal(target->shape,settings,[&](BRepAlgoAPI_Cut& operation){faces=PlanarFaces::transfer(operation,target->supportFaces,operation.Shape());});}
                catch(const std::exception& error){reject("thread_geometry_failed",QString::fromUtf8(error.what()));}
                target->shape=threaded;target->supportFaces=std::move(faces);target->solidEdges.clear();
                target->summary=summarize(threaded);target->summary["lastModifierFeatureId"]=id;target->summary["operation"]="thread";
                target->summary["supportFaces"]=PlanarFaces::descriptors(target->supportFaces);
                auto retained=parameters;retained["evaluatedLengthMm"]=settings.length;retained["usableLengthMm"]=span;
                target->summary["thread"]=retained;
            } else if(type=="hole") {
                QJsonObject source;for(const auto& entry:authored["features"].toArray())if(entry.toObject()["id"]==parameters["sourceSketchId"]) {source=entry.toObject();break;}
                try {SolidHole::evaluate(evaluated,feature,source);}
                catch(const EvaluationError& error) {
                    auto details=error.diagnostics();
                    details["featureId"]=id;details["featureType"]="hole";
                    for(const auto& key:{"sourceSketchId","sourceEntityId","sourcePointId","targetBodyId"})details[key]=parameters[key];
                    throw EvaluationError(details);
                }
            } else if(type=="shell") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Shell target must be active earlier in history"}});
                if(!ClosedBlendShell::evaluate(*target,parameters,id,features,featureIndex,evaluated))
                    SolidShell::evaluate(*target,parameters,id);
            } else if(type=="splitBody") {
                const auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["targetBodyId"].toString();});
                if(target==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Split target must be active earlier in history"}});
                const auto plane=std::find_if(evaluated.constructionPlanes.begin(),evaluated.constructionPlanes.end(),[&](const auto& value){return value.featureId==parameters["planeId"].toString();});
                if(plane==evaluated.constructionPlanes.end())throw EvaluationError({{"code","plane_unavailable"},{"featureId",id},{"message","Splitting plane must be retained earlier in history"}});
                const auto index=static_cast<size_t>(target-evaluated.bodies.begin());
                auto halves=SolidSplit::evaluate(*target,plane->frame,id,feature["bodyId"].toString(),feature["name"].toString());
                halves.second.visible=feature["visible"].toBool(true);
                evaluated.bodies[index]=std::move(halves.first);evaluated.bodies.push_back(std::move(halves.second));
            } else if(type=="rectangularBodyPattern"){
                const auto source=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& b){return b.bodyId==parameters["sourceBodyId"].toString();});
                if(source==evaluated.bodies.end())throw EvaluationError({{"code","pattern_body_unavailable"},{"featureId",id},{"message","Body Pattern source must be active earlier in history"}});
                auto occurrences=RectangularBodyPattern::evaluate(feature,*source);for(size_t n=1;n<occurrences.size();++n)evaluated.bodies.push_back(std::move(occurrences[n]));
            } else if(type=="circularPattern"){
                auto source=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& b){return b.bodyId==parameters["sourceBodyId"].toString();});
                if(source==evaluated.bodies.end())throw EvaluationError({{"code","pattern_body_unavailable"},{"featureId",id},{"message","Circular Pattern source must be active earlier in history"}});
                auto occurrences=CircularPattern::evaluate(feature,*source);*source=occurrences[0];for(size_t n=1;n<occurrences.size();++n)evaluated.bodies.push_back(occurrences[n]);
            } else if(type=="mirror") {
                const auto source=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==parameters["sourceBodyId"].toString();});
                if(source==evaluated.bodies.end())throw EvaluationError({{"code","body_unavailable"},{"featureId",id},{"message","Mirror source must be active earlier in history"}});
                const auto plane=resolveSketchSupport(evaluated,parameters["mirrorPlane"].toObject());
                auto mirrored=SolidMirror::evaluate(*source,plane,id,feature["bodyId"].toString(),feature["name"].toString(),feature["visible"].toBool());
                evaluated.bodies.push_back(mirrored);
            } else if(type=="scale") {
                auto scaled=Scale::evaluate(evaluated,feature,authored);
                for(auto& body:evaluated.bodies)if(body.bodyId==scaled.bodyId){body=std::move(scaled);break;}
            } else if(type=="align") {
                auto moved=Align::evaluate(evaluated,feature,authored);
                for(auto& body:evaluated.bodies)if(body.bodyId==moved.bodyId){body=std::move(moved);break;}
            } else if (type == "move") {
                const auto bodyId=parameters["targetBodyId"].toString();
                auto body=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& item){return item.bodyId==bodyId;});
                if(body==evaluated.bodies.end()){context["code"]="body_unavailable";context["message"]="Move body must be active earlier in history";throw EvaluationError(context);}
                const auto transform=moveTransform(parameters);
                BRepBuilderAPI_Transform builder(body->shape,transform,true);if(!builder.IsDone())throw std::runtime_error("Move transform failed");
                body->shape=builder.Shape();body->summary=summarize(body->shape);body->summary["lastModifierFeatureId"]=id;body->summary["operation"]="move";
                // Transported cap lineage is outside this bounded solid-body slice.
                body->supportFaces.clear();body->solidEdges.clear();
            } else if (type == "boolean") {
                const auto targetId = parameters["targetBodyId"].toString();
                const auto toolIds = parameters["toolBodyIds"].toArray();
                context["targetBodyId"] = targetId;context["toolBodyIds"] = toolIds;context["operation"] = parameters["operation"];
                auto findBody = [&](const QString& bodyId) {
                    return std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId == bodyId;});
                };
                auto target = findBody(targetId);
                if (target == evaluated.bodies.end()) {
                    context["code"] = "body_unavailable";context["message"] = "Boolean target must be an active body earlier in feature history";
                    throw EvaluationError(context);
                }
                std::vector<TopoDS_Shape> tools;
                for (const auto& toolId : toolIds) {
                    const auto tool = findBody(toolId.toString());
                    if (tool == evaluated.bodies.end()) {
                        context["code"] = "body_unavailable";context["bodyId"] = toolId;context["message"] = "Boolean tool must be an active body earlier in feature history";
                        throw EvaluationError(context);
                    }
                    tools.push_back(tool->shape);
                    if(parameters["operation"]=="join") {
                        for(auto face:tool->supportFaces){face.bodyId=targetId;target->supportFaces.push_back(face);}
                        target->solidEdges.insert(target->solidEdges.end(),tool->solidEdges.begin(),tool->solidEdges.end());
                    }
                }
                TopoDS_Shape shape;
                try {shape = booleanOperation(parameters["operation"].toString(),target->shape,tools,&target->supportFaces,&target->solidEdges);}
                catch (const EvaluationError& error) {
                    const auto details = error.diagnostics();
                    for (auto it = details.begin(); it != details.end(); ++it) context[it.key()] = it.value();
                    throw EvaluationError(context);
                }
                auto summary = summarize(shape);
                summary["operation"] = parameters["operation"];summary["lastModifierFeatureId"] = id;
                summary["producerFeatureId"] = target->featureId;summary["targetBodyId"] = targetId;
                summary["toolBodyIds"] = toolIds;summary["keepTools"] = parameters["keepTools"];
                summary["supportFaces"]=PlanarFaces::descriptors(target->supportFaces);
                summary["solidEdges"]=SolidFillet::descriptors(target->solidEdges);
                target->shape = shape;target->summary = summary;
                if (!parameters["keepTools"].toBool())
                    std::erase_if(evaluated.bodies,[&](const auto& body){return toolIds.contains(body.bodyId);});
            } else {
                TopoDS_Shape shape;
                QJsonObject summary;std::vector<EvaluatedSupportFace> supportFaces;std::vector<EvaluatedSolidEdge> solidEdges;
                if (type == "extrude") {
                    const auto sketchId = parameters["sketchId"].toString();
                    const bool aggregate=parameters.contains("profileSelection"),plural=parameters.contains("profileIds") || aggregate;
                    QJsonArray profileIds=parameters.contains("profileIds") ? parameters["profileIds"].toArray() : aggregate ? QJsonArray{} : QJsonArray{parameters["profileId"]};
                    context["sketchId"] = sketchId;
                    context[plural ? "profileIds" : "profileId"] = plural ? QJsonValue(profileIds) : parameters["profileId"];
                    if (!priorSketches.contains(sketchId)) {
                        context["code"] = "source_unavailable"; context["message"] = "Extrusion source sketch must exist earlier in feature history";
                        throw EvaluationError(context);
                    }
                    const auto& source = evaluated.sketches[priorSketches.value(sketchId)];
                    std::vector<EvaluatedProfile> selected;
                    if(aggregate) {
                        QJsonObject sketchFeature;
                        for(const auto& row:features)if(row.toObject().value("id")==sketchId) {sketchFeature=row.toObject();break;}
                        try {selected=closedRectOffsetUnionCells(source,sketchFeature,parameters.value("profileSelection").toObject().value("groupIds").toArray());}
                        catch(const std::exception& error) {context["code"]="offset_union_unavailable";context["message"]=QString::fromUtf8(error.what());throw EvaluationError(context);}
                        for(const auto& cell:selected)profileIds.append(cell.profileId);
                        context["profileSelection"]=parameters.value("profileSelection");
                    }
                    if(parameters.contains("sourceEntityId"))selected.push_back(thinLineProfile(source,parameters.value("sourceEntityId").toString(),parameters.value("wallThicknessMm").toDouble(),parameters.value("wallLocation").toString()));
                    if(!aggregate && !parameters.contains("sourceEntityId"))for(const auto& requested:profileIds){const auto found=std::find_if(source.profiles.begin(),source.profiles.end(),[&](const auto& profile){return profile.profileId==requested.toString();});
                        if(found==source.profiles.end()) {context["code"]="profile_unavailable";context["message"]="The referenced profile no longer exists; repair the sketch or select a current closed profile";
                            context["missingProfileId"]=requested;context["profileDiagnostics"]=source.summary["profileDiagnostics"];throw EvaluationError(context);}
                        selected.push_back(*found);}
                    if(parameters["operation"]=="join" && feature.contains("bodyResults")) {
                        std::vector<EvaluatedBody> participants;
                        const auto targets=parameters["targetBodyIds"].toArray();
                        for(const auto& targetId:targets) {
                            const auto found=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==targetId.toString();});
                            if(found==evaluated.bodies.end())reject("body_unavailable","Join requires active earlier participant bodies");
                            participants.push_back(*found);
                        }
                        auto joined=joinResults(feature,participants,selected);
                        if(joined.sourceTools.size()==1){evaluated.joinFeatureTools[id]=joined.sourceTools;
                            evaluated.joinFeatureEdges[id]=joined.sourceEdges;
                            evaluated.joinFeatureCaps[id]=joined.sourceCaps;
                            evaluated.joinFeatureBases[id]=joined.sourceBases;}
                        std::erase_if(evaluated.bodies,[&](const auto& body){return targets.contains(body.bodyId);});
                        evaluated.bodies.insert(evaluated.bodies.end(),joined.bodies.begin(),joined.bodies.end());
                        evaluated.joinBodyResults[id]=joined.ownership;
                        evaluated.featureOrder.push_back(id);continue;
                    }
                    if(parameters["operation"]=="cut" || parameters["operation"]=="join" || parameters["operation"]=="intersect") {
                        const bool joining=parameters["operation"]=="join";
                        const auto targets=parameters["targetBodyIds"].toArray();context["targetBodyIds"]=targets;context["operation"]=parameters["operation"];
                        if(targets.size()!=1)reject("unsupported_extrude_targets","Extrude modifier requires one retained target body");
                        auto target=std::find_if(evaluated.bodies.begin(),evaluated.bodies.end(),[&](const auto& body){return body.bodyId==targets[0].toString();});
                        if(target==evaluated.bodies.end()) {context["code"]="body_unavailable";context["message"]="The exact extrusion target must be an active body earlier in feature history";throw EvaluationError(context);}
                        const double distance=extrusionDistance(parameters,selected.front(),evaluated),start=extrusionStart(parameters);std::vector<TopoDS_Shape> tools;std::vector<EvaluatedSupportFace> joinCaps;double area=0;QStringList sourceEntities;
                        const bool circularCut=parameters["operation"]=="cut" && selected.size()==1
                            && selected.front().boundaryEdges.size()==1 && selected.front().boundaryEdges.front().type=="circle";
                        const bool linearCut=parameters["operation"]=="cut" && selected.size()==1
                            && std::all_of(selected.front().boundaryEdges.begin(),selected.front().boundaryEdges.end(),[](const auto& e){return e.type=="line";});
                        std::vector<EvaluatedSolidEdge> cutToolEdges;
                        std::vector<EvaluatedSupportFace> cutToolFaces;
                        for(const auto& profile:selected) {EvaluatedSupportFace cap;std::vector<EvaluatedSolidEdge> toolEdges;
                            std::vector<EvaluatedSupportFace> toolFaces;
                            tools.push_back(extrusion(profile,distance,joining ? &cap : nullptr,
                                (joining || circularCut || linearCut) ? &toolEdges : nullptr,id,
                                linearCut ? &toolFaces : nullptr,start));
                            if(linearCut) {
                                for(auto& face:toolFaces)face.bodyId=target->bodyId;
                                cutToolFaces.insert(cutToolFaces.end(),toolFaces.begin(),toolFaces.end());
                            }
                            if(joining)cutToolEdges.insert(cutToolEdges.end(),toolEdges.begin(),toolEdges.end());
                            if(linearCut)cutToolEdges.insert(cutToolEdges.end(),toolEdges.begin(),toolEdges.end());
                            if(circularCut)for(auto& edge:toolEdges)if(edge.provenance["role"]=="baseRim")
                                target->solidEdges.push_back(std::move(edge));
                            if(!cap.face.IsNull())joinCaps.push_back(cap);area+=profile.summary["areaMm2"].toDouble();
                            for(const auto& entity:profile.summary["sourceEntityIds"].toArray())sourceEntities.append(entity.toString());}
                        if(linearCut){
                            evaluated.cutFeatureTools[id]=tools;
                            evaluated.cutFeatureEdges[id]=cutToolEdges;
                        }
                        if(joining && selected.size()==1){
                            evaluated.joinFeatureTools[id]=tools;
                            evaluated.joinFeatureEdges[id]=cutToolEdges;
                            evaluated.joinFeatureCaps[id]=joinCaps;
                        }
                        const double previousVolume=target->summary["volumeMm3"].toDouble();
                        TopoDS_Shape cutShape;
                        std::vector<EvaluatedSolidEdge> joinedEdges;
                        if(joining){joinedEdges=target->solidEdges;
                            joinedEdges.insert(joinedEdges.end(),cutToolEdges.begin(),cutToolEdges.end());}
                        try {cutShape=booleanOperation(parameters["operation"].toString(),target->shape,tools,&target->supportFaces,joining ? &joinedEdges : &target->solidEdges,linearCut ? &cutToolEdges : nullptr,id,linearCut ? &cutToolFaces : nullptr);}
                        catch(const EvaluationError& error){const auto details=error.diagnostics();for(auto it=details.begin();it!=details.end();++it)context[it.key()]=it.value();throw EvaluationError(context);}
                        if(joining)target->solidEdges=std::move(joinedEdges);
                        auto cutSummary=summarize(cutShape);
                        const double removed=joining ? cutSummary["volumeMm3"].toDouble()-previousVolume : previousVolume-cutSummary["volumeMm3"].toDouble();
                        if(parameters["operation"]!="intersect" && removed<=std::max(1e-6,previousVolume*1e-12)){context["code"]=joining ? "join_no_addition" : "cut_no_intersection";context["message"]=joining ? "Selected profile prisms do not add positive volume to the retained target" : "Selected profile prisms do not remove positive volume from the retained target";throw EvaluationError(context);}
                        cutSummary["operation"]=parameters["operation"];cutSummary["lastModifierFeatureId"]=id;cutSummary["producerFeatureId"]=target->featureId;
                        cutSummary["targetBodyIds"]=targets;cutSummary["sourceSketchId"]=sketchId;cutSummary["distanceMm"]=distance;cutSummary["startOffsetMm"]=start;
                        if(aggregate)cutSummary["sourceProfileSelection"]=parameters["profileSelection"];
                        else if(plural)cutSummary["sourceProfileIds"]=profileIds;else cutSummary["sourceProfileId"]=parameters["profileId"];
                        sourceEntities.removeDuplicates();sourceEntities.sort();QJsonArray refs;for(const auto& entity:sourceEntities)refs.append(entity);
                        cutSummary["sourceEntityIds"]=refs;cutSummary["sourceAreaMm2"]=area;cutSummary[joining ? "addedVolumeMm3" : "removedVolumeMm3"]=removed;
                        if(parameters["operation"]=="intersect")cutSummary["overlapVolumeMm3"]=cutSummary.value("volumeMm3");
                        target->shape=cutShape;
                        if(joining && joinCaps.size()==1) {
                            auto cap=joinCaps.front();
                            // Retain only the independently named positive cap when the
                            // complete prism cap survives as a face of the joined solid.
                            for(TopExp_Explorer faces(cutShape,TopAbs_FACE);faces.More();faces.Next())if(faces.Current().IsSame(cap.face)) {
                                cap.producerFeatureId=id;cap.bodyId=target->bodyId;
                                cap.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-face/extrude-end-cap/1")).toString(QUuid::WithoutBraces);
                                target->supportFaces.push_back(cap);
                                cutSummary["supportFaces"]=QJsonArray{QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",id},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",supportEdgeDescriptors(cap)}}};break;
                            }
                        }
                        cutSummary["supportFaces"]=PlanarFaces::descriptors(target->supportFaces);
                        cutSummary["solidEdges"]=SolidFillet::descriptors(target->solidEdges);
                        target->summary=cutSummary;
                        evaluated.featureOrder.push_back(id);continue;
                    }
                    const auto united=unionProfiles(selected);const double distance=extrusionDistance(parameters,selected.front(),evaluated),start=extrusionStart(parameters);
                    EvaluatedSupportFace cap;
                    const double taper=parameters.value("taperAngleDeg").toDouble();
                    // Draft history qualifies only the far cap; do not reuse prism side/edge lineage.
                    shape=taper!=0 ? taperedExtrusion(united,distance,taper,&cap)
                        : extrusion(united,distance,&cap,parameters.contains("sourceEntityId") ? nullptr : &solidEdges,id,parameters.contains("sourceEntityId") ? nullptr : &supportFaces,start);summary=summarize(shape);
                    summary["taperAngleDeg"]=taper;
                    summary["solidEdges"]=SolidFillet::descriptors(solidEdges);
                    if(!cap.face.IsNull()) {
                        cap.producerFeatureId=id;cap.bodyId=feature["bodyId"].toString();
                        cap.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-face/extrude-end-cap/1")).toString(QUuid::WithoutBraces);
                        supportFaces.insert(supportFaces.begin(),cap);
                        summary["supportFaces"]=QJsonArray{QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",id},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",supportEdgeDescriptors(cap)}}};
                    }
                    for(auto& face:supportFaces)face.bodyId=feature["bodyId"].toString();
                    summary["supportFaces"]=PlanarFaces::descriptors(supportFaces);
                    summary["operation"]="extrude";summary["distanceMm"]=distance;summary["startOffsetMm"]=start;summary["sourceSketchId"]=sketchId;
                    if(aggregate)summary["sourceProfileSelection"]=parameters["profileSelection"];
                    else if(plural)summary["sourceProfileIds"]=profileIds;else summary["sourceProfileId"]=parameters["profileId"];
                    if(parameters.contains("sourceEntityId")){summary["sourceEntityId"]=parameters.value("sourceEntityId");summary["wallThicknessMm"]=parameters.value("wallThicknessMm");summary["wallLocation"]=parameters.value("wallLocation");summary.remove("sourceProfileId");}
                    summary["sourceEntityIds"]=united.summary["sourceEntityIds"];summary["sourceAreaMm2"]=united.summary["areaMm2"];
                    if(taper==0)summary["analyticVolumeMm3"]=united.summary["areaMm2"].toDouble()*std::abs(distance);
                } else {
                    shape=primitive(type,parameters);summary=summarize(shape);
                    if(type=="box") {
                        PrimitiveTopology::box(shape,parameters,id,feature["bodyId"].toString(),supportFaces,solidEdges);
                        summary["supportFaces"]=PlanarFaces::descriptors(supportFaces);
                        summary["solidEdges"]=SolidFillet::descriptors(solidEdges);
                    } else if(type=="cylinder") {
                        PrimitiveTopology::cylinder(shape,parameters,id,feature["bodyId"].toString(),supportFaces,solidEdges);
                        summary["supportFaces"]=PlanarFaces::descriptors(supportFaces);
                        summary["solidEdges"]=SolidFillet::descriptors(solidEdges);
                    }
                }
                evaluated.bodies.push_back({id,feature["bodyId"].toString(),feature["name"].toString(),feature["visible"].toBool(),shape,summary,supportFaces,solidEdges});
            }
            evaluated.featureOrder.push_back(id);
        } catch (const EvaluationError&) {throw;}
          catch (const Standard_Failure& error) {
            context["code"] = "kernel_failure"; context["message"] = "OCCT feature evaluation failed: "+QString::fromUtf8(error.GetMessageString());
            throw EvaluationError(context);
        } catch (const std::exception& error) {
            context["code"] = "evaluation_failed"; context["message"] = QString::fromUtf8(error.what());
            throw EvaluationError(context);
        }
    }
    QMap<QString,QString> owners,featureTypes;
    for(const auto& value:authored["features"].toArray()){const auto f=value.toObject();owners[f["id"].toString()]=f["ownerId"].toString();featureTypes[f["id"].toString()]=f["type"].toString();}
    QMap<QString,double> opacity;
    for(const auto& value:authored["components"].toArray()){
        const auto component=value.toObject();opacity[component["id"].toString()]=component.value("opacity").toDouble(1.0);
    }
    const auto organization=authored.value("objectOwners").toObject();
    const auto bodyOwners=organization.value("bodies").toObject(),sketchOwners=organization.value("sketches").toObject();
    const auto bodyNames=authored.value("bodyNames").toObject();
    QSet<QString> currentSketches,localBodies,intentionallyRemoved;
    for(const auto& evaluatedId:evaluated.featureOrder)for(const auto& row:features){
        const auto feature=row.toObject();if(feature["id"]==evaluatedId && feature["type"]=="removeBody")
            intentionallyRemoved.insert(feature["parameters"].toObject()["bodyId"].toString());
        if(feature["id"]==evaluatedId && feature["type"]=="boundaryFill" && feature["parameters"].toObject()["removeTools"].toBool())
            for(const auto& ref:feature["parameters"].toObject()["tools"].toArray())intentionallyRemoved.insert(ref.toObject()["bodyId"].toString());
        // Consumed tools still exist in authored history. Keep their metadata
        // dormant so Undo and Keep Tools edits restore the same body identity.
        if(feature["id"]==evaluatedId && feature["type"]=="boolean" && !feature["parameters"].toObject()["keepTools"].toBool())
            for(const auto& id:feature["parameters"].toObject()["toolBodyIds"].toArray())intentionallyRemoved.insert(id.toString());
    }
    // Retain authored name/organization for dormant source bodies, as for
    // consumed tools. Restoration reuses the original body and feature IDs.
    for(const auto& row:features) {const auto f=row.toObject();
        if(evaluated.featureStates.value(f.value("id").toString()).toObject().value("effectiveSuppressed").toBool()
            && !f.value("bodyId").toString().isEmpty())intentionallyRemoved.insert(f.value("bodyId").toString());
    }
    for(auto& body:evaluated.bodies){
        if(body.occurrenceId.isEmpty() && featureTypes.value(body.featureId)!="stepImport"
            && featureTypes.value(body.featureId)!="stepAssemblyImport")localBodies.insert(body.bodyId);
        body.ownerId=bodyOwners.value(body.bodyId).toString(owners[body.featureId]);body.summary["ownerId"]=body.ownerId;
        if(bodyNames.contains(body.bodyId))body.name=bodyNames[body.bodyId].toString();
        body.summary["opacity"]=opacity.value(body.ownerId,1.0);}
    for(auto& sketch:evaluated.sketches){
        currentSketches.insert(sketch.featureId);
        sketch.ownerId=sketchOwners.value(sketch.featureId).toString(owners[sketch.featureId]);sketch.summary["ownerId"]=sketch.ownerId;}
    for(auto it=bodyOwners.begin();it!=bodyOwners.end();++it)if(!localBodies.contains(it.key()) && !intentionallyRemoved.contains(it.key()))
        reject("reparent_body_unavailable","Reparented local body is no longer active; repair or undo the upstream edit");
    for(auto it=bodyNames.begin();it!=bodyNames.end();++it)if(!localBodies.contains(it.key()) && !intentionallyRemoved.contains(it.key()))
        reject("renamed_body_unavailable","Renamed local body is no longer active; repair or undo the upstream edit");
    for(auto it=sketchOwners.begin();it!=sketchOwners.end();++it)if(!currentSketches.contains(it.key()))
        reject("reparent_sketch_unavailable","Reparented sketch is no longer available; repair or undo the upstream edit");
    for(auto& plane:evaluated.constructionPlanes)plane.ownerId=owners[plane.featureId];
    return evaluated;
}
QJsonObject GeometryKernel::policy() {
    return {{"kernel","OCCT"},{"version",ALLOY_OCCT_VERSION},{"canonicalLengthUnit","mm"},
        {"step",QJsonObject{{"importSource","embedded"},{"maximumSourceBytes",static_cast<qint64>(StepExchange::maximumSourceBytes)},
            {"importSolids",1},{"exportSolids",1},{"exportLengthUnits",QJsonArray{"mm","in"}},
            {"translator","OCCT STEPControl"}}},
        {"minimumDimensionMm",0.001},{"maximumDimensionMm",100000},
        {"referenceRoleToleranceMm",1e-6},{"analyticRelativeTolerance",1e-9},
        {"analyticAbsoluteTolerance",1e-6},{"automaticHealing",false},
        {"boolean",QJsonObject{{"targetIdentity","preserved"},{"inputs","explicit_earlier_active_body_ids"},
            {"hiddenInputsAllowed",true},{"resultSolids",1},{"emptyOrDisconnectedResult","reject_candidate"},
            {"toolGroup","union"},{"additionalToleranceMm",0.0},{"nonDestructive",true}}}};
}
