#pragma once
#include "Tactility/app/App.h"
#include <cstdint>
#include <lvgl.h>

// Pet states and types
enum class PetMood {
    HAPPY,
    NEUTRAL,
    SAD,
    SICK,
    SLEEPING,
    PLAYING
};

enum class PetType {
    EGG,
    BABY,
    CHILD,
    TEEN,
    ADULT
};

// Core pet data structure
struct PetData {
    // Core stats (0–100)
    int hunger = 80;
    int happiness = 80;
    int health = 100;
    int energy = 100;

    // Pet info
    PetType type = PetType::EGG;
    PetMood mood = PetMood::NEUTRAL;
    uint32_t age_minutes = 0;
    uint32_t last_update_time = 0;

    // Care tracking
    int times_fed = 0;
    int times_played = 0;
    int times_cleaned = 0;
    bool needs_cleaning = false;

    // Save/load helpers
    void updateStats(uint32_t current_time);
    PetMood calculateMood() const;
    bool needsEvolution() const;
    void evolve();
};

// Forward declaration of mini-game
class PatternGame;

class TamagotchiApp final : public tt::App {
private:
    PetData pet;

    // UI Elements
    lv_obj_t* pet_container;
    lv_obj_t* pet_sprite;
    lv_obj_t* hunger_bar;
    lv_obj_t* happiness_bar;
    lv_obj_t* health_bar;
    lv_obj_t* energy_bar;
    lv_obj_t* status_label;

    // Buttons
    lv_obj_t* feed_btn;
    lv_obj_t* play_btn;
    lv_obj_t* clean_btn;
    lv_obj_t* sleep_btn;

    // Timer for updates
    lv_timer_t* update_timer;

    // Mini-game
    PatternGame* current_minigame = nullptr;

public:
    void onShow(AppContext& context, lv_obj_t* parent) final;
    void onHide() final;

private:
    // UI Creation
    void createPetDisplay(lv_obj_t* parent);
    void createStatBars(lv_obj_t* parent);
    void createActionButtons(lv_obj_t* parent);

    // Game Logic
    void updatePetDisplay();
    void updateStatBars();
    void feedPet();
    void playWithPet();
    void cleanPet();
    void putPetToSleep();

    // Mini-game Callbacks
    void gameSuccess();
    void gameFailed();
    void endMiniGame();

    // Visual Feedback
    void animatePet(lv_color_t color);

    // Event handlers
    static void feed_btn_cb(lv_event_t* e);
    static void play_btn_cb(lv_event_t* e);
    static void clean_btn_cb(lv_event_t* e);
    static void sleep_btn_cb(lv_event_t* e);
    static void update_timer_cb(lv_timer_t* timer);

    // Save/Load
    void savePetData();
    void loadPetData();
};
