// SPDX-License-Identifier: MIT
#include "BodyCopy.h"
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <sstream>
#include <cmath>
namespace {
constexpr qsizetype maximumBytes=32*1024*1024;
void requireSolid(const TopoDS_Shape& shape){
    if(shape.IsNull())throw std::invalid_argument("Body Copy snapshot is empty");
    const auto s=GeometryKernel::summarize(shape);const auto volume=s["volumeMm3"].toDouble();
    if(!s["valid"].toBool() || !s["closed"].toBool() || s["topology"].toObject()["solids"].toInt()!=1
        || !std::isfinite(volume) || volume<=0)throw std::invalid_argument("Body Copy requires one valid closed solid");
}
QByteArray bytes(const QJsonObject& source){
    const auto encoded=source["bytesBase64"].toString().toLatin1();
    const auto decoded=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
    if(!decoded || decoded.decoded.isEmpty() || decoded.decoded.size()>maximumBytes
        || decoded.decoded.size()!=source["byteCount"].toInteger() || decoded.decoded.toBase64()!=encoded)
        throw std::invalid_argument("Body Copy snapshot encoding or byte count is invalid");
    if(QString::fromLatin1(QCryptographicHash::hash(decoded.decoded,QCryptographicHash::Sha256).toHex())!=source["sha256"].toString())
        throw std::invalid_argument("Body Copy snapshot hash mismatch");
    return decoded.decoded;
}
}
QJsonObject BodyCopy::capture(const EvaluatedBody& source){
    requireSolid(source.shape);
    std::ostringstream stream;BRepTools::Write(source.shape,stream,false,false,TopTools_FormatVersion_VERSION_3);
    const auto data=QByteArray::fromStdString(stream.str());
    if(data.isEmpty() || data.size()>maximumBytes)throw std::invalid_argument("Body Copy snapshot exceeds 32 MiB");
    return {{"format","occt-brep-v3"},{"bytesBase64",QString::fromLatin1(data.toBase64())},{"byteCount",data.size()},
        {"sha256",QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex())}};
}
QString BodyCopy::validate(const QJsonObject& p){
    if(p.size()!=4 || !p["snapshot"].isObject())return "Body Copy requires a frozen snapshot and explicit XYZ translation";
    const auto s=p["snapshot"].toObject();
    static const QRegularExpression sha("^[0-9a-f]{64}$");
    if(s.size()!=4 || s["format"]!="occt-brep-v3" || !s["bytesBase64"].isString() || s["bytesBase64"].toString().size()>44739244
        || !s["byteCount"].isDouble() || s["byteCount"].toInteger()<1 || s["byteCount"].toInteger()>maximumBytes
        || !sha.match(s["sha256"].toString()).hasMatch())return "Invalid Body Copy snapshot metadata";
    for(const auto* key:{"xMm","yMm","zMm"})if(!p[key].isDouble() || !std::isfinite(p[key].toDouble()) || std::abs(p[key].toDouble())>100000)
        return "Body Copy translation must be finite and within 100000 mm";
    try{bytes(s);}catch(const std::exception& e){return QString::fromUtf8(e.what());}return {};
}
EvaluatedBody BodyCopy::evaluate(const QJsonObject& feature){
    const auto p=feature["parameters"].toObject();const auto error=validate(p);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());
    std::istringstream stream(bytes(p["snapshot"].toObject()).toStdString());BRep_Builder builder;TopoDS_Shape shape;BRepTools::Read(shape,stream,builder);
    if(stream.fail())throw std::invalid_argument("Body Copy snapshot could not be read");requireSolid(shape);
    gp_Trsf transform;transform.SetTranslation(gp_Vec(p["xMm"].toDouble(),p["yMm"].toDouble(),p["zMm"].toDouble()));
    BRepBuilderAPI_Transform placed(shape,transform,false);if(!placed.IsDone())throw std::invalid_argument("Body Copy placement failed");shape=placed.Shape();requireSolid(shape);
    auto summary=GeometryKernel::summarize(shape);summary["operation"]="bodyCopy";summary["snapshotSha256"]=p["snapshot"].toObject()["sha256"];
    // Stable copied face/edge support is a separate capability. Body-targeted
    // solid operations work without inventing source-linked topology identities.
    return {feature["id"].toString(),feature["bodyId"].toString(),feature["name"].toString(),feature["visible"].toBool(),shape,summary,{}, {},feature["ownerId"].toString()};
}
