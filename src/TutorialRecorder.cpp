#include "TutorialRecorder.h"
#include <QApplication>
#include <QAbstractSpinBox>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextEdit>
#include <QWheelEvent>
#include <QWidget>
#include <algorithm>

namespace {
QJsonObject fail(const QString& code, const QString& message) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
QString keyName(int key) {
    const auto label=QKeySequence(key).toString(QKeySequence::NativeText);
    return label.isEmpty() ? QString::number(key) : label;
}
bool belongsTo(QWidget* widget, QWidget* root) {
    for(auto* current=widget;current;current=current->parentWidget())if(current==root)return true;
    return false;
}
}

TutorialRecorder::TutorialRecorder(QWidget* window,std::function<QImage()> capture)
    : QObject(window),window_(window),capture_(std::move(capture)) {
    timer_.setTimerType(Qt::PreciseTimer);
    // A GPU readback can take longer than the requested frame interval. A
    // repeating timer then stays overdue and starves native accessibility/input
    // between captures. Always give the UI a fresh interval after each capture;
    // the existing slot accounting preserves wall-clock timing with repeats.
    timer_.setSingleShot(true);
    connect(&timer_,&QTimer::timeout,this,[this]{
        frame();
        if(active_&&failure_.isEmpty())timer_.start(std::max(1,1000/fps_));
    });
}
TutorialRecorder::~TutorialRecorder() {
    timer_.stop();
    if(active_) {
        qApp->removeEventFilter(this);
        encoder_.closeWriteChannel();
        if(!encoder_.waitForFinished(3000))encoder_.kill();
        log_.close();
    }
}
void TutorialRecorder::writeEvent(QJsonObject event) {
    if(!log_.isOpen())return;
    event.insert("elapsedMs",elapsed_.isValid()?elapsed_.elapsed():0);
    log_.write(QJsonDocument(event).toJson(QJsonDocument::Compact)+'\n');
    log_.flush();
}
QJsonObject TutorialRecorder::start(const QString& path,int fps,qint64 documentRevision) {
    if(active_)return fail("recording_active","A recording is already active");
    if(!QDir::isAbsolutePath(path)||!path.endsWith(".mp4",Qt::CaseInsensitive)||!QFileInfo(path).dir().exists())
        return fail("invalid_arguments","Recording requires an absolute .mp4 path in an existing directory");
    if(fps<1||fps>30)return fail("invalid_arguments","FPS must be between 1 and 30");
    const auto ffmpeg=QStandardPaths::findExecutable("ffmpeg");
    if(ffmpeg.isEmpty())return fail("encoder_unavailable","ffmpeg is not on PATH");
    auto image=capture_();
    if(image.isNull())return fail("capture_failed","OCCT framebuffer capture failed before recording started");
    auto size=image.size();size.scale(QSize(1280,800),Qt::KeepAspectRatio);
    frameWidth_=std::max(2,size.width()&~1);contentHeight_=std::max(2,size.height()&~1);
    frameHeight_=contentHeight_+102;
    if(QFileInfo::exists(path))return fail("destination_exists","Recording destination already exists");
    path_=path;logPath_=path+".jsonl";metadataPath_=path+".json";fps_=fps;
    if(QFileInfo::exists(logPath_)||QFileInfo::exists(metadataPath_))return fail("destination_exists","Recording sidecar already exists");
    log_.setFileName(logPath_);
    if(!log_.open(QIODevice::WriteOnly|QIODevice::NewOnly))return fail("recording_failed","Could not create event log");
    encoder_.setProgram(ffmpeg);
    encoder_.setArguments({"-hide_banner","-loglevel","error","-nostdin","-y",
        "-f","rawvideo","-pixel_format","rgba","-video_size",QString::number(frameWidth_)+"x"+QString::number(frameHeight_),
        "-framerate",QString::number(fps_),"-i","pipe:0","-an","-c:v","libx264","-preset","ultrafast",
        "-crf","23","-pix_fmt","yuv420p","-movflags","+faststart",path_});
    encoder_.setProcessChannelMode(QProcess::SeparateChannels);
    encoder_.start();
    if(!encoder_.waitForStarted(5000)) {log_.close();QFile::remove(logPath_);return fail("encoder_unavailable","Could not start ffmpeg");}
    elapsed_.start();frames_=drops_=delayedTicks_=captureFailures_=0;lastSlot_=-1;
    heldKeys_.clear();caption_.clear();lastCommand_.clear();lastOrigin_.clear();lastFrame_.clear();failure_.clear();
    recentKey_.clear();recentKeyAt_=-10000;lastMotionAt_=-10000;observedCursor_=false;
    active_=true;qApp->installEventFilter(this);
    writeEvent({{"type","start"},{"path",path_},{"fps",fps_},{"frameWidth",frameWidth_},{"frameHeight",frameHeight_},
        {"contentHeight",contentHeight_},{"contentTop",38},
        {"source","Alloy Qt chrome + OCCT OpenGL framebuffer"},{"documentRevision",documentRevision},
        {"startedUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
    frame();
    if(!failure_.isEmpty())return stop(documentRevision);
    timer_.start(std::max(1,1000/fps_));
    return receipt();
}
QJsonObject TutorialRecorder::step(const QString& text) {
    if(!active_)return fail("recording_inactive","No recording is active");
    if(text.trimmed().isEmpty()||text.size()>200)return fail("invalid_arguments","Step text must contain 1–200 characters");
    caption_=text.trimmed();writeEvent({{"type","step"},{"text",caption_}});
    return receipt();
}
QJsonObject TutorialRecorder::receipt() const {
    return {{"active",active_},{"path",path_},{"logPath",logPath_},{"metadataPath",metadataPath_},
        {"fps",fps_},{"pixelWidth",frameWidth_},{"pixelHeight",frameHeight_},{"contentPixelHeight",contentHeight_},
        {"contentTop",38},{"frames",frames_},
        {"repeatedFrames",drops_},{"delayedTicks",delayedTicks_},{"captureFailures",captureFailures_},
        {"elapsedMs",elapsed_.isValid()?elapsed_.elapsed():0},{"caption",caption_},{"failure",failure_}};
}
QJsonObject TutorialRecorder::status() const {return receipt();}
bool TutorialRecorder::sendFrame(const QByteArray& bytes) {
    // Bound Qt's outgoing buffer. A saturated encoder delays the UI visibly and
    // is recorded; it never drops a timeline slot and silently speeds the clip.
    const qint64 ceiling=64*1024*1024;
    QElapsedTimer drain;
    drain.start();
    while(encoder_.bytesToWrite()+bytes.size()>ceiling && drain.elapsed()<5000
          && encoder_.state()==QProcess::Running) {
        encoder_.waitForBytesWritten(100);
    }
    if(drain.elapsed()>50)writeEvent({{"type","encoder_delay"},{"waitMs",drain.elapsed()},{"queuedBytes",encoder_.bytesToWrite()}});
    if(encoder_.state()!=QProcess::Running||encoder_.bytesToWrite()+bytes.size()>ceiling) {
        failure_="Encoder could not drain its bounded input queue";return false;
    }
    if(encoder_.write(bytes)!=bytes.size()) {failure_="Could not write frame to encoder";return false;}
    ++frames_;return true;
}
void TutorialRecorder::frame() {
    if(!active_||capturing_||!failure_.isEmpty())return;
    capturing_=true;
    const qint64 slot=elapsed_.elapsed()*fps_/1000;
    if(slot<=lastSlot_) {capturing_=false;return;}
    const qint64 missed=slot-lastSlot_-1;
    if(missed>300) {
        failure_="Frame gap exceeded 300 slots; refusing a shortened clip";
        writeEvent({{"type","frame_gap_failure"},{"missingSlots",missed}});
        capturing_=false;return;
    }
    if(missed>0 && !lastFrame_.isEmpty()) {
        ++delayedTicks_;
        writeEvent({{"type","repeat"},{"count",missed},{"fromSlot",lastSlot_+1},{"reason","capture_or_event_loop_delay"}});
        for(qint64 i=0;i<missed;++i) {
            if(!sendFrame(lastFrame_))break;
            ++drops_;
        }
    }
    if(failure_.isEmpty()) {
        auto image=capture_();
        if(image.isNull()) {
            ++captureFailures_;writeEvent({{"type","capture_failure"},{"slot",slot}});
            if(lastFrame_.isEmpty())failure_="Initial OCCT framebuffer capture failed";
            else if(sendFrame(lastFrame_))++drops_;
        } else {
            image=image.scaled(frameWidth_,contentHeight_,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            image.setDevicePixelRatio(1);
            QImage composed(frameWidth_,frameHeight_,QImage::Format_RGBA8888);
            composed.fill(QColor(16,27,35));
            QPainter painter(&composed);
            painter.setRenderHint(QPainter::Antialiasing);
            const int margin=18;
            painter.drawImage(0,38,image);
            if(!caption_.isEmpty()) {
                painter.setPen(Qt::white);QFont font=painter.font();font.setPixelSize(25);font.setBold(true);painter.setFont(font);
                painter.drawText(QRect(margin,frameHeight_-59,frameWidth_-2*margin,52),Qt::AlignVCenter|Qt::AlignLeft,caption_);
            }
            if(!lastCommand_.isEmpty()) {
                painter.setPen(Qt::white);QFont font=painter.font();font.setPixelSize(17);painter.setFont(font);
                painter.drawText(QRect(margin,2,frameWidth_-2*margin,34),Qt::AlignVCenter,
                    lastOrigin_+" command: "+lastCommand_);
            }
            if(observedCursor_) {
                const double sx=double(frameWidth_)/window_->width(),sy=double(contentHeight_)/window_->height();
                const QPoint point(qRound(cursor_.x()*sx),38+qRound(cursor_.y()*sy));
                painter.setPen(QPen(QColor(255,198,36),3));
                if(elapsed_.elapsed()-clickAt_<550)painter.drawEllipse(point,18,18);
                painter.drawLine(point+QPoint(-7,0),point+QPoint(7,0));
                painter.drawLine(point+QPoint(0,-7),point+QPoint(0,7));
            }
            if(!heldKeys_.isEmpty()) {
                QStringList keys;for(int key:heldKeys_)keys<<keyName(key);
                painter.fillRect(QRect(frameWidth_-215,42,205,31),QColor(16,27,35,205));
                painter.setPen(Qt::white);painter.drawText(QRect(frameWidth_-208,42,196,31),Qt::AlignVCenter,keys.join(" + "));
            }
            if(!recentKey_.isEmpty()&&elapsed_.elapsed()-recentKeyAt_<900) {
                painter.fillRect(QRect(frameWidth_-215,78,205,31),QColor(16,27,35,205));
                painter.setPen(Qt::white);painter.drawText(QRect(frameWidth_-208,78,196,31),Qt::AlignVCenter,recentKey_);
            }
            painter.end();
            lastFrame_=QByteArray(reinterpret_cast<const char*>(composed.constBits()),composed.sizeInBytes());
            sendFrame(lastFrame_);
            writeEvent({{"type","frame"},{"slot",slot},{"documentSource","OCCT OpenGL framebuffer"}});
        }
    }
    lastSlot_=slot;capturing_=false;
}
QJsonObject TutorialRecorder::stop(qint64 documentRevision) {
    if(!active_)return fail("recording_inactive","No recording is active");
    timer_.stop();qApp->removeEventFilter(this);
    const auto duration=elapsed_.elapsed();
    // The final image occupies the final wall-clock slot, including time spent
    // in synchronous API commands since the preceding timer callback.
    const qint64 finalSlot=std::max<qint64>(lastSlot_,(duration*fps_+999)/1000-1);
    for(qint64 slot=lastSlot_+1;slot<=finalSlot&&failure_.isEmpty();++slot) {
        if(!sendFrame(lastFrame_))break;++drops_;
    }
    if(finalSlot>lastSlot_)writeEvent({{"type","repeat"},{"count",finalSlot-lastSlot_},{"reason","final_wall_clock_fill"}});
    writeEvent({{"type","stop"},{"durationMs",duration},{"documentRevision",documentRevision},
        {"frames",frames_},{"repeatedFrames",drops_},{"failure",failure_}});
    encoder_.closeWriteChannel();
    if(!encoder_.waitForFinished(60000)) {encoder_.kill();encoder_.waitForFinished(2000);failure_="Encoder did not finish within 60 seconds";}
    else if(encoder_.exitStatus()!=QProcess::NormalExit||encoder_.exitCode()!=0)
        failure_="ffmpeg exited unsuccessfully: "+QString::fromUtf8(encoder_.readAllStandardError()).left(500);
    active_=false;log_.close();
    auto result=receipt();result.insert("elapsedMs",duration);
    result.insert("encodedDurationMs",frames_*1000/fps_);
    result.insert("documentRevision",documentRevision);
    result.insert("source","Alloy Qt chrome + OCCT OpenGL framebuffer");
    result.insert("inputScope","Qt events delivered to Alloy widgets while recording");
    result.insert("textInputPolicy","Editor text redacted except observed numeric characters in Alloy numeric fields; password fields always redacted");
    result.insert("separateNativePopupCoverage","not guaranteed");
    QSaveFile metadata(metadataPath_);
    if(metadata.open(QIODevice::WriteOnly)) {
        metadata.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
        if(!metadata.commit()&&failure_.isEmpty())failure_="Could not commit metadata";
    } else if(failure_.isEmpty())failure_="Could not write metadata";
    if(!failure_.isEmpty())return fail("recording_failed",failure_);
    return result;
}
void TutorialRecorder::command(const QString& id,const QJsonObject& result,bool api) {
    if(!active_||id.startsWith("recording."))return;
    if(!id.startsWith("app.")&&!id.startsWith("capture.")&&!id.startsWith("document.inspect")
        &&!id.startsWith("document.state")&&!id.startsWith("geometry.inspect")
        &&!id.startsWith("commands.list")&&!id.startsWith("view.state")) {
        lastCommand_=id;lastOrigin_=api?"API":"Alloy UI";
    }
    writeEvent({{"type","command"},{"id",id},{"origin",api?"api":"qt_ui"},
        {"ok",result.value("ok")},{"revision",result.value("revision")},
        {"documentRevision",result.value("documentRevision")}});
}
bool TutorialRecorder::eventFilter(QObject* watched,QEvent* event) {
    if(!active_)return false;
    auto* widget=qobject_cast<QWidget*>(watched);
    if(!widget||!belongsTo(widget,window_))return false;
    const auto type=event->type();
    if(type==QEvent::MouseMove||type==QEvent::MouseButtonPress||type==QEvent::MouseButtonRelease) {
        const auto* mouse=static_cast<QMouseEvent*>(event);
        const auto position=window_->mapFromGlobal(mouse->globalPosition().toPoint());
        if(event->spontaneous()){cursor_=position;observedCursor_=true;}
        if(type==QEvent::MouseMove&&elapsed_.elapsed()-lastMotionAt_>=50) {
            lastMotionAt_=elapsed_.elapsed();
            writeEvent({{"type","mouse_move"},{"origin",event->spontaneous()?"native_qt":"synthetic_qt"},
                {"x",position.x()},{"y",position.y()},{"buttons",int(mouse->buttons())},
                {"modifiers",int(mouse->modifiers())},{"widget",widget->objectName()}});
        } else if(type!=QEvent::MouseMove) {
            if(type==QEvent::MouseButtonPress&&event->spontaneous())clickAt_=elapsed_.elapsed();
            writeEvent({{"type",type==QEvent::MouseButtonPress?"mouse_press":"mouse_release"},
                {"origin",event->spontaneous()?"native_qt":"synthetic_qt"},{"button",int(mouse->button())},
                {"x",position.x()},{"y",position.y()},{"modifiers",int(mouse->modifiers())},
                {"widget",widget->objectName()}});
        }
    } else if(type==QEvent::Wheel) {
        const auto* wheel=static_cast<QWheelEvent*>(event);
        const auto position=window_->mapFromGlobal(wheel->globalPosition().toPoint());
        if(event->spontaneous()){cursor_=position;observedCursor_=true;}
        writeEvent({{"type","wheel"},{"origin",event->spontaneous()?"native_qt":"synthetic_qt"},
            {"x",position.x()},{"y",position.y()},{"angleX",wheel->angleDelta().x()},
            {"angleY",wheel->angleDelta().y()},{"modifiers",int(wheel->modifiers())}});
    } else if(type==QEvent::KeyPress||type==QEvent::KeyRelease||type==QEvent::ShortcutOverride) {
        const auto* key=static_cast<QKeyEvent*>(event);
        // Qt can propagate a key event from its focused editor to a parent.
        // Apply the focused editor's policy on every delivery of that event.
        const auto isEditor=[](const QWidget* candidate) {
            return qobject_cast<const QLineEdit*>(candidate)
                ||qobject_cast<const QTextEdit*>(candidate)
                ||qobject_cast<const QPlainTextEdit*>(candidate);
        };
        auto* focused=QApplication::focusWidget();
        const QWidget* input=focused&&belongsTo(focused,window_)&&isEditor(focused)
            ?focused:widget;
        const auto* line=qobject_cast<const QLineEdit*>(input);
        const bool editor=isEditor(input);
        const bool password=line&&line->echoMode()!=QLineEdit::Normal;
        bool numeric=line&&!password;
        if(numeric) {
            numeric=false;
            for(auto* parent=input->parentWidget();parent;parent=parent->parentWidget())
                if(qobject_cast<QAbstractSpinBox*>(parent)){numeric=true;break;}
            numeric=numeric||input->objectName()=="sketch.inlineValue"
                ||input->objectName()=="sketch.fillet.radius";
        }
        if(type==QEvent::KeyPress&&!editor)heldKeys_.insert(key->key());
        else if(type==QEvent::KeyRelease)heldKeys_.remove(key->key());
        const bool safeNumber=numeric&&key->text().size()==1&&QString("0123456789.-+").contains(key->text());
        const auto shown=editor&&key->key()!=Qt::Key_Return&&key->key()!=Qt::Key_Enter&&!safeNumber
            ?QString("Text input"):safeNumber?key->text():keyName(key->key());
        if(type==QEvent::KeyPress||type==QEvent::ShortcutOverride) {
            recentKey_=shown;recentKeyAt_=elapsed_.elapsed();
        }
        writeEvent({{"type",type==QEvent::KeyPress?"key_press":type==QEvent::KeyRelease?"key_release":"shortcut_override"},
            {"origin",event->spontaneous()?"native_qt":"synthetic_qt"},
            {"key",shown},
            {"editorRedacted",editor&&shown=="Text input"},{"numericAllowed",safeNumber},
            {"modifiers",int(key->modifiers())},{"widget",widget->objectName()}});
    }
    return false;
}
