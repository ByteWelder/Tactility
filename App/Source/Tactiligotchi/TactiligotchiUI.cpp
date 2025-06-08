#include "Tactiligotchi.h"
#include <ctime>
#include <fstream>

// Color definitions for pet states
#define PET_COLOR_HAPPY    lv_color_hex(0x00FF00)
#define PET_COLOR_NEUTRAL  lv_color_hex(0xFFFF00) 
#define PET_COLOR_SAD      lv_color_hex(0xFF8800)
#define PET_COLOR_SICK     lv_color_hex(0xFF0000)
#define PET_COLOR_SLEEPING lv_color_hex(0x8888FF)

void TamagotchiApp::onShow(AppContext& context, lv_obj_t* parent) {
    // Load saved pet data
    loadPetData();
    
    // Create main container
    pet_container = lv_obj_create(parent);
    lv_obj_set_size(pet_container, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(pet_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create toolbar
    lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create UI elements
    createPetDisplay(pet_container);
    createStatBars(pet_container);
    createActionButtons(pet_container);
    
    // Initial display update
    updatePetDisplay();
    updateStatBars();
    
    // Start update timer (every 30 seconds)
    update_timer = lv_timer_create(update_timer_cb, 30000, this);
}

void TamagotchiApp::onHide() {
    // Save pet data when app closes
    savePetData();
    
    // Clean up timer
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = nullptr;
    }
}

void TamagotchiApp::createPetDisplay(lv_obj_t* parent) {
    // Pet display area (top half of screen)
    lv_obj_t* pet_area = lv_obj_create(parent);
    lv_obj_set_size(pet_area, LV_PCT(100), LV_PCT(50));
    lv_obj_align(pet_area, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_clear_flag(pet_area, LV_OBJ_FLAG_SCROLLABLE);
    
    // Pet sprite (simple circle for now, can be replaced with images)
    pet_sprite = lv_obj_create(pet_area);
    lv_obj_set_size(pet_sprite, 80, 80);
    lv_obj_align(pet_sprite, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_radius(pet_sprite, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_flag(pet_sprite, LV_OBJ_FLAG_CLICKABLE);
    
    // Add click event for petting
    lv_obj_add_event_cb(pet_sprite, [](lv_event_t* e) {
        TamagotchiApp* app = (TamagotchiApp*)lv_event_get_user_data(e);
        app->petAnimal();
    }, LV_EVENT_CLICKED, this);
    
    // Status text
    status_label = lv_label_create(pet_area);
    lv_label_set_text(status_label, "Your Pet");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
}

void TamagotchiApp::createStatBars(lv_obj_t* parent) {
    // Stat bars container
    lv_obj_t* stats_area = lv_obj_create(parent);
    lv_obj_set_size(stats_area, LV_PCT(90), 100);
    lv_obj_align(stats_area, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(stats_area, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create stat bars with labels
    const char* stat_names[] = {"Hunger", "Happy", "Health", "Energy"};
    lv_obj_t** stat_bars[] = {&hunger_bar, &happiness_bar, &health_bar, &energy_bar};
    lv_color_t stat_colors[] = {
        lv_color_hex(0xFF6B35), // Orange for hunger
        lv_color_hex(0xF7931E), // Yellow for happiness  
        lv_color_hex(0x00A651), // Green for health
        lv_color_hex(0x0072CE)  // Blue for energy
    };
    
    for (int i = 0; i < 4; i++) {
        // Label
        lv_obj_t* label = lv_label_create(stats_area);
        lv_label_set_text(label, stat_names[i]);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, i * 20);
        
        // Progress bar
        *stat_bars[i] = lv_bar_create(stats_area);
        lv_obj_set_size(*stat_bars[i], 120, 15);
        lv_obj_align(*stat_bars[i], LV_ALIGN_TOP_LEFT, 60, i * 20);
        lv_obj_set_style_bg_color(*stat_bars[i], stat_colors[i], LV_PART_INDICATOR);
        lv_bar_set_range(*stat_bars[i], 0, 100);
    }
}

void TamagotchiApp::createActionButtons(lv_obj_t* parent) {
    // Button container (bottom of screen)
    lv_obj_t* btn_area = lv_obj_create(parent);
    lv_obj_set_size(btn_area, LV_PCT(100), 80);
    lv_obj_align(btn_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(btn_area, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create buttons in 2x2 grid
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
    // Update pet color based on mood
    lv_color_t pet_color;
    const char* status_text;
    
    switch (pet.mood) {
        case PetMood::HAPPY:
            pet_color = PET_COLOR_HAPPY;
            status_text = "😊 Happy!";
            break;
        case PetMood::SAD:
            pet_color = PET_COLOR_SAD;
            status_text = "😢 Sad...";
            break;
        case PetMood::SICK:
            pet_color = PET_COLOR_SICK;
            status_text = "🤒 Sick!";
            break;
        case PetMood::SLEEPING:
            pet_color = PET_COLOR_SLEEPING;
            status_text = "😴 Sleeping";
            break;
        default:
            pet_color = PET_COLOR_NEUTRAL;
            status_text = "😐 Okay";
            break;
    }
    
    lv_obj_set_style_bg_color(pet_sprite, pet_color, 0);
    lv_label_set_text(status_label, status_text);
    
    // Add cleaning indicator
    if (pet.needs_cleaning) {
        lv_label_set_text_fmt(status_label, "%s\n💩 Needs cleaning!", status_text);
    }
    
    // Size changes based on pet type
    int size = 60 + (int)pet.type * 10; // Grows as it evolves
    lv_obj_set_size(pet_sprite, size, size);
}

void TamagotchiApp::updateStatBars() {
    lv_bar_set_value(hunger_bar, pet.hunger, LV_ANIM_ON);
    lv_bar_set_value(happiness_bar, pet.happiness, LV_ANIM_ON);
    lv_bar_set_value(health_bar, pet.health, LV_ANIM_ON);
    lv_bar_set_value(energy_bar, pet.energy, LV_ANIM_ON);
}

// Animation helper for pet interactions
void TamagotchiApp::animatePet(lv_color_t flash_color) {
    // Flash the pet sprite briefly
    lv_obj_set_style_bg_color(pet_sprite, flash_color, 0);
    
    // Scale animation
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, pet_sprite);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t val) {
        lv_obj_set_size((lv_obj_t*)obj, val, val);
    });
    
    int current_size = 60 + (int)pet.type * 10;
    lv_anim_set_values(&anim, current_size, current_size + 10);
    lv_anim_set_time(&anim, 200);
    lv_anim_set_playback_time(&anim, 200);
    lv_anim_set_repeat_count(&anim, 1);
    lv_anim_start(&anim);
    
    // Reset color after animation
    lv_timer_create([](lv_timer_t* timer) {
        TamagotchiApp* app = (TamagotchiApp*)timer->user_data;
        app->updatePetDisplay();
        lv_timer_del(timer);
    }, 500, this);
}

// Game actions
void TamagotchiApp::feedPet() {
    if (pet.hunger >= 95) return; // Already full
    
    pet.hunger = std::min(100, pet.hunger + 25);
    pet.happiness = std::min(100, pet.happiness + 5);
    pet.times_fed++;
    
    animatePet(lv_color_hex(0x00FF00)); // Green flash
    updateStatBars();
    savePetData();
}

void TamagotchiApp::petAnimal() {
    pet.happiness = std::min(100, pet.happiness + 3);
    animatePet(lv_color_hex(0xFFFF00)); // Yellow flash
    updateStatBars();
}

void TamagotchiApp::cleanPet() {
    if (!pet.needs_cleaning) return;
    
    pet.needs_cleaning = false;
    pet.happiness = std::min(100, pet.happiness + 10);
    pet.health = std::min(100, pet.health + 5);
    pet.times_cleaned++;
    
    animatePet(lv_color_hex(0x00FFFF)); // Cyan flash
    updatePetDisplay();
    updateStatBars();
    savePetData();
}

void TamagotchiApp::putPetToSleep() {
    pet.energy = 100;
    pet.mood = PetMood::SLEEPING;
    
    animatePet(lv_color_hex(0x8888FF)); // Purple flash
    updatePetDisplay();
    updateStatBars();
    savePetData();
}
