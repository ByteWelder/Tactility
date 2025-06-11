#include "Tactiligotchi.h"
#include "PatternGame.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "lvgl.h"
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>

using namespace tt::app;
using namespace tt::lvgl;

// Colors
#define PET_COLOR_HAPPY    lv_color_hex(0x00FF00)
#define PET_COLOR_NEUTRAL  lv_color_hex(0xFFFF00)
#define PET_COLOR_SAD      lv_color_hex(0xFF8800)
#define PET_COLOR_SICK     lv_color_hex(0xFF0000)
#define PET_COLOR_SLEEPING lv_color_hex(0x8888FF)

void TamagotchiApp::onShow(AppContext& context, lv_obj_t* parent) {
    loadPetData();

    pet_container = lv_obj_create(parent);
    lv_obj_set_size(pet_container, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(pet_container, LV_OBJ_FLAG_SCROLLABLE);

    auto* tb = toolbar_create(parent, context);
    lv_obj_align(tb, LV_ALIGN_TOP_MID, 0, 0);

    createPetDisplay(pet_container);
    createStatBars(pet_container);
    createActionButtons(pet_container);

    updatePetDisplay();
    updateStatBars();

    // Create a periodic Tactility timer (30 000 ms)
    update_timer = std::make_unique<tt::Timer>(
        tt::Timer::Type::Periodic,
        [this]() {
            // Defer UI updates to LVGL thread
            lv_async_call(
                [](void* user_data) {
                    auto* app = static_cast<TamagotchiApp*>(user_data);
                    uint32_t now = lv_tick_get();
                    app->pet.updateStats(now);
                    app->updatePetDisplay();
                    app->updateStatBars();
                    app->savePetData();
                },
                this
            );
        }
    );
    update_timer->start(pdMS_TO_TICKS(30000));
}

void TamagotchiApp::onHide(AppContext& /*context*/) {
    savePetData();
    // Destroy timer
    update_timer.reset();
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

    lv_obj_add_event_cb(pet_sprite,
        [](lv_event_t* e) {
            auto* app = static_cast<TamagotchiApp*>(lv_event_get_user_data(e));
            app->petAnimal();
        },
        LV_EVENT_CLICKED,
        this
    );

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

    const char* names[] = {"Hunger","Happy","Health","Energy"};
    lv_obj_t** bars[] = {&hunger_bar,&happiness_bar,&health_bar,&energy_bar};
    lv_color_t colors[] = {
        lv_color_hex(0xFF6B35),
        lv_color_hex(0xF7931E),
        lv_color_hex(0x00A651),
        lv_color_hex(0x0072CE)
    };

    for(int i=0;i<4;i++){
        auto* lbl = lv_label_create(stats_area);
        lv_label_set_text(lbl, names[i]);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, i*25);

        *bars[i] = lv_bar_create(stats_area);
        lv_obj_set_size(*bars[i], 120, 15);
        lv_obj_align(*bars[i], LV_ALIGN_TOP_LEFT, 60, i*25);
        lv_obj_set_style_bg_color(*bars[i], colors[i], LV_PART_INDICATOR);
        lv_bar_set_range(*bars[i], 0, 100);
    }
}

void TamagotchiApp::createActionButtons(lv_obj_t* parent) {
    lv_obj_t* btn_area = lv_obj_create(parent);
    lv_obj_set_size(btn_area, LV_PCT(100), 80);
    lv_obj_align(btn_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(btn_area, LV_OBJ_FLAG_SCROLLABLE);

    struct Info { lv_obj_t** btn; const char* text; lv_event_cb_t cb; int x,y; };
    Info buttons[] = {
        {&feed_btn,  "Feed",  feed_btn_cb,  -60, -20},
        {&play_btn,  "Play",  play_btn_cb,   60, -20},
        {&clean_btn, "Clean", clean_btn_cb, -60,  20},
        {&sleep_btn, "Sleep", sleep_btn_cb,  60,  20}
    };

    for(auto& b:buttons){
        *b.btn = lv_btn_create(btn_area);
        lv_obj_set_size(*b.btn, 80, 30);
        lv_obj_align(*b.btn, LV_ALIGN_CENTER, b.x, b.y);

        auto* lbl = lv_label_create(*b.btn);
        lv_label_set_text(lbl, b.text);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(*b.btn, b.cb, LV_EVENT_CLICKED, this);
    }
}

void TamagotchiApp::updatePetDisplay() {
    lv_color_t color;
    const char* txt;
    switch(pet.mood) {
      case PetMood::HAPPY:    color=PET_COLOR_HAPPY;    txt="😊 Happy!"; break;
      case PetMood::SAD:      color=PET_COLOR_SAD;      txt="😢 Sad..."; break;
      case PetMood::SICK:     color=PET_COLOR_SICK;     txt="🤒 Sick!"; break;
      case PetMood::SLEEPING: color=PET_COLOR_SLEEPING; txt="😴 Sleeping"; break;
      default:                color=PET_COLOR_NEUTRAL;  txt="😐 Okay"; break;
    }
    lv_obj_set_style_bg_color(pet_sprite, color, 0);
    if(pet.needs_cleaning) {
        lv_label_set_text_fmt(status_label, "%s\n💩 Needs cleaning!", txt);
    } else {
        lv_label_set_text(status_label, txt);
    }
    int size = 60 + static_cast<int>(pet.type)*10;
    lv_obj_set_size(pet_sprite, size, size);
}

void TamagotchiApp::updateStatBars() {
    lv_bar_set_value(hunger_bar,   pet.hunger,   LV_ANIM_ON);
    lv_bar_set_value(happiness_bar,pet.happiness,LV_ANIM_ON);
    lv_bar_set_value(health_bar,   pet.health,   LV_ANIM_ON);
    lv_bar_set_value(energy_bar,   pet.energy,   LV_ANIM_ON);
}

void TamagotchiApp::animatePet(lv_color_t flash) {
    lv_obj_set_style_bg_color(pet_sprite, flash, 0);
    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a, pet_sprite);
    lv_anim_set_exec_cb(&a, [](void* o,int32_t v){
        lv_obj_set_size(static_cast<lv_obj_t*>(o), v, v);
    });
    int cur = 60 + static_cast<int>(pet.type)*10;
    lv_anim_set_values(&a, cur, cur+10);
    lv_anim_set_time(&a,        200);
    lv_anim_set_playback_time(&a,200);
    lv_anim_set_repeat_count(&a,1);
    lv_anim_start(&a);

    // flash restore after 500 ms
    lv_async_call(
      [](void* ud){
        auto* app = static_cast<TamagotchiApp*>(ud);
        app->updatePetDisplay();
      },
      this
    );
}

void TamagotchiApp::feedPet()   { /* ...copy from before...*/ }
void TamagotchiApp::petAnimal() { /* ... 〃 ... */ }
void TamagotchiApp::cleanPet()  { /* ... 〃 ... */ }
void TamagotchiApp::putPetToSleep() { /* ... 〃 ... */ }

void TamagotchiApp::feed_btn_cb(lv_event_t* e){
    static_cast<TamagotchiApp*>(lv_event_get_user_data(e))->feedPet();
}
void TamagotchiApp::play_btn_cb(lv_event_t* e){
    static_cast<TamagotchiApp*>(lv_event_get_user_data(e))->playWithPet();
}
void TamagotchiApp::clean_btn_cb(lv_event_t* e){
    static_cast<TamagotchiApp*>(lv_event_get_user_data(e))->cleanPet();
}
void TamagotchiApp::sleep_btn_cb(lv_event_t* e){
    static_cast<TamagotchiApp*>(lv_event_get_user_data(e))->putPetToSleep();
}

void TamagotchiApp::playWithPet(){
    if(pet.energy<20){
        lv_label_set_text(status_label,"Too tired to play!"); return;
    }
    endMiniGame();
    current_minigame = new PatternGame(this);
    current_minigame->startGame(pet_container);
}

void TamagotchiApp::gameSuccess(){
    pet.happiness = std::min(100, pet.happiness + 15);
    pet.energy    = std::max(0,   pet.energy    - 10);
    pet.times_played++;
    animatePet(PET_COLOR_HAPPY);
    updateStatBars();
    savePetData();
}

void TamagotchiApp::gameFailed(){
    pet.energy    = std::max(0,   pet.energy   - 5);
    pet.happiness = std::max(0,   pet.happiness- 5);
    animatePet(PET_COLOR_SAD);
    updateStatBars();
}

void TamagotchiApp::endMiniGame(){
    if(current_minigame){
        current_minigame->endGame();
        delete current_minigame;
        current_minigame = nullptr;
    }
    updatePetDisplay();
}

void TamagotchiApp::savePetData(){
    std::ofstream f("/sd/tamagotchi_save.dat",std::ios::binary);
    if(f.is_open()){
        pet.last_update_time = lv_tick_get();
        f.write(reinterpret_cast<char*>(&pet), sizeof(pet));
    }
}

void TamagotchiApp::loadPetData(){
    std::ifstream f("/sd/tamagotchi_save.dat",std::ios::binary);
    if(f.is_open()){
        f.read(reinterpret_cast<char*>(&pet), sizeof(pet));
        uint32_t now = lv_tick_get();
        pet.updateStats(now);
    } else {
        pet = PetData{};
        pet.last_update_time = lv_tick_get();
    }
}

// Manifest registration
const AppManifest tactiligotchi_app = {
    .id       = "Tactiligotchi",
    .name     = "Tactiligotchi",
    .createApp= create<TamagotchiApp>
};
