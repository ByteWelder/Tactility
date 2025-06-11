#pragma once

#include "lvgl.h"
#include "Tactiligotchi.h"

// Forward declaration of PatternGame
class PatternGame;

class TactiligotchiMiniGames {
private:
    TamagotchiApp* parent_app;
    
    // Add PatternGame pointer
    PatternGame* pattern_game = nullptr;

public:
    TactiligotchiMiniGames(TamagotchiApp* app);

    void startPatternGame(lv_obj_t* parent);
    void endPatternGame();

    // Callback hooks from games
    void gameSuccess();
    void gameFailed();
    void endMiniGame();
};
