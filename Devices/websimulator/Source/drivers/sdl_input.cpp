// SPDX-License-Identifier: Apache-2.0
#include "sdl_input.h"
#include "sdl_display.h"

#include <tactility/drivers/keyboard.h>

#include <SDL2/SDL.h>

#include <cstdlib>
#include <mutex>

namespace {

constexpr size_t KEY_QUEUE_CAPACITY = 32;

std::mutex state_mutex;

SdlPointerState pointer_state = { 0, 0, false };

uint32_t key_queue[KEY_QUEUE_CAPACITY];
size_t key_queue_head = 0;
size_t key_queue_count = 0;

bool text_input_started = false;

void push_key(uint32_t key) {
    if (key == 0 || key_queue_count >= KEY_QUEUE_CAPACITY) {
        return;
    }
    key_queue[(key_queue_head + key_queue_count) % KEY_QUEUE_CAPACITY] = key;
    key_queue_count++;
}

// The canvas can be freely resized by the browser (SDL_WINDOW_RESIZABLE /
// SDL_RenderSetLogicalSize()), so raw SDL mouse coordinates are in canvas-pixel space, not the
// fixed logical resolution LVGL renders at. SDL_RenderWindowToLogical() is the renderer's own
// inverse of that scaling, accounting for both the scale factor and any letterbox offset.
void set_pointer_position(int32_t window_x, int32_t window_y) {
    SDL_Renderer* renderer = sdl_display_get_renderer();
    if (renderer == nullptr) {
        pointer_state.x = window_x;
        pointer_state.y = window_y;
        return;
    }
    float logical_x, logical_y;
    SDL_RenderWindowToLogical(renderer, window_x, window_y, &logical_x, &logical_y);
    pointer_state.x = static_cast<int32_t>(logical_x);
    pointer_state.y = static_cast<int32_t>(logical_y);
}

uint32_t keycode_to_key(SDL_Keycode sdl_key, bool shift) {
    switch (sdl_key) {
        case SDLK_RIGHT: return CODEPOINT_ARROW_RIGHT;
        case SDLK_LEFT: return CODEPOINT_ARROW_LEFT;
        case SDLK_UP: return CODEPOINT_ARROW_UP;
        case SDLK_DOWN: return CODEPOINT_ARROW_DOWN;
        case SDLK_ESCAPE: return CODEPOINT_ESCAPE;
        case SDLK_BACKSPACE: return CODEPOINT_BACKSPACE;
        case SDLK_DELETE: return CODEPOINT_DELETE;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return CODEPOINT_ENTER;
        case SDLK_TAB: return CODEPOINT_TAB;
        case SDLK_HOME: return CODEPOINT_HOME;
        case SDLK_END: return CODEPOINT_END;
        default: return 0;
    }
}

}

void sdl_input_pump() {
    std::lock_guard<std::mutex> lock(state_mutex);

    if (!text_input_started) {
        SDL_StartTextInput();
        text_input_started = true;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_MOUSEMOTION:
                set_pointer_position(event.motion.x, event.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int live_x, live_y;
                    SDL_GetMouseState(&live_x, &live_y);
                    set_pointer_position(live_x, live_y);
                    pointer_state.pressed = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    pointer_state.pressed = false;
                }
                break;
            case SDL_KEYDOWN:
                push_key(keycode_to_key(event.key.keysym.sym, (event.key.keysym.mod & KMOD_SHIFT) != 0));
                break;
            case SDL_TEXTINPUT:
                push_key(static_cast<uint8_t>(event.text.text[0]));
                break;
            case SDL_WINDOWEVENT:
                // Re-present the existing frame at the new scale immediately, rather than leaving
                // stale-looking content on the canvas until the next LVGL-driven flush.
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    sdl_display_present_now();
                }
                break;
            case SDL_QUIT:
                break;
            default:
                break;
        }
    }
}

void sdl_input_get_pointer_state(SdlPointerState* out_state) {
    std::lock_guard<std::mutex> lock(state_mutex);
    *out_state = pointer_state;
}

bool sdl_input_pop_key(uint32_t* out_key) {
    std::lock_guard<std::mutex> lock(state_mutex);
    if (key_queue_count == 0) {
        return false;
    }
    *out_key = key_queue[key_queue_head];
    key_queue_head = (key_queue_head + 1) % KEY_QUEUE_CAPACITY;
    key_queue_count--;
    return true;
}

bool sdl_input_has_queued_key() {
    std::lock_guard<std::mutex> lock(state_mutex);
    return key_queue_count > 0;
}
