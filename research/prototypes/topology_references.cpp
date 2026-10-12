#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools_History.hxx>
#include <Bnd_Box.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <stdexcept>

constexpr double roleToleranceMm = 1e-6;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool near(double a, double b) { return std::abs(a-b) <= roleToleranceMm; }

// Only valid for the authored axis-aligned box generator, never arbitrary results.
TopoDS_Edge boxCorner(const TopoDS_Shape& box, double width, double depth, double z0, double z1) {
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(box, TopAbs_EDGE, edges);
    TopoDS_Edge found;
    int matches = 0;
    for (int i=1; i<=edges.Extent(); ++i) {
        Bnd_Box bounds; BRepBndLib::AddOptimal(edges(i), bounds, false, false);
        double x0,y0,low,x1,y1,high; bounds.Get(x0,y0,low,x1,y1,high);
        if (near(x0,width) && near(x1,width) && near(y0,depth) && near(y1,depth)
            && near(low,z0) && near(high,z1)) { found = TopoDS::Edge(edges(i)); ++matches; }
    }
    require(matches == 1, "Semantic generator role must resolve uniquely");
    return found;
}

TopoDS_Face positiveXFace(const TopoDS_Shape& box, double width) {
    TopTools_IndexedMapOfShape faces; TopExp::MapShapes(box,TopAbs_FACE,faces);
    TopoDS_Face found; int matches = 0;
    for (int i=1; i<=faces.Extent(); ++i) {
        Bnd_Box bounds; BRepBndLib::AddOptimal(faces(i),bounds,false,false);
        double x0,y0,z0,x1,y1,z1; bounds.Get(x0,y0,z0,x1,y1,z1);
        if (near(x0,width) && near(x1,width)) { found = TopoDS::Face(faces(i)); ++matches; }
    }
    require(matches == 1,"Generator face role must be unique"); return found;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc,argv);
    try {
        require(argc == 2, "Expected evidence directory");
        QJsonArray edits;
        for (double width : {10.0,22.0}) {
            const auto box = BRepPrimAPI_MakeBox(width,20,30).Shape();
            const auto edge = boxCorner(box,width,20,0,30);
            const auto face = positiveXFace(box,width);
            BRepFilletAPI_MakeFillet fillet(box); fillet.Add(1,edge); fillet.Build();
            require(fillet.IsDone() && BRepCheck_Analyzer(fillet.Shape()).IsValid(), "Fillet failed after generator edit");
            const int generated = fillet.Generated(edge).Extent();
            require(generated > 0, "Fillet must expose generated topology from the selected edge");
            require(fillet.Modified(face).Extent() == 1,"Fillet must expose the modified adjacent face");
            edits.append(QJsonObject{{"widthMm",width},{"role","edge:positive-x:positive-y"},
                {"resolution","unique"},{"filletRadiusMm",1},{"generatedShapes",generated},
                {"faceRole","face:positive-x"},{"modifiedFaceDescendants",fillet.Modified(face).Extent()},
                {"originalEdgeDeleted",bool(fillet.IsDeleted(edge))},{"valid",true}});
        }
        const auto original = BRepPrimAPI_MakeBox(10,20,30).Shape();
        const auto corner = boxCorner(original,10,20,0,30);
        BRepAlgoAPI_Cut split(original,BRepPrimAPI_MakeBox(gp_Pnt(9,19,14),gp_Pnt(11,21,16)).Shape());
        require(split.IsDone() && BRepCheck_Analyzer(split.Shape()).IsValid(), "Split fixture invalid");
        const auto splitCount = split.Modified(corner).Extent();
        require(splitCount == 2, "Notch must split the original edge into two descendants");
        BRepAlgoAPI_Cut deletion(original,BRepPrimAPI_MakeBox(gp_Pnt(9,-1,-1),gp_Pnt(11,21,31)).Shape());
        require(deletion.IsDone() && BRepCheck_Analyzer(deletion.Shape()).IsValid(), "Deletion fixture invalid");
        require(deletion.IsDeleted(corner) && deletion.Modified(corner).IsEmpty(), "Deleted edge must not resolve to the new boundary");

        BRepAlgoAPI_Fuse joined(BRepPrimAPI_MakeBox(10,20,15).Shape(),
            BRepPrimAPI_MakeBox(gp_Pnt(0,0,15),10,20,15).Shape());
        require(joined.IsDone() && BRepCheck_Analyzer(joined.Shape()).IsValid(), "Merge source invalid");
        const auto lower = boxCorner(joined.Shape(),10,20,0,15);
        const auto upper = boxCorner(joined.Shape(),10,20,15,30);
        ShapeUpgrade_UnifySameDomain unify(joined.Shape(),true,true,false); unify.Build();
        require(BRepCheck_Analyzer(unify.Shape()).IsValid(), "Unified shape invalid");
        const auto history = unify.History();
        require(!history.IsNull(), "Unifier must retain history");
        const auto& first = history->Modified(lower);
        const auto& second = history->Modified(upper);
        require(first.Extent()==1 && second.Extent()==1 && first.First().IsSame(second.First()), "Two source edges must map to one merged descendant");
        QJsonObject result{{"result","passed"},{"occt","7.9.3"},{"linearRoleToleranceMm",roleToleranceMm},
            {"reference",QJsonObject{{"producerFeatureId","286b6725-8aab-44d2-90d2-e22d85380411"},
                {"bodyId","c38031dd-5112-4e5b-836a-4746bc12bfe1"},{"kind","edge"},
                {"generatorRole","edge:positive-x:positive-y"}}},
            {"widthEdits",edits},
            {"split",QJsonObject{{"modifiedDescendants",splitCount},{"resolution","ambiguous"},{"automaticReattachment",false},{"valid",true}}},
            {"deletion",QJsonObject{{"deleted",true},{"resolution","unresolved"},{"automaticReattachment",false},{"valid",true}}},
            {"merge",QJsonObject{{"sourceEdges",2},{"sameDescendant",true},{"resolution","merged_requires_repair"},{"valid",true}}},
            {"limits",QJsonArray{"OCCT-only fixture; no application feature or persistent-reference subsystem implemented",
                "Semantic roles apply only to this primitive generator; no topology-index or nearest-edge fallback",
                "Generic history composition, load/recompute reference resolution and user repair remain follow-up work"}}};
        QDir().mkpath(argv[1]); QFile output(QDir(argv[1]).filePath("topology-references.json"));
        require(output.open(QIODevice::WriteOnly),"Cannot write evidence");
        output.write(QJsonDocument(result).toJson());
        std::cout << "PASS: generator edit, fillet history, split, deletion and many-to-one merge\n";
        return 0;
    } catch (const Standard_Failure& failure) { std::cerr << failure.GetMessageString() << '\n'; }
      catch (const std::exception& failure) { std::cerr << failure.what() << '\n'; }
    return 1;
}
