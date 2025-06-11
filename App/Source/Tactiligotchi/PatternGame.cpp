#include "PatternGame.h"
#include <lvgl.h>
#include "Tactiligotchi.h"  // For TamagotchiApp definition

PatternGame::PatternGame(TamagotchiApp* app)
    : parent_app(app),
      game_container(nullptr),
      pattern_display(nullptr),
      pattern_length(3),
      current_input(0),
      score(0),
      showing_pattern(true),
      pattern_timer(nullptr),
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
    lv_label_set_text(pattern_display, "Watch carefully!");

    if (pattern_timer) {
        lv_timer_del(pattern_timer);
    }

    pattern_timer = lv_timer_create([](lv_timer_t* timer) {
        PatternGame* game = static_cast<PatternGame*>(timer->user_data);

        if (game->pattern_step < game->pattern_length) {
            int btn_id = game->pattern[game->pattern_step];
            lv_obj_add_state(game->input_buttons[btn_id], LV_STATE_PRESSED);

            lv_timer_create([](lv_timer_t* t) {
                PatternGame* g = static_cast<PatternGame*>(t->user_data);
                for (int i = 0; i < 4; i++) {
                    lv_obj_clear_state(g->input_buttons[i], LV_STATE_PRESSED);
                }
                lv_timer_del(t);
            }, 300, game);

            game->pattern_step++;
        } else {
            game->showing_pattern = false;
            lv_label_set_text(game->pattern_display, "Your turn! Repeat the pattern");
            lv_timer_del(timer);
            game->pattern_timer = nullptr;
        }
    }, 600, this);
}

void PatternGame::buttonPressed(int button_id) {
    if (showing_pattern) return;

    if (button_id == pattern[current_input]) {
        current_input++;

        if (current_input >= pattern_length) {
            score++;
            lv_label_set_text_fmt(pattern_display, "Great! Score: %d", score);
            if (pattern_length < 8) pattern_length++;

            parent_app->gameSuccess();

            lv_timer_create([](lv_timer_t* timer) {
                PatternGame* game = static_cast<PatternGame*>(timer->user_data);
                game->generatePattern();
                game->showPattern();
                lv_timer_del(timer);
            }, 1500, this);
        }
    } else {
        lv_label_set_text(pattern_display, "Wrong! Try again");
        parent_app->gameFailed();

        lv_timer_create([](lv_timer_t* timer) {
            PatternGame* game = static_cast<PatternGame*>(timer->user_data);
            game->pattern_length = std::max(3, game->pattern_length - 1);
            game->generatePattern();
            game->showPattern();
            lv_timer_del(timer);
        }, 1500, this);
    }
}

void PatternGame::endGame() {
    if (pattern_timer) {
        lv_timer_del(pattern_timer);
        pattern_timer = nullptr;
    }
    lv_obj_del(game_container);
    parent_app->endMiniGame();
}
