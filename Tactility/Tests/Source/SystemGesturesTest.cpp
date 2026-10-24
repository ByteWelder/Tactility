#include "doctest.h"
#include <Tactility/lvgl/SystemGestures.h>

using namespace tt::lvgl;

TEST_CASE("only drags that start in an edge strip belong to the system") {
    // Left edge, including its last pixel
    CHECK_EQ(systemGestureEdgeAt(0, 160, 240, 320), SystemGestureEdge::Left);
    CHECK_EQ(systemGestureEdgeAt(SYSTEM_GESTURE_EDGE_SIZE - 1, 160, 240, 320), SystemGestureEdge::Left);
    CHECK_EQ(systemGestureEdgeAt(SYSTEM_GESTURE_EDGE_SIZE, 160, 240, 320), SystemGestureEdge::None);
    // Bottom edge, so apps keep the rest of their swipes and scrolls
    CHECK_EQ(systemGestureEdgeAt(120, 320 - SYSTEM_GESTURE_EDGE_SIZE, 240, 320), SystemGestureEdge::Bottom);
    CHECK_EQ(systemGestureEdgeAt(120, 319, 240, 320), SystemGestureEdge::Bottom);
    CHECK_EQ(systemGestureEdgeAt(120, 320 - SYSTEM_GESTURE_EDGE_SIZE - 1, 240, 320), SystemGestureEdge::None);
    // The bottom corner goes home, not back
    CHECK_EQ(systemGestureEdgeAt(0, 319, 240, 320), SystemGestureEdge::Bottom);
    // The top edge is handled by the statusbar itself
    CHECK_EQ(systemGestureEdgeAt(120, 0, 240, 320), SystemGestureEdge::None);
}

TEST_CASE("an upward swipe must clear the threshold and be mostly vertical") {
    CHECK(systemGestureIsSwipeUp(120, 300, 120, 300 - SYSTEM_GESTURE_SWIPE_DISTANCE));
    CHECK_FALSE(systemGestureIsSwipeUp(120, 300, 120, 300 - SYSTEM_GESTURE_SWIPE_DISTANCE + 1));
    CHECK_FALSE(systemGestureIsSwipeUp(120, 260, 120, 300));
    // A diagonal with equal travel on both axes is ambiguous: the app keeps it
    CHECK_FALSE(systemGestureIsSwipeUp(120, 300, 80, 260));
    CHECK_FALSE(systemGestureIsSwipeUp(120, 300, 70, 260));
    CHECK(systemGestureIsSwipeUp(120, 300, 130, 260));
}

TEST_CASE("a rightward swipe must clear the threshold and be mostly horizontal") {
    CHECK(systemGestureIsSwipeRight(0, 160, SYSTEM_GESTURE_SWIPE_DISTANCE, 160));
    CHECK_FALSE(systemGestureIsSwipeRight(0, 160, SYSTEM_GESTURE_SWIPE_DISTANCE - 1, 160));
    CHECK_FALSE(systemGestureIsSwipeRight(40, 160, 0, 160));
    CHECK_FALSE(systemGestureIsSwipeRight(0, 160, 40, 200));
    CHECK(systemGestureIsSwipeRight(0, 160, 40, 150));
    // Equal travel on both axes is ambiguous: the app keeps it
    CHECK_FALSE(systemGestureIsSwipeRight(0, 160, 40, 120));
}
