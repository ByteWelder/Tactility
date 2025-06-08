#pragma once
#include "tt/app/App.h"
#include <cstdint>

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
    // Core stats (0-100)
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

// Pet behavior implementation
void PetData::updateStats(uint32_t current_time) {
    if (last_update_time == 0) {
        last_update_time = current_time;
        return;
    }
    
    uint32_t minutes_passed = (current_time - last_update_time) / (1000 * 60);
    if (minutes_passed == 0) return;
    
    // Age the pet
    age_minutes += minutes_passed;
    
    // Decrease stats over time
    hunger = std::max(0, hunger - (int)(minutes_passed * 2));
    energy = std::max(0, energy - (int)(minutes_passed * 1));
    
    // Health affected by other stats
    if (hunger < 20 || happiness < 20) {
        health = std::max(0, health - (int)minutes_passed);
    }
    
    // Happiness decreases if neglected
    if (current_time - last_update_time > 30 * 60 * 1000) { // 30 min
        happiness = std::max(0, happiness - (int)(minutes_passed / 10));
    }
    
    // Random events
    if (minutes_passed > 0 && (rand() % 100) < 5) {
        needs_cleaning = true;
    }
    
    // Update mood based on current stats
    mood = calculateMood();
    
    // Check for evolution
    if (needsEvolution()) {
        evolve();
    }
    
    last_update_time = current_time;
}

PetMood PetData::calculateMood() const {
    if (health < 30) return PetMood::SICK;
    if (energy < 20) return PetMood::SLEEPING;
    if (happiness > 70 && hunger > 50) return PetMood::HAPPY;
    if (happiness < 30 || hunger < 30) return PetMood::SAD;
    return PetMood::NEUTRAL;
}

bool PetData::needsEvolution() const {
    switch(type) {
        case PetType::EGG: return age_minutes > 60; // 1 hour
        case PetType::BABY: return age_minutes > 24 * 60; // 1 day
        case PetType::CHILD: return age_minutes > 3 * 24 * 60; // 3 days
        case PetType::TEEN: return age_minutes > 7 * 24 * 60; // 1 week
        default: return false;
    }
}

void PetData::evolve() {
    switch(type) {
        case PetType::EGG: type = PetType::BABY; break;
        case PetType::BABY: type = PetType::CHILD; break;
        case PetType::CHILD: type = PetType::TEEN; break;
        case PetType::TEEN: type = PetType::ADULT; break;
        default: break;
    }
}
