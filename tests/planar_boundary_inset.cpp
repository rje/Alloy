// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "PlanarBoundaryInset.h"
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <QTemporaryDir>
#include <QDir>
#include <QSet>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
const EvaluatedSupportFace& cap(const CadDocument& document) {
    for(const auto& body:document.bodies())for(const auto& support:body.supportFaces)
        if(support.role=="split.negative.cap")return support;
    throw std::runtime_error("saved split cap unavailable");
}
QSet<QString> ids(const PlanarBoundaryInset::Result& result) {
    QSet<QString> output;for(const auto& edge:result.boundaryEdges)output.insert(edge.edgeId);return output;
}
void validate(const EvaluatedSupportFace& source,const PlanarBoundaryInset::Result& inset) {
    check(BRepCheck_Analyzer(inset.face).IsValid(),"inset face invalid");
    int faceCount=0,wireCount=0;
    for(TopExp_Explorer it(inset.face,TopAbs_FACE);it.More();it.Next())++faceCount;
    for(TopExp_Explorer it(inset.face,TopAbs_WIRE);it.More();it.Next())++wireCount;
    check(faceCount==1 && wireCount==1,"inset must be one connected face");
    check(inset.boundaryEdges.size()==17 && inset.vanishedSourceEdgeIds.size()==2,
        "source-sized inset topology/vanishing lips changed");
    check(ids(inset).size()==int(inset.boundaryEdges.size()),"output ancestry IDs duplicate");
    for(const auto& edge:inset.boundaryEdges) {
        check(!edge.provenance["sourceEdgeIds"].toArray().isEmpty()
            || !edge.provenance["sourceVertexKeys"].toArray().isEmpty(),"inset edge has no stable source ancestry");
        BRepAdaptor_Curve curve(edge.edge);
        for(int i=0;i<=16;++i) {
            const auto point=curve.Value(curve.FirstParameter()
                +(curve.LastParameter()-curve.FirstParameter())*i/16.0);
            BRepClass_FaceClassifier classifier(source.face,point,1e-7);
            BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(point).Vertex(),BRepTools::OuterWire(source.face));
            check(classifier.State()!=TopAbs_OUT && distance.IsDone() && distance.Value()>=1.25-1e-5,
                "inset boundary containment/clearance failed");
        }
    }
}
}

int main(int argc,char** argv) {
    try {
        check(argc==2 || argc==3,
            "usage: planar_boundary_inset_test /absolute/path/split-prefix.alloy [artifact-directory]");
        CadDocument document;check(document.open(argv[1]).isEmpty(),"saved source could not reopen");
        const auto before=document.authored();
        const auto insetFeatureId=QUuid::createUuidV5(QUuid(cap(document).faceId),"focused-planar-inset")
            .toString(QUuid::WithoutBraces);
        const auto baseline=PlanarBoundaryInset::evaluate(cap(document),1.25,insetFeatureId);
        validate(cap(document),baseline);
        check(std::abs(baseline.summary["areaMm2"].toDouble()-233.8576962999306)<1e-5,
            "saved exact seam inset area changed");
        auto edited=before;auto features=edited["features"].toArray();bool changed=false;
        for(int i=0;i<features.size();++i) {
            auto feature=features[i].toObject();
            if(feature["name"]!="Centered case stock")continue;
            auto parameters=feature["parameters"].toObject();
            auto dimensions=parameters["dimensions"].toArray();
            for(int j=0;j<dimensions.size();++j){auto dimension=dimensions[j].toObject();
                if(dimension["kind"]!="horizontal" || dimension["valueMm"].toDouble()!=64)continue;
                dimension["valueMm"]=65;dimensions[j]=dimension;changed=true;break;
            }
            parameters["dimensions"]=dimensions;feature["parameters"]=parameters;features[i]=feature;break;
        }
        check(changed,"upstream stock width dimension unavailable");edited["features"]=features;
        const auto commitError=document.commit(edited,"Widen source stock 64 to 65");
        check(commitError.isEmpty(),"upstream width edit refused");
        const auto accepted=document.authored();
        const auto widened=PlanarBoundaryInset::evaluate(cap(document),1.25,insetFeatureId);
        validate(cap(document),widened);
        check(std::abs(widened.summary["areaMm2"].toDouble()-baseline.summary["areaMm2"].toDouble())>0.01,
            "inset ignored upstream stock width edit");
        check(ids(widened)==ids(baseline),"source edit changed retained inset edge identities");
        check(widened.vanishedSourceEdgeIds==baseline.vanishedSourceEdgeIds,
            "source edit changed vanished lip source IDs");
        document.history().undo();check(document.authored()==before,"undo lost original source");
        const auto undone=PlanarBoundaryInset::evaluate(cap(document),1.25,insetFeatureId);
        check(ids(undone)==ids(baseline) && std::abs(undone.summary["areaMm2"].toDouble()
            -baseline.summary["areaMm2"].toDouble())<1e-5,"undo did not restore inset");
        document.history().redo();check(document.authored()==accepted,"redo lost widened source");
        QTemporaryDir directory;check(directory.isValid(),"temporary directory failed");
        const auto artifactDirectory=argc==3?QString::fromLocal8Bit(argv[2]):directory.path();
        check(QDir().mkpath(artifactDirectory),"artifact directory unavailable");
        BRepTools::Write(baseline.face,(artifactDirectory+"/inset-before.brep").toLocal8Bit().constData());
        BRepTools::Write(widened.face,(artifactDirectory+"/inset-after.brep").toLocal8Bit().constData());
        const auto path=artifactDirectory+"/edited-seam.alloy";
        check(document.save(path).isEmpty(),"edited source save failed");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==accepted,
            "edited source reopen failed");
        const auto replay=PlanarBoundaryInset::evaluate(cap(reopened),1.25,insetFeatureId);
        validate(cap(reopened),replay);
        check(ids(replay)==ids(widened) && std::abs(replay.summary["areaMm2"].toDouble()
            -widened.summary["areaMm2"].toDouble())<1e-5,"reopened inset geometry/lineage changed");
        std::cout<<"PASS exact planar inset sourceArea="<<baseline.summary["sourceAreaMm2"].toDouble()
            <<" insetArea="<<baseline.summary["areaMm2"].toDouble()
            <<" widenedArea="<<widened.summary["areaMm2"].toDouble()
            <<" outputEdges="<<widened.boundaryEdges.size()
            <<" vanished="<<widened.vanishedSourceEdgeIds.size()
            <<" source-edit/undo/redo/reopen stable\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL "<<error.what()<<'\n';return 1;
    }
}
