// SPDX-License-Identifier: MIT
#include "ReferenceCanvas.h"
#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QUuid>
#include <QSet>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace ReferenceCanvas {
QImage image(const QJsonObject& row){return QImage::fromData(QByteArray::fromBase64(row["pngBase64"].toString().toLatin1()),"PNG");}
QString validate(const QJsonObject& row){
    const QSet<QString> allowed{"id","name","sourceName","pngBase64","pixelWidth","pixelHeight","planeSource","centerUMm","centerVMm","angleDegrees","widthMm","opacity","visible","calibration"};
    for(auto it=row.begin();it!=row.end();++it)if(!allowed.contains(it.key()))return "Unknown canvas field";
    if(QUuid(row["id"].toString()).isNull() || row["name"].toString().trimmed().isEmpty() || !row["visible"].isBool())return "Invalid canvas identity, name or visibility";
    for(const auto& key:QStringList{"centerUMm","centerVMm","angleDegrees","widthMm","opacity"})if(!row[key].isDouble() || !std::isfinite(row[key].toDouble()))return "Canvas values must be finite numbers";
    if(row["widthMm"].toDouble()<=0 || row["widthMm"].toDouble()>100000 || std::abs(row["centerUMm"].toDouble())>100000 || std::abs(row["centerVMm"].toDouble())>100000 || row["opacity"].toDouble()<0 || row["opacity"].toDouble()>1)return "Canvas placement or opacity is outside supported range";
    const auto source=row["planeSource"].toObject();if(source.isEmpty())return "Canvas requires a plane source";
    const auto bytes=QByteArray::fromBase64(row["pngBase64"].toString().toLatin1());if(bytes.isEmpty() || bytes.size()>32*1024*1024)return "Canvas image is missing or too large";
    QBuffer buffer;buffer.setData(bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer,"PNG");const auto size=reader.size();
    if(!size.isValid() || size.width()>8192 || size.height()>8192 || qint64(size.width())*size.height()>32000000 || size.width()!=row["pixelWidth"].toInt() || size.height()!=row["pixelHeight"].toInt())return "Canvas pixel dimensions are invalid or exceed the supported image limit";
    if(reader.read().isNull())return "Canvas image cannot be decoded";
    if(row.contains("calibration")){
        const auto c=row["calibration"].toObject();const auto a=c["firstPixel"].toArray(),b=c["secondPixel"].toArray();
        if(c.size()!=3 || a.size()!=2 || b.size()!=2 || !c["distanceMm"].isDouble() || c["distanceMm"].toDouble()<=0)return "Invalid canvas calibration";
        for(const auto& point:{a,b})for(int i=0;i<2;++i)if(!point[i].isDouble() || !std::isfinite(point[i].toDouble()) || point[i].toDouble()<0 || point[i].toDouble()>(i==0?size.width():size.height()))return "Calibration points must lie on the image";
        if(std::hypot(a[0].toDouble()-b[0].toDouble(),a[1].toDouble()-b[1].toDouble())<1e-6)return "Calibration points must be distinct";
    }
    return {};
}
QJsonObject candidate(const QJsonObject& document,const QJsonObject& args){
    const QSet<QString> allowed{"canvasId","path","name","planeSource","centerUMm","centerVMm","angleDegrees","widthMm","opacity","visible"};
    for(auto it=args.begin();it!=args.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unsupported canvas input");
    auto rows=document["canvases"].toArray();int index=-1;QJsonObject row;
    if(args.contains("canvasId")){for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==args["canvasId"]){index=i;row=rows[i].toObject();break;}if(index<0)throw std::invalid_argument("Canvas is unavailable");}
    else row={{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"name","Canvas"},{"planeSource",QJsonObject{{"plane","XY"}}},{"centerUMm",0},{"centerVMm",0},{"angleDegrees",0},{"widthMm",17.5},{"opacity",.5},{"visible",true}};
    if(args.contains("path")){
        QImageReader reader(args["path"].toString());reader.setAutoTransform(true);const auto size=reader.size();if(!size.isValid() || size.width()>8192 || size.height()>8192 || qint64(size.width())*size.height()>32000000)throw std::invalid_argument("Unsupported canvas image or pixel limit");
        auto pixels=reader.read();if(pixels.isNull())throw std::invalid_argument("Cannot read the local canvas image");QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);if(!pixels.save(&buffer,"PNG") || png.size()>32*1024*1024)throw std::invalid_argument("Cannot embed this canvas image");
        row["pngBase64"]=QString::fromLatin1(png.toBase64());row["sourceName"]=QFileInfo(args["path"].toString()).fileName();row["pixelWidth"]=pixels.width();row["pixelHeight"]=pixels.height();row.remove("calibration");
        if(index<0){row["name"]=QFileInfo(args["path"].toString()).completeBaseName();row["widthMm"]=17.5*pixels.width()/std::max(pixels.width(),pixels.height());}
    }
    for(auto it=args.begin();it!=args.end();++it)if(it.key()!="canvasId" && it.key()!="path")row[it.key()]=it.value();
    if(args.contains("widthMm"))row.remove("calibration");const auto error=validate(row);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());
    if(index<0)rows.append(row);else rows[index]=row;auto result=document;result["canvases"]=rows;return result;
}
QJsonObject frame(const QJsonObject& row,const EvaluatedDocument& evaluation){return GeometryKernel::resolvePlaneSource(evaluation,row["planeSource"].toObject());}
QPointF pixelToPlane(const QJsonObject& row,double x,double y){const auto scale=row["widthMm"].toDouble()/row["pixelWidth"].toDouble();const auto a=row["angleDegrees"].toDouble()*std::numbers::pi/180;const double u=(x-row["pixelWidth"].toDouble()/2)*scale,v=(row["pixelHeight"].toDouble()/2-y)*scale;return {row["centerUMm"].toDouble()+u*std::cos(a)-v*std::sin(a),row["centerVMm"].toDouble()+u*std::sin(a)+v*std::cos(a)};}
QPointF planeToPixel(const QJsonObject& row,double u,double v){const auto scale=row["widthMm"].toDouble()/row["pixelWidth"].toDouble();const auto a=row["angleDegrees"].toDouble()*std::numbers::pi/180;u-=row["centerUMm"].toDouble();v-=row["centerVMm"].toDouble();return {(u*std::cos(a)+v*std::sin(a))/scale+row["pixelWidth"].toDouble()/2,row["pixelHeight"].toDouble()/2-(-u*std::sin(a)+v*std::cos(a))/scale};}
QJsonObject calibrate(const QJsonObject& row,const QJsonArray& first,const QJsonArray& second,double distanceMm){
    auto result=row;result["calibration"]=QJsonObject{{"firstPixel",first},{"secondPixel",second},{"distanceMm",distanceMm}};auto error=validate(result);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());const auto span=std::hypot(first[0].toDouble()-second[0].toDouble(),first[1].toDouble()-second[1].toDouble());result["widthMm"]=distanceMm*row["pixelWidth"].toDouble()/span;error=validate(result);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return result;
}
QJsonObject inspect(const QJsonObject& row,const EvaluatedDocument& evaluation){auto result=row;result.remove("pngBase64");result["embedded"]=true;result["heightMm"]=row["widthMm"].toDouble()*row["pixelHeight"].toDouble()/row["pixelWidth"].toDouble();try{result["frame"]=frame(row,evaluation);result["availability"]="available";}catch(const std::exception& e){result["availability"]="unavailable";result["message"]=QString::fromUtf8(e.what());}return result;}
}
