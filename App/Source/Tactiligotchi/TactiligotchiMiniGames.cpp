#include "TactiligotchiMiniGames.h"
#include "PatternGame.h"  // Include the PatternGame header

TactiligotchiMiniGames::TactiligotchiMiniGames(TamagotchiApp* app)
    : parent_app(app), pattern_game(nullptr)
{
}

void TactiligotchiMiniGames::startPatternGame(lv_obj_t* parent) {
    if (pattern_game == nullptr) {
        pattern_game = new PatternGame(parent_app);
    }
    pattern_game->startGame(parent);
}

void TactiligotchiMiniGames::endPatternGame() {
    if (pattern_game) {
        pattern_game->endGame();
        delete pattern_game;
        pattern_game = nullptr;
    }
}

void TactiligotchiMiniGames::gameSuccess() {
    // Forward to parent app for handling success
    if (parent_app) parent_app->gameSuccess();
}

void TactiligotchiMiniGames::gameFailed() {
    // Forward to parent app for handling failure
    if (parent_app) parent_app->gameFailed();
}

void TactiligotchiMiniGames::endMiniGame() {
    // Called when any mini game ends (pattern game calls this too)
    // Cleanup if pattern game was active
    if (pattern_game) {
        delete pattern_game;
        pattern_game = nullptr;
    }

    // Notify parent app
    if (parent_app) parent_app->endMiniGame();
}
