#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QPoint>
#include <QProcess>
#include <QSet>
#include <QTimer>
#include <QByteArray>
#include <functional>

class QWidget;

// Opt-in recording of this Alloy window. Input events are observed, never replayed.
class TutorialRecorder final : public QObject {
public:
    TutorialRecorder(QWidget* window, std::function<QImage()> capture);
    ~TutorialRecorder() override;
    QJsonObject start(const QString& path, int fps, qint64 documentRevision);
    QJsonObject step(const QString& text);
    QJsonObject status() const;
    QJsonObject stop(qint64 documentRevision);
    void command(const QString& id, const QJsonObject& result, bool api);
    bool active() const { return active_; }
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void frame();
    bool sendFrame(const QByteArray& bytes);
    void writeEvent(QJsonObject event);
    QJsonObject receipt() const;
    QWidget* window_;
    std::function<QImage()> capture_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    QFile log_;
    QProcess encoder_;
    QString path_, logPath_, metadataPath_, caption_, lastCommand_, lastOrigin_;
    QString recentKey_;
    QSet<int> heldKeys_;
    QPoint cursor_;
    qint64 clickAt_ = -10000;
    qint64 lastMotionAt_ = -10000;
    qint64 recentKeyAt_ = -10000;
    int fps_ = 15, frameWidth_ = 0, frameHeight_ = 0, contentHeight_ = 0;
    qint64 frames_ = 0, drops_ = 0, delayedTicks_ = 0, captureFailures_ = 0;
    qint64 lastSlot_ = -1;
    QByteArray lastFrame_;
    QString failure_;
    bool active_ = false, capturing_ = false;
    bool observedCursor_ = false;
};
