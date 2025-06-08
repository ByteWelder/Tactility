// Mini-game implementations and save system

// Simple pattern matching game
class PatternGame {
private:
    TamagotchiApp* parent_app;
    lv_obj_t* game_container;
    lv_obj_t* pattern_display;
    lv_obj_t* input_buttons[4];
    
    int pattern[8] = {0}; // Pattern to remember
    int pattern_length = 3;
    int current_input = 0;
    int score = 0;
    bool showing_pattern = true;
    lv_timer_t* pattern_timer;
    
public:
    PatternGame(TamagotchiApp* app) : parent_app(app) {}
    
    void startGame(lv_obj_t* parent) {
        // Create game container
        game_container = lv_obj_create(parent);
        lv_obj_set_size(game_container, LV_PCT(100), LV_PCT(100));
        lv_obj_clear_flag(game_container, LV_OBJ_FLAG_SCROLLABLE);
        
        // Pattern display area
        pattern_display = lv_label_create(game_container);
        lv_label_set_text(pattern_display, "Watch the pattern!");
        lv_obj_align(pattern_display, LV_ALIGN_TOP_MID, 0, 50);
        
        // Create colored input buttons
        lv_color_t colors[] = {
            lv_color_hex(0xFF0000), // Red
            lv_color_hex(0x00FF00), // Green  
            lv_color_hex(0x0000FF), // Blue
            lv_color_hex(0xFFFF00)  // Yellow
        };
        
        for (int i = 0; i < 4; i++) {
            input_buttons[i] = lv_btn_create(game_container);
            lv_obj_set_size(input_buttons[i], 60, 60);
            lv_obj_align(input_buttons[i], LV_ALIGN_CENTER, 
                        (i % 2) * 80 - 40, (i / 2) * 80 - 40);
            lv_obj_set_style_bg_color(input_buttons[i], colors[i], 0);
            
            // Add click handler
            lv_obj_add_event_cb(input_buttons[i], [](lv_event_t* e) {
                PatternGame* game = (PatternGame*)lv_event_get_user_data(e);
                int btn_id = (int)(intptr_t)lv_event_get_param(e);
                game->buttonPressed(btn_id);
            }, LV_EVENT_CLICKED, this);
            
            // Store button ID in param
            lv_obj_set_user_data(input_buttons[i], (void*)(intptr_t)i);
        }
        
        // Close button
        lv_obj_t* close_btn = lv_btn_create(game_container);
        lv_obj_set_size(close_btn, 60, 30);
        lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
        
        lv_obj_t* close_label = lv_label_create(close_btn);
        lv_label_set_text(close_label, "Exit");
        lv_obj_center(close_label);
        
        lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
            PatternGame* game = (PatternGame*)lv_event_get_user_data(e);
            game->endGame();
        }, LV_EVENT_CLICKED, this);
        
        generatePattern();
        showPattern();
    }
    
    void generatePattern() {
        for (int i = 0; i < pattern_length; i++) {
            pattern[i] = rand() % 4;
        }
        current_input = 0;
        showing_pattern = true;
    }
    
    void showPattern() {
        lv_label_set_text(pattern_display, "Watch carefully!");
        
        // Show pattern sequence
        pattern_timer = lv_timer_create([](lv_timer_t* timer) {
            PatternGame* game = (PatternGame*)timer->user_data;
            static int step = 0;
            
            if (step < game->pattern_length) {
                // Flash the button
                int btn_id = game->pattern[step];
                lv_obj_add_state(game->input_buttons[btn_id], LV_STATE_PRESSED);
                
                // Un-flash after 300ms
                lv_timer_create([](lv_timer_t* t) {
                    PatternGame* g = (PatternGame*)t->user_data;
                    for (int i = 0; i < 4; i++) {
                        lv_obj_clear_state(g->input_buttons[i], LV_STATE_PRESSED);
                    }
                    lv_timer_del(t);
                }, 300, game);
                
                step++;
            } else {
                // Pattern shown, now player's turn
                game->showing_pattern = false;
                lv_label_set_text(game->pattern_display, "Your turn! Repeat the pattern");
                step = 0;
                lv_timer_del(timer);
            }
        }, 600, this);
    }
    
    void buttonPressed(int button_id) {
        if (showing_pattern) return;
        
        if (button_id == pattern[current_input]) {
            current_input++;
            
            if (current_input >= pattern_length) {
                // Pattern completed successfully!
                score++;
                lv_label_set_text_fmt(pattern_display, "Great! Score: %d", score);
                
                // Make next pattern harder
                if (pattern_length < 8) pattern_length++;
                
                // Reward the pet
                parent_app->gameSuccess();
                
                // Start next round after delay
                lv_timer_create([](lv_timer_t* timer) {
                    PatternGame* game = (PatternGame*)timer->user_data;
                    game->generatePattern();
                    game->showPattern();
                    lv_timer_del(timer);
                }, 1500, this);
            }
        } else {
            // Wrong button pressed
            lv_label_set_text(pattern_display, "Wrong! Try again");
            parent_app->gameFailed();
            
            // Restart after delay
            lv_timer_create([](lv_timer_t* timer) {
                PatternGame* game = (PatternGame*)timer->user_data;
                game->pattern_length = std::max(3, game->pattern_length - 1);
                game->generatePattern();
                game->showPattern();
                lv_timer_del(timer);
            }, 1500, this);
        }
    }
    
    void endGame() {
        if (pattern_timer) {
            lv_timer_del(pattern_timer);
            pattern_timer = nullptr;
        }
        lv_obj_del(game_container);
        parent_app->endMiniGame();
    }
};

// Add to TamagotchiApp class:
void TamagotchiApp::playWithPet() {
    if (pet.energy < 20) {
        // Too tired to play
        lv_label_set_text(status_label, "Too tired to play!");
        return;
    }
    
    // Start mini-game
    PatternGame* game = new PatternGame(this);
    game->startGame(pet_container);
    current_minigame = game;
}

void TamagotchiApp::gameSuccess() {
    pet.happiness = std::min(100, pet.happiness + 15);
    pet.energy = std::max(0, pet.energy - 10);
    pet.times_played++;
    
    animatePet(lv_color_hex(0x00FF00)); // Green flash for success
    updateStatBars();
    savePetData();
}

void TamagotchiApp::gameFailed() {
    pet.energy = std::max(0, pet.energy - 5);
    pet.happiness = std::max(0, pet.happiness - 5);
    
    animatePet(lv_color_hex(0xFF0000)); // Red flash for failure
    updateStatBars();
}

void TamagotchiApp::endMiniGame() {
    if (current_minigame) {
        delete current_minigame;
        current_minigame = nullptr;
    }
    updatePetDisplay();
}

// Save/Load System
void TamagotchiApp::savePetData() {
    // Save to SD card as JSON-like format
    std::ofstream file("/sd/tamagotchi_save.dat", std::ios::binary);
    if (file.is_open()) {
        // Update timestamp before saving
        pet.last_update_time = lv_tick_get();
        
        file.write((char*)&pet, sizeof(PetData));
        file.close();
    }
}

void TamagotchiApp::loadPetData() {
    std::ifstream file("/sd/tamagotchi_save.dat", std::ios::binary);
    if (file.is_open()) {
        file.read((char*)&pet, sizeof(PetData));
        file.close();
        
        // Update pet stats based on time elapsed
        uint32_t current_time = lv_tick_get();
        pet.updateStats(current_time);
    } else {
        // New pet - start with egg
        pet = PetData(); // Default values
        pet.last_update_time = lv_tick_get();
    }
}

// Event callback implementations
void TamagotchiApp::feed_btn_cb(lv_event_t* e) {
    TamagotchiApp* app = (TamagotchiApp*)lv_event_get_user_data(e);
    app->feedPet();
}

void TamagotchiApp::play_btn_cb(lv_event_t* e) {
    TamagotchiApp* app = (TamagotchiApp*)lv_event_get_user_data(e);
    app->playWithPet();
}

void TamagotchiApp::clean_btn_cb(lv_event_t* e) {
    TamagotchiApp* app = (TamagotchiApp*)lv_event_get_user_data(e);
    app->cleanPet();
}

void TamagotchiApp::sleep_btn_cb(lv_event_t* e) {
    TamagotchiApp* app = (TamagotchiApp*)lv_event_get_user_data(e);
    app->putPetToSleep();
}

void TamagotchiApp::update_timer_cb(lv_timer_t* timer) {
    TamagotchiApp* app = (TamagotchiApp*)timer->user_data;
    
    // Update pet stats
    uint32_t current_time = lv_tick_get();
    app->pet.updateStats(current_time);
    
    // Refresh display
    app->updatePetDisplay();
    app->updateStatBars();
    
    // Auto-save periodically
    app->savePetData();
}
