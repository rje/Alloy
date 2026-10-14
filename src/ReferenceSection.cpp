#include "ReferenceView.h"
#include <Graphic3d_Texture2Dplane.hxx>
#include <Image_PixMap.hxx>
#include <gp_Pln.hxx>

void ReferenceView::setSectionAnalysis(const QJsonObject& analysis) {
    if(sectionAnalysis_==analysis)return;
    for(auto& model:models_)if(!sectionClip_.IsNull())model->RemoveClipPlane(sectionClip_);
    sectionAnalysis_=analysis;sectionClip_.Nullify();
    if(!analysis.isEmpty() && analysis["visible"].toBool()) {
        const auto plane=analysis["plane"].toString();
        // Positive world half-space is retained, independent of camera direction.
        gp_Pnt origin(0,0,0);
        gp_Dir normal=plane=="XY" ? gp_Dir(0,0,1) : plane=="XZ" ? gp_Dir(0,1,0) : gp_Dir(1,0,0);
        if(plane=="face") {
            const auto resolved=analysis["resolvedPlane"].toObject();
            const auto p=resolved["origin"].toArray(),n=resolved["normal"].toArray();
            if(p.size()!=3 || n.size()!=3)return;
            origin=gp_Pnt(p[0].toDouble(),p[1].toDouble(),p[2].toDouble());
            normal=gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
        }
        sectionClip_=new Graphic3d_ClipPlane(gp_Pln(origin,normal));
        sectionClip_->SetCapping(true);sectionClip_->SetUseObjectMaterial(true);sectionClip_->SetCappingHatchOff();
        if(analysis["hatch"].toBool()) {
            Handle(Image_PixMap) pixels=new Image_PixMap;pixels->InitZero(Image_Format_RGB,32,32);
            for(int y=0;y<32;++y)for(int x=0;x<32;++x){const double c=(x+y)%32<3 ? .12 : 1.;pixels->SetPixelColor(x,y,Quantity_Color(c,c,c,Quantity_TOC_RGB));}
            Handle(Graphic3d_Texture2Dplane) texture=new Graphic3d_Texture2Dplane(pixels);
            texture->EnableRepeat();texture->EnableModulate();
            if(plane=="face") {
                const auto frame=analysis["resolvedFrame"].toObject();const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
                if(u.size()==3 && v.size()==3){texture->SetPlaneS(.2*u[0].toDouble(),.2*u[1].toDouble(),.2*u[2].toDouble(),0);
                    texture->SetPlaneT(.2*v[0].toDouble(),.2*v[1].toDouble(),.2*v[2].toDouble(),0);}
            } else {if(plane=="YZ")texture->SetPlaneS(0,.2,0,0);else texture->SetPlaneS(.2,0,0,0);
                if(plane=="XY")texture->SetPlaneT(0,.2,0,0);else texture->SetPlaneT(0,0,.2,0);}
            sectionClip_->SetCappingTexture(texture);
        }
        for(auto& model:models_)model->AddClipPlane(sectionClip_);
    }
    if(!view_.IsNull())view_->Redraw();
}
