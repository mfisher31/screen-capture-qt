#include <QtTest>
#include "mousepanner.hpp"

namespace sc {

class MousePannerTest : public QObject {
    Q_OBJECT

    // Rect at (100,100), 800×450. Screen 0,0 1920×1080.
    const QRect kRect   { 100, 100, 800, 450 };  // right=899, bottom=549
    const QRect kScreen { 0,   0,   1920, 1080 };

private slots:
    // -----------------------------------------------------------------------
    // Cursor well inside, far from all edges — no pan
    // -----------------------------------------------------------------------

    void insideFarFromEdges_noPan()
    {
        // Center of the rect
        QPoint cursor { 500, 300 };
        MousePanner p;
        QCOMPARE(p.pan(cursor, kRect, kScreen), kRect);
    }

    // -----------------------------------------------------------------------
    // Inner zone: cursor inside, near each edge
    // -----------------------------------------------------------------------

    void innerLeft_pansLeft()
    {
        // 10px from left edge, well inside
        QPoint cursor { 110, 300 };
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.left() < kRect.left());   // moved left
        QCOMPARE(result.size(), kRect.size());   // size unchanged
    }

    void innerRight_pansRight()
    {
        QPoint cursor { 889, 300 };  // 10px from right edge (899), inside
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.left() > kRect.left());
        QCOMPARE(result.size(), kRect.size());
    }

    void innerTop_pansUp()
    {
        QPoint cursor { 500, 110 };  // 10px from top edge (100), inside
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.top() < kRect.top());
        QCOMPARE(result.size(), kRect.size());
    }

    void innerBottom_pansDown()
    {
        QPoint cursor { 500, 539 };  // 10px from bottom edge (549), inside
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.top() > kRect.top());
        QCOMPARE(result.size(), kRect.size());
    }

    // -----------------------------------------------------------------------
    // Outer zone: cursor outside, within outerThreshold
    // -----------------------------------------------------------------------

    void outerLeft_pansLeft()
    {
        QPoint cursor { 80, 300 };  // 20px outside left edge (100)
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.left() < kRect.left());
        QCOMPARE(result.size(), kRect.size());
    }

    void outerRight_pansRight()
    {
        QPoint cursor { 919, 300 };  // 20px outside right edge (899)
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.left() > kRect.left());
        QCOMPARE(result.size(), kRect.size());
    }

    void outerTop_pansUp()
    {
        QPoint cursor { 500, 80 };  // 20px outside top edge (100)
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.top() < kRect.top());
        QCOMPARE(result.size(), kRect.size());
    }

    void outerBottom_pansDown()
    {
        QPoint cursor { 500, 569 };  // 20px outside bottom edge (549)
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QVERIFY(result.top() > kRect.top());
        QCOMPARE(result.size(), kRect.size());
    }

    // -----------------------------------------------------------------------
    // Outer zone: cursor outside beyond outerThreshold — no pan
    // -----------------------------------------------------------------------

    void outsideBeyondThreshold_noPan()
    {
        // 60px outside left — beyond default outerThreshold(40)
        QPoint cursor { 40, 300 };
        MousePanner p;
        QCOMPARE(p.pan(cursor, kRect, kScreen), kRect);
    }

    // -----------------------------------------------------------------------
    // Inner zone must NOT fire when cursor is outside
    // (the original bug: distRight < 0 satisfied distRight < innerThreshold)
    // -----------------------------------------------------------------------

    void innerZoneDoesNotFireWhenOutside()
    {
        // 60px outside right — beyond outerThreshold but inside innerThreshold
        // (distRight = 899 - 959 = -60; -60 < 80 but > outerThreshold=40)
        // Expect: no pan at all
        QPoint cursor { 959, 300 };
        MousePanner p;
        QCOMPARE(p.pan(cursor, kRect, kScreen), kRect);
    }

    // -----------------------------------------------------------------------
    // Screen clamping
    // -----------------------------------------------------------------------

    void clampedAtScreenLeftEdge()
    {
        // Rect already against the left screen edge
        QRect leftRect { 0, 100, 800, 450 };
        QPoint cursor { 5, 300 };  // inside, near left edge
        MousePanner p;
        QRect result = p.pan(cursor, leftRect, kScreen);
        QCOMPARE(result.left(), 0);  // must not go negative
    }

    void clampedAtScreenRightEdge()
    {
        QRect rightRect { 1120, 100, 800, 450 };  // right = 1919
        QPoint cursor { 1915, 300 };  // inside, near right edge
        MousePanner p;
        QRect result = p.pan(cursor, rightRect, kScreen);
        QVERIFY(result.right() <= kScreen.right());
    }

    // -----------------------------------------------------------------------
    // Speed ramp — at edge: maxSpeed; at threshold boundary: 0
    // -----------------------------------------------------------------------

    void speedAtEdge_isMaxSpeed()
    {
        // Cursor exactly on left edge (distLeft = 0)
        QPoint cursor { 100, 300 };
        MousePanner p;
        QRect result = p.pan(cursor, kRect, kScreen);
        QCOMPARE(kRect.left() - result.left(), p.maxSpeed);
    }

    void speedAtInnerThreshold_isZero()
    {
        // Cursor exactly at innerThreshold from left (distLeft == innerThreshold → speed 0)
        MousePanner p;
        QPoint cursor { 100 + p.innerThreshold, 300 };
        QRect result = p.pan(cursor, kRect, kScreen);
        QCOMPARE(result, kRect);  // no movement
    }
};

} // namespace sc

QTEST_MAIN(sc::MousePannerTest)
#include "mouse_panner_test.moc"
