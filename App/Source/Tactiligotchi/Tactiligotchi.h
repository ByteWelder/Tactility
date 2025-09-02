#pragma once

#include <Tactility/app/App.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppManifest.h> // Already added
#include <cstdint>
#include <lvgl.h>
#include "PatternGame.h"
#include "Tactility/Timer.h" // Added for tt::Timer

// Pet states and types
enum class PetMood { HAPPY, NEUTRAL, SAD, SICK, SLEEPING, PLAYING };
enum class PetType { EGG, BABY, CHILD, TEEN, ADULT };

struct PetData {
    int hunger = 80, happiness = 80, health = 100, energy = 100;
    PetType type = PetType::EGG;
    PetMood mood = PetMood::NEUTRAL;
    uint32_t age_minutes = 0, last_update_time = 0;
    int times_fed = 0, times_played = 0, times_cleaned = 0;
    bool needs_cleaning = false;
    void updateStats(uint32_t current_time);
    PetMood calculateMood() const;
    bool needsEvolution() const;
    void evolve();
};

class TamagotchiApp final : public tt::app::App {
public:
    TamagotchiApp() = default;
    ~TamagotchiApp() override = default;

    void onShow(tt::app::AppContext& context, lv_obj_t* parent) override;
    void onHide(tt::app::AppContext& appContext) override;

    void gameSuccess();
    void gameFailed();
    void endMiniGame();
    void petAnimal();

private:
    PetData pet;
    lv_obj_t* pet_container = nullptr;
    lv_obj_t* pet_sprite = nullptr;
    lv_obj_t* status_label = nullptr;
    lv_obj_t* hunger_bar = nullptr;
    lv_obj_t* happiness_bar = nullptr;
    lv_obj_t* health_bar = nullptr;
    lv_obj_t* energy_bar = nullptr;
    lv_obj_t* feed_btn = nullptr;
    lv_obj_t* play_btn = nullptr;
    lv_obj_t* clean_btn = nullptr;
    lv_obj_t* sleep_btn = nullptr;
    tt::Timer* update_timer = nullptr;
    PatternGame* current_minigame = nullptr;

    void createPetDisplay(lv_obj_t* parent);
    void createStatBars(lv_obj_t* parent);
    void createActionButtons(lv_obj_t* parent);

    void updatePetDisplay();
    void updateStatBars();
    void feedPet();
    void playWithPet();
    void cleanPet();
    void putPetToSleep();
    void animatePet(lv_color_t color);

    void update_timer_cb();

    static void feed_btn_cb(lv_event_t* e);
    static void play_btn_cb(lv_event_t* e);
    static void clean_btn_cb(lv_event_t* e);
    static void sleep_btn_cb(lv_event_t* e);

    void savePetData();
    void loadPetData();
};

// App manifest export (corrected namespace)
extern const tt::app::AppManifest tactiligotchi_app;
