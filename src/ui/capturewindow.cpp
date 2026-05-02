#include "capturewindow.hpp"

#include <QColor>
#include <QFont>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QTimer>

#ifdef Q_OS_MACOS
#include "../platform/macos_window.h"
#endif
#ifdef Q_OS_LINUX
#include "../platform/x11_window.hpp"
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace sc {

CaptureWindow::CaptureWindow(QObject* /*controller*/, QWidget* parent)
    : QGraphicsView(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                 | Qt::WindowStaysOnTopHint
                 | Qt::Tool);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);

    // The QGraphicsView has an internal viewport widget — both need
    // transparency set so macOS composites the window correctly.
    viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    viewport()->setAttribute(Qt::WA_NoSystemBackground, true);

    // No frame, no scroll bars — the view IS the window.
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // All mouse handling is at the view level; items are display-only.
    setInteractive(false);

    // Near-zero alpha so macOS still delivers mouse events to this window
    // even over the visually transparent interior. 1/255 ≈ 0.4% opacity,
    // imperceptible to the eye.
    setBackgroundBrush(QColor(0, 0, 0, 1));

    // Scene — rect is kept in sync with the view size via updateSceneGeometry()
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // Border rectangle
    m_borderItem = m_scene->addRect(QRectF(),
                                    QPen(borderColor(), kBorderWidth),
                                    Qt::NoBrush);
    m_borderItem->setZValue(1);

    // Dimension label (e.g. "800×450") shown in the top-left corner
    m_labelItem = m_scene->addSimpleText(QString());
    QFont labelFont;
    labelFont.setPointSize(9);
    m_labelItem->setFont(labelFont);
    m_labelItem->setBrush(borderColor());
    m_labelItem->setZValue(2);
    // hide the dimensions label. redundant saving for later.
    m_labelItem->setVisible(false);

    setGeometry(0, 0, 800, 450);  // placeholder; AppController sets real position
    updateSceneGeometry();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void CaptureWindow::onStateChanged(sc::AppState state)
{
    m_state = state;

    // Lock aspect ratio at the moment recording starts so that resizing
    // during recording produces a zoom effect rather than a crop change.
    if (state == AppState::Recording)
        m_lockedAspect = width() > 0 ? double(width()) / double(height()) : 0.0;
    else
        m_lockedAspect = 0.0;

    // Keep the frame click-through in every state so underlying apps remain
    // interactive. Movement is handled by CenterHandle.
    // WA_TransparentForMouseEvents alone is insufficient — it only affects
    // Qt's internal dispatch. Real click-through requires a platform call:
    //   macOS: NSWindow.ignoresMouseEvents
    //   Linux/X11: XShape input region set to empty
    const bool passthrough = true;
    setAttribute(Qt::WA_TransparentForMouseEvents, passthrough);
#ifdef Q_OS_MACOS
    setWindowClickThrough(reinterpret_cast<void*>(winId()), passthrough);
#endif
#ifdef Q_OS_LINUX
    sc::setWindowClickThrough(winId(), passthrough);
#endif
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (passthrough)
        exStyle |= WS_EX_TRANSPARENT;
    else
        exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
#endif

    // Update border/handle colors and handle visibility — no repaint needed;
    // QGraphicsItem::setPen/setBrush triggers only the affected item's redraw.
    updateSceneGeometry();
}

void CaptureWindow::onRegionChanged(const sc::CaptureRegion& region)
{
    // Suppress re-emission from resizeEvent/moveEvent so we don't loop
    // back through AppController when geometry is set programmatically.
    m_suppressSignal = true;
    setGeometry(region.rect);
    m_suppressSignal = false;
}

// ---------------------------------------------------------------------------
// Scene layout
// ---------------------------------------------------------------------------

void CaptureWindow::updateSceneGeometry()
{
    const qreal w  = width();
    const qreal ht = height();
    const qreal bw = kBorderWidth;

    setSceneRect(0, 0, w, ht);

    // Border
    m_borderItem->setRect(QRectF(bw / 2.0, bw / 2.0, w - bw, ht - bw));
    m_borderItem->setPen(QPen(borderColor(), bw));

    // Dimension label — color follows state, positioned just inside top-left corner
    const QColor bc = borderColor();
    m_labelItem->setBrush(bc);
    m_labelItem->setText(QString("%1\xc3\x97%2").arg(int(w)).arg(int(ht)));
    m_labelItem->setPos(bw + 4, bw + 4);

}

// resizeEvent and moveEvent fire after macOS has committed the window
// geometry change to the compositor.
void CaptureWindow::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);
#ifdef Q_OS_MACOS
    WId wid = winId();
    QTimer::singleShot(0, this, [wid]() {
        excludeWindowFromScreenCapture(reinterpret_cast<void*>(wid));
        setWindowHidesOnDeactivate(reinterpret_cast<void*>(wid), false);
    });
#endif
#ifdef Q_OS_WIN
    QTimer::singleShot(0, this, [hwnd = reinterpret_cast<HWND>(winId())]() {
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    });
#endif
}

void CaptureWindow::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    updateSceneGeometry();
    if (!m_suppressSignal)
        emit regionChanged(geometry());
}

void CaptureWindow::moveEvent(QMoveEvent* event)
{
    QGraphicsView::moveEvent(event);
    if (!m_suppressSignal)
        emit regionChanged(geometry());
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QColor CaptureWindow::borderColor() const
{
    switch (m_state) {
    case AppState::Recording:  return QColor(0xEF, 0x44, 0x44); // red
    case AppState::Paused:     return QColor(0xFA, 0xCC, 0x15); // yellow
    default:                   return QColor(0x94, 0xA3, 0xB8); // slate
    }
}

} // namespace sc
