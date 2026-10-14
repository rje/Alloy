#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& result){
    if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;
}
QJsonObject dimension(const QJsonObject& sketch,const QString& name){
    for(const auto& entry:sketch["parameters"].toObject()["dimensions"].toArray())
        if(entry.toObject()["name"]==name)return entry.toObject();
    return {};
}
void near(double actual,double expected){check(std::abs(actual-expected)<1e-5,"unexpected dimension value");}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        auto sketch=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        sketch=accept(SketchGeometry::addCenterToCenterSlot(sketch,{{"firstUMm",0},{"firstVMm",0},
            {"secondUMm",40},{"secondVMm",0},{"widthMm",10}}));
        const auto group=sketch["parameters"].toObject()["slotGroups"].toArray().first().toObject();
        sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",group["centerlineId"]},
            {"valueMm",45}}));
        sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","radius"},
            {"entityId",group["arcIds"].toArray().first()},{"valueMm",5}}));
        check(dimension(sketch,"d1")["kind"]=="linear" && dimension(sketch,"d2")["kind"]=="radius",
            "slot length and radius consume first two stable local names");
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",100},{"v1Mm",0},{"u2Mm",100},{"v2Mm",90}});
        const auto side=sketch["parameters"].toObject()["entities"].toArray().last().toObject()
            .value("id").toString();
        sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"entityId",side},
            {"valueMm",100}}));
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",100},{"v1Mm",100},{"u2Mm",200},{"v2Mm",100}});
        const auto top=sketch["parameters"].toObject()["entities"].toArray().last().toObject()
            .value("id").toString();
        sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",top},
            {"expression","d3 + 50"},{"expressionUnits","mm"}}));
        const auto vertical=dimension(sketch,"d3"),formula=dimension(sketch,"d4");
        near(vertical["valueMm"].toDouble(),100);near(formula["valueMm"].toDouble(),150);
        check(formula["referenceIds"].toArray()==QJsonArray{vertical["id"]},
            "formula binds stable local UUID, not a display name lookup on replay");
        const auto before=sketch;
        auto refused=SketchGeometry::updateDimension(sketch,{{"dimensionId",formula["id"]},
            {"expression","d3 + 50"},{"expressionUnits","mm"}}, {},{"d3"});
        check(refused.ok && refused.feature==sketch,
            "unchanged formula retains old UUID binding after later user-name collision");
        refused=SketchGeometry::updateDimension(sketch,{{"dimensionId",formula["id"]},
            {"expression","d3 + 51"},{"expressionUnits","mm"}}, {},{"d3"});
        check(!refused.ok && refused.feature==before,"new ambiguous local/user binding refuses atomically");
        refused=SketchGeometry::updateDimension(sketch,{{"dimensionId",vertical["id"]},
            {"expression","d4 + 1"},{"expressionUnits","mm"}});
        check(!refused.ok && refused.feature==before,"local cycle refuses atomically");
        sketch=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",vertical["id"]},
            {"valueMm",120}}));
        near(dimension(sketch,"d3")["valueMm"].toDouble(),120);
        near(dimension(sketch,"d4")["valueMm"].toDouble(),170);
        sketch=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",formula["id"]},
            {"expression","d3 + 60"},{"expressionUnits","mm"}}));
        near(dimension(sketch,"d4")["valueMm"].toDouble(),180);
        check(dimension(sketch,"d4")["referenceIds"].toArray()==QJsonArray{vertical["id"]},
            "formula edit retains the stable source dimension reference");
        sketch=accept(SketchGeometry::updateDimension(sketch,{{"dimensionId",formula["id"]},
            {"expression","d3 + 50"},{"expressionUnits","mm"}}));
        near(dimension(sketch,"d4")["valueMm"].toDouble(),170);
        auto deletedSource=sketch;
        auto deletedParameters=deletedSource["parameters"].toObject();
        QJsonArray remainingDimensions;
        for(const auto& entry:deletedParameters["dimensions"].toArray())
            if(entry.toObject().value("id")!=vertical.value("id"))remainingDimensions.append(entry);
        deletedParameters["dimensions"]=remainingDimensions;
        deletedSource["parameters"]=deletedParameters;
        check(!SketchGeometry::solveDraft(deletedSource).ok &&
            dimension(sketch,"d3").value("id")==vertical.value("id"),
            "deleting a referenced source dimension refuses without changing the accepted sketch");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),
            "Sketch length expression").isEmpty(),"commit expression sketch");
        const auto authored=document.authored();
        bool readOnly=false;
        for(const auto& row:CadDocument::modelLengthRows(authored))if(row.toObject()["target"].toObject()["dimensionId"]==formula["id"])
            readOnly=row.toObject()["readOnly"].toBool() && row.toObject()["expression"]=="d3 + 50";
        check(readOnly,"model parameter table exposes expression as read-only local source");
        document.history().undo();document.history().redo();check(document.authored()==authored,"undo/redo exact");
        QTemporaryDir directory;check(directory.isValid(),"temporary directory");
        const auto path=directory.path()+"/sketch-expression.alloy";
        check(document.save(path).isEmpty(),"save expression sketch");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==authored,
            "reopen preserves local expression identity");
        std::cout<<"Sketch dimension expression core contract passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
