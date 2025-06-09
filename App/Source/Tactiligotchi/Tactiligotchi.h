#pragma once

#include <lvgl.h>
#include <cstdint>
#include <tt/app/App.h>

// Forward declaration
namespace tt {
    class AppContext;
}

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

struct PetData {
    int hunger = 80;
    int happiness = 80;
    int health = 100;
    int energy = 100;

    PetType type = PetType::EGG;
    PetMood mood = PetMood::NEUTRAL;
    uint32_t age_minutes = 0;
    uint32_t last_update_time = 0;

    int times_fed = 0;
    int times_played = 0;
    int times_cleaned = 0;
    bool needs_cleaning = false;

    void updateStats(uint32_t current_time);
    PetMood calculateMood() const;
    bool needsEvolution() const;
    void evolve();
};

class TamagotchiApp final : public tt::App {
private:
    PetData pet;

    lv_obj_t* pet_container = nullptr;
    lv_obj_t* pet_sprite = nullptr;
    lv_obj_t* hunger_bar = nullptr;
    lv_obj_t* happiness_bar = nullptr;
    lv_obj_t* health_bar = nullptr;
    lv_obj_t* energy_bar = nullptr;
    lv_obj_t* status_label = nullptr;

    lv_obj_t* feed_btn = nullptr;
    lv_obj_t* play_btn = nullptr;
    lv_obj_t* clean_btn = nullptr;
    lv_obj_t* sleep_btn = nullptr;

    lv_timer_t* update_timer = nullptr;

public:
    void onShow(tt::AppContext& context, lv_obj_t* parent) override;
    void onHide() override;

private:
    void createPetDisplay(lv_obj_t* parent);
    void createStatBars(lv_obj_t* parent);
    void createActionButtons(lv_obj_t* parent);

    void updatePetDisplay();
    void updateStatBars();

    void feedPet();
    void petAnimal(); // <- matches .cpp now
    void cleanPet();
    void putPetToSleep();

    void animatePet(lv_color_t flash_color);

    static void feed_btn_cb(lv_event_t* e);
    static void play_btn_cb(lv_event_t* e);
    static void clean_btn_cb(lv_event_t* e);
    static void sleep_btn_cb(lv_event_t* e);
    static void update_timer_cb(lv_timer_t* timer);

    void savePetData();
    void loadPetData();
};

// App manifest export
extern const tt::AppManifest tactiligotchi_app;
