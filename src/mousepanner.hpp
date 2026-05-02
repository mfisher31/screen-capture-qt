#pragma once

#include <QPoint>
#include <QRect>
#include <QtMath>

namespace sc {

// Computes a panned rect based on cursor proximity to the edges of `current`.
//
// Two activation zones per edge:
//   innerThreshold — cursor is inside the rect, within this many px of an edge
//   outerThreshold — cursor is outside the rect, within this many px of an edge
//
// Speed ramps linearly from 0 at the zone boundary to maxSpeed at the edge itself.
// The two zones meet at the edge and their speeds add if both fire simultaneously
// (which only happens at distance 0, i.e. exactly on the border).
//
// When outerThreshold < 0 the outer zone is unlimited: panning fires at maxSpeed
// whenever the cursor is anywhere outside the rect (screen-wide follow mode).
//
// Returns `current` unchanged when the cursor is far from all edges or the rect
// is already against the screen bounds in the direction of pan.
//
// All coordinates are in logical screen pixels (same space as QCursor::pos()).
struct MousePanner {
    int innerThreshold = 150;   // px inside  the rect where panning begins
    int outerThreshold = -1;   // px outside the rect where panning begins (<0 = unlimited)
    int maxSpeed       = 10;   // px/tick at maximum proximity (at the edge itself)

    [[nodiscard]] QRect pan(const QPoint& cursor,
                            const QRect&  current,
                            const QRect&  screenBounds) const
    {
        // Signed distances to each edge. Positive = cursor is on the inside.
        const int distLeft   = cursor.x() - current.left();
        const int distRight  = current.right()  - cursor.x();
        const int distTop    = cursor.y() - current.top();
        const int distBottom = current.bottom() - cursor.y();

        int dx = 0, dy = 0;

        // Each edge has two zones. The guards ensure they don't overlap:
        //   inner zone: cursor is inside  (dist >= 0) and within innerThreshold
        //   outer zone: cursor is outside (dist <  0) and within outerThreshold

        // Left edge.
        if (distLeft >= 0 && distLeft < innerThreshold)                        dx -= innerSpeed(distLeft);
        if (distLeft <  0 && (outerThreshold < 0 || -distLeft < outerThreshold)) dx -= outerSpeed(-distLeft);

        // Right edge.
        if (distRight >= 0 && distRight < innerThreshold)                         dx += innerSpeed(distRight);
        if (distRight <  0 && (outerThreshold < 0 || -distRight < outerThreshold)) dx += outerSpeed(-distRight);

        // Top edge.
        if (distTop >= 0 && distTop < innerThreshold)                        dy -= innerSpeed(distTop);
        if (distTop <  0 && (outerThreshold < 0 || -distTop < outerThreshold)) dy -= outerSpeed(-distTop);

        // Bottom edge.
        if (distBottom >= 0 && distBottom < innerThreshold)                         dy += innerSpeed(distBottom);
        if (distBottom <  0 && (outerThreshold < 0 || -distBottom < outerThreshold)) dy += outerSpeed(-distBottom);

        if (dx == 0 && dy == 0)
            return current;

        QRect r = current.translated(dx, dy);

        // Clamp to screen bounds.
        if (r.left()   < screenBounds.left())   r.moveLeft(screenBounds.left());
        if (r.top()    < screenBounds.top())     r.moveTop(screenBounds.top());
        if (r.right()  > screenBounds.right())   r.moveRight(screenBounds.right());
        if (r.bottom() > screenBounds.bottom())  r.moveBottom(screenBounds.bottom());

        return r;
    }

private:
    // Speed when cursor is inside, `dist` px from the edge (0 = at the edge).
    int innerSpeed(int dist) const
    {
        if (dist <= 0)               return maxSpeed;
        if (dist >= innerThreshold)  return 0;
        return qRound((1.0 - (qreal)dist / innerThreshold) * maxSpeed);
    }

    // Speed when cursor is outside, `dist` px past the edge (0 = at the edge).
    // When outerThreshold < 0 the zone is unlimited and speed is always maxSpeed.
    int outerSpeed(int dist) const
    {
        if (outerThreshold < 0)      return maxSpeed;
        if (dist <= 0)               return maxSpeed;
        if (dist >= outerThreshold)  return 0;
        return qRound((1.0 - (qreal)dist / outerThreshold) * maxSpeed);
    }
};

} // namespace sc
