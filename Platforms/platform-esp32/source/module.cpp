#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#include <tactility/module.h>

#include <soc/soc_caps.h>

extern "C" {

extern Driver esp32_adc_oneshot_driver;
extern Driver esp32_gpio_driver;
extern Driver esp32_i2c_driver;
extern Driver esp32_i2c_master_driver;
extern Driver esp32_i2s_driver;
#if SOC_LCD_I80_SUPPORTED
extern Driver esp32_i8080_driver;
#endif
extern Driver esp32_pwm_ledc_driver;
#if SOC_SDMMC_HOST_SUPPORTED
extern Driver esp32_sdmmc_driver;
#endif
extern Driver esp32_sdspi_driver;
extern Driver esp32_spi_driver;
extern Driver esp32_uart_driver;
extern Driver esp32_grove_driver;
#if defined(CONFIG_SOC_WIFI_SUPPORTED) || defined(CONFIG_SLAVE_SOC_WIFI_SUPPORTED)
extern Driver esp32_wifi_driver;
extern Driver esp32_wifi_pinned_driver;
#endif
#if defined(CONFIG_BT_NIMBLE_ENABLED)
extern Driver esp32_bluetooth_driver;
extern Driver esp32_ble_serial_driver;
extern Driver esp32_ble_midi_driver;
extern Driver esp32_ble_hid_device_driver;
#endif
#if SOC_USB_OTG_SUPPORTED
extern Driver esp32_usbhost_driver;
extern Driver esp32_usbhost_hid_driver;
extern Driver esp32_usbhost_hid_keyboard_driver;
extern Driver esp32_usbhost_midi_driver;
extern Driver esp32_usbhost_msc_driver;
#endif
#if SOC_USB_OTG_SUPPORTED && (CONFIG_TINYUSB_HID_COUNT || CONFIG_TINYUSB_MSC_ENABLED || CONFIG_TINYUSB_MIDI_COUNT || CONFIG_TINYUSB_CDC_ENABLED)
extern Driver esp32_usb_device_controller_driver;
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_HID_COUNT
extern Driver esp32_usb_hid_device_driver;
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MSC_ENABLED
extern Driver esp32_usb_msc_device_driver;
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MIDI_COUNT
extern Driver esp32_usb_midi_device_driver;
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_CDC_ENABLED
extern Driver esp32_usb_cdc_device_driver;
#endif

static Driver* const platform_esp32_drivers[] = {
    &esp32_adc_oneshot_driver,
    &esp32_gpio_driver,
    &esp32_i2c_driver,
    &esp32_i2c_master_driver,
    &esp32_i2s_driver,
#if SOC_LCD_I80_SUPPORTED
    &esp32_i8080_driver,
#endif
    &esp32_pwm_ledc_driver,
#if SOC_SDMMC_HOST_SUPPORTED
    &esp32_sdmmc_driver,
#endif
    &esp32_sdspi_driver,
    &esp32_spi_driver,
    &esp32_uart_driver,
    &esp32_grove_driver,
#if defined(CONFIG_SOC_WIFI_SUPPORTED) || defined(CONFIG_SLAVE_SOC_WIFI_SUPPORTED)
    &esp32_wifi_driver,
    &esp32_wifi_pinned_driver,
#endif
#if defined(CONFIG_BT_NIMBLE_ENABLED)
    &esp32_bluetooth_driver,
    &esp32_ble_serial_driver,
    &esp32_ble_midi_driver,
    &esp32_ble_hid_device_driver,
#endif
#if SOC_USB_OTG_SUPPORTED
    &esp32_usbhost_driver,
    &esp32_usbhost_hid_driver,
    &esp32_usbhost_hid_keyboard_driver,
    &esp32_usbhost_midi_driver,
    &esp32_usbhost_msc_driver,
#endif
#if SOC_USB_OTG_SUPPORTED && (CONFIG_TINYUSB_HID_COUNT || CONFIG_TINYUSB_MSC_ENABLED || CONFIG_TINYUSB_MIDI_COUNT || CONFIG_TINYUSB_CDC_ENABLED)
    &esp32_usb_device_controller_driver,
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_HID_COUNT
    &esp32_usb_hid_device_driver,
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MSC_ENABLED
    &esp32_usb_msc_device_driver,
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_MIDI_COUNT
    &esp32_usb_midi_device_driver,
#endif
#if SOC_USB_OTG_SUPPORTED && CONFIG_TINYUSB_CDC_ENABLED
    &esp32_usb_cdc_device_driver,
#endif
    nullptr
};

Module platform_esp32_module = {
    .name = "platform-esp32",
    .drivers = platform_esp32_drivers,
    .symbols = nullptr,
    .internal = nullptr
};

}
