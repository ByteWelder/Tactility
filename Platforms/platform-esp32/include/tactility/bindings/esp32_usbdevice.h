// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/bindings/bindings.h>
#include <tactility/drivers/esp32_usbdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_DEVICETREE(esp32_usbdevice, struct Esp32UsbDeviceConfig)
DEFINE_DEVICETREE(esp32_usbdevice_hid, struct Esp32UsbDeviceChildConfig)
DEFINE_DEVICETREE(esp32_usbdevice_msc, struct Esp32UsbDeviceChildConfig)
DEFINE_DEVICETREE(esp32_usbdevice_midi, struct Esp32UsbDeviceChildConfig)
DEFINE_DEVICETREE(esp32_usbdevice_cdc, struct Esp32UsbDeviceChildConfig)

#ifdef __cplusplus
}
#endif
