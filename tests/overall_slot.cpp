// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <iostream>

namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void near(double a,double b,const char* why){require(std::abs(a-b)<1e-4,why);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);
    try{
        CadDocument document;auto draft=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        auto created=SketchGeometry::addOverallSlot(draft,{{"firstUMm",0},{"firstVMm",0},{"lastUMm",30},
            {"lastVMm",0},{"widthMm",6.5}});
        require(created.ok,created.error.toUtf8().constData());draft=created.feature;
        auto p=draft["parameters"].toObject();const auto group=p["slotGroups"].toArray().first().toObject();
        require(group["mode"]=="overall" && p["entities"].toArray().size()==5,"real analytic overall slot");
        const auto dimensions=p["dimensions"].toArray();require(dimensions.size()==3,"length width angle driving dimensions");
        const auto profile=SketchGeometry::evaluateFeature(draft);require(profile.profiles.size()==1,"closed selectable slot profile");
        near(profile.summary["boundsMm"].toObject()["size"].toArray()[0].toDouble(),30,"overall length is end to end");
        near(profile.summary["boundsMm"].toObject()["size"].toArray()[1].toDouble(),6.5,"full source width");
        near(profile.profiles[0].summary["areaMm2"].toDouble(),23.5*6.5+std::numbers::pi*3.25*3.25,"slot area");
        const auto lengthId=dimensions[0].toObject()["id"],widthId=dimensions[1].toObject()["id"],angleId=dimensions[2].toObject()["id"];
        auto changed=SketchGeometry::updateDimension(draft,{{"dimensionId",lengthId},{"valueMm",40}});
        require(changed.ok,changed.error.toUtf8().constData());draft=changed.feature;
        near(SketchGeometry::evaluateFeature(draft).summary["boundsMm"].toObject()["size"].toArray()[0].toDouble(),40,"length edit follows L-W");
        changed=SketchGeometry::updateDimension(draft,{{"dimensionId",widthId},{"valueMm",8}});
        require(changed.ok,changed.error.toUtf8().constData());draft=changed.feature;
        near(SketchGeometry::evaluateFeature(draft).summary["boundsMm"].toObject()["size"].toArray()[0].toDouble(),40,"width edit retains overall length");
        changed=SketchGeometry::updateDimension(draft,{{"dimensionId",angleId},{"valueDeg",25}});
        require(changed.ok,changed.error.toUtf8().constData());draft=changed.feature;
        near(SketchGeometry::evaluateFeature(draft).profiles[0].summary["areaMm2"].toDouble(),32*8+std::numbers::pi*16,"angle edit retains overall area");
        changed=SketchGeometry::updateDimension(draft,{{"dimensionId",lengthId},{"valueMm",7}});
        require(!changed.ok && changed.feature==draft,"L below W fails atomically");
        require(document.commit(SketchGeometry::replaceFeature(document.authored(),draft),"Overall Slot").isEmpty(),"commit slot");
        const auto authored=document.authored();document.history().undo();document.history().redo();require(document.authored()==authored,"undo redo");
        QTemporaryDir temp;const auto path=temp.path()+"/overall-slot.alloy";require(document.save(path).isEmpty(),"save");
        CadDocument reopened;require(reopened.open(path).isEmpty() && reopened.authored()==authored,"exact reopen");
        std::cout<<"Overall Slot core contract passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
