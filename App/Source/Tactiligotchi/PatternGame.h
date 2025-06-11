#pragma once

#include <cstdlib>     // For rand()
#include "lvgl.h"

// Forward declaration of TamagotchiApp
class TamagotchiApp;

class PatternGame {
private:
    TamagotchiApp* parent_app;
    lv_obj_t* game_container;
    lv_obj_t* pattern_display;
    lv_obj_t* input_buttons[4];

    int pattern[8];
    int pattern_length;
    int current_input;
    int score;
    bool showing_pattern;
    lv_timer_t* pattern_timer;
    int pattern_step;

public:
    explicit PatternGame(TamagotchiApp* app);

    void startGame(lv_obj_t* parent);
    void generatePattern();
    void showPattern();
    void buttonPressed(int button_id);
    void endGame();
};
