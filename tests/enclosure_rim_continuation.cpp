// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "RoundedLoopOffset.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GeomTools.hxx>
#include <Geom_Surface.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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

}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        if(argc!=3 && argc!=4)throw std::runtime_error("usage: enclosure_rim_continuation_test source.alloy output.json [fillet|contour|common]");
        if(argc==4 && QString::fromUtf8(argv[3])=="inspect")return inspectSaved(QString::fromUtf8(argv[1]),QString::fromUtf8(argv[2]));
        CadDocument document;
        if(!document.open(argv[1]).isEmpty())throw std::runtime_error("source did not reopen");
        const auto sourceFeatureCount=document.authored().value("features").toArray().size();
        const auto support=cap(document);
        QString owner;
        for(const auto& body:document.bodies())if(body.bodyId==support.bodyId)owner=body.ownerId;
        const QJsonObject reference{{"producerFeatureId",support.producerFeatureId},
            {"bodyId",support.bodyId},{"faceId",support.faceId}};
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
