#ifndef ESP_PLATFORM

int gpio_get_level(int gpio_num) {
    return gpio_num % 3;
}

#endif
