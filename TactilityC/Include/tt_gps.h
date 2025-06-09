#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool tt_gps_has_coordinates();

bool tt_gps_get_coordinates(
    float* longitude,
    float* latitude,
    float* speed,
    float* course,
    int* day,
    int* month,
    int* year
);

#ifdef __cplusplus
}
#endif
