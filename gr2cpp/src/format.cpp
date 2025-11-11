#include "gr2/format.h"
#include <cstring>

namespace gr2 {

// ============================================================================
// Transform Implementation
// ============================================================================

Matrix4 Transform::ToMatrix4() const {
    Matrix4 transform = Matrix4::Identity();

    if (HasTranslation()) {
        transform = Matrix4::CreateTranslation(translation);
    }

    if (HasRotation()) {
        transform = Matrix4::CreateFromQuaternion(rotation) * transform;
    }

    if (HasScaleShear()) {
        Matrix4 scaleShearMat = Matrix4::Identity();
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                scaleShearMat[i][j] = scaleShear[i][j];
            }
        }
        transform = scaleShearMat * transform;
    }

    return transform;
}

// ============================================================================
// Magic Format Detection
// ============================================================================

Magic::Format Magic::FormatFromSignature(const uint8_t* sig) {
    // Little-endian 32-bit magic values
    static const uint8_t le32_1[] = {0x29, 0xDE, 0x6C, 0xC0, 0xBA, 0xA4, 0x53, 0x2B,
                                     0x25, 0xF5, 0xB7, 0xA5, 0xF6, 0x66, 0xE2, 0xEE};
    static const uint8_t le32_2[] = {0x29, 0x75, 0x31, 0x82, 0xBA, 0x02, 0x11, 0x77,
                                     0x25, 0x3A, 0x60, 0x2F, 0xF6, 0x6A, 0x8C, 0x2E};
    static const uint8_t le32_v6[] = {0xB8, 0x67, 0xB0, 0xCA, 0xF8, 0x6D, 0xB1, 0x0F,
                                      0x84, 0x72, 0x8C, 0x7E, 0x5E, 0x19, 0x00, 0x1E};

    // Big-endian 32-bit magic values
    static const uint8_t be32_1[] = {0x0E, 0x11, 0x95, 0xB5, 0x6A, 0xA5, 0xB5, 0x4B,
                                     0xEB, 0x28, 0x28, 0x50, 0x25, 0x78, 0xB3, 0x04};
    static const uint8_t be32_2[] = {0x0E, 0x74, 0xA2, 0x0A, 0x6A, 0xEB, 0xEB, 0x64,
                                     0xEB, 0x4E, 0x1E, 0xAB, 0x25, 0x91, 0xDB, 0x8F};

    // Little-endian 64-bit magic values
    static const uint8_t le64_1[] = {0xE5, 0x9B, 0x49, 0x5E, 0x6F, 0x63, 0x1F, 0x14,
                                     0x1E, 0x13, 0xEB, 0xA9, 0x90, 0xBE, 0xED, 0xC4};
    static const uint8_t le64_2[] = {0xE5, 0x2F, 0x4A, 0xE1, 0x6F, 0xC2, 0x8A, 0xEE,
                                     0x1E, 0xD2, 0xB4, 0x4C, 0x90, 0xD7, 0x55, 0xAF};

    // Big-endian 64-bit magic values
    static const uint8_t be64_1[] = {0x31, 0x95, 0xD4, 0xE3, 0x20, 0xDC, 0x4F, 0x62,
                                     0xCC, 0x36, 0xD0, 0x3A, 0xB1, 0x82, 0xFF, 0x89};
    static const uint8_t be64_2[] = {0x31, 0xC2, 0x4E, 0x7C, 0x20, 0x40, 0xA3, 0x25,
                                     0xCC, 0xE1, 0xC2, 0x7A, 0xB1, 0x32, 0x49, 0xF3};

    if (std::memcmp(sig, le32_1, 16) == 0 || std::memcmp(sig, le32_2, 16) == 0 ||
        std::memcmp(sig, le32_v6, 16) == 0) {
        return Format::LittleEndian32;
    }

    if (std::memcmp(sig, be32_1, 16) == 0 || std::memcmp(sig, be32_2, 16) == 0) {
        return Format::BigEndian32;
    }

    if (std::memcmp(sig, le64_1, 16) == 0 || std::memcmp(sig, le64_2, 16) == 0) {
        return Format::LittleEndian64;
    }

    if (std::memcmp(sig, be64_1, 16) == 0 || std::memcmp(sig, be64_2, 16) == 0) {
        return Format::BigEndian64;
    }

    return Format::Unknown;
}

} // namespace gr2
