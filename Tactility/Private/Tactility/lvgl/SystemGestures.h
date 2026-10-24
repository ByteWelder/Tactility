#pragma once

#include <stdint.h>

namespace tt::lvgl {

/** The width of the strip along an edge in which a drag belongs to the system instead of the app. */
constexpr int32_t SYSTEM_GESTURE_EDGE_SIZE = 20;
/** The minimum travel for a drag that starts in an edge strip to count as a swipe. */
constexpr int32_t SYSTEM_GESTURE_SWIPE_DISTANCE = 40;

/** The screen edges that the system takes drags from. */
enum class SystemGestureEdge {
    None,
    Bottom,
    Left,
};

/**
 * Which system gesture a drag that starts at a screen edge belongs to.
 * A drag that starts in a strip is the system's, whatever it is over: that's what makes the
 * gestures work on top of scrollable apps. Everywhere else, apps keep their own swipes.
 * @param[in] x,y where the press started, in screen coordinates
 * @param[in] width,height the display size
 * @return the edge that owns the drag, or SystemGestureEdge::None
 */
constexpr SystemGestureEdge systemGestureEdgeAt(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (y >= height - SYSTEM_GESTURE_EDGE_SIZE) {
        return SystemGestureEdge::Bottom;
    }
    if (x < SYSTEM_GESTURE_EDGE_SIZE) {
        return SystemGestureEdge::Left;
    }
    return SystemGestureEdge::None;
}

/**
 * @return true when the travel from the press to the release point is an upward swipe, rather
 * than a mostly-sideways drag
 */
constexpr bool systemGestureIsSwipeUp(int32_t pressX, int32_t pressY, int32_t x, int32_t y) {
    const int32_t dy = pressY - y;
    return dy >= SYSTEM_GESTURE_SWIPE_DISTANCE && dy > (x > pressX ? x - pressX : pressX - x);
}

/**
 * @return true when the travel from the press to the release point is a rightward swipe, rather
 * than a mostly-vertical drag
 */
constexpr bool systemGestureIsSwipeRight(int32_t pressX, int32_t pressY, int32_t x, int32_t y) {
    const int32_t dx = x - pressX;
    return dx >= SYSTEM_GESTURE_SWIPE_DISTANCE && dx > (y > pressY ? y - pressY : pressY - y);
}

/**
 * Starts listening to the pointer devices for the system gestures: swipe up from the bottom edge
 * to go home (holding it opens the recents switcher), and swipe right from the left edge to close
 * the topmost app. Does nothing on devices without a pointer device, so keypad/encoder-only
 * devices behave as before.
 * @warning Caller must hold the LVGL lock.
 */
void systemGesturesAttach();

/**
 * Stops listening to the pointer devices.
 * @warning Caller must hold the LVGL lock.
 */
void systemGesturesDetach();

}
