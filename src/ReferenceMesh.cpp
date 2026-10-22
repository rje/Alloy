// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Graphic3d_Group.hxx>
#include <Prs3d_Presentation.hxx>
#include <Poly_Triangulation.hxx>
#include <Select3D_SensitiveTriangulation.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <SelectMgr_Selection.hxx>

namespace {
// Retained triangle data only: no TopoDS faces, edges or solid promotion.
class MeshPresentation final : public AIS_InteractiveObject {
public:
    explicit MeshPresentation(const QJsonObject& mesh) {
        const auto vertices=mesh.value("verticesMm").toArray(),faces=mesh.value("triangles").toArray();
        triangles_=new Poly_Triangulation(vertices.size(),faces.size(),false);
        for(int i=0;i<vertices.size();++i){const auto v=vertices[i].toArray();triangles_->SetNode(i+1,gp_Pnt(v[0].toDouble(),v[1].toDouble(),v[2].toDouble()));}
        for(int i=0;i<faces.size();++i){const auto t=faces[i].toArray();triangles_->SetTriangle(i+1,Poly_Triangle(t[0].toInt()+1,t[1].toInt()+1,t[2].toInt()+1));}
    }
private:
    Handle(Poly_Triangulation) triangles_;
    void Compute(const Handle(PrsMgr_PresentationManager)&,const Handle(Prs3d_Presentation)& p,const Standard_Integer) override {
        Handle(Graphic3d_ArrayOfTriangles) facets=new Graphic3d_ArrayOfTriangles(triangles_->NbTriangles()*3,0,true);
        for(int i=1;i<=triangles_->NbTriangles();++i){int a,b,c;triangles_->Triangle(i).Get(a,b,c);
            const auto pa=triangles_->Node(a),pb=triangles_->Node(b),pc=triangles_->Node(c);
            const auto cross=gp_Vec(pa,pb).Crossed(gp_Vec(pa,pc));
            const gp_Dir normal=cross.SquareMagnitude()>1e-24 ? gp_Dir(cross) : gp_Dir(0,0,1);
            for(const auto& point:{pa,pb,pc})facets->AddVertex(point,normal);
        }
        Handle(Graphic3d_AspectFillArea3d) aspect=new Graphic3d_AspectFillArea3d;
        aspect->SetInteriorStyle(Aspect_IS_SOLID);aspect->SetInteriorColor(Quantity_Color(.63,.69,.75,Quantity_TOC_sRGB));
        aspect->SetShadingModel(Graphic3d_TOSM_FACET);aspect->SetEdgeOn();
        aspect->SetEdgeColor(Quantity_Color(.20,.26,.31,Quantity_TOC_sRGB));aspect->SetEdgeWidth(1);
        auto group=p->NewGroup();group->SetPrimitivesAspect(aspect);group->AddPrimitiveArray(facets);
    }
    void ComputeSelection(const Handle(SelectMgr_Selection)& s,const Standard_Integer mode) override {
        if(mode!=0)return;
        Handle(SelectMgr_EntityOwner) owner=new SelectMgr_EntityOwner(this,5);
        s->Add(new Select3D_SensitiveTriangulation(owner,triangles_,TopLoc_Location(),true));
    }
};
}
void ReferenceView::setMeshes(const QJsonArray& meshes) {
    if(context_.IsNull())return;
    QSet<QString> live;
    for(const auto& value:meshes){const auto mesh=value.toObject();const auto id=mesh.value("id").toString();live.insert(id);
        // Immutable geometry at a stable UUID; visibility changes reuse GPU/BVH data.
        if(!meshModels_.contains(id))meshModels_[id]=new MeshPresentation(mesh);
        const auto model=meshModels_.value(id);
        if(mesh.value("visible").toBool(true)){context_->Display(model,0,0,false);context_->Activate(model,0,false);}
        else context_->Erase(model,false);
    }
    for(auto it=meshModels_.begin();it!=meshModels_.end();)if(!live.contains(it.key())){context_->Remove(it.value(),false);it=meshModels_.erase(it);}else ++it;
    meshRecords_=meshes;context_->UpdateCurrentViewer();
}
QJsonObject ReferenceView::projectMesh(const QString& id) const {
    if(view_.IsNull())return {};
    for(const auto& value:meshRecords_){const auto mesh=value.toObject();if(mesh.value("id")!=id || !mesh.value("visible").toBool(true))continue;
        const auto b=mesh.value("summary").toObject().value("boundsMm").toObject();const auto lo=b.value("min").toArray(),hi=b.value("max").toArray();
        int x,y;view_->Convert((lo[0].toDouble()+hi[0].toDouble())/2,(lo[1].toDouble()+hi[1].toDouble())/2,(lo[2].toDouble()+hi[2].toDouble())/2,x,y);const auto scale=windowScale();
        return {{"x",x/scale.x()},{"y",y/scale.y()},{"meshId",id},{"bodyId",id},{"viewportLocal",true}};
    }return {};
}
QJsonObject ReferenceView::meshPresentationState() const {
    QJsonArray rows;for(const auto& value:meshRecords_){const auto mesh=value.toObject();const auto model=meshModels_.value(mesh.value("id").toString());
        rows.append(QJsonObject{{"meshId",mesh.value("id")},{"name",mesh.value("name")},{"displayed",!context_.IsNull() && !model.IsNull() && context_->IsDisplayed(model)},{"facets",mesh.value("triangles").toArray().size()},{"brep",false}});
    }return {{"bodies",rows},{"selectionMode","whole mesh"}};
}
