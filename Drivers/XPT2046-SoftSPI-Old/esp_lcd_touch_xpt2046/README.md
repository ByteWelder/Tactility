# esp_lcd_touch driver for XPT2046 devices

Implementation of the XPT2046 Touch controller with esp_lcd_touch component. 

| Touch controller | Communication interface | Component name | Link to datasheet |
| :------------: | :---------------------: | :------------: | :---------------: |
| XPT2046        | SPI                     | esp_lcd_touch_xpt2046     | [Specification](https://grobotronics.com/images/datasheets/xpt2046-datasheet.pdf) |

## Adding this component in your project

This package can be added to your project in two ways:

1. Using [Espressif's component service](https://components.espressif.com/) as:
```
dependencies:
  atanisoft/esp_lcd_touch_xpt2046: "~1.0.0"
```

2. Using the git repository directly:

```
dependencies:
  esp_lcd_touch_xpt2046:
    git: https://github.com/atanisoft/esp_lcd_touch_xpt2046.git
```

For more information on the usage of the `idf_component.yml` file please refer to [Espressif's documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

## Supported platforms

At this time testing is limited to ESP32 and ESP32-S3, other ESP32 variants should work but are not tested.

## Configuration options

`Kconfig.projbuild` contains a handful of options to allow customization of the XPT2046 interface:

* XPT2046_Z_THRESHOLD - This is the minimum ADC threshold to use for detecting touch points.
* XPT2046_INTERRUPT_MODE - This option enables / disables the PENIRQ output from the chip. If enabled (and the `int_gpio_num` is set) the driver can check for touch more efficiently by reading the input.
* XPT2046_VREF_ON_MODE - This option enables / disables keeping the internal Vref permanently enabled. This uses more power, but requires fewer SPI transactions when reading the battery voltage, aux voltage and temperature.
* XPT2046_CONVERT_ADC_TO_COORDS - This option enables / disables the conversion of raw ADC values into screen coordinates. When disabled it will be necessary to define the `process_coordinates` method in `esp_lcd_touch_config_t`. This can be useful for applying calibration or other offsets to adjust the screen coordinates.
* XPT2046_ENABLE_LOCKING - This option enables / disables the usage of critical sections to protect internal data structures. This has been seen to cause conflicts with other components at times.

## Example usage

### Initialization

```
    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(TOUCH_CS_PIN);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
```

### Updating touch point data

This will read new data from the touch controller and store it in SRAM. This method
should be called on a regular basis.

```
    ESP_ERROR_CHECK(esp_lcd_touch_read_data(tp));
```

### Retrieving touch point(s)

This will retrieve the latest averaged touch point data from SRAM.

```
    uint16_t x[1];
    uint16_t y[1];
    uint16_t  strength[1];
    uint8_t count = 0;

    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, x, y, strength, &count, 1);
```

### Integrating with LVGL

This driver can be integrated with LVGL using code similar to below:

```
void touch_driver_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x[1];
    uint16_t y[1];
    uint16_t strength[1];
    uint8_t count = 0;

    // Update touch point data.
    ESP_ERROR_CHECK(esp_lcd_touch_read_data(tp));

    data->state = LV_INDEV_STATE_REL;

    if (esp_lcd_touch_get_coordinates(tp, x, y, strength, &count, 1))
    {
        data->point.x = x[0];
        data->point.y = y[0];
        data->state = LV_INDEV_STATE_PR;
    }

    data->continue_reading = false;
}

void initialize_input()
{
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read;
    lv_indev_drv_register( &indev_drv );
}

```