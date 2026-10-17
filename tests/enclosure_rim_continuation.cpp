// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "RoundedLoopOffset.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Result.hxx>
#include <BRepCheck_Face.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <BRepCheck_Wire.hxx>
#include <BRepCheck_ListIteratorOfListOfStatus.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepLProp_SLProps.hxx>
#include <Geom2d_Curve.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <Bnd_Box.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GeomTools.hxx>
#include <Geom_Surface.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Torus.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace {
EvaluatedSupportFace cap(const CadDocument& document) {
    for(const auto& body:document.bodies())for(const auto& face:body.supportFaces)
        if(face.role=="split.negative.cap")return face;
    throw std::runtime_error("split lower cap unavailable");
}
QJsonArray point(const gp_Pnt& value) {return {value.X(),value.Y(),value.Z()};}
struct Rail {QString id;gp_Pnt a,b;gp_Vec direction;double length;QJsonObject provenance;};

// Full kernel incidence, independent of the subset with authored edge names.
QJsonObject vertexTopology(const EvaluatedBody& body,const TopoDS_Vertex& vertex,
                           const TopTools_IndexedMapOfShape& edges,
                           const TopTools_IndexedMapOfShape& faces,const QString& prefix) {
    QJsonArray incidentEdges,incidentFaces;
    for(int i=1;i<=edges.Extent();++i) {
        const auto edge=TopoDS::Edge(edges(i));bool incident=false;
        for(TopExp_Explorer ex(edge,TopAbs_VERTEX);ex.More();ex.Next())
            if(ex.Current().IsSame(vertex))incident=true;
        if(!incident)continue;
        QJsonArray names;
        for(const auto& named:body.solidEdges)if(!named.edge.IsNull() && named.edge.IsSame(edge))names.append(named.edgeId);
        incidentEdges.append(QJsonObject{{"shapeEdgeIndex",i},{"namedEdgeIds",names},
            {"degenerate",BRep_Tool::Degenerated(edge)},{"tolerance",BRep_Tool::Tolerance(edge)}});
    }
    for(int i=1;i<=faces.Extent();++i) {
        const auto face=TopoDS::Face(faces(i));bool incident=false;
        for(TopExp_Explorer ex(face,TopAbs_VERTEX);ex.More();ex.Next())
            if(ex.Current().IsSame(vertex))incident=true;
        if(!incident)continue;
        QJsonArray wires;
        for(TopExp_Explorer wire(face,TopAbs_WIRE);wire.More();wire.Next()) {
            QJsonArray boundary;
            for(TopExp_Explorer ex(wire.Current(),TopAbs_EDGE);ex.More();ex.Next()) {
                const auto edge=TopoDS::Edge(ex.Current());TopoDS_Vertex first,last;
                TopExp::Vertices(edge,first,last,true);
                QJsonObject item{{"shapeEdgeIndex",edges.FindIndex(edge)},
                    {"orientation",static_cast<int>(edge.Orientation())}};
                if(!first.IsNull())item["start"]=point(BRep_Tool::Pnt(first));
                if(!last.IsNull())item["end"]=point(BRep_Tool::Pnt(last));
                boundary.append(item);
            }
            wires.append(QJsonObject{{"orientation",static_cast<int>(wire.Current().Orientation())},
                {"edges",boundary}});
        }
        const QString path=prefix+QString("-face-%1.brep").arg(i);
        const bool written=BRepTools::Write(face,path.toUtf8().constData());
        const BRepAdaptor_Surface surface(face);
        incidentFaces.append(QJsonObject{{"shapeFaceIndex",i},{"surfaceType",static_cast<int>(surface.GetType())},
            {"orientation",static_cast<int>(face.Orientation())},{"wires",wires},
            {"trimmedFaceBrep",path},{"written",written}});
    }
    return {{"position",point(BRep_Tool::Pnt(vertex))},{"edges",incidentEdges},
        {"edgeCount",incidentEdges.size()},{"faces",incidentFaces}};
}
int certifySaved(const QString& savedPrefix,const QString& output) {
    QFile file(output);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("certification output unavailable");
    QJsonObject result{{"savedPrefix",savedPrefix},{"operation","sample existing blend seams and original terminal faces"}};
    try {
        BRep_Builder reader;TopoDS_Shape shape,original;
        if(!BRepTools::Read(shape,(savedPrefix+"-common.brep").toUtf8().constData(),reader)
            || !BRepTools::Read(original,(savedPrefix+"-input.brep").toUtf8().constData(),reader))throw std::runtime_error("saved shapes unavailable");
        TopTools_IndexedMapOfShape faces,originalFaces;TopExp::MapShapes(shape,TopAbs_FACE,faces);TopExp::MapShapes(original,TopAbs_FACE,originalFaces);
        TopTools_IndexedDataMapOfShapeListOfShape adjacency;TopExp::MapShapesAndAncestors(shape,TopAbs_EDGE,TopAbs_FACE,adjacency);
        const auto blend=[](const TopoDS_Face& face) {
            const BRepAdaptor_Surface surface(face);
            if(surface.GetType()==GeomAbs_Torus)return std::abs(surface.Torus().MinorRadius()-0.5)<1e-7 && std::abs(surface.Torus().Location().Z()-16.5)<1e-7;
            if(surface.GetType()==GeomAbs_Cylinder)return std::abs(surface.Cylinder().Radius()-0.5)<1e-7 && std::abs(surface.Cylinder().Location().Z()-16.5)<1e-7;
            return false;
        };
        QJsonArray blendFaces,seams;int failed=0,unknown=0,tangent=0,terminal=0;
        for(int i=1;i<=faces.Extent();++i)if(blend(TopoDS::Face(faces(i))))blendFaces.append(i);
        for(int i=1;i<=adjacency.Extent();++i) {
            const auto edge=TopoDS::Edge(adjacency.FindKey(i));std::vector<TopoDS_Face> incident;
            for(TopTools_ListIteratorOfListOfShape it(adjacency(i));it.More();it.Next()) {
                const auto face=TopoDS::Face(it.Value());
                if(std::none_of(incident.begin(),incident.end(),[&](const auto& old){return old.IsSame(face);}))incident.push_back(face);
            }
            const int count=std::count_if(incident.begin(),incident.end(),blend);if(!count)continue;
            QJsonObject record{{"edgeIndex",i},{"incidentFaceCount",static_cast<int>(incident.size())},{"blendFaceCount",count}};QJsonArray ids;
            for(const auto& face:incident)ids.append(faces.FindIndex(face));record["faceIndices"]=ids;
            if(incident.size()!=2 || BRep_Tool::Degenerated(edge)) {record["status"]="unknown topology";++unknown;seams.append(record);continue;}
            const BRepAdaptor_Curve curve(edge);const double first=curve.FirstParameter(),last=curve.LastParameter();
            std::vector<gp_Pnt> points;QJsonArray samples;double worst=0;bool normals=true,base=true;
            for(double fraction:{0.1,0.3,0.5,0.7,0.9}) {
                const double parameter=first+(last-first)*fraction;const auto position=curve.Value(parameter);points.push_back(position);base&=std::abs(position.Z()-16.5)<1e-6;
                gp_Dir normal[2];bool defined=true;
                for(int side=0;side<2;++side) {
                    double a,b;const auto pc=BRep_Tool::CurveOnSurface(edge,incident[side],a,b);
                    if(pc.IsNull()) {defined=false;continue;}
                    const auto uv=pc->Value(parameter);const BRepAdaptor_Surface surface(incident[side]);
                    BRepLProp_SLProps props(surface,uv.X(),uv.Y(),1,1e-9);
                    if(!props.IsNormalDefined())defined=false;else normal[side]=props.Normal();
                }
                QJsonObject sample{{"fraction",fraction},{"position",point(position)},{"normalsDefined",defined}};
                if(defined) {const double angle=std::acos(std::clamp(std::abs(normal[0].Dot(normal[1])),0.0,1.0));sample["tangentPlaneAngleRadians"]=angle;worst=std::max(worst,angle);}
                normals&=defined;samples.append(sample);
            }
            record["samples"]=samples;record["maxTangentPlaneAngleRadians"]=worst;
            if(count==2 || base) {
                record["expected"]="tangent blend join";++tangent;
                const bool pass=normals && worst<1e-5;record["sampledTangentPass"]=pass;if(!pass)++failed;
            } else {
                record["expected"]="sharp terminal trim (requires original-face containment)";++terminal;QJsonArray containing;
                for(int f=1;f<=originalFaces.Extent();++f) {
                    const auto face=TopoDS::Face(originalFaces(f));bool all=true;
                    for(const auto& p:points) {
                        BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(p).Vertex(),face);
                        if(!distance.IsDone() || distance.Value()>1e-6) {all=false;break;}
                    }
                    if(all)containing.append(f);
                }
                record["originalTrimmedFacesWithin1eMinus6Mm"]=containing;
                record["sampledTerminalContainmentPass"]=!containing.isEmpty();
                if(containing.isEmpty())++unknown;
            }
            seams.append(record);
        }
        result["blendFaceIndices"]=blendFaces;result["seams"]=seams;result["expectedTangentSeams"]=tangent;result["terminalTrimSeams"]=terminal;result["failedTangentSeams"]=failed;result["unknownSeams"]=unknown;
        result["certification"]="bounded seam samples; not an exhaustive surface proof";
    } catch(const Standard_Failure& error) {result["kernelException"]=QString::fromUtf8(error.GetMessageString());}
      catch(const std::exception& error) {result["error"]=QString::fromUtf8(error.what());}
    file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return result.contains("error") || result.contains("kernelException") ? 2 : 0;
}

int inspectSaved(const QString& savedPrefix,const QString& output) {
    QFile file(output);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("inspection output unavailable");
    QJsonObject result{{"savedPrefix",savedPrefix},{"operation","inspect preserved BReps; no fillet or Common reconstruction"}};
    const QString prefix=output;
    const auto finish=[&](int status) {file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return status;};
    try {
        BRep_Builder reader;TopoDS_Shape original,shape,branches[2];
        const auto read=[&](TopoDS_Shape& target,const QString& suffix) {
            if(!BRepTools::Read(target,(savedPrefix+suffix).toUtf8().constData(),reader))throw std::runtime_error("saved BRep read failed");
        };
        read(original,"-input.brep");read(shape,"-common.brep");
        read(branches[0],"-branch-1.brep");read(branches[1],"-branch-2.brep");
        result["savedCommonSummary"]=GeometryKernel::summarize(shape);
                        QJsonArray subsets;
                        for(const auto& candidate:std::vector<TopoDS_Shape>{branches[0],branches[1],shape}) {
                            BRepAlgoAPI_Cut outside(candidate,original);QJsonObject check{{"cutIsDone",outside.IsDone()}};
                            if(outside.IsDone()) {
                                const auto remainderShape=outside.Shape();
                                check["noOutsideSolids"]=!TopExp_Explorer(remainderShape,TopAbs_SOLID).More();
                                Bnd_Box remainderBounds;BRepBndLib::Add(remainderShape,remainderBounds);
                                check["outsideBoundsVoid"]=remainderBounds.IsVoid();
                                if(!remainderBounds.IsVoid())check["outsideSummary"]=GeometryKernel::summarize(remainderShape);
                            }
                            subsets.append(check);
                            result["subsetChecksBranch1Branch2Common"]=subsets;
                        }
                        result["subsetChecksBranch1Branch2Common"]=subsets;
                        BRepAlgoAPI_Section section(shape,gp_Pln(gp_Pnt(0,0,0),gp_Dir(1,0,0)));
                        result["rearSectionIsDone"]=section.IsDone();QJsonArray arcs;
                        if(section.IsDone()) {
                            result["sectionBrepWritten"]=BRepTools::Write(section.Shape(),(prefix+"-section-x0.brep").toUtf8().constData());
                            for(TopExp_Explorer ex(section.Shape(),TopAbs_EDGE);ex.More();ex.Next()) {
                                const BRepAdaptor_Curve curve(TopoDS::Edge(ex.Current()));const double a=curve.FirstParameter(),b=curve.LastParameter();const auto mid=curve.Value((a+b)/2);
                                if(mid.Y()>-43 || mid.Z()<16.49)continue;
                                QJsonObject edge{{"curveType",static_cast<int>(curve.GetType())},{"start",point(curve.Value(a))},{"end",point(curve.Value(b))},{"mid",point(mid)}};
                                if(curve.GetType()==GeomAbs_Circle) {const auto circle=curve.Circle();edge["radiusMm"]=circle.Radius();edge["center"]=point(circle.Location());edge["parameterSpanRadians"]=b-a;}
                                arcs.append(edge);
                            }
                        }
                        result["rearCrownSectionEdges"]=arcs;QJsonArray mouthFaces;int faceIndex=0;
                        for(TopExp_Explorer ex(shape,TopAbs_FACE);ex.More();ex.Next()) {
                            ++faceIndex;Bnd_Box bounds;BRepBndLib::Add(ex.Current(),bounds);if(bounds.IsVoid())continue;
                            double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
                            if(x1<28 || x0>31 || y1<41 || y0>45 || z1<15)continue;
                            const QString path=prefix+QString("-right-mouth-face-%1.brep").arg(faceIndex);
                            mouthFaces.append(QJsonObject{{"faceIndex",faceIndex},{"path",path},{"written",BRepTools::Write(ex.Current(),path.toUtf8().constData())}});
                        }
                        result["rightMouthFaceArtifacts"]=mouthFaces;
                        result["g1AndMouthCertification"]="unknown; independent inspection required";
                        return finish(0);
    } catch(const Standard_Failure& error) {result["kernelException"]=QString::fromUtf8(error.GetMessageString());}
      catch(const std::exception& error) {result["error"]=QString::fromUtf8(error.what());}
    return finish(2);
}


int repairShoehornOrientation(const QString& path,const QString& output) {
    BRep_Builder reader;TopoDS_Shape shape;
    if(!BRepTools::Read(shape,path.toUtf8().constData(),reader))throw std::runtime_error("saved result unavailable");
    Handle(ShapeBuild_ReShape) replacements=new ShapeBuild_ReShape;
    QJsonArray repairs;TopTools_IndexedMapOfShape oldEdges;TopExp::MapShapes(shape,TopAbs_EDGE,oldEdges);
    for(TopExp_Explorer ex(shape,TopAbs_FACE);ex.More();ex.Next()) {
        const auto face=TopoDS::Face(ex.Current());BRepCheck_Face check(face);
        if(check.OrientationOfWires()!=BRepCheck_BadOrientationOfSubshape)continue;
        ShapeFix_Face fixer(face);const bool changed=fixer.FixOrientation();const auto fixed=fixer.Face();
        replacements->Replace(face,fixed);
        TopLoc_Location oldLoc,newLoc;const auto oldSurface=BRep_Tool::Surface(face,oldLoc),newSurface=BRep_Tool::Surface(fixed,newLoc);
        repairs.append(QJsonObject{{"changed",changed},{"sameSupportSurface",oldSurface==newSurface && oldLoc==newLoc},
            {"oldOrientation",static_cast<int>(face.Orientation())},{"newOrientation",static_cast<int>(fixed.Orientation())}});
    }
    const auto result=replacements->Apply(shape);const auto summary=GeometryKernel::summarize(result);
    const auto resultPath=output+"-orientation-only.brep";if(!BRepTools::Write(result,resultPath.toUtf8().constData()))throw std::runtime_error("repair save failed");
    TopTools_IndexedMapOfShape newEdges;TopExp::MapShapes(result,TopAbs_EDGE,newEdges);int retained=0;
    for(int i=1;i<=oldEdges.Extent();++i)if(newEdges.Contains(oldEdges(i)))++retained;
    BRepCheck_Analyzer analyzer(result,true,false,true);QJsonArray errors;
    TopTools_IndexedMapOfShape all;TopExp::MapShapes(result,all);
    for(int i=1;i<=all.Extent();++i) {const auto check=analyzer.Result(all(i));if(check.IsNull())continue;
        for(BRepCheck_ListIteratorOfListOfStatus it(check->Status());it.More();it.Next())if(it.Value()!=BRepCheck_NoError)
            errors.append(QJsonObject{{"shapeIndex",i},{"shapeType",static_cast<int>(all(i).ShapeType())},{"status",static_cast<int>(it.Value())}});
    }
    QJsonObject report{{"sourceBrep",path},{"operation","ShapeFix_Face FixOrientation only; replacements applied to saved shape"},
        {"repairs",repairs},{"oldEdgeCount",oldEdges.Extent()},{"newEdgeCount",newEdges.Extent()},{"retainedSameEdges",retained},
        {"exactAnalyzerValid",analyzer.IsValid()},{"summary",summary},{"ownStatusErrors",errors},{"resultBrep",resultPath},
        {"materialAndAuthoredHistoryCertified",false}};
    QFile file(output);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("repair report unavailable");
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));return analyzer.IsValid()?0:2;
}

// Readback only: independently invoke mature face/wire checks without changing topology.
int inspectShoehorn(const QString& path,const QString& output) {
    BRep_Builder reader;TopoDS_Shape shape;
    if(!BRepTools::Read(shape,path.toUtf8().constData(),reader))throw std::runtime_error("saved result unavailable");
    BRepCheck_Analyzer analyzer(shape,true,false,true);TopTools_IndexedMapOfShape subshapes;TopExp::MapShapes(shape,subshapes);
    QJsonArray records;
    for(TopExp_Explorer ex(shape,TopAbs_FACE);ex.More();ex.Next()) {
        const auto face=TopoDS::Face(ex.Current());BRepCheck_Face check(face);QJsonArray wires;
        for(TopExp_Explorer wx(face,TopAbs_WIRE);wx.More();wx.Next()) {
            const auto wire=TopoDS::Wire(wx.Current());BRepCheck_Wire wc(wire);TopoDS_Edge e1,e2;QJsonArray edges;
            const auto self=wc.SelfIntersect(face,e1,e2);
            for(TopExp_Explorer ee(wire,TopAbs_EDGE);ee.More();ee.Next()) {
                const auto edge=TopoDS::Edge(ee.Current());double first,last;const auto curve=BRep_Tool::CurveOnSurface(edge,face,first,last);
                QJsonObject item{{"shapeIndex",subshapes.FindIndex(edge)},{"orientation",static_cast<int>(edge.Orientation())},
                    {"sameParameter",BRep_Tool::SameParameter(edge)},{"sameRange",BRep_Tool::SameRange(edge)},
                    {"tolerance",BRep_Tool::Tolerance(edge)},{"hasPcurve",!curve.IsNull()}};
                if(!curve.IsNull()) {const auto a=curve->Value(first),b=curve->Value(last);item["uvStart"]=QJsonArray{a.X(),a.Y()};item["uvEnd"]=QJsonArray{b.X(),b.Y()};}
                edges.append(item);
            }
            wires.append(QJsonObject{{"shapeIndex",subshapes.FindIndex(wire)},{"orientation",static_cast<int>(wire.Orientation())},
                {"closedStatus",static_cast<int>(wc.Closed())},{"closed2dStatus",static_cast<int>(wc.Closed2d(face))},
                {"orientationStatus",static_cast<int>(wc.Orientation(face))},{"selfIntersectionStatus",static_cast<int>(self)},
                {"intersectingEdge1",e1.IsNull()?0:subshapes.FindIndex(e1)},{"intersectingEdge2",e2.IsNull()?0:subshapes.FindIndex(e2)},
                {"edges",edges}});
        }
        records.append(QJsonObject{{"shapeIndex",subshapes.FindIndex(face)},{"orientation",static_cast<int>(face.Orientation())},
            {"surfaceType",static_cast<int>(BRepAdaptor_Surface(face).GetType())},
            {"exactAnalyzerValid",analyzer.IsValid(face)},{"intersectWiresStatus",static_cast<int>(check.IntersectWires())},
            {"classifyWiresStatus",static_cast<int>(check.ClassifyWires())},{"orientationOfWiresStatus",static_cast<int>(check.OrientationOfWires())},
            {"wires",wires}});
    }
    QFile file(output);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("inspection output unavailable");
    file.write(QJsonDocument(QJsonObject{{"sourceBrep",path},{"operation","saved result face and wire checks; no geometry reconstruction or repair"},
        {"exactAnalyzerValid",analyzer.IsValid()},{"faces",records}}).toJson(QJsonDocument::Indented));
    return 0;
}

// One fixed saved-fixture diagnostic; no production fallback or parameter sweep.
int shoehornDiagnostic(const CadDocument& document,const QString& output) {
    const QStringList ids{"3c195c62-9e43-530c-b50e-7279ceb40127","540a3de0-6956-561e-9937-131eb386bff0",
        "287083bc-809d-56dd-88b1-ea6ffa6d8cbb","9ca2891e-1a8b-513d-9b6c-60ecd16c6b19"};
    QJsonObject record{{"operation","exact saved shoehorn grouped R12/R16 diagnostic"},
        {"authored",document.authored()},{"terminalContainment","unknown; incidence alone is not finite containment proof"}};
    QFile file(output);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("diagnostic output unavailable");
    auto finish=[&](int status){file.write(QJsonDocument(record).toJson(QJsonDocument::Indented));return status;};
    try {
        const EvaluatedBody* input=nullptr;
        for(const auto& body:document.bodies())for(const auto& edge:body.solidEdges)
            if(edge.edgeId==ids.front())input=&body;
        if(!input)throw std::runtime_error("exact retained shoehorn edge unavailable");
        const auto& body=*input;const auto summary=GeometryKernel::summarize(body.shape);
        record["inputSummary"]=summary;record["inputOrientation"]=static_cast<int>(body.shape.Orientation());
        if(!summary.value("valid").toBool() || !summary.value("closed").toBool()
           || summary.value("topology").toObject().value("solids").toInt()!=1
           || summary.value("volumeMm3").toDouble()<=0)throw std::runtime_error("input is not valid positive closed solid");
        const auto inputPath=output+"-input.brep";
        if(!BRepTools::Write(body.shape,inputPath.toUtf8().constData()))throw std::runtime_error("input save failed");
        record["inputBrep"]=inputPath;
        TopTools_IndexedMapOfShape edges,faces;TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);TopExp::MapShapes(body.shape,TopAbs_FACE,faces);
        BRepFilletAPI_MakeFillet builder(body.shape);QJsonArray seeds;QMap<int,double> radii;
        for(int i=0;i<ids.size();++i) {
            const auto found=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& edge){return edge.edgeId==ids[i];});
            if(found==body.solidEdges.end() || found->edge.IsNull())throw std::runtime_error("retained source corner unavailable");
            if(!found->provenance.value("role").toString().startsWith("corner/"))throw std::runtime_error("expected retained Thicken corner provenance");
            const double radius=i<2?12:16;builder.Add(radius,found->edge);const int contour=builder.Contour(found->edge);
            if(contour==0 || (radii.contains(contour) && radii[contour]!=radius))throw std::runtime_error("invalid grouped contour ownership");
            radii[contour]=radius;seeds.append(QJsonObject{{"edgeId",ids[i]},{"radiusMm",radius},{"contour",contour},{"provenance",found->provenance}});
        }
        record["seeds"]=seeds;QJsonArray contours;
        for(int i=1;i<=builder.NbContours();++i) {
            if(!radii.contains(i))throw std::runtime_error("unowned contour");
            builder.SetRadius(radii[i],i,1);QJsonArray chain;
            for(int n=1;n<=builder.NbEdges(i);++n)chain.append(edges.FindIndex(builder.Edge(i,n)));
            QJsonObject item{{"index",i},{"radiusMm",radii[i]},{"shapeEdgeIndices",chain}};
            const auto first=builder.FirstVertex(i),last=builder.LastVertex(i);
            if(!first.IsNull())item["firstVertex"]=vertexTopology(body,first,edges,faces,output);
            if(!last.IsNull())item["lastVertex"]=vertexTopology(body,last,edges,faces,output);
            contours.append(item);
        }
        record["contours"]=contours;
        try {builder.Build();record["buildThrew"]=false;}
        catch(const Standard_Failure& error){record["buildThrew"]=true;record["buildException"]=QString::fromUtf8(error.GetMessageString());}
        record["isDone"]=builder.IsDone();record["hasPartialResult"]=builder.HasResult();
        QJsonArray statuses,faultyContours,faultyVertices;
        for(int i=1;i<=builder.NbContours();++i)statuses.append(QJsonObject{{"contour",i},{"stripeStatus",static_cast<int>(builder.StripeStatus(i))}});
        for(int i=1;i<=builder.NbFaultyContours();++i)faultyContours.append(builder.FaultyContour(i));
        for(int i=1;i<=builder.NbFaultyVertices();++i)faultyVertices.append(vertexTopology(body,builder.FaultyVertex(i),edges,faces,output));
        record["stripeStatuses"]=statuses;record["faultyContours"]=faultyContours;record["faultyVertices"]=faultyVertices;
        TopoDS_Shape result;
        if(builder.IsDone())result=builder.Shape();else if(builder.HasResult())result=builder.BadShape();
        if(result.IsNull()){record["resultAvailable"]=false;return finish(2);}
        record["resultAvailable"]=true;const auto resultPath=output+"-result.brep";
        if(!BRepTools::Write(result,resultPath.toUtf8().constData()))throw std::runtime_error("result save failed");
        record["resultBrep"]=resultPath;const auto resultSummary=GeometryKernel::summarize(result);record["resultSummary"]=resultSummary;
        BRepCheck_Analyzer analyzer(result);TopTools_IndexedMapOfShape subshapes;TopExp::MapShapes(result,subshapes);
        QJsonArray checks;
        auto codes=[](const BRepCheck_ListOfStatus& list){QJsonArray values;for(BRepCheck_ListIteratorOfListOfStatus it(list);it.More();it.Next())values.append(static_cast<int>(it.Value()));return values;};
        for(int i=1;i<=subshapes.Extent();++i) {
            const auto& shape=subshapes(i);const auto check=analyzer.Result(shape);if(check.IsNull())continue;
            QJsonArray contexts;bool failed=false;
            auto own=codes(check->Status());for(const auto& v:own)if(v.toInt()!=0)failed=true;
            for(check->InitContextIterator();check->MoreShapeInContext();check->NextShapeInContext()) {
                const auto status=codes(check->StatusOnShape());for(const auto& v:status)if(v.toInt()!=0)failed=true;
                contexts.append(QJsonObject{{"shapeIndex",subshapes.FindIndex(check->ContextualShape())},{"statuses",status}});
            }
            if(!failed)continue;
            const auto path=output+QString("-invalid-subshape-%1.brep").arg(i);
            const bool written=BRepTools::Write(shape,path.toUtf8().constData());
            checks.append(QJsonObject{{"shapeIndex",i},{"shapeType",static_cast<int>(shape.ShapeType())},
                {"orientation",static_cast<int>(shape.Orientation())},{"statuses",own},{"contexts",contexts},{"brep",path},{"written",written}});
        }
        record["invalidSubshapes"]=checks;
        const bool accepted=builder.IsDone() && analyzer.IsValid() && resultSummary.value("closed").toBool()
            && resultSummary.value("topology").toObject().value("solids").toInt()==1 && resultSummary.value("volumeMm3").toDouble()>0;
        record["acceptedGeometry"]=accepted;return finish(accepted?0:2);
    } catch(const Standard_Failure& error){record["kernelException"]=QString::fromUtf8(error.GetMessageString());}
      catch(const std::exception& error){record["error"]=QString::fromUtf8(error.what());}
    return finish(2);
}

}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        if(argc!=3 && argc!=4)throw std::runtime_error("usage: enclosure_rim_continuation_test source.alloy output.json [fillet|contour|common]");
        if(argc==4 && QString::fromUtf8(argv[3])=="certify")return certifySaved(QString::fromUtf8(argv[1]),QString::fromUtf8(argv[2]));
        if(argc==4 && QString::fromUtf8(argv[3])=="inspect")return inspectSaved(QString::fromUtf8(argv[1]),QString::fromUtf8(argv[2]));
        if(argc==4 && QString::fromUtf8(argv[3])=="shoehorn-repair")return repairShoehornOrientation(QString::fromUtf8(argv[1]),QString::fromUtf8(argv[2]));
        if(argc==4 && QString::fromUtf8(argv[3])=="shoehorn-inspect")return inspectShoehorn(QString::fromUtf8(argv[1]),QString::fromUtf8(argv[2]));
        CadDocument document;
        const auto openError=document.open(argv[1]);
        if(!openError.isEmpty())throw std::runtime_error(("source did not reopen: "+openError).toStdString());
        if(argc==4 && QString::fromUtf8(argv[3])=="shoehorn")return shoehornDiagnostic(document,QString::fromUtf8(argv[2]));
        if(argc==4 && QString::fromUtf8(argv[3])=="clearance") {
            QJsonObject split;
            for(const auto& value:document.authored().value("features").toArray())
                if(value.toObject().value("type")=="splitBody")split=value.toObject();
            const QString top=split.value("bodyId").toString();
            const QString bottom=split.value("parameters").toObject().value("targetBodyId").toString();
            std::vector<const EvaluatedBody*> halves,imports;
            for(const auto& body:document.bodies()) {
                if(body.bodyId==top || body.bodyId==bottom)halves.push_back(&body);
                else if(!body.definitionId.isEmpty())imports.push_back(&body);
            }
            if(halves.size()!=2 || imports.size()!=12)throw std::runtime_error("expected two halves and twelve reference solids");
            const QString prefix=QString::fromUtf8(argv[2]);
            QJsonArray pairs,inputs;int positive=0,index=0;
            for(const auto* half:halves) {
                const auto summary=GeometryKernel::summarize(half->shape);
                if(!summary.value("valid").toBool() || !summary.value("closed").toBool()
                    || summary.value("topology").toObject().value("solids").toInt()!=1)
                    throw std::runtime_error("invalid saved half");
                const auto halfPath=prefix+"-"+(half->bodyId==top ? "top" : "bottom")+".brep";
                if(!BRepTools::Write(half->shape,halfPath.toUtf8().constData()))throw std::runtime_error("half BRep write failed");
                inputs.append(QJsonObject{{"bodyId",half->bodyId},{"name",half->name},{"summary",summary},{"brep",halfPath}});
                for(const auto* reference:imports) {
                    BRepAlgoAPI_Common common;TopTools_ListOfShape objects,tools;
                    objects.Append(half->shape);tools.Append(reference->shape);
                    common.SetArguments(objects);common.SetTools(tools);
                    common.SetNonDestructive(true);common.SetRunParallel(false);common.SetFuzzyValue(0.0);common.Build();
                    if(!common.IsDone() || common.HasErrors())throw std::runtime_error("reference intersection failed");
                    const auto intersection=common.Shape();
                    const bool solids=!intersection.IsNull() && TopExp_Explorer(intersection,TopAbs_SOLID).More();
                    GProp_GProps mass;if(solids)BRepGProp::VolumeProperties(intersection,mass);
                    BRepExtrema_DistShapeShape separation(half->shape,reference->shape);
                    separation.Perform();
                    if(!separation.IsDone())throw std::runtime_error("reference distance measurement failed");
                    QJsonObject row{{"halfBodyId",half->bodyId},{"half",half->bodyId==top ? "Top" : "Bottom"},
                        {"referenceBodyId",reference->bodyId},{"referenceName",reference->name},
                        {"hasSolids",solids},{"intersectionVolumeMm3",mass.Mass()},
                        {"minimumDistanceMm",separation.Value()}};
                    if(solids && std::abs(mass.Mass())>1e-7) {
                        ++positive;const auto path=prefix+QString("-intersection-%1.brep").arg(index);
                        if(!BRepTools::Write(intersection,path.toUtf8().constData()))throw std::runtime_error("intersection BRep write failed");
                        row["intersectionBrep"]=path;row["summary"]=GeometryKernel::summarize(intersection);
                        const auto refPath=prefix+QString("-reference-%1.brep").arg(index);
                        if(!BRepTools::Write(reference->shape,refPath.toUtf8().constData()))throw std::runtime_error("reference BRep write failed");
                        row["referenceBrep"]=refPath;
                    }
                    pairs.append(row);++index;
                }
            }
            QJsonObject result{{"source",argv[1]},{"authored",document.authored()},
                {"halves",inputs},{"pairs",pairs},{"positiveIntersectionCount",positive},
                {"physicalClearance","not established; zero volume does not prove positive distance"}};
            QFile output(argv[2]);if(!output.open(QIODevice::WriteOnly))throw std::runtime_error("clearance output unavailable");
            output.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return positive ? 2 : 0;
        }
        if(argc==4 && QString::fromUtf8(argv[3])=="cut") {
            QJsonObject split;
            for(const auto& value:document.authored().value("features").toArray())if(value.toObject().value("type")=="splitBody")split=value.toObject();
            TopoDS_Shape target,tool;
            for(const auto& body:document.bodies()) {
                if(body.bodyId==split.value("bodyId").toString())target=body.shape;
                if(body.bodyId==split.value("parameters").toObject().value("targetBodyId").toString())tool=body.shape;
            }
            if(target.IsNull() || tool.IsNull())throw std::runtime_error("cut operands missing");
            BRepAlgoAPI_BooleanOperation operation;TopTools_ListOfShape objects,tools;objects.Append(target);tools.Append(tool);
            operation.SetArguments(objects);operation.SetTools(tools);operation.SetOperation(BOPAlgo_CUT);
            operation.SetNonDestructive(true);operation.SetRunParallel(false);operation.SetFuzzyValue(0.0);operation.Build();
            const QString prefix=QString::fromUtf8(argv[2]);QJsonObject result{{"source",argv[1]},
                {"isDone",operation.IsDone()},{"hasErrors",operation.HasErrors()}};
            if(operation.IsDone() && !operation.HasErrors()) {
                result["raw"]=GeometryKernel::summarize(operation.Shape());
                result["rawWritten"]=BRepTools::Write(operation.Shape(),(prefix+"-raw.brep").toUtf8().constData());
                operation.SimplifyResult(true,true,1e-12);
                result["simplified"]=GeometryKernel::summarize(operation.Shape());
                result["simplifiedWritten"]=BRepTools::Write(operation.Shape(),(prefix+"-simplified.brep").toUtf8().constData());
            }
            QFile output(argv[2]);if(!output.open(QIODevice::WriteOnly))throw std::runtime_error("cut diagnostic output unavailable");
            output.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return 0;
        }
        if(argc==4 && QString::fromUtf8(argv[3])=="fit") {
            const auto features=document.authored().value("features").toArray();
            const auto last=features.last().toObject();const auto parameters=last.value("parameters").toObject();
            if(last.value("type")!="boolean" || parameters.value("operation")!="cut" || !parameters.value("keepTools").toBool()
                || parameters.value("toolBodyIds").toArray().size()!=1)throw std::runtime_error("expected final retained-tool Cut");
            TopoDS_Shape target,tool;
            for(const auto& body:document.bodies()) {
                if(body.bodyId==parameters.value("targetBodyId").toString())target=body.shape;
                if(body.bodyId==parameters.value("toolBodyIds").toArray().first().toString())tool=body.shape;
            }
            if(target.IsNull() || tool.IsNull())throw std::runtime_error("retained halves unavailable");
            BRepAlgoAPI_Common intersection(target,tool);if(!intersection.IsDone())throw std::runtime_error("mating intersection failed");
            GProp_GProps mass;BRepGProp::VolumeProperties(intersection.Shape(),mass);
            QJsonObject result{{"source",argv[1]},{"target",GeometryKernel::summarize(target)},
                {"tool",GeometryKernel::summarize(tool)},{"intersectionVolumeMm3",mass.Mass()},
                {"intersectionHasSolids",TopExp_Explorer(intersection.Shape(),TopAbs_SOLID).More()},
                {"physicalClearance","not established"}};
            QFile output(argv[2]);if(!output.open(QIODevice::WriteOnly))throw std::runtime_error("fit output unavailable");
            output.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return std::abs(mass.Mass())<1e-7 ? 0:2;
        }
        const auto sourceFeatureCount=document.authored().value("features").toArray().size();
        const auto support=cap(document);
        QString owner;
        for(const auto& body:document.bodies())if(body.bodyId==support.bodyId)owner=body.ownerId;
        const QJsonObject reference{{"producerFeatureId",support.producerFeatureId},
            {"bodyId",support.bodyId},{"faceId",support.faceId}};
        if(argc==4 && QString::fromUtf8(argv[3])=="boundary") {
            QJsonArray edges;int wireCount=0;
            for(TopExp_Explorer wire(support.face,TopAbs_WIRE);wire.More();wire.Next())++wireCount;
            for(const auto& boundary:support.boundaryEdges) {
                const BRepAdaptor_Curve curve(boundary.edge);
                const auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());
                QJsonObject item{{"edgeId",boundary.edgeId},{"edgeType",boundary.type},
                    {"curveType",static_cast<int>(curve.GetType())},
                    {"start",point(first)},{"end",point(last)},
                    {"lengthMm",GCPnts_AbscissaPoint::Length(curve,curve.FirstParameter(),curve.LastParameter())},
                    {"toleranceMm",BRep_Tool::Tolerance(boundary.edge)},
                    {"provenance",boundary.provenance}};
                if(curve.GetType()==GeomAbs_Circle) {
                    item["circleCenter"]=point(curve.Circle().Location());
                    item["circleRadiusMm"]=curve.Circle().Radius();
                }
                edges.append(item);
            }
            QJsonObject result{{"source",argv[1]},{"sourceFeatureCount",sourceFeatureCount},
                {"supportRole",support.role},{"support",reference},{"supportFrame",support.frame},
                {"wireCount",wireCount},{"boundaryEdgeCount",static_cast<int>(support.boundaryEdges.size())},
                {"insetLoopId",RoundedLoopOffset::insetLoopId(support)},{"boundaryEdges",edges},
                {"note","Diagnostic prefix input only; not a user-saved model or accepted design."}};
            QFile output(argv[2]);if(!output.open(QIODevice::WriteOnly))throw std::runtime_error("boundary output unavailable");
            output.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return 0;
        }
        auto sketch=SketchGeometry::newSupportedFeature(owner,reference,"Rim inset",{});
        auto parameters=sketch.value("parameters").toObject();
        parameters["loopOffsets"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},
            {"kind","planarInsetV1"},{"outerLoopId",RoundedLoopOffset::insetLoopId(support)},
            {"distanceMm",-1.25},{"chain",true}}};
        sketch["parameters"]=parameters;
        if(!document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Rim inset").isEmpty())
            throw std::runtime_error("inset refused");
        const auto profile=document.sketches().back().profiles.front();
        const auto join=CadDocument::extrusionCandidate(document.authored(),
            {{"ownerId",owner},{"sketchId",sketch.value("id")},{"profileId",profile.profileId},
             {"distanceMm",2.0},{"operation","join"},{"targetBodyIds",QJsonArray{support.bodyId}}});
        if(!document.commit(join,"Rim Join2").isEmpty())throw std::runtime_error("Join2 refused");
        std::vector<Rail> rails;QJsonArray rows;
        for(const auto& body:document.bodies())if(body.bodyId==support.bodyId)
            for(const auto& edge:body.solidEdges) {
                if(edge.edge.IsNull() || edge.provenance.value("sourceSketchId")!=sketch.value("id")
                    || edge.provenance.value("role")!="endRim")continue;
                const BRepAdaptor_Curve curve(edge.edge);
                if(curve.GetType()!=GeomAbs_Line)continue;
                const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
                gp_Vec direction(a,b);const double length=direction.Magnitude();
                if(length<1e-8)continue;direction.Normalize();
                rails.push_back({edge.edgeId,a,b,direction,length,edge.provenance});
                rows.append(QJsonObject{{"edgeId",edge.edgeId},{"start",point(a)},{"end",point(b)},
                    {"lengthMm",length},{"provenance",edge.provenance}});
            }
        QJsonArray pairs;
        for(size_t i=0;i<rails.size();++i)for(size_t j=i+1;j<rails.size();++j) {
            const auto& a=rails[i];const auto& b=rails[j];
            const double parallel=std::abs(a.direction.Dot(b.direction));
            if(parallel<0.9999)continue;
            const gp_Vec from(a.a,b.a);const double distance=from.Crossed(a.direction).Magnitude();
            const double b0=from.Dot(a.direction),b1=gp_Vec(a.a,b.b).Dot(a.direction);
            const double overlap=std::max(0.0,std::min(a.length,std::max(b0,b1))-
                std::max(0.0,std::min(b0,b1)));
            if(overlap<1)continue;
            pairs.append(QJsonObject{{"first",a.id},{"second",b.id},{"parallel",parallel},
                {"distanceMm",distance},{"overlapMm",overlap}});
        }
        QFile file(argv[2]);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("output unavailable");
        QJsonObject result{{"source",argv[1]},{"sourceFeatureCount",sourceFeatureCount},{"supportFaceId",support.faceId},
            {"profileId",profile.profileId},{"rails",rows},{"parallelPairs",pairs}};
        if(argc==4) {
            const auto mode=QString::fromUtf8(argv[3]);
            if(mode!="fillet" && mode!="contour" && mode!="common")throw std::runtime_error("unsupported diagnostic mode");
            QStringList selected;
            for(const auto& rail:rails) {
                const bool rear=std::abs(rail.length-54)<1e-7 &&
                    (std::abs(rail.a.Y()+45.25)<1e-7 || std::abs(rail.a.Y()+44.25)<1e-7);
                if(rear)selected.append(rail.id);
            }
            if(selected.size()!=2)throw std::runtime_error("rear opposing 54mm rail pair unavailable");
            result["selectedRearPair"]=QJsonArray{selected[0],selected[1]};
            result["attemptedRadiusMm"]=0.5;
            if(mode=="common") {
                result["experiment"]="Common of two independent original-base single-chain R0.5 fillets";
                result["semanticEquivalence"]="unverified";
                const QString prefix=QString::fromUtf8(argv[2]);
                const auto finish=[&](int status) {file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));return status;};
                const auto solidValid=[](const QJsonObject& s) {
                    return s.value("valid").toBool() && s.value("closed").toBool()
                        && s.value("topology").toObject().value("solids").toInt()==1
                        && s.value("volumeMm3").toDouble()>0;
                };
                try {
                    for(const auto& body:document.bodies())if(body.bodyId==support.bodyId) {
                        result["inputSummary"]=GeometryKernel::summarize(body.shape);
                        result["inputBrepWritten"]=BRepTools::Write(body.shape,(prefix+"-input.brep").toUtf8().constData());
                        TopoDS_Shape branches[2];QJsonArray branchRecords;
                        for(int branch=0;branch<2;++branch) {
                            BRepFilletAPI_MakeFillet builder(body.shape);
                            for(const auto& edge:body.solidEdges)if(edge.edgeId==selected[branch])builder.Add(0.5,edge.edge);
                            QJsonObject record{{"seed",selected[branch]},{"contourCount",builder.NbContours()}};
                            if(builder.NbContours()!=1) {record["error"]="Expected one original tangent chain";branchRecords.append(record);result["branches"]=branchRecords;return finish(2);}
                            record["expandedEdgeCount"]=builder.NbEdges(1);builder.SetRadius(0.5,1,1);
                            builder.Build();record["isDone"]=builder.IsDone();record["stripeStatus"]=static_cast<int>(builder.StripeStatus(1));
                            record["faultyVertexCount"]=builder.NbFaultyVertices();
                            if(!builder.IsDone()) {branchRecords.append(record);result["branches"]=branchRecords;return finish(2);}
                            branches[branch]=builder.Shape();const auto summary=GeometryKernel::summarize(branches[branch]);record["summary"]=summary;
                            record["brepWritten"]=BRepTools::Write(branches[branch],(prefix+QString("-branch-%1.brep").arg(branch+1)).toUtf8().constData());
                            branchRecords.append(record);result["branches"]=branchRecords;
                            if(!solidValid(summary))return finish(2);
                        }
                        BRepAlgoAPI_Common common(branches[0],branches[1]);result["commonIsDone"]=common.IsDone();
                        if(!common.IsDone())return finish(2);
                        const auto shape=common.Shape();const auto summary=GeometryKernel::summarize(shape);result["commonSummary"]=summary;
                        result["commonBrepWritten"]=BRepTools::Write(shape,(prefix+"-common.brep").toUtf8().constData());
                        if(!solidValid(summary))return finish(2);
                        QJsonArray subsets;
                        for(const auto& candidate:std::vector<TopoDS_Shape>{branches[0],branches[1],shape}) {
                            BRepAlgoAPI_Cut outside(candidate,body.shape);QJsonObject check{{"cutIsDone",outside.IsDone()}};
                            if(outside.IsDone()) {
                                const auto remainderShape=outside.Shape();
                                check["noOutsideSolids"]=!TopExp_Explorer(remainderShape,TopAbs_SOLID).More();
                                Bnd_Box remainderBounds;BRepBndLib::Add(remainderShape,remainderBounds);
                                check["outsideBoundsVoid"]=remainderBounds.IsVoid();
                                if(!remainderBounds.IsVoid())check["outsideSummary"]=GeometryKernel::summarize(remainderShape);
                            }
                            subsets.append(check);
                            result["subsetChecksBranch1Branch2Common"]=subsets;
                        }
                        result["subsetChecksBranch1Branch2Common"]=subsets;
                        BRepAlgoAPI_Section section(shape,gp_Pln(gp_Pnt(0,0,0),gp_Dir(1,0,0)));
                        result["rearSectionIsDone"]=section.IsDone();QJsonArray arcs;
                        if(section.IsDone()) {
                            result["sectionBrepWritten"]=BRepTools::Write(section.Shape(),(prefix+"-section-x0.brep").toUtf8().constData());
                            for(TopExp_Explorer ex(section.Shape(),TopAbs_EDGE);ex.More();ex.Next()) {
                                const BRepAdaptor_Curve curve(TopoDS::Edge(ex.Current()));const double a=curve.FirstParameter(),b=curve.LastParameter();const auto mid=curve.Value((a+b)/2);
                                if(mid.Y()>-43 || mid.Z()<16.49)continue;
                                QJsonObject edge{{"curveType",static_cast<int>(curve.GetType())},{"start",point(curve.Value(a))},{"end",point(curve.Value(b))},{"mid",point(mid)}};
                                if(curve.GetType()==GeomAbs_Circle) {const auto circle=curve.Circle();edge["radiusMm"]=circle.Radius();edge["center"]=point(circle.Location());edge["parameterSpanRadians"]=b-a;}
                                arcs.append(edge);
                            }
                        }
                        result["rearCrownSectionEdges"]=arcs;QJsonArray mouthFaces;int faceIndex=0;
                        for(TopExp_Explorer ex(shape,TopAbs_FACE);ex.More();ex.Next()) {
                            ++faceIndex;Bnd_Box bounds;BRepBndLib::Add(ex.Current(),bounds);if(bounds.IsVoid())continue;
                            double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
                            if(x1<28 || x0>31 || y1<41 || y0>45 || z1<15)continue;
                            const QString path=prefix+QString("-right-mouth-face-%1.brep").arg(faceIndex);
                            mouthFaces.append(QJsonObject{{"faceIndex",faceIndex},{"path",path},{"written",BRepTools::Write(ex.Current(),path.toUtf8().constData())}});
                        }
                        result["rightMouthFaceArtifacts"]=mouthFaces;
                        result["g1AndMouthCertification"]="unknown; independent inspection required";
                        return finish(0);
                    }
                    result["error"]="Target body missing";
                } catch(const Standard_Failure& error) {result["kernelException"]=QString::fromUtf8(error.GetMessageString());}
                  catch(const std::exception& error) {result["error"]=QString::fromUtf8(error.what());}
                return finish(2);
            }
            if(mode=="contour") {
                for(const auto& body:document.bodies())if(body.bodyId==support.bodyId) {
                    const QString evidencePrefix=QString::fromUtf8(argv[2]);
                    const QString inputPath=evidencePrefix+"-input.brep";
                    result["inputBrep"]=inputPath;
                    result["inputBrepWritten"]=BRepTools::Write(body.shape,inputPath.toUtf8().constData());
                    TopTools_IndexedMapOfShape shapeEdges,shapeFaces;
                    TopExp::MapShapes(body.shape,TopAbs_EDGE,shapeEdges);
                    TopExp::MapShapes(body.shape,TopAbs_FACE,shapeFaces);
                    BRepFilletAPI_MakeFillet builder(body.shape);
                    QJsonArray seedContours;
                    for(const auto& id:selected)for(const auto& edge:body.solidEdges)if(edge.edgeId==id) {
                        builder.Add(0.5,edge.edge);
                        seedContours.append(QJsonObject{{"edgeId",id},{"contour",builder.Contour(edge.edge)}});
                    }
                    result["seedContours"]=seedContours;
                    QJsonArray contours;int expandedCount=0;
                    for(int index=1;index<=builder.NbContours();++index) {
                        builder.SetRadius(0.5,index,1);
                        QJsonArray chain;
                        for(int edgeIndex=1;edgeIndex<=builder.NbEdges(index);++edgeIndex) {
                            const auto actual=builder.Edge(index,edgeIndex);
                            const BRepAdaptor_Curve curve(actual);
                            const auto start=curve.Value(curve.FirstParameter()),end=curve.Value(curve.LastParameter());
                            QJsonArray identities;
                            for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull() && edge.edge.IsSame(actual))
                                identities.append(QJsonObject{{"edgeId",edge.edgeId},{"provenance",edge.provenance}});
                            chain.append(QJsonObject{{"index",edgeIndex},{"start",point(start)},{"end",point(end)},
                                {"identities",identities}});
                            ++expandedCount;
                        }
                        QJsonObject contour{{"index",index},{"edges",chain}};
                        const auto first=builder.FirstVertex(index),last=builder.LastVertex(index);
                        if(!first.IsNull())contour["firstVertexTopology"]=vertexTopology(body,first,shapeEdges,shapeFaces,evidencePrefix);
                        if(!last.IsNull())contour["lastVertexTopology"]=vertexTopology(body,last,shapeEdges,shapeFaces,evidencePrefix);
                        contours.append(contour);
                    }
                    result["contours"]=contours;result["expandedEdgeCount"]=expandedCount;
                    try {builder.Build();result["buildThrew"]=false;}
                    catch(const Standard_Failure& error) {
                        result["buildThrew"]=true;result["kernelException"]=QString::fromUtf8(error.GetMessageString());
                    }
                    result["isDone"]=builder.IsDone();result["hasPartialResult"]=builder.HasResult();
                    QJsonArray computed;
                    for(int index=1;index<=builder.NbContours();++index) {
                        QJsonObject entry{{"contour",index}};QJsonArray surfaces;
                        try {
                            const int count=builder.NbComputedSurfaces(index);entry["count"]=count;
                            for(int n=1;n<=count;++n) {
                                const auto surface=builder.ComputedSurface(index,n);
                                QJsonObject item{{"index",n},{"null",surface.IsNull()}};
                                if(!surface.IsNull()) {
                                    const QString path=evidencePrefix+QString("-contour-%1-surface-%2.geom").arg(index).arg(n);
                                    std::ofstream stream(path.toStdString());stream.precision(17);
                                    GeomTools::Write(surface,stream);stream.flush();
                                    item["surfaceFile"]=path;item["written"]=stream.good();
                                    double u0,u1,v0,v1;surface->Bounds(u0,u1,v0,v1);
                                    const auto bound=[](double v)->QJsonValue {
                                        return std::isfinite(v) ? QJsonValue(v) : QJsonValue(QString::number(v));
                                    };
                                    item["parameterBounds"]=QJsonArray{bound(u0),bound(u1),bound(v0),bound(v1)};
                                }
                                surfaces.append(item);
                            }
                        } catch(const Standard_Failure& error) {entry["error"]=QString::fromUtf8(error.GetMessageString());}
                        entry["surfaces"]=surfaces;computed.append(entry);
                    }
                    result["computedStripeSurfaces"]=computed;
                    result["terminalContainment"]=QJsonObject{{"status","unknown"},
                        {"reason","Computed support surfaces do not identify the finite terminal intersection interval or the kernel-selected limiting face. Trimmed incident faces and contour endpoints are preserved for inspection; unrestricted surface intersection is not containment evidence."}};
                    QJsonArray statuses,faultyContours;
                    for(int index=1;index<=builder.NbContours();++index)
                        statuses.append(QJsonObject{{"index",index},{"stripeStatus",static_cast<int>(builder.StripeStatus(index))}});
                    for(int index=1;index<=builder.NbFaultyContours();++index)faultyContours.append(builder.FaultyContour(index));
                    result["statuses"]=statuses;result["faultyContours"]=faultyContours;
                    result["faultyVertexCount"]=builder.NbFaultyVertices();
                    QJsonArray faultyVertices;
                    for(int index=1;index<=builder.NbFaultyVertices();++index) {
                        const auto vertex=builder.FaultyVertex(index);
                        QJsonArray incidentEdges,incidentEdgeIds;
                        for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()) {
                            TopoDS_Vertex first,last;TopExp::Vertices(edge.edge,first,last);
                            if((!first.IsNull() && first.IsSame(vertex)) || (!last.IsNull() && last.IsSame(vertex)))
                                {incidentEdgeIds.append(edge.edgeId);
                                 incidentEdges.append(QJsonObject{{"edgeId",edge.edgeId},{"provenance",edge.provenance}});}
                        }
                        faultyVertices.append(QJsonObject{{"position",point(BRep_Tool::Pnt(vertex))},
                            {"tolerance",BRep_Tool::Tolerance(vertex)},
                            {"fullTopology",vertexTopology(body,vertex,shapeEdges,shapeFaces,evidencePrefix)},
                            {"incidentEdgeIds",incidentEdgeIds},{"incidentEdges",incidentEdges}});
                    }
                    result["faultyVertices"]=faultyVertices;
                }
                file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
                return result.value("isDone").toBool() ? 0 : 2;
            }
            try {
                const auto candidate=CadDocument::filletCandidate(document.authored(),
                    {{"ownerId",owner},{"targetBodyId",support.bodyId},
                     {"edgeIds",QJsonArray{selected[0],selected[1]}},{"radiusMm",0.5}});
                const auto error=document.commit(candidate,"Two upper rim rails R0.5");
                if(!error.isEmpty())throw std::runtime_error(error.toStdString());
            } catch(const std::exception& error) {
                result["filletAccepted"]=false;result["filletError"]=QString::fromUtf8(error.what());
                file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
                qCritical("two-edge R0.5 refused: %s",error.what());return 2;
            }
            result["filletAccepted"]=true;
            for(const auto& body:document.bodies())if(body.bodyId==support.bodyId) {
                result["filletedBody"]=body.summary;
                result["filletedSolidEdgeCount"]=static_cast<int>(body.solidEdges.size());
            }
            const auto save=QString::fromUtf8(argv[2])+".alloy";
            if(!document.save(save).isEmpty())throw std::runtime_error("filleted model save refused");
            result["savedModel"]=save;
        }
        file.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
        return 0;
    } catch(const std::exception& error) {qCritical("%s",error.what());return 1;}
}
