// SPDX-License-Identifier: MIT
#include "SketchText.h"
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Geom_BezierCurve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Pln.hxx>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QLineF>
#include <QPainterPath>
#include <QRawFont>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

static void initializeTextFontResource(){Q_INIT_RESOURCE(alloy_text_font);}
namespace {
constexpr auto digest="7da195a74c55bef988d0d48f9508bd5d849425c1770dba5d7bfc6ce9ed848954";
constexpr auto asset="dejavu-sans-2.37-regular";
[[noreturn]] void fail(const QString& message){throw std::invalid_argument(message.toStdString());}
bool validId(const QString& id){return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces)==id;}
QByteArray fontBytes(){
    initializeTextFontResource();
    QFile font(":/alloy/fonts/DejaVuSans.ttf");
    if(!font.open(QIODevice::ReadOnly))fail("Sketch Text font is missing: "+QString::fromLatin1(asset));
    const auto bytes=font.readAll();
    if(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=digest)
        fail("Sketch Text font hash mismatch: "+QString::fromLatin1(asset));
    return bytes;
}
gp_Pnt point(const QJsonObject& frame,const QJsonObject& text,const QPointF& p,double scale){
    const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
    const auto angle=text["angleDeg"].toDouble()*std::numbers::pi/180.;
    const double x=p.x()*scale,y=-p.y()*scale;
    const double a=text["uMm"].toDouble()+x*std::cos(angle)-y*std::sin(angle);
    const double b=text["vMm"].toDouble()+x*std::sin(angle)+y*std::cos(angle);
    return {origin[0].toDouble()+a*u[0].toDouble()+b*v[0].toDouble(),
        origin[1].toDouble()+a*u[1].toDouble()+b*v[1].toDouble(),
        origin[2].toDouble()+a*u[2].toDouble()+b*v[2].toDouble()};
}
struct Contour { QPainterPath path; TopoDS_Wire wire; double area=0; };
Contour contour(const QPainterPath& path,const QJsonObject& frame,const QJsonObject& text,double scale){
    if(path.elementCount()<3)fail("Sketch Text has an empty glyph contour");
    BRepBuilderAPI_MakeWire builder;
    QPointF first,current;
    bool started=false;
    for(int i=0;i<path.elementCount();++i){
        const auto e=path.elementAt(i);const QPointF next(e.x,e.y);
        if(e.isMoveTo()){if(started)fail("Sketch Text contour has multiple starts");first=current=next;started=true;continue;}
        if(!started)fail("Sketch Text contour has no start");
        if(e.isLineTo()){
            if(QLineF(current,next).length()>1e-9)builder.Add(BRepBuilderAPI_MakeEdge(point(frame,text,current,scale),point(frame,text,next,scale)));
            current=next;
        }else if(e.isCurveTo()){
            if(i+2>=path.elementCount())fail("Sketch Text cubic is incomplete");
            const auto b=path.elementAt(++i),c=path.elementAt(++i);
            TColgp_Array1OfPnt poles(1,4);poles.SetValue(1,point(frame,text,current,scale));poles.SetValue(2,point(frame,text,next,scale));poles.SetValue(3,point(frame,text,{b.x,b.y},scale));poles.SetValue(4,point(frame,text,{c.x,c.y},scale));
            builder.Add(BRepBuilderAPI_MakeEdge(new Geom_BezierCurve(poles)));current={c.x,c.y};
        }else fail("Sketch Text contour contains unsupported path element");
    }
    if(QLineF(current,first).length()>1e-9)builder.Add(BRepBuilderAPI_MakeEdge(point(frame,text,current,scale),point(frame,text,first,scale)));
    if(!builder.IsDone())fail("Sketch Text outline did not make a closed wire");
    Contour result;result.path=path;result.wire=builder.Wire();
    const auto polygons=path.toSubpathPolygons();if(polygons.size()!=1)fail("Sketch Text contour classification failed");
    const auto polygon=polygons.first();for(int i=0;i<polygon.size();++i){const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];result.area+=a.x()*b.y()-b.x()*a.y();}
    result.area*=-.5*scale*scale; // Qt Y-down reflected into sketch Y-up.
    if(std::abs(result.area)<1e-8)fail("Sketch Text has a degenerate contour");
    return result;
}
QList<QPainterPath> subpaths(const QPainterPath& path){
    QList<QPainterPath> result;QPainterPath current;
    for(int i=0;i<path.elementCount();++i){const auto e=path.elementAt(i);
        if(e.isMoveTo()){if(!current.isEmpty()){result.append(current);current=QPainterPath{};}current.moveTo(e.x,e.y);}
        else if(e.isLineTo())current.lineTo(e.x,e.y);
        else if(e.isCurveTo()){if(i+2>=path.elementCount())fail("Sketch Text cubic is incomplete");const auto b=path.elementAt(++i),c=path.elementAt(++i);current.cubicTo(e.x,e.y,b.x,b.y,c.x,c.y);}
        else fail("Sketch Text outline has unsupported element");
    }
    if(!current.isEmpty())result.append(current);
    return result;
}
}

QString SketchText::fontId(){return QString::fromLatin1(asset);}
QString SketchText::fontSha256(){return QString::fromLatin1(digest);}
QString SketchText::validate(const QJsonObject& text,QSet<QString>& ids){
    if(text.size()!=9 || !validId(text["id"].toString()) || ids.contains(text["id"].toString()))return "Sketch Text requires a distinct stable text ID";
    if(text["fontId"]!=fontId() || text["fontSha256"]!=fontSha256() || text["style"]!="regular")return "Unsupported Sketch Text font asset, digest or style";
    const auto value=text["text"].toString();
    if(value.isEmpty() || value.size()>16)return "Sketch Text requires 1 to 16 uppercase Latin letters or digits";
    for(const auto c:value)if(!((c>=u'A' && c<=u'Z') || (c>=u'0' && c<=u'9')))return "Sketch Text supports uppercase A-Z and 0-9 only";
    for(const auto key:{"heightMm","uMm","vMm","angleDeg"})if(!text[key].isDouble() || !std::isfinite(text[key].toDouble()))return QString("Invalid Sketch Text ")+key;
    if(text["heightMm"].toDouble()<.1 || text["heightMm"].toDouble()>1000 || std::abs(text["uMm"].toDouble())>100000 || std::abs(text["vMm"].toDouble())>100000 || std::abs(text["angleDeg"].toDouble())>360000)return "Sketch Text size or placement is outside supported range";
    ids.insert(text["id"].toString());return {};
}
QJsonObject SketchText::create(const QJsonObject& a){
    return {{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"text",a["text"]},{"fontId",fontId()},{"fontSha256",fontSha256()},
        {"style","regular"},{"heightMm",a["heightMm"]},{"uMm",a["uMm"]},{"vMm",a["vMm"]},{"angleDeg",a["angleDeg"]}};
}
SketchText::Result SketchText::evaluate(const QJsonObject& sketch,const QJsonObject& frame){
    const auto parameters=sketch["parameters"].toObject();const auto texts=parameters["texts"].toArray();
    if(texts.size()!=1 || !parameters["entities"].toArray().isEmpty())fail("Sketch Text currently requires a text-only sketch");
    const auto text=texts.first().toObject();QSet<QString> ids;const auto error=validate(text,ids);if(!error.isEmpty())fail(error);
    QRawFont font; font.loadFromData(fontBytes(),1000,QFont::PreferNoHinting);if(!font.isValid())fail("Pinned Sketch Text font could not be loaded");
    const auto cap=font.pathForGlyph(font.glyphIndexesForString("H").value(0)).boundingRect().height();
    if(!(cap>0))fail("Pinned Sketch Text font has no cap height");
    const auto scale=text["heightMm"].toDouble()/cap;
    TopoDS_Compound shape;BRep_Builder builder;builder.MakeCompound(shape);
    Result result;QJsonArray summaries;double advance=0,totalArea=0;
    const auto glyphs=font.glyphIndexesForString(text["text"].toString());
    const auto advances=font.advancesForGlyphIndexes(glyphs);
    for(int index=0;index<glyphs.size();++index){
        if(glyphs[index]==0)fail("Sketch Text character is absent from pinned font");
        auto path=font.pathForGlyph(glyphs[index]);path.translate(advance,0);
        const auto paths=subpaths(path);if(paths.isEmpty())fail("Sketch Text character has no outline");
        QList<Contour> contours;for(const auto& sub:paths)contours.append(contour(sub,frame,text,scale));
        int outer=0;for(int i=1;i<contours.size();++i)if(std::abs(contours[i].area)>std::abs(contours[outer].area))outer=i;
        const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
        gp_Pln plane(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()),gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble()));
        auto outside=contours[outer].wire;if(contours[outer].area<0)outside.Reverse();
        BRepBuilderAPI_MakeFace face(plane,outside,true);
        if(!face.IsDone())fail("Sketch Text outer contour did not make a face");
        for(int i=0;i<contours.size();++i)if(i!=outer){
            auto hole=contours[i].wire;if(contours[i].area>0)hole.Reverse();face.Add(hole);
        }
        if(!face.IsDone() || !BRepCheck_Analyzer(face.Face()).IsValid())fail("Sketch Text has invalid or overlapping glyph contours");
        GProp_GProps mass;BRepGProp::SurfaceProperties(face.Face(),mass);if(!(mass.Mass()>1e-6))fail("Sketch Text profile has no positive area");
        const auto id=QUuid::createUuidV5(QUuid(text["id"].toString()),QByteArray("alloy-text-slot/1/")+QByteArray::number(index)).toString(QUuid::WithoutBraces);
        QJsonObject summary{{"profileId",id},{"sketchId",sketch["id"]},{"plane",parameters["plane"]},{"frame",frame},
            {"kernel","OCCT"},{"valid",true},{"areaMm2",mass.Mass()},
            {"sourceEntityIds",QJsonArray{}},{"textId",text["id"]},{"glyphIndex",index},{"glyph",text["text"].toString().mid(index,1)},
            {"outerLoopCount",1},{"innerLoopCount",contours.size()-1}};
        result.profiles.push_back({id,sketch["id"].toString(),parameters["plane"].toString(),face.Face(),summary,{}});
        summaries.append(summary);totalArea+=mass.Mass();builder.Add(shape,face.Face());
        advance+=advances[index].x();
    }
    result.shape=shape;result.summary={{"textId",text["id"]},{"text",text["text"]},{"fontId",fontId()},{"fontSha256",fontSha256()},
        {"heightMm",text["heightMm"]},{"uMm",text["uMm"]},{"vMm",text["vMm"]},{"angleDeg",text["angleDeg"]},
        {"profileIds",QJsonArray{}},{"profiles",summaries},{"areaMm2",totalArea}};
    QJsonArray idsOut;for(const auto& profile:result.profiles)idsOut.append(profile.profileId);result.summary["profileIds"]=idsOut;
    return result;
}
