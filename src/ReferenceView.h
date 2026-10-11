#pragma once
#include <QWidget>
#include <QImage>
#include <QJsonObject>
#include <functional>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Trihedron.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

// GPU presentation only. Reference axes/grid are not authored document objects.
class ReferenceView final : public QWidget {
public:
    explicit ReferenceView(QWidget* parent = nullptr);
    ~ReferenceView() override;
    QString orientation = "isometric";
    bool grid = true, origin = true;
    double zoom = 1.0;
    QPointF pan;
    std::function<void(const QString&, const QJsonObject&)> dispatch;
    void sync();
    QJsonObject diagnostics() const;
    QImage framebuffer();
    QPaintEngine* paintEngine() const override { return nullptr; }
protected:
    void showEvent(QShowEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
private:
    void initialize();
    Handle(V3d_Viewer) viewer_;
    Handle(V3d_View) view_;
    Handle(AIS_InteractiveContext) context_;
    Handle(AIS_Trihedron) axes_;
    Handle(Graphic3d_Camera) baseCamera_;
    QString error_;
    QPointF last_;
};
