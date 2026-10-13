// SPDX-License-Identifier: MIT
#include "CircularPattern.h"
#include "SolidTransform.h"
#include <QUuid>
#include <gp_Ax1.hxx>
#include <numbers>
QString CircularPattern::bodyId(const QString& featureId,const QString& sourceBodyId,int index){
 return index==0?sourceBodyId:QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-circular-pattern/1/")+QByteArray::number(index)+"/body/"+sourceBodyId.toUtf8()).toString(QUuid::WithoutBraces);
}
std::vector<EvaluatedBody> CircularPattern::evaluate(const QJsonObject& feature,const EvaluatedBody& source){
 const auto p=feature["parameters"].toObject();const int quantity=p["quantity"].toInt();std::vector<EvaluatedBody> result;
 for(int index=0;index<quantity;++index){const double angle=2*std::numbers::pi*index/quantity;gp_Trsf rotation;rotation.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(0,0,1)),angle);
  auto body=index==0?source:SolidTransform::copy(source,rotation,feature["id"].toString(),bodyId(feature["id"].toString(),source.bodyId,index),QString("%1 %2").arg(feature["name"].toString()).arg(index+1),feature["visible"].toBool(),QByteArray("alloy-circular-pattern/1/")+QByteArray::number(index)+'/',"circularPattern","patternedSourceEdgeId");
  body.summary["circularPattern"]=QJsonObject{{"featureId",feature["id"]},{"sourceBodyId",source.bodyId},{"index",index},{"quantity",quantity},{"angleDeg",angle*180/std::numbers::pi},{"axis",p["axis"]},{"extent","full"}};result.push_back(body);
 }
 return result;
}
