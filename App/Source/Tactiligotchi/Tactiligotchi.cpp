#include "Tactiligotchi.h"
#include "PatternGame.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "lvgl.h"
#include "Tactility/Timer.h"
#include <Tactility/app/AppManifest.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/lvgl/Toolbar.h>

using namespace tt::app;
using namespace tt::lvgl;

#define PET_COLOR_HAPPY    lv_color_hex(0x00FF00)
#define PET_COLOR_NEUTRAL  lv_color_hex(0xFFFF00)
#define PET_COLOR_SAD      lv_color_hex(0xFF8800)
#define PET_COLOR_SICK     lv_color_hex(0xFF0000)
#define PET_COLOR_SLEEPING lv_color_hex(0x8888FF)

void PetData::updateStats(uint32_t current_time) {
    if (current_time < last_update_time) return;
    uint32_t elapsed_minutes = (current_time - last_update_time) / 60000;
    if (elapsed_minutes == 0) return;

    hunger = std::max(0, hunger - static_cast<int>(elapsed_minutes * 2));
    happiness = std::max(0, happiness - static_cast<int>(elapsed_minutes));
    energy = std::max(0, energy - static_cast<int>(elapsed_minutes));
    health = std::max(0, health - static_cast<int>(elapsed_minutes * (needs_cleaning ? 2 : 1)));

    age_minutes += elapsed_minutes;
    last_update_time = current_time;

    if (needsEvolution()) evolve();
    mood = calculateMood();
}

PetMood PetData::calculateMood() const {
    if (health < 30 || hunger < 20) return PetMood::SICK;
    if (energy < 20) return PetMood::SLEEPING;
    if (happiness < 40 || hunger < 40) return PetMood::SAD;
    if (happiness > 70 && hunger > 70 && health > 70) return PetMood::HAPPY;
    return PetMood::NEUTRAL;
}

bool PetData::needsEvolution() const {
    if (type == PetType::EGG && age_minutes >= 5) return true;
    if (type == PetType::BABY && age_minutes >= 60) return true;
    if (type == PetType::CHILD && age_minutes >= 180) return true;
    if (type == PetType::TEEN && age_minutes >= 360) return true;
    return false;
}

void PetData::evolve() {
    switch (type) {
        case PetType::EGG: type = PetType::BABY; break;
        case PetType::BABY: type = PetType::CHILD; break;
        case PetType::CHILD: type = PetType::TEEN; break;
        case PetType::TEEN: type = PetType::ADULT; break;
        default: break;
    }
}

void TamagotchiApp::onShow(AppContext& context, lv_obj_t* parent) {
    loadPetData();

    pet_container = lv_obj_create(parent);
    lv_obj_set_size(pet_container, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(pet_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* toolbar = toolbar_create(parent, context);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    createPetDisplay(pet_container);
    createStatBars(pet_container);
    createActionButtons(pet_container);

    updatePetDisplay();
    updateStatBars();

    update_timer = new tt::Timer(tt::Timer::Type::Periodic, [this]() {
        update_timer_cb();
    });
    update_timer->start(pdMS_TO_TICKS(30000));
}

void TamagotchiApp::onHide(AppContext& context) {
    savePetData();
    if (update_timer) {
        update_timer->stop();
        delete update_timer;
        update_timer = nullptr;
    }
    endMiniGame();
}

void TamagotchiApp::createPetDisplay(lv_obj_t* parent) {
    lv_obj_t* pet_area = lv_obj_create(parent);
    lv_obj_set_size(pet_area, LV_PCT(100), LV_PCT(50));
    lv_obj_align(pet_area, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_clear_flag(pet_area, LV_OBJ_FLAG_SCROLLABLE);

    pet_sprite = lv_obj_create(pet_area);
    lv_obj_set_size(pet_sprite, 80, 80);
    lv_obj_align(pet_sprite, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_radius(pet_sprite, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_flag(pet_sprite, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(pet_sprite, [](lv_event_t* e) {
        auto* app = static_cast<TamagotchiApp*>(lv_event_get_user_data(e));
        app->petAnimal();
    }, LV_EVENT_CLICKED, this);

    status_label = lv_label_create(pet_area);
    lv_label_set_text(status_label, "Your Pet");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
}

void TamagotchiApp::createStatBars(lv_obj_t* parent) {
    lv_obj_t* stats_area = lv_obj_create(parent);
    lv_obj_set_size(stats_area, LV_PCT(90), 100);
    lv_obj_align(stats_area, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(stats_area, LV_OBJ_FLAG_SCROLLABLE);

    const char* stat_names[] = {"Hunger", "Happy", "Health", "Energy"};
    lv_obj_t** stat_bars[] = {&hunger_bar, &happiness_bar, &health_bar, &energy_bar};
    lv_color_t stat_colors[] = {
        lv_color_hex(0xFF6B35),
        lv_color_hex(0xF7931E),
        lv_color_hex(0x00A651),
        lv_color_hex(0x0072CE)
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t* label = lv_label_create(stats_area);
        lv_label_set_text(label, stat_names[i]);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, i * 25);

        *stat_bars[i] = lv_bar_create(stats_area);
        lv_obj_set_size(*stat_bars[i], 120, 15);
        lv_obj_align(*stat_bars[i], LV_ALIGN_TOP_LEFT, 60, i * 25);
        lv_obj_set_style_bg_color(*stat_bars[i], stat_colors[i], LV_PART_INDICATOR);
        lv_bar_set_range(*stat_bars[i], 0, 100);
    }
}

void TamagotchiApp::createActionButtons(lv_obj_t* parent) {
    lv_obj_t* btn_area = lv_obj_create(parent);
    lv_obj_set_size(btn_area, LV_PCT(100), 80);
    lv_obj_align(btn_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(btn_area, LV_OBJ_FLAG_SCROLLABLE);

    struct ButtonInfo {
        lv_obj_t** btn;
        const char* text;
        lv_event_cb_t callback;
        int x, y;
    };

    ButtonInfo buttons[] = {
        {&feed_btn, "Feed", feed_btn_cb, -60, -20},
        {&play_btn, "Play", play_btn_cb, 60, -20},
        {&clean_btn, "Clean", clean_btn_cb, -60, 20},
        {&sleep_btn, "Sleep", sleep_btn_cb, 60, 20}
    };

    for (auto& btn_info : buttons) {
        *btn_info.btn = lv_btn_create(btn_area);
        lv_obj_set_size(*btn_info.btn, 80, 30);
        lv_obj_align(*btn_info.btn, LV_ALIGN_CENTER, btn_info.x, btn_info.y);

        lv_obj_t* label = lv_label_create(*btn_info.btn);
        lv_label_set_text(label, btn_info.text);
        lv_obj_center(label);

        lv_obj_add_event_cb(*btn_info.btn, btn_info.callback, LV_EVENT_CLICKED, this);
    }
}

void TamagotchiApp::updatePetDisplay() {
    lv_color_t pet_color;
    const char* status_text;

    switch (pet.mood) {
        case PetMood::HAPPY:    pet_color = PET_COLOR_HAPPY; status_text = "😊 Happy!"; break;
        case PetMood::SAD:      pet_color = PET_COLOR_SAD;   status_text = "😢 Sad..."; break;
        case PetMood::SICK:     pet_color = PET_COLOR_SICK;  status_text = "🤒 Sick!"; break;
        case PetMood::SLEEPING: pet_color = PET_COLOR_SLEEPING; status_text = "😴 Sleeping"; break;
        default:                pet_color = PET_COLOR_NEUTRAL; status_text = "😐 Okay"; break;
    }

    lv_obj_set_style_bg_color(pet_sprite, pet_color, 0);

    if (pet.needs_cleaning) {
        lv_label_set_text_fmt(status_label, "%s\n💩 Needs cleaning!", status_text);
    } else {
        lv_label_set_text(status_label, status_text);
    }

    int size = 60 + static_cast<int>(pet.type) * 10;
    lv_obj_set_size(pet_sprite, size, size);
}

void TamagotchiApp::updateStatBars() {
    lv_bar_set_value(hunger_bar, pet.hunger, LV_ANIM_ON);
    lv_bar_set_value(happiness_bar, pet.happiness, LV_ANIM_ON);
    lv_bar_set_value(health_bar, pet.health, LV_ANIM_ON);
    lv_bar_set_value(energy_bar, pet.energy, LV_ANIM_ON);
}

void TamagotchiApp::animatePet(lv_color_t flash_color) {
    lv_obj_set_style_bg_color(pet_sprite, flash_color, 0);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, pet_sprite);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t val) {
        lv_obj_set_size(static_cast<lv_obj_t*>(obj), val, val);
    });

    int current_size = 60 + static_cast<int>(pet.type) * 10;
    lv_anim_set_values(&anim, current_size, current_size + 10);
    lv_anim_set_time(&anim, 200);
    lv_anim_set_playback_time(&anim, 200);
    lv_anim_set_repeat_count(&anim, 1);
    lv_anim_start(&anim);

    auto* anim_timer = new tt::Timer(tt::Timer::Type::Once, [this]() {
        updatePetDisplay();
    });
    anim_timer->start(pdMS_TO_TICKS(500));
}

void TamagotchiApp::feedPet() {
    if (pet.hunger >= 95) return;
    pet.hunger = std::min(100, pet.hunger + 25);
    pet.happiness = std::min(100, pet.happiness + 5);
    pet.times_fed++;

    animatePet(lv_color_hex(0x00FF00));
    updateStatBars();
    savePetData();
}

void TamagotchiApp::petAnimal() {
    pet.happiness = std::min(100, pet.happiness + 3);
    animatePet(lv_color_hex(0xFFFF00));
    updateStatBars();
}

void TamagotchiApp::cleanPet() {
    if (!pet.needs_cleaning) return;
    pet.needs_cleaning = false;
    pet.happiness = std::min(100, pet.happiness + 10);
    pet.health = std::min(100, pet.health + 5);
    pet.times_cleaned++;

    animatePet(lv_color_hex(0x00FFFF));
    updatePetDisplay();
    updateStatBars();
    savePetData();
}

void TamagotchiApp::putPetToSleep() {
    pet.energy = 100;
    pet.mood = PetMood::SLEEPING;

    animatePet(lv_color_hex(0x8888FF));
    updatePetDisplay();
    updateStatBars();
    savePetData();
}

void TamagotchiApp::update_timer_cb() {
    uint32_t current_time = lv_tick_get();
    pet.updateStats(current_time);
    updatePetDisplay();
    updateStatBars();
    savePetData();
}

void TamagotchiApp::feed_btn_cb(lv_event_t* e) {
    auto* app = static_cast<TamagotchiApp*>(lv_event_get_user_data(e));
    app->feedPet();
}

void TamagotchiApp::play_btn_cb(lv_event_t* e) {
    auto* app = static_cast<TamagotchiApp*>(lv_event_get_user_data(e));
    app->playWithPet();
}

void TamagotchiApp::clean_btn_cb(lv_event_t* e) {
    auto* app = static_cast<TamagotchiApp*>(lv_event_get_user_data(e));
    app->cleanPet();
}

void TamagotchiApp::sleep_btn_cb(lv_event_t* e) {
    auto* app = static_cast<TamagotchiApp*>(lv_event_get_user_data(e));
    app->putPetToSleep();
}

void TamagotchiApp::playWithPet() {
    if (pet.energy < 20) {
        lv_label_set_text(status_label, "Too tired to play!");
        return;
    }

    endMiniGame();

    current_minigame = new PatternGame(this);
    current_minigame->startGame(pet_container);
}

void TamagotchiApp::gameSuccess() {
    pet.happiness = std::min(100, pet.happiness + 15);
    pet.energy = std::max(0, pet.energy - 10);
    pet.times_played++;

    animatePet(lv_color_hex(0x00FF00));
    updateStatBars();
    savePetData();
}

void TamagotchiApp::gameFailed() {
    pet.energy = std::max(0, pet.energy - 5);
    pet.happiness = std::max(0, pet.happiness - 5);

    animatePet(lv_color_hex(0xFF0000));
    updateStatBars();
}

void TamagotchiApp::endMiniGame() {
    if (current_minigame) {
        current_minigame->endGame();
        delete current_minigame;
        current_minigame = nullptr;
    }
    updatePetDisplay();
}

void TamagotchiApp::savePetData() {
    std::ofstream file("/sd/tamagotchi_save.dat", std::ios::binary);
    if (file.is_open()) {
        pet.last_update_time = lv_tick_get();
        file.write(reinterpret_cast<const char*>(&pet), sizeof(PetData));
        file.close();
    }
}

void TamagotchiApp::loadPetData() {
    std::ifstream file("/sd/tamagotchi_save.dat", std::ios::binary);
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(&pet), sizeof(PetData));
        file.close();

        uint32_t current_time = lv_tick_get();
        pet.updateStats(current_time);
    } else {
        pet = PetData();
        pet.last_update_time = lv_tick_get();
    }
}

const AppManifest tactiligotchi_app = {
    .id = "Tactiligotchi",
    .name = "Tactiligotchi",
    .createApp = create<TamagotchiApp>
};
