// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>
#include <tactility/drivers/imu.h>
#include <tactility/error.h>

struct Device;

#ifdef __cplusplus
extern "C" {
#endif

struct Mpu6886Config {
    /** Address on bus */
    uint8_t address;
    /**
     * Row-major 3x3 rotation matrix mapping the sensor frame onto the device frame (Linux IIO
     * "mount-matrix" convention: `out[i] = m[i * 3] * x + m[i * 3 + 1] * y + m[i * 3 + 2] * z`).
     * Identity means the sensor frame already matches the device frame.
     */
    int32_t mount_matrix[9];
};

/**
 * Read accelerometer data only.
 * @param[in]  device mpu6886 device
 * @param[out] data   Pointer to ImuAccelData to populate (±8g range)
 * @return ERROR_NONE on success
 */
error_t mpu6886_read_accel(struct Device* device, struct ImuAccelData* data);

/**
 * Read gyroscope data only.
 * @param[in]  device mpu6886 device
 * @param[out] data   Pointer to ImuGyroData to populate (±2000°/s range)
 * @return ERROR_NONE on success
 */
error_t mpu6886_read_gyro(struct Device* device, struct ImuGyroData* data);

/**
 * Read the chip's on-die temperature.
 * @param[in]  device mpu6886 device
 * @param[out] temperature_c Pointer to store the temperature, in °C
 * @return ERROR_NONE on success
 */
error_t mpu6886_read_temperature(struct Device* device, float* temperature_c);

#ifdef __cplusplus
}
#endif
