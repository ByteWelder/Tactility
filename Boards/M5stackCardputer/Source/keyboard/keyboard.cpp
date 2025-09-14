/**
 * @file keyboard.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "keyboard.h"
#include <driver/gpio.h>


#define digitalWrite(pin, level) gpio_set_level((gpio_num_t)pin, level)
#define digitalRead(pin) gpio_get_level((gpio_num_t)pin)


using namespace KEYBOARD;


void Keyboard::_set_output(const std::vector<int>& pinList, uint8_t output)
{
    output = output & 0B00000111;

    digitalWrite(pinList[0], (output & 0B00000001));
    digitalWrite(pinList[1], (output & 0B00000010));
    digitalWrite(pinList[2], (output & 0B00000100));
}


uint8_t Keyboard::_get_input(const std::vector<int>& pinList)
{
    uint8_t buffer = 0x00;
    uint8_t pin_value = 0x00;

    for (int i = 0; i < 7; i++) 
    {
        pin_value = (digitalRead(pinList[i]) == 1) ? 0x00 : 0x01;
        pin_value = pin_value << i;
        buffer = buffer | pin_value;
    }

    return buffer;
}


void Keyboard::init()
{
    // for (auto i : output_list) {
    //     gpio_reset_pin((gpio_num_t)i);
    //     pinMode(i, OUTPUT);
    //     digitalWrite(i, 0);
    // }

    // for (auto i : input_list) {
    //     gpio_reset_pin((gpio_num_t)i);
    //     pinMode(i, INPUT_PULLUP);
    // }

    // _set_output(output_list, 0);


    for (auto i : output_list) 
    {
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_PULLDOWN);
        digitalWrite(i, 0);
    }

    for (auto i : input_list) 
    {
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_ONLY);
    }

    _set_output(output_list, 0);
}


Point2D_t Keyboard::getKey()
{
    Point2D_t coor;
    coor.x = -1;
    coor.y = -1;

    uint8_t input_value = 0;

    for (int i = 0; i < 8; i++) {
        _set_output(output_list, i);
        // printf("% 3d,\t", get_input(inputList));

        input_value = _get_input(input_list);

        /* If key pressed */
        if (input_value) {

            /* Get X */
            for (int j = 0; j < 7; j++) {
                if (input_value == X_map_chart[j].value) {
                    coor.x = (i > 3) ? X_map_chart[j].x_1 : X_map_chart[j].x_2;
                    break;
                }
            }

            /* Get Y */
            coor.y = (i > 3) ? (i - 4) : i;

            /* Keep the same as picture */
            coor.y = -coor.y;
            coor.y = coor.y + 3;


            break;
        }
    }

    // printf("%d,%d\n", x, y);
    return coor;
}


uint8_t Keyboard::getKeyNum(Point2D_t keyCoor)
{
    uint8_t ret = 0;

    if ((keyCoor.x < 0) || (keyCoor.y < 0)) {
        return 0;
    }

    ret = (keyCoor.y * 14) + (keyCoor.x + 1);

    return ret;
}


void Keyboard::updateKeyList()
{
    _key_list_buffer.clear();


    Point2D_t coor;

    uint8_t input_value = 0;

    for (int i = 0; i < 8; i++) {
        _set_output(output_list, i);
        // printf("% 3d,\t", get_input(inputList));

        input_value = _get_input(input_list);
        

        /* If key pressed */
        if (input_value) {

            // printf("c: ");
            
            /* Get X */
            for (int j = 0; j < 7; j++) {

                // if (input_value == X_map_chart[j].value) {
                //     coor.x = (i > 3) ? X_map_chart[j].x_1 : X_map_chart[j].x_2;
                //     break;
                // }



                if (input_value & (0x01 << j)) {
                    coor.x = (i > 3) ? X_map_chart[j].x_1 : X_map_chart[j].x_2;
                
                    /* Get Y */
                    coor.y = (i > 3) ? (i - 4) : i;
                    // printf("%d,%d\t", coor.x, coor.y); 


                    /* Keep the same as picture */
                    coor.y = -coor.y;
                    coor.y = coor.y + 3;

                    _key_list_buffer.push_back(coor);
                }


            }

            // printf("\n");


            // /* Get Y */
            // coor.y = (i > 3) ? (i - 4) : i;
        }
    }
}


bool Keyboard::isKeyPressing(int keyNum)
{
    if (_key_list_buffer.size())
    {
        for (const auto& i : _key_list_buffer)
        {
            if (getKeyNum(i) == keyNum)
                return true;
        }
    }
    return false;
}


#include <cstring>

void Keyboard::updateKeysState()
{
    _keys_state_buffer.reset();
    _key_values_without_special_keys.clear();

    // Get special keys 
    for (auto& i : _key_list_buffer)
    {
        if (strcmp(getKeyValue(i).value_first, "tab") == 0)
        {
            _keys_state_buffer.tab = true;
            continue;
        }

        if (strcmp(getKeyValue(i).value_first, "fn") == 0)
        {
            _keys_state_buffer.fn = true;
            continue;
        }

        if (strcmp(getKeyValue(i).value_first, "shift") == 0)
        {
            _keys_state_buffer.shift = true;
            continue;
        }

        if (strcmp(getKeyValue(i).value_first, "ctrl") == 0)
        {
            _keys_state_buffer.ctrl = true;
            continue;
        }
        if (strcmp(getKeyValue(i).value_first, "opt") == 0)
        {
            _keys_state_buffer.opt = true;
            continue;
        }

        if (strcmp(getKeyValue(i).value_first, "alt") == 0)
        {
            _keys_state_buffer.alt = true;
            continue;
        } 

        if (strcmp(getKeyValue(i).value_first, "del") == 0)
        {
            _keys_state_buffer.del = true;
            _keys_state_buffer.hidKey.push_back(KEY_BACKSPACE);
            continue;
        }

        if (strcmp(getKeyValue(i).value_first, "enter") == 0)
        {
            _keys_state_buffer.enter = true;
            _keys_state_buffer.hidKey.push_back(KEY_ENTER);
            continue;
        }

        if (strcmp(getKeyValue(i).value_first, "space") == 0)
        {
            _keys_state_buffer.space = true;
            _keys_state_buffer.hidKey.push_back(KEY_SPACE);
            continue;
        } 

        _key_values_without_special_keys.push_back(i);
    }

    // Deal what left
    for (auto& i : _key_values_without_special_keys)
    {
        if (_keys_state_buffer.ctrl || _keys_state_buffer.shift || _is_caps_locked)
        {
            _keys_state_buffer.values.push_back(*getKeyValue(i).value_second);
            _keys_state_buffer.hidKey.push_back(getKeyValue(i).value_num_second);
        }
        else
        {
            _keys_state_buffer.values.push_back(*getKeyValue(i).value_first);
            _keys_state_buffer.hidKey.push_back(getKeyValue(i).value_num_first);
        }
    }
}


bool Keyboard::isChanged() 
{
    if (_last_key_size != _key_list_buffer.size()) 
    {
        _last_key_size = _key_list_buffer.size();
        return true;
    } 
    return false;
}
