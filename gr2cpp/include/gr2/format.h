#pragma once

#include "math_types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace gr2 {

// ============================================================================
// Transform
// ============================================================================

struct Transform {
    enum class Flags : uint32_t {
        None = 0,
        HasTranslation = 0x01,
        HasRotation = 0x02,
        HasScaleShear = 0x04
    };

    uint32_t flags = 0;
    Vector3 translation = Vector3::Zero();
    Quaternion rotation = Quaternion::Identity();
    Matrix3 scaleShear = Matrix3::Identity();

    bool HasTranslation() const { return (flags & (uint32_t)Flags::HasTranslation) != 0; }
    bool HasRotation() const { return (flags & (uint32_t)Flags::HasRotation) != 0; }
    bool HasScaleShear() const { return (flags & (uint32_t)Flags::HasScaleShear) != 0; }

    Matrix4 ToMatrix4() const;
};

// ============================================================================
// Magic Header
// ============================================================================

struct Magic {
    enum class Format {
        LittleEndian32,
        BigEndian32,
        LittleEndian64,
        BigEndian64,
        Unknown
    };

    static constexpr uint32_t MagicSize = 0x20;

    uint8_t signature[16];
    uint32_t headersSize;
    uint32_t headerFormat;
    uint32_t reserved1;
    uint32_t reserved2;

    Format format = Format::Unknown;

    bool Is32Bit() const {
        return format == Format::LittleEndian32 || format == Format::BigEndian32;
    }

    bool Is64Bit() const {
        return format == Format::LittleEndian64 || format == Format::BigEndian64;
    }

    bool IsLittleEndian() const {
        return format == Format::LittleEndian32 || format == Format::LittleEndian64;
    }

    static Format FormatFromSignature(const uint8_t* sig);
};

// ============================================================================
// Section Reference
// ============================================================================

struct SectionReference {
    uint32_t section;
    uint32_t offset;
};

// ============================================================================
// File Header
// ============================================================================

struct Header {
    static constexpr uint32_t ExtraTagCount = 4;

    uint32_t version;
    uint32_t fileSize;
    uint32_t crc;
    uint32_t sectionsOffset;
    uint32_t numSections;
    SectionReference rootType;
    SectionReference rootNode;
    uint32_t tag;
    uint32_t extraTags[ExtraTagCount];

    // Version 7 fields
    uint32_t stringTableCrc = 0;
    uint32_t reserved1 = 0;
    uint32_t reserved2 = 0;
    uint32_t reserved3 = 0;

    uint32_t Size() const {
        return version >= 7 ? 0x58 : 0x48;
    }
};

// ============================================================================
// Section Header
// ============================================================================

enum class SectionType : uint32_t {
    Main = 0,
    RigidVertex = 1,
    DeformableVertex = 2,
    RigidIndex = 3,
    DeformableIndex = 4,
    Skeleton = 5,
    Mesh = 6,
    TrackGroup = 7,
    DiscardablePadding = 8
};

struct SectionHeader {
    static constexpr uint32_t SectionHeaderSize = 0x40;

    uint32_t compression;
    uint32_t offsetInFile;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint32_t alignment;
    uint32_t first16bit;
    uint32_t first8bit;
    uint32_t relocationsOffset;
    uint32_t numRelocations;
    uint32_t mixedMarshallingDataOffset;
    uint32_t numMixedMarshallingData;
};

struct Section {
    SectionHeader header;
    std::vector<uint8_t> data;
};

// ============================================================================
// Type System
// ============================================================================

enum class MemberType {
    None = 0,
    Inline = 1,
    Reference = 2,
    ReferenceToArray = 3,
    ArrayOfReferences = 4,
    VariantReference = 5,
    ReferenceToVariantArray = 6,
    String = 7,
    Transform = 8,
    Real32 = 9,
    Int8 = 10,
    UInt8 = 11,
    BinormalInt8 = 12,
    NormalUInt8 = 13,
    Int16 = 14,
    UInt16 = 15,
    BinormalInt16 = 16,
    NormalUInt16 = 17,
    Int32 = 18,
    UInt32 = 19,
    Real16 = 20,
    EmptyReference = 21
};

enum class SerializationKind {
    None = 0,
    UserMember = 1,
    UserMemberBool = 2
};

struct MemberDefinition {
    std::string name;
    MemberType type;
    StructReference definition;  // Pointer to struct definition for complex types
    uint32_t arraySize;
    uint32_t extra[3];
    uint32_t unknown;

    bool IsValid() const { return type != MemberType::None; }
};

struct StructDefinition {
    std::vector<MemberDefinition> members;

    // Calculate size based on members and alignment
    uint32_t CalculateSize() const;
};

struct StructReference {
    uint64_t offset = 0;

    bool IsValid() const { return offset != 0; }
};

} // namespace gr2
