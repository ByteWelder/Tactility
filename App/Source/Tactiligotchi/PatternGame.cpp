#include "PatternGame.h"
#include "lvgl.h"
#include "Tactility/Timer.h"
#include "Tactiligotchi.h"  // For TamagotchiApp definition
#include <memory>
#include <functional>

PatternGame::PatternGame(TamagotchiApp* app)
    : parent_app(app),
      game_container(nullptr),
      pattern_display(nullptr),
      pattern_length(3),
      current_input(0),
      score(0),
      showing_pattern(true),
      pattern_step(0)
{
    // Initialize pattern array to zero
    for (int i = 0; i < 8; i++) {
        pattern[i] = 0;
    }
}

void PatternGame::startGame(lv_obj_t* parent) {
    game_container = lv_obj_create(parent);
    lv_obj_set_size(game_container, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(game_container, LV_OBJ_FLAG_SCROLLABLE);

    pattern_display = lv_label_create(game_container);
    lv_label_set_text(pattern_display, "Watch the pattern!");
    lv_obj_align(pattern_display, LV_ALIGN_TOP_MID, 0, 50);

    lv_color_t colors[] = {
        lv_color_hex(0xFF0000), // Red
        lv_color_hex(0x00FF00), // Green
        lv_color_hex(0x0000FF), // Blue
        lv_color_hex(0xFFFF00)  // Yellow
    };

    for (int i = 0; i < 4; i++) {
        input_buttons[i] = lv_btn_create(game_container);
        lv_obj_set_size(input_buttons[i], 60, 60);
        lv_obj_align(input_buttons[i], LV_ALIGN_CENTER, (i % 2) * 80 - 40, (i / 2) * 80 - 40);
        lv_obj_set_style_bg_color(input_buttons[i], colors[i], 0);

        lv_obj_add_event_cb(input_buttons[i], [](lv_event_t* e) {
            PatternGame* game = static_cast<PatternGame*>(lv_event_get_user_data(e));
            lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));

            for (int id = 0; id < 4; ++id) {
                if (btn == game->input_buttons[id]) {
                    game->buttonPressed(id);
                    break;
                }
            }
        }, LV_EVENT_CLICKED, this);
    }

    // Exit button
    lv_obj_t* close_btn = lv_btn_create(game_container);
    lv_obj_set_size(close_btn, 60, 30);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Exit");
    lv_obj_center(close_label);
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        PatternGame* game = static_cast<PatternGame*>(lv_event_get_user_data(e));
        game->endGame();
    }, LV_EVENT_CLICKED, this);

    generatePattern();
    showPattern();
}

void PatternGame::generatePattern() {
    for (int i = 0; i < pattern_length; i++) {
        pattern[i] = rand() % 4;
    }
    current_input = 0;
    showing_pattern = true;
    pattern_step = 0;
}

void PatternGame::showPattern() {
    // Defer UI update to LVGL context
    lv_async_call([](void* user_data) {
        PatternGame* game = static_cast<PatternGame*>(user_data);
        lv_label_set_text(game->pattern_display, "Watch carefully!");
    }, this);

    // Stop any existing timer
    pattern_timer = nullptr;

    // Create new timer for pattern display
    pattern_timer = std::make_unique<tt::Timer>(tt::Timer::Type::Once, [this]() {
        if (pattern_step < pattern_length) {
            int btn_id = pattern[pattern_step];

            // Defer button highlight to LVGL
            lv_async_call([](void* user_data) {
                auto* data = static_cast<std::pair<PatternGame*, int>*>(user_data);
                PatternGame* game = data->first;
                int btn_id = data->second;
                lv_obj_add_state(game->input_buttons[btn_id], LV_STATE_PRESSED);
                delete data;
            }, new std::pair<PatternGame*, int>(this, btn_id));

            // Create timer for clearing highlight
            auto highlight_timer = std::make_unique<tt::Timer>(tt::Timer::Type::Once, [this, timer = std::move(highlight_timer)]() {
                // Defer clearing highlights to LVGL
                lv_async_call([](void* user_data) {
                    PatternGame* game = static_cast<PatternGame*>(user_data);
                    for (int i = 0; i < 4; i++) {
                        lv_obj_clear_state(game->input_buttons[i], LV_STATE_PRESSED);
                    }
                }, this);
            });

            // Start highlight timer (300 ms)
            highlight_timer->start(300); // Assuming 1 tick = 1 ms
            highlight_timer->setThreadPriority(tt::Timer::Priority::Normal);

            pattern_step++;

            // Restart pattern timer for next step
            pattern_timer = std::make_unique<tt::Timer>(tt::Timer::Type::Once, [this]() {
                showPattern();
            });
            pattern_timer->start(600); // Assuming 1 tick = 1 ms
            pattern_timer->setThreadPriority(tt::Timer::Priority::Normal);
        } else {
            // Pattern display complete
            showing_pattern = false;
            lv_async_call([](void* user_data) {
                PatternGame* game = static_cast<PatternGame*>(user_data);
                lv_label_set_text(game->pattern_display, "Your turn! Repeat the pattern");
            }, this);
            pattern_timer = nullptr;
        }
    });

    // Start initial timer
    pattern_timer->start(600); // 600 ms = 600 ticks
    pattern_timer->setThreadPriority(tt::Timer::Priority::Normal);
}

void PatternGame::buttonPressed(int button_id) {
    if (showing_pattern) return;

    if (button_id == pattern[current_input]) {
        current_input++;

        if (current_input >= pattern_length) {
            score++;
            lv_async_call([](void* user_data) {
                auto* data = static_cast<std::pair<PatternGame*, int>*>(user_data);
                PatternGame* game = data->first;
                int score = data->second;
                lv_label_set_text_fmt(game->pattern_display, "Great! Score: %d", score);
                delete data;
            }, new std::pair<PatternGame*, int>(this, score));

            parent_app->gameSuccess();
            if (pattern_length < 8) pattern_length++;

            // Delay before next pattern
            auto delay_timer = std::make_unique<tt::Timer>(tt::Timer::Type::Once, [this, delay_timer = std::move(delay_timer)]() {
                generatePattern();
                showPattern();
            });
            delay_timer->start(1500); // 1500 ms
            delay_timer->setThreadPriority(tt::Timer::Priority::Normal);
        }
    } else {
        lv_async_call([](void* user_data) {
            PatternGame* game = static_cast<PatternGame*>(user_data);
            lv_label_set_text(game->pattern_display, "Wrong! Try again");
        }, this);

        parent_app->gameFailed();

        // Delay before retry
        auto delay_timer = std::make_unique<tt::Timer>(tt::Timer::Type::Once, [this, delay_timer = std::move(delay_timer)]() {
            pattern_length = std::max(3, pattern_length - 1);
            generatePattern();
            showPattern();
        });
        delay_timer->start(1500); // 1500 ms
        delay_timer->setThreadPriority(tt::Timer::Priority::Normal);
    }
}

void PatternGame::endGame() {
    pattern_timer = nullptr; // Clean up timer
    lv_async_call([](void* user_data) {
        PatternGame* game = static_cast<PatternGame*>(user_data);
        lv_obj_delete(game->game_container);
        game->parent_app->endMiniGame();
    }, this);
}
