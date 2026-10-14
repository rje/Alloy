// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include "ReferenceCanvas.h"
#include <AIS_TexturedShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Image_PixMap.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax3.hxx>
#include <cmath>
#include <numbers>
#include <cstring>
void ReferenceView::setCanvases(const QJsonArray& rows,const EvaluatedDocument& evaluation){
    if(context_.IsNull())return;
    QJsonArray states;for(const auto& value:rows)states.append(ReferenceCanvas::inspect(value.toObject(),evaluation));
    QJsonObject stamp{{"rows",rows},{"states",states}};if(stamp==canvasStamp_)return;canvasStamp_=stamp;
    for(const auto& model:canvasModels_)context_->Remove(model,false);canvasModels_.clear();canvasState_=states;
    for(int i=0;i<rows.size();++i){const auto row=rows[i].toObject(),state=states[i].toObject();if(!row["visible"].toBool() || state["availability"]!="available" || row["opacity"].toDouble()==0)continue;
        const auto f=state["frame"].toObject();const auto o=f["origin"].toArray(),u=f["uAxis"].toArray(),v=f["vAxis"].toArray();
        gp_Pnt origin(o[0].toDouble(),o[1].toDouble(),o[2].toDouble());gp_Vec ua(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()),va(v[0].toDouble(),v[1].toDouble(),v[2].toDouble());
        origin.Translate(ua*row["centerUMm"].toDouble()+va*row["centerVMm"].toDouble());const auto angle=row["angleDegrees"].toDouble()*std::numbers::pi/180;
        const auto x=ua*std::cos(angle)+va*std::sin(angle);const auto width=row["widthMm"].toDouble(),height=state["heightMm"].toDouble();
        const gp_Pln plane(gp_Ax3(origin,gp_Dir(ua.Crossed(va)),gp_Dir(x)));auto face=BRepBuilderAPI_MakeFace(plane,-width/2,width/2,-height/2,height/2).Face();
        Handle(AIS_TexturedShape) model=new AIS_TexturedShape(face);const auto pixels=ReferenceCanvas::image(row).convertToFormat(QImage::Format_RGBA8888);
        Handle(Image_PixMap) texture=new Image_PixMap;texture->InitTrash(Image_Format_RGBA,pixels.width(),pixels.height());texture->SetTopDown(true);
        for(int y=0;y<pixels.height();++y)std::memcpy(texture->ChangeRow(y),pixels.constScanLine(y),pixels.width()*4);
        model->SetTexturePixMap(texture);model->SetTextureMapOn();model->SetTextureRepeat(false);model->DisableTextureModulate();model->SetColor(Quantity_NOC_WHITE);model->SetTransparency(1-row["opacity"].toDouble());
        context_->Display(model,3,-1,false);context_->Deactivate(model);canvasModels_[row["id"].toString()]=model;
    }
    view_->Redraw();
}
