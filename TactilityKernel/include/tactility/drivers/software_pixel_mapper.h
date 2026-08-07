// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Opaque per-instance state for a software pixel mapper, allocated by SoftwarePixelMapperCreateFn
 * and released by SoftwarePixelMapperDestroyFn.
 */
typedef void* SoftwarePixelMapperData;

/**
 * Allocates a mapper instance sized for a width x height source buffer.
 * @param width source buffer width in pixels
 * @param height source buffer height in pixels
 * @return a non-null instance handle, or NULL on allocation failure
 */
typedef SoftwarePixelMapperData (*SoftwarePixelMapperCreateFn)(uint16_t width, uint16_t height);

/**
 * Converts pixel_count pixels from the mapper's source format to its destination format.
 * @param data instance handle from SoftwarePixelMapperCreateFn
 * @param src RGB565 source pixels
 * @param dst destination buffer, must hold at least pixel_count output bytes
 * @param pixel_count number of pixels to convert
 */
typedef void (*SoftwarePixelMapperMapFn)(SoftwarePixelMapperData data, const uint16_t* src, uint8_t* dst, uint32_t pixel_count);

/**
 * Releases a mapper instance created by SoftwarePixelMapperCreateFn.
 * @param data instance handle to release
 */
typedef void (*SoftwarePixelMapperDestroyFn)(SoftwarePixelMapperData data);

/**
 * A software pixel-format conversion, used to translate a driver's native source depth into a
 * panel's scan-out depth when the two differ. Implementations are stateless where possible; the
 * create/destroy pair exists so stateful mappers (LUTs, buffers, context) can manage their own
 * lifetime.
 */
struct SoftwarePixelMapper {
    SoftwarePixelMapperCreateFn create;
    SoftwarePixelMapperMapFn map;
    SoftwarePixelMapperDestroyFn destroy;
};

/**
 * RGB565 to packed 8-bit RGB332 (3 bits red, 3 bits green, 2 bits blue) mapper for panels wired
 * with only 8 data lines.
 */
extern const struct SoftwarePixelMapper software_pixel_mapper_rgb332;

#ifdef __cplusplus
}
#endif
