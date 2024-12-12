#include <cstdint>
#include <cassert>
#include "QrHelpers.h"

/**
 * Maps QR version (starting at a fictitious 0)  to the usable byte size for L(ow) CRC checking QR.
 *
 * See https://www.qrcode.com/en/about/version.html
 */
uint16_t qr_version_binary_sizes[] = {
    0, 17, 32, 53, 78, 106, 134, 154, 192, 230, // 0 - 9
    271, 321, 367, 425, 458, 520, 586, 644, 718, 792, // 10-19
    858, 929, 1003, 1091, 1171, 1273, 1367, 1465, 1528, 1682, // 20-29
    1732, 1840, 1952, 2068, 2188, 2303, 2431, 2563, 2699, 2809, 2953 // 30-40
};

bool getQrVersionForBinaryDataLength(size_t inLength, int& outVersion) {
    static_assert(sizeof(qr_version_binary_sizes) == 41 * sizeof(int16_t));
    for (int version = 1; version < 41; version++) {
        if (inLength <= qr_version_binary_sizes[version]) {
            outVersion = version;
            return true;
        }
    }
    return false;
}

