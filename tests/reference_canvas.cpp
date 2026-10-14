// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "ReferenceCanvas.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <iostream>
#include <cmath>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir temp;const auto source=temp.filePath("local.png");QImage image(400,200,QImage::Format_RGB32);image.fill(Qt::red);check(image.save(source),"fixture image");
    CadDocument doc;const auto initial=doc.authored();auto candidate=ReferenceCanvas::candidate(initial,{{"path",source},{"planeSource",QJsonObject{{"plane","XZ"}}},{"centerUMm",12},{"centerVMm",3},{"angleDegrees",30}});auto row=candidate["canvases"].toArray()[0].toObject();const auto id=row["id"].toString();check(row["widthMm"].toDouble()==17.5,"documented initial width");
    const auto mapped=ReferenceCanvas::pixelToPlane(row,60,110),pixel=ReferenceCanvas::planeToPixel(row,mapped.x(),mapped.y());check(std::abs(pixel.x()-60)<1e-10 && std::abs(pixel.y()-110)<1e-10,"rotated pixel plane inversion");check(doc.commitCanvases(candidate["canvases"].toArray(),"Canvas").isEmpty(),"insert");const auto inserted=doc.authored();
    row=ReferenceCanvas::calibrate(row,{50,100},{350,100},120);const auto first=ReferenceCanvas::pixelToPlane(row,50,100),second=ReferenceCanvas::pixelToPlane(row,350,100);check(std::abs(std::hypot(first.x()-second.x(),first.y()-second.y())-120)<1e-10,"calibrated physical distance");check(doc.commitCanvases({row},"Calibrate").isEmpty(),"calibrate commit");const auto calibrated=doc.authored();check(row["id"]==id && row["widthMm"].toDouble()==160,"stable calibrated identity");
    doc.history().undo();check(doc.authored()==inserted,"undo calibration");doc.history().redo();check(doc.authored()==calibrated,"redo calibration");
    bool refused=false;try{ReferenceCanvas::calibrate(row,{50,100},{50,100},20);}catch(const std::exception&){refused=true;}check(refused && doc.authored()==calibrated,"distinct point atomic refusal");
    auto invalid=row;invalid["opacity"]=2;check(!doc.commitCanvases({invalid},"Invalid").isEmpty() && doc.authored()==calibrated,"opacity atomic refusal");
    const auto saved=temp.filePath("reference.alloy");check(doc.save(saved).isEmpty(),"save");QFile::remove(source);CadDocument reopened;check(reopened.open(saved).isEmpty() && reopened.authored()==calibrated,"portable embedded image reopen");check(!ReferenceCanvas::image(reopened.authored()["canvases"].toArray()[0].toObject()).isNull(),"embedded image decode");
    auto hidden=ReferenceCanvas::candidate(doc.authored(),{{"canvasId",id},{"visible",false},{"opacity",.3}});check(doc.commitCanvases(hidden["canvases"].toArray(),"Visibility").isEmpty(),"visibility edit");check(doc.bodies().empty() && doc.sketches().empty(),"reference excluded evaluated geometry");doc.history().undo();check(doc.authored()==calibrated,"visibility undo");
    CadDocument linked;const auto planeId=CadDocument::newId();auto planeDocument=linked.authored();QJsonObject plane{{"id",planeId},{"ownerId",linked.activeOwnerId()},{"type","constructionPlane"},{"name","Reference support"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XY"}}},{"offsetMm",5}}}};planeDocument["features"]=QJsonArray{plane};check(linked.commit(planeDocument,"Support").isEmpty(),"retained support plane");auto supported=row;supported["planeSource"]=QJsonObject{{"planeId",planeId}};check(linked.commitCanvases({supported},"Supported canvas").isEmpty(),"supported canvas");check(ReferenceCanvas::frame(supported,linked.evaluation())["origin"].toArray()[2].toDouble()==5,"initial plane frame");auto shifted=linked.authored();auto params=plane["parameters"].toObject();params["offsetMm"]=8;plane["parameters"]=params;shifted["features"]=QJsonArray{plane};check(linked.commit(shifted,"Edit support").isEmpty(),"upstream plane edit");check(ReferenceCanvas::frame(supported,linked.evaluation())["origin"].toArray()[2].toDouble()==8 && linked.authored()["canvases"].toArray()[0].toObject()==supported,"retained canvas follows upstream edit");linked.history().undo();check(ReferenceCanvas::frame(supported,linked.evaluation())["origin"].toArray()[2].toDouble()==5,"support edit undo");
    std::cout<<"reference_canvas pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
