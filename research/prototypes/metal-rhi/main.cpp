// SPDX-License-Identifier: MIT
// Disposable native-Metal investigation; no Alloy document or renderer authority.
#include <QApplication>
#include <QRhiWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCommandLineParser>
#include <rhi/qrhi.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <gp_Lin.hxx>
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <vector>

namespace {
struct Vertex {float x,y,z,r,g,b;};
struct Face {QString id,object;quint32 code;TopoDS_Face shape;std::vector<Vertex> vertices;};
struct Body {QString id;TopoDS_Shape shape;};
struct Hit {quint32 code=0;QString id,object;double distance=0;};
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void writeJson(const QString& path,const QJsonObject& value){QFile f(path);check(f.open(QIODevice::WriteOnly),"Cannot write report");check(f.write(QJsonDocument(value).toJson())>0,"Report write failed");}
QJsonArray point(const QPointF& p){return {p.x(),p.y()};}
QJsonArray sizeArray(const QSize& s){return {s.width(),s.height()};}
QShader shader(const QString& name){QFile f(name);check(f.open(QIODevice::ReadOnly),"Missing pinned shader bundle");auto value=QShader::fromSerialized(f.readAll());check(value.isValid(),"Invalid pinned shader");bool metal=false;for(const auto& key:value.availableShaders())metal=metal || key.source()==QShader::MslShader || key.source()==QShader::MetalLibShader;check(metal,"Pinned shader has no Metal variant");return value;}
Vertex vertex(const gp_Pnt& p,float r,float g,float b){return {float(p.X()),float(p.Y()),float(p.Z()),r,g,b};}
}

class MetalScene final : public QRhiWidget {
public:
    explicit MetalScene(QWidget* parent=nullptr):QRhiWidget(parent){setApi(Api::Metal);setDebugLayerEnabled(true);setSampleCount(4);setColorBufferFormat(TextureFormat::RGBA8);setMinimumSize(320,240);buildFixture();connect(this,&QRhiWidget::frameSubmitted,this,[this]{++frames_;auto submitted=afterSubmission;if(submitted)submitted();if(onFrame)QTimer::singleShot(0,this,onFrame);});connect(this,&QRhiWidget::renderFailed,this,[this]{error_="QRhiWidget render failed";if(onFrame)QTimer::singleShot(0,this,onFrame);});}
    ~MetalScene() override{releaseResources();}
    std::function<void()> onFrame,afterSubmission;
    bool hasSubmittedReadback() const{return bool(readback_);}
    bool nativeReadbackComplete() const{return readback_ && !readback_->data.isEmpty();}
    void releaseForLifetimeProbe(){releaseResources();update();}
    QString error_;
    bool ready() const{return bool(colorPipeline_);}
    bool busy() const{return pending_ || bool(readback_);}
    void recreate(){check(!busy(),"Cannot recreate during an asynchronous pick");releaseResources();update();}
    QJsonArray faceManifest() const{QJsonArray result;for(const auto& f:faces_)result.append(QJsonObject{{"id",f.id},{"objectId",f.object},{"code",int(f.code)},{"triangles",int(f.vertices.size()/3)}});return result;}
    QJsonObject inspect() const {
        QJsonArray bodies;for(const auto& b:bodies_){GProp_GProps properties;BRepGProp::VolumeProperties(b.shape,properties);bodies.append(QJsonObject{{"id",b.id},{"valid",BRepCheck_Analyzer(b.shape).IsValid()},{"volumeMm3",properties.Mass()}});}
        QJsonObject value{{"qt",QT_VERSION_STR},{"occt",ALLOY_OCCT_VERSION},{"logicalSize",sizeArray(this->size())},{"devicePixelRatio",devicePixelRatioF()},
            {"pixelSize",sizeArray(pixelSize_)},{"samples",sampleCount()},{"idSamples",1},{"idFormat","linear_RGBA8_single_sample"},{"integerIdAttachmentTested",false},
            {"frames",frames_},{"resourceCreations",creations_},{"resourceReleases",releases_},{"pendingReadback",busy()},{"canceledReadbacks",canceledReadbacks_},
            {"selectedFaceId",semantic(selected_)},{"faces",faceManifest()},{"edges",edgeManifest_},{"bodies",bodies},{"error",error_}};
        if(rhi()) {const auto info=rhi()->driverInfo();value["backend"]=rhi()->backendName();value["deviceName"]=QString::fromUtf8(info.deviceName);value["vendorId"]=QString::number(info.vendorId);value["deviceId"]=QString::number(info.deviceId);value["deviceType"]=int(info.deviceType);value["framebufferYUp"]=rhi()->isYUpInFramebuffer();value["ndcYUp"]=rhi()->isYUpInNDC();value["rgba8Supported"]=rhi()->isTextureFormatSupported(QRhiTexture::RGBA8,QRhiTexture::RenderTarget|QRhiTexture::UsedAsTransferSource);value["readbackAnyFormatCapability"]=rhi()->isFeatureSupported(QRhi::ReadBackAnyTextureFormat);value["maximumAsyncReadbackFrames"]=rhi()->resourceLimit(QRhi::MaxAsyncReadbackFrames);}
        return value;
    }
    QPointF project(const QVector3D& world) const{const auto v=camera_*QVector4D(world,1);return {(v.x()/v.w()+1)*width()/2,(1-v.y()/v.w())*height()/2};}
    std::vector<Hit> rayHits(const QPointF& p) const {
        const auto inverse=camera_.inverted();const float x=float(p.x()/width()*2-1),y=float(1-p.y()/height()*2);
        auto a=inverse*QVector4D(x,y,-1,1),b=inverse*QVector4D(x,y,1,1);a/=a.w();b/=b.w();const gp_Pnt start(a.x(),a.y(),a.z());const gp_Dir direction(b.x()-a.x(),b.y()-a.y(),b.z()-a.z());std::vector<Hit> hits;
        for(const auto& body:bodies_){IntCurvesFace_ShapeIntersector intersector;intersector.Load(body.shape,1e-7);intersector.Perform(gp_Lin(start,direction),0,1000);for(int i=1;i<=intersector.NbPnt();++i)for(const auto& f:faces_)if(f.shape.IsSame(intersector.Face(i)))hits.push_back({f.code,f.id,f.object,intersector.WParameter(i)});}
        std::sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.distance<b.distance;});return hits;
    }
    QPointF occlusionPoint() const{for(int y=height()/4;y<height()*3/4;y+=8)for(int x=width()/4;x<width()*3/4;x+=8){const auto hits=rayHits({double(x),double(y)});std::set<QString> objects;for(const auto& h:hits)objects.insert(h.object);if(objects.size()==2 && hits.front().object=="cylinder")return {double(x),double(y)};}throw std::runtime_error("No two-body depth-occlusion sample found");}
    void pick(const QPointF& logical,std::function<void(QJsonObject)> completed){check(ready() && !busy(),"Pick requires initialized idle scene");++pickGeneration_;requested_=logical;pickCompleted_=std::move(completed);pending_=true;pickTime_.start();update();}
    bool capture(const QString& path){const auto image=grabFramebuffer();return !image.isNull() && image.save(path);}
protected:
    void mousePressEvent(QMouseEvent* e) override{if(e->button()==Qt::LeftButton && ready() && !busy())pick(e->position(),[](const auto&){});}
    void initialize(QRhiCommandBuffer* cb) override {
        try {
            if(colorPipeline_ && rhi_==rhi() && pixelSize_==renderTarget()->pixelSize())return;
            check(!busy(),"Resize during pending readback requires completion before recreation");releaseResources();rhi_=rhi();pixelSize_=renderTarget()->pixelSize();check(rhi_ && rhi_->backend()==QRhi::Metal,"Metal backend required; fallback rejected");
            colorBuffer_.reset(rhi_->newBuffer(QRhiBuffer::Immutable,QRhiBuffer::VertexBuffer,quint32(color_.size()*sizeof(Vertex))));check(colorBuffer_->create(),"Color vertex buffer failed");
            idBuffer_.reset(rhi_->newBuffer(QRhiBuffer::Immutable,QRhiBuffer::VertexBuffer,quint32(ids_.size()*sizeof(Vertex))));check(idBuffer_->create(),"ID vertex buffer failed");
            edgeBuffer_.reset(rhi_->newBuffer(QRhiBuffer::Immutable,QRhiBuffer::VertexBuffer,quint32(edges_.size()*sizeof(Vertex))));check(edgeBuffer_->create(),"Edge buffer failed");
            selectedBuffer_.reset(rhi_->newBuffer(QRhiBuffer::Dynamic,QRhiBuffer::VertexBuffer,quint32(color_.size()*sizeof(Vertex))));check(selectedBuffer_->create(),"Selection buffer failed");
            uniform_.reset(rhi_->newBuffer(QRhiBuffer::Dynamic,QRhiBuffer::UniformBuffer,64));check(uniform_->create(),"Uniform buffer failed");
            bindings_.reset(rhi_->newShaderResourceBindings());bindings_->setBindings({QRhiShaderResourceBinding::uniformBuffer(0,QRhiShaderResourceBinding::VertexStage,uniform_.get())});check(bindings_->create(),"Shader bindings failed");
            idTexture_.reset(rhi_->newTexture(QRhiTexture::RGBA8,pixelSize_,1,QRhiTexture::RenderTarget|QRhiTexture::UsedAsTransferSource));check(idTexture_->create(),"Linear RGBA8 ID texture failed");
            idDepth_.reset(rhi_->newRenderBuffer(QRhiRenderBuffer::DepthStencil,pixelSize_,1));check(idDepth_->create(),"ID depth buffer failed");
            QRhiTextureRenderTargetDescription description(QRhiColorAttachment(idTexture_.get()));description.setDepthStencilBuffer(idDepth_.get());idTarget_.reset(rhi_->newTextureRenderTarget(description));idPass_.reset(idTarget_->newCompatibleRenderPassDescriptor());idTarget_->setRenderPassDescriptor(idPass_.get());check(idTarget_->create(),"ID target failed");
            colorPipeline_=pipeline(renderTarget()->renderPassDescriptor(),QRhiGraphicsPipeline::Triangles,true,true);
            edgePipeline_=pipeline(renderTarget()->renderPassDescriptor(),QRhiGraphicsPipeline::Lines,true,false);
            selectionPipeline_=pipeline(renderTarget()->renderPassDescriptor(),QRhiGraphicsPipeline::Triangles,true,false);
            idPipeline_=pipeline(idPass_.get(),QRhiGraphicsPipeline::Triangles,true,true);
            auto* uploads=rhi_->nextResourceUpdateBatch();uploads->uploadStaticBuffer(colorBuffer_.get(),color_.data());uploads->uploadStaticBuffer(idBuffer_.get(),ids_.data());uploads->uploadStaticBuffer(edgeBuffer_.get(),edges_.data());cb->resourceUpdate(uploads);++creations_;
        }catch(const std::exception& e){error_=e.what();}
    }
    void render(QRhiCommandBuffer* cb) override {
        try {
            if(!colorPipeline_)initialize(cb);if(!error_.isEmpty() || !colorPipeline_)return;
            QMatrix4x4 projection;projection.ortho(-35,35,-35*float(height())/width(),35*float(height())/width(),0.1f,250.0f);
            QMatrix4x4 view;view.lookAt({48,-65,55},{-3,0,9},{0,0,1});camera_=projection*view;const auto matrix=rhi_->clipSpaceCorrMatrix()*camera_;
            auto* updates=rhi_->nextResourceUpdateBatch();updates->updateDynamicBuffer(uniform_.get(),0,64,matrix.constData());
            int selectedCount=0;if(selected_)for(const auto& f:faces_)if(f.code==selected_){auto vertices=f.vertices;for(auto& v:vertices){v.r=.10f;v.g=.43f;v.b=.85f;}selectedCount=int(vertices.size());updates->updateDynamicBuffer(selectedBuffer_.get(),0,quint32(vertices.size()*sizeof(Vertex)),vertices.data());}
            cb->beginPass(renderTarget(),QColor::fromRgbF(.96,.97,.98),{1,0},updates);draw(cb,colorPipeline_.get(),colorBuffer_.get(),int(color_.size()));if(selectedCount)draw(cb,selectionPipeline_.get(),selectedBuffer_.get(),selectedCount);draw(cb,edgePipeline_.get(),edgeBuffer_.get(),int(edges_.size()));cb->endPass();
            if(pending_) {
                pending_=false;cb->beginPass(idTarget_.get(),Qt::black,{1,0});draw(cb,idPipeline_.get(),idBuffer_.get(),int(ids_.size()));cb->endPass();
                readback_=std::make_unique<QRhiReadbackResult>();readback_->completed=[this]{
                    const auto dpr=devicePixelRatioF();const int x=std::clamp(int(requested_.x()*dpr),0,pixelSize_.width()-1);int y=std::clamp(int(requested_.y()*dpr),0,pixelSize_.height()-1);if(rhi_->isYUpInFramebuffer())y=pixelSize_.height()-1-y;
                    const int offset=(y*pixelSize_.width()+x)*4;quint32 id=0;if(offset>=0 && offset+3<readback_->data.size()){const auto* p=reinterpret_cast<const unsigned char*>(readback_->data.constData()+offset);id=p[0]|(quint32(p[1])<<8)|(quint32(p[2])<<16);}const auto latency=pickTime_.nsecsElapsed()/1e6;const auto hits=rayHits(requested_);QJsonArray objects;for(const auto& h:hits)if(!objects.contains(h.object))objects.append(h.object);
                    auto result=QJsonObject{{"logicalPoint",point(requested_)},{"physicalReadPoint",QJsonArray{x,y}},{"code",int(id)},{"faceId",semantic(id)},{"expectedCode",hits.empty() ? 0 : int(hits.front().code)},{"expectedFaceId",hits.empty() ? QString{} : hits.front().id},{"rayObjectIds",objects},{"latencyMs",latency},{"readbackBytes",readback_->data.size()},{"pixelSize",sizeArray(readback_->pixelSize)}};
                    const auto generation=pickGeneration_;
                    QTimer::singleShot(0,this,[this,id,result,generation]{if(generation!=pickGeneration_)return;auto completed=std::move(pickCompleted_);readback_.reset();selected_=id;update();if(completed)completed(result);});
                };
                auto* read=rhi_->nextResourceUpdateBatch();read->readBackTexture(QRhiReadbackDescription(idTexture_.get()),readback_.get());cb->resourceUpdate(read);
            }
            if(readback_)update();
        }catch(const std::exception& e){error_=e.what();}
    }
    void releaseResources() override {
        // The result storage must outlive GPU completion, even if Qt requests
        // early release during a resize/reparent/teardown. Stop callbacks first,
        // drain the old device, then report cancellation on a live widget.
        if(pending_ || readback_) {
            ++pickGeneration_;
            if(readback_) {readback_->completed=nullptr;if(rhi_)rhi_->finish();readback_.reset();}
            pending_=false;++canceledReadbacks_;auto completed=std::move(pickCompleted_);
            if(completed)QTimer::singleShot(0,this,[completed]{completed(QJsonObject{{"ok",false},{"error","readback_canceled_for_resource_release"}});});
        }
        if(colorPipeline_)++releases_;colorPipeline_.reset();edgePipeline_.reset();selectionPipeline_.reset();idPipeline_.reset();idTarget_.reset();idPass_.reset();idDepth_.reset();idTexture_.reset();bindings_.reset();uniform_.reset();colorBuffer_.reset();idBuffer_.reset();edgeBuffer_.reset();selectedBuffer_.reset();rhi_=nullptr;
    }
private:
    QString semantic(quint32 code) const{for(const auto& f:faces_)if(f.code==code)return f.id;return {};}
    std::unique_ptr<QRhiGraphicsPipeline> pipeline(QRhiRenderPassDescriptor* pass,QRhiGraphicsPipeline::Topology topology,bool depth,bool write) {
        auto value=std::unique_ptr<QRhiGraphicsPipeline>(rhi_->newGraphicsPipeline());value->setShaderStages({{QRhiShaderStage::Vertex,shader(":/metal-rhi/color.vert.qsb")},{QRhiShaderStage::Fragment,shader(":/metal-rhi/color.frag.qsb")}});
        QRhiVertexInputLayout layout;layout.setBindings({{sizeof(Vertex)}});layout.setAttributes({{0,0,QRhiVertexInputAttribute::Float3,0},{0,1,QRhiVertexInputAttribute::Float3,3*sizeof(float)}});value->setVertexInputLayout(layout);value->setShaderResourceBindings(bindings_.get());value->setRenderPassDescriptor(pass);value->setTopology(topology);value->setSampleCount(pass==idPass_.get() ? 1 : renderTarget()->sampleCount());if(write && pass!=idPass_.get()){value->setDepthBias(2);value->setSlopeScaledDepthBias(1.0f);}value->setDepthTest(depth);value->setDepthWrite(write);value->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);value->setCullMode(QRhiGraphicsPipeline::None);check(value->create(),"GPU pipeline creation failed");return value;
    }
    void draw(QRhiCommandBuffer* cb,QRhiGraphicsPipeline* pipeline,QRhiBuffer* buffer,int count){if(!count)return;cb->setGraphicsPipeline(pipeline);cb->setViewport({0,0,float(pixelSize_.width()),float(pixelSize_.height())});cb->setShaderResources(bindings_.get());const QRhiCommandBuffer::VertexInput input(buffer,0);cb->setVertexInput(0,1,&input);cb->draw(count);}
    void buildFixture() {
        bodies_={{"box",BRepPrimAPI_MakeBox(gp_Pnt(-18,-8,0),20,16,16).Shape()},{"cylinder",BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(7,2,0),gp_Dir(0,0,1)),7,20).Shape()}};std::reverse(bodies_.begin(),bodies_.end());quint32 code=1;
        for(auto& b:bodies_){BRepMesh_IncrementalMesh mesh(b.shape,.08,false,.15,false);check(mesh.IsDone(),"OCCT triangulation failed");int faceIndex=0;
            for(TopExp_Explorer it(b.shape,TopAbs_FACE);it.More();it.Next()){const auto face=TopoDS::Face(it.Current());Face f{b.id+"/face/"+QString::number(faceIndex++),b.id,code++,face,{}};TopLoc_Location location;const auto triangles=BRep_Tool::Triangulation(face,location);check(!triangles.IsNull(),"Face triangulation missing");
                for(int i=1;i<=triangles->NbTriangles();++i){int a,c,d;triangles->Triangle(i).Get(a,c,d);if(face.Orientation()==TopAbs_REVERSED)std::swap(c,d);const auto p=triangles->Node(a).Transformed(location.Transformation()),q=triangles->Node(c).Transformed(location.Transformation()),r=triangles->Node(d).Transformed(location.Transformation());auto normal=gp_Vec(p,q).Crossed(gp_Vec(p,r));if(normal.SquareMagnitude()==0)continue;normal.Normalize();const float shade=float(.64+.22*std::max(0.0,normal.Dot(gp_Vec(.2,-.5,.84))));for(const auto& node:{p,q,r})f.vertices.push_back(vertex(node,shade,shade+.025f,shade+.04f));}
                color_.insert(color_.end(),f.vertices.begin(),f.vertices.end());for(auto v:f.vertices){v.r=float(f.code&255)/255;v.g=float((f.code>>8)&255)/255;v.b=float((f.code>>16)&255)/255;ids_.push_back(v);}faces_.push_back(std::move(f));}
            TopTools_IndexedMapOfShape edgeMap;TopExp::MapShapes(b.shape,TopAbs_EDGE,edgeMap);
            for(int i=1;i<=edgeMap.Extent();++i){const auto edge=TopoDS::Edge(edgeMap(i));BRepAdaptor_Curve curve(edge);GCPnts_QuasiUniformDeflection points(curve,.04);if(!points.IsDone())continue;edgeManifest_.append(QJsonObject{{"id",b.id+"/edge/"+QString::number(i-1)},{"objectId",b.id},{"segments",points.NbPoints()-1}});for(int j=2;j<=points.NbPoints();++j){edges_.push_back(vertex(points.Value(j-1),.15f,.20f,.24f));edges_.push_back(vertex(points.Value(j),.15f,.20f,.24f));}}
        }
        for(int i=-30;i<=30;i+=5){edges_.push_back(vertex(gp_Pnt(i,-30,-.05),.80f,.83f,.86f));edges_.push_back(vertex(gp_Pnt(i,30,-.05),.80f,.83f,.86f));edges_.push_back(vertex(gp_Pnt(-30,i,-.05),.80f,.83f,.86f));edges_.push_back(vertex(gp_Pnt(30,i,-.05),.80f,.83f,.86f));}
    }
    std::vector<Body> bodies_;std::vector<Face> faces_;std::vector<Vertex> color_,ids_,edges_;QJsonArray edgeManifest_;QMatrix4x4 camera_;QSize pixelSize_;QRhi* rhi_=nullptr;
    std::unique_ptr<QRhiBuffer> colorBuffer_,idBuffer_,edgeBuffer_,selectedBuffer_,uniform_;std::unique_ptr<QRhiShaderResourceBindings> bindings_;
    std::unique_ptr<QRhiGraphicsPipeline> colorPipeline_,edgePipeline_,selectionPipeline_,idPipeline_;std::unique_ptr<QRhiTexture> idTexture_;std::unique_ptr<QRhiRenderBuffer> idDepth_;std::unique_ptr<QRhiTextureRenderTarget> idTarget_;std::unique_ptr<QRhiRenderPassDescriptor> idPass_;
    std::unique_ptr<QRhiReadbackResult> readback_;bool pending_=false;QPointF requested_;QElapsedTimer pickTime_;std::function<void(QJsonObject)> pickCompleted_;quint32 selected_=0;quint64 pickGeneration_=0;int frames_=0,creations_=0,releases_=0,canceledReadbacks_=0;
};

int main(int argc,char** argv) {
    QApplication app(argc,argv);QCoreApplication::setApplicationName("Alloy Metal Spike");
    QCommandLineParser parser;parser.addHelpOption();parser.addOption({"output","Existing or new local evidence directory.","directory"});parser.addOption({"self-test","Run the bounded fixture sequence and exit."});parser.process(app);
    const auto output=QDir(parser.value("output").isEmpty() ? QDir::tempPath()+"/alloy-metal-spike" : parser.value("output"));check(QDir().mkpath(output.absolutePath()),"Cannot create evidence directory");
    QJsonObject report{{"purpose","isolated_Metal_investigation"},{"rendererAdopted",false},{"multiDisplayTransitionTested",false},{"integerPickingTested",false}};QJsonArray snapshots,picks;
    auto window=std::make_unique<QMainWindow>();window->setWindowTitle("Alloy · Metal investigation");auto* central=new QWidget;auto* layout=new QVBoxLayout(central);auto* controls=new QHBoxLayout;auto* status=new QLabel("Native Metal · OCCT box + cylinder");controls->addWidget(status,1);auto* capture=new QPushButton("Capture");auto* recreate=new QPushButton("Recreate GPU resources");auto* inspect=new QPushButton("Inspect");controls->addWidget(capture);controls->addWidget(recreate);controls->addWidget(inspect);layout->addLayout(controls);
    auto* scene=new MetalScene;layout->addWidget(scene,1);window->setCentralWidget(central);window->resize(900,640);
    QObject::connect(capture,&QPushButton::clicked,scene,[&]{scene->capture(output.filePath("manual.png"));});QObject::connect(recreate,&QPushButton::clicked,scene,[&]{if(!scene->busy())scene->recreate();});QObject::connect(inspect,&QPushButton::clicked,scene,[&]{writeJson(output.filePath("state.json"),scene->inspect());});
    int stage=0;bool waiting=false;int oldCreations=0;QElapsedTimer deadline;deadline.start();QTimer timer;
    auto fail=[&](const QString& error){report["passed"]=false;report["error"]=error;report["finalState"]=scene->inspect();report["snapshots"]=snapshots;report["picks"]=picks;writeJson(output.filePath("report.json"),report);app.exit(1);};
    auto snapshot=[&](const QString& name){check(scene->capture(output.filePath(name+".png")),"Framebuffer capture failed");auto value=scene->inspect();value["name"]=name;snapshots.append(value);writeJson(output.filePath("state.json"),value);};
    auto pick=[&](const QPointF& p,const QString& purpose){waiting=true;scene->pick(p,[&,purpose](QJsonObject result){result["purpose"]=purpose;picks.append(result);if(result.contains("error")){fail(result["error"].toString());return;}if(result["code"]!=result["expectedCode"]){fail("GPU pick disagrees with independent OCCT ray: "+purpose);return;}if(purpose=="background" && result["code"].toInt()!=0){fail("Background ID must be zero");return;}if(purpose=="depth_occlusion" && result["rayObjectIds"].toArray().size()!=2){fail("Depth sample did not intersect both bodies");return;}waiting=false;++stage;});};
    if(parser.isSet("self-test")) {
        QObject::connect(&timer,&QTimer::timeout,scene,[&]{try {
            if(deadline.elapsed()>30000){fail("Bounded fixture timed out");return;}
            if(!scene->error_.isEmpty()){fail(scene->error_);return;}if(waiting || !scene->ready() || scene->busy())return;
            switch(stage) {
            case 0: {const auto state=scene->inspect();check(state["backend"]=="Metal","Actual backend is not Metal");snapshot("initial");++stage;break;}
            case 1: pick(scene->project({-8,0,16}),"box_top");break;
            case 2: snapshot("selected-box");++stage;break;
            case 3: pick(scene->project({7,2,20}),"cylinder_top");break;
            case 4: pick({5,5},"background");break;
            case 5: pick(scene->occlusionPoint(),"depth_occlusion");break;
            case 6: window->resize(640,480);++stage;break;
            case 7: {const auto expected=QSize(qRound(scene->width()*scene->devicePixelRatioF()),qRound(scene->height()*scene->devicePixelRatioF()));if(scene->inspect()["pixelSize"].toArray()!=sizeArray(expected))return;snapshot("narrow");window->resize(1100,720);++stage;break;}
            case 8: {const auto expected=QSize(qRound(scene->width()*scene->devicePixelRatioF()),qRound(scene->height()*scene->devicePixelRatioF()));if(scene->inspect()["pixelSize"].toArray()!=sizeArray(expected))return;snapshot("wide");oldCreations=scene->inspect()["resourceCreations"].toInt();scene->recreate();++stage;break;}
            case 9: if(scene->inspect()["resourceCreations"].toInt()<=oldCreations)return;pick(scene->project({-8,0,16}),"after_resource_recreation");break;
            case 10:
                snapshot("recreated-selected");waiting=true;
                scene->afterSubmission=[&]{if(!scene->hasSubmittedReadback())return;scene->afterSubmission={};report["pendingReadbackRelease"]=QJsonObject{{"submittedResultPresent",true},{"nativeResultAlreadyComplete",scene->nativeReadbackComplete()}};scene->releaseForLifetimeProbe();};
                scene->pick(scene->project({-8,0,16}),[&](QJsonObject result){if(result["error"]!="readback_canceled_for_resource_release"){fail("Submitted-readback release did not report cancellation");return;}report["cancellation"]=result;waiting=false;++stage;});break;
            case 11: pick(scene->project({7,2,20}),"after_pending_release");break;
            case 12: snapshot("after-pending-release");report["passed"]=true;report["snapshots"]=snapshots;report["picks"]=picks;report["finalState"]=scene->inspect();timer.stop();window->close();app.quit();break;
            }
        }catch(const std::exception& e){fail(e.what());}});timer.start(100);
    }
    window->show();const auto exitCode=app.exec();window.reset();report["applicationExitCode"]=exitCode;report["cleanWindowTeardown"]=true;if(parser.isSet("self-test"))writeJson(output.filePath("report.json"),report);return exitCode;
}
