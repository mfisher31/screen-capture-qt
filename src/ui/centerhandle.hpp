#pragma once

#include "../appcontroller.hpp"

#include <QPoint>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

namespace sc {

// Small translucent drag handle centered in the capture region.
// It remains interactive even when CaptureWindow is click-through.
class CenterHandle : public QWidget {
    Q_OBJECT

public:
    explicit CenterHandle(QWidget* parent = nullptr);

signals:
    void dragDelta(const QPoint& delta);
    void wheelResizeRequested(int direction);
    void screenshotRequested();

public slots:
    void onRegionChanged(const sc::CaptureRegion& region);

protected:
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    static constexpr int kSize = 44;
    static constexpr int kWheelStepDelta = 120;
    static constexpr int kWheelThrottleMs = 60;

    void flushWheelResize();

    bool   m_dragging = false;
    bool   m_hovered  = false;
    int    m_wheelAccumulator = 0;
    QPoint m_lastGlobal;
    QTimer m_wheelTimer;
};

} // namespace sc
