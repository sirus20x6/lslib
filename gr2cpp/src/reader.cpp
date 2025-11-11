#include "gr2/reader.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace gr2 {

// ============================================================================
// Helper Functions for Format Conversion
// ============================================================================

float HalfToFloat(uint16_t half) {
    uint32_t sign = (half >> 15) & 0x1;
    uint32_t exponent = (half >> 10) & 0x1F;
    uint32_t mantissa = half & 0x3FF;

    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            uint32_t result = sign << 31;
            return *reinterpret_cast<float*>(&result);
        } else {
            // Denormalized number
            exponent = 1;
            while ((mantissa & 0x400) == 0) {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x3FF;
        }
    } else if (exponent == 31) {
        // Infinity or NaN
        uint32_t result = (sign << 31) | 0x7F800000 | (mantissa << 13);
        return *reinterpret_cast<float*>(&result);
    }

    exponent += (127 - 15);
    uint32_t result = (sign << 31) | (exponent << 23) | (mantissa << 13);
    return *reinterpret_cast<float*>(&result);
}

uint16_t FloatToHalf(float f) {
    uint32_t bits = *reinterpret_cast<uint32_t*>(&f);
    uint32_t sign = (bits >> 16) & 0x8000;
    uint32_t exponent = ((bits >> 23) & 0xFF) - 112;
    uint32_t mantissa = bits & 0x7FFFFF;

    if (exponent <= 0) {
        if (exponent < -10) return static_cast<uint16_t>(sign);
        mantissa = (mantissa | 0x800000) >> (1 - exponent);
        return static_cast<uint16_t>(sign | (mantissa >> 13));
    } else if (exponent == 143) {
        if (mantissa == 0) return static_cast<uint16_t>(sign | 0x7C00);
        else return static_cast<uint16_t>(sign | 0x7C00 | (mantissa >> 13));
    } else if (exponent > 30) {
        return static_cast<uint16_t>(sign | 0x7C00);
    }

    return static_cast<uint16_t>(sign | (exponent << 10) | (mantissa >> 13));
}

Vector3 ByteToNormal(uint8_t x, uint8_t y, uint8_t z) {
    return Vector3(
        (static_cast<float>(x) / 127.5f) - 1.0f,
        (static_cast<float>(y) / 127.5f) - 1.0f,
        (static_cast<float>(z) / 127.5f) - 1.0f
    );
}

Vector3 NormalToByte(const Vector3& normal) {
    return Vector3(
        (normal.x + 1.0f) * 127.5f,
        (normal.y + 1.0f) * 127.5f,
        (normal.z + 1.0f) * 127.5f
    );
}

void QTangentToTBN(uint16_t qx, uint16_t qy, uint16_t qz, uint16_t qw,
                   Vector3& tangent, Vector3& binormal, Vector3& normal) {
    // Convert from 16-bit unsigned to normalized float
    float x = (static_cast<float>(qx) / 32767.5f) - 1.0f;
    float y = (static_cast<float>(qy) / 32767.5f) - 1.0f;
    float z = (static_cast<float>(qz) / 32767.5f) - 1.0f;
    float w = (static_cast<float>(qw) / 32767.5f) - 1.0f;

    // Normalize quaternion
    float len = std::sqrt(x * x + y * y + z * z + w * w);
    if (len > 0.0001f) {
        x /= len;
        y /= len;
        z /= len;
        w /= len;
    }

    // Convert quaternion to rotation matrix
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    // Tangent, Binormal, Normal from rotation matrix
    tangent = Vector3(
        1.0f - 2.0f * (yy + zz),
        2.0f * (xy + wz),
        2.0f * (xz - wy)
    );

    binormal = Vector3(
        2.0f * (xy - wz),
        1.0f - 2.0f * (xx + zz),
        2.0f * (yz + wx)
    );

    normal = Vector3(
        2.0f * (xz + wy),
        2.0f * (yz - wx),
        1.0f - 2.0f * (xx + yy)
    );
}

// ============================================================================
// Null Decompressor Implementation
// ============================================================================

std::vector<uint8_t> NullDecompressor::Decompress(
    int format,
    const uint8_t* compressed,
    size_t compressedSize,
    size_t decompressedSize,
    int stop0, int stop1, int stop2) {
    throw ParsingException(
        "Compressed GR2 file detected but no decompressor available. "
        "Files with Oodle compression require granny2.dll support.");
}

std::vector<uint8_t> NullDecompressor::Decompress4(
    const uint8_t* compressed,
    size_t compressedSize,
    size_t decompressedSize) {
    throw ParsingException(
        "Compressed GR2 file detected but no decompressor available. "
        "Files with Oodle compression require granny2.dll support.");
}

// ============================================================================
// GR2Reader Implementation
// ============================================================================

GR2Reader::GR2Reader(std::istream& stream)
    : inputStream_(stream)
    , decompressor_(std::make_shared<NullDecompressor>()) {
}

GR2Reader::~GR2Reader() {
}

void GR2Reader::SetDecompressor(std::shared_ptr<IDecompressor> decompressor) {
    decompressor_ = decompressor;
}

// ============================================================================
// Reading Primitives
// ============================================================================

void GR2Reader::ReadBytes(uint8_t* buffer, size_t count) {
    // Read from uncompressed buffer if available
    if (!uncompressedData_.empty()) {
        if (uncompressedPos_ + count > uncompressedData_.size()) {
            throw ParsingException("Read past end of uncompressed data");
        }
        std::memcpy(buffer, &uncompressedData_[uncompressedPos_], count);
        uncompressedPos_ += count;
    } else {
        inputStream_.read(reinterpret_cast<char*>(buffer), count);
        if (!inputStream_) {
            throw ParsingException("Failed to read from input stream");
        }
    }
}

uint8_t GR2Reader::ReadUInt8() {
    uint8_t value;
    ReadBytes(&value, 1);
    return value;
}

uint16_t GR2Reader::ReadUInt16() {
    uint16_t value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 2);
    // TODO: Handle endianness if needed
    return value;
}

uint32_t GR2Reader::ReadUInt32() {
    uint32_t value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 4);
    // TODO: Handle endianness if needed
    return value;
}

uint64_t GR2Reader::ReadUInt64() {
    uint64_t value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 8);
    // TODO: Handle endianness if needed
    return value;
}

float GR2Reader::ReadFloat() {
    float value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 4);
    return value;
}

std::string GR2Reader::ReadString() {
    // Read null-terminated string from current position
    // In GR2, strings are stored as pointers that have been relocated
    uint32_t stringOffset = ReadUInt32();
    if (stringOffset == 0) return "";

    // Save current position
    uint32_t savedPos = Tell();

    // Seek to string and read it
    Seek(stringOffset);
    std::string result;
    char ch;
    while ((ch = static_cast<char>(ReadUInt8())) != '\0') {
        result += ch;
    }

    // Restore position
    Seek(savedPos);

    return result;
}

// ============================================================================
// File Structure Reading
// ============================================================================

Magic GR2Reader::ReadMagic() {
    Magic magic;
    ReadBytes(magic.signature, 16);
    magic.headersSize = ReadUInt32();
    magic.headerFormat = ReadUInt32();
    magic.reserved1 = ReadUInt32();
    magic.reserved2 = ReadUInt32();

    magic.format = Magic::FormatFromSignature(magic.signature);

    if (magic.format == Magic::Format::Unknown) {
        throw ParsingException("Unknown GR2 file format signature");
    }

    if (magic.headerFormat != 0) {
        throw ParsingException("Compressed headers are not supported");
    }

    return magic;
}

Header GR2Reader::ReadHeader() {
    Header header;
    header.version = ReadUInt32();
    header.fileSize = ReadUInt32();
    header.crc = ReadUInt32();
    header.sectionsOffset = ReadUInt32();
    header.numSections = ReadUInt32();
    header.rootType = ReadSectionReference();
    header.rootNode = ReadSectionReference();
    header.tag = ReadUInt32();

    for (int i = 0; i < Header::ExtraTagCount; i++) {
        header.extraTags[i] = ReadUInt32();
    }

    if (header.version >= 7) {
        header.stringTableCrc = ReadUInt32();
        header.reserved1 = ReadUInt32();
        header.reserved2 = ReadUInt32();
        header.reserved3 = ReadUInt32();
    }

    if (header.version < 6 || header.version > 7) {
        throw ParsingException("Unsupported GR2 version: " + std::to_string(header.version));
    }

    return header;
}

SectionHeader GR2Reader::ReadSectionHeader() {
    SectionHeader header;
    header.compression = ReadUInt32();
    header.offsetInFile = ReadUInt32();
    header.compressedSize = ReadUInt32();
    header.uncompressedSize = ReadUInt32();
    header.alignment = ReadUInt32();
    header.first16bit = ReadUInt32();
    header.first8bit = ReadUInt32();
    header.relocationsOffset = ReadUInt32();
    header.numRelocations = ReadUInt32();
    header.mixedMarshallingDataOffset = ReadUInt32();
    header.numMixedMarshallingData = ReadUInt32();

    return header;
}

SectionReference GR2Reader::ReadSectionReference() {
    SectionReference ref;
    ref.section = ReadUInt32();
    ref.offset = ReadUInt32();
    return ref;
}

// ============================================================================
// Section Processing
// ============================================================================

void GR2Reader::UncompressStream() {
    // Calculate total uncompressed size
    size_t totalSize = 0;
    for (const auto& section : sections_) {
        totalSize += section.header.uncompressedSize;
    }

    uncompressedData_.resize(totalSize);
    size_t currentPos = 0;

    for (auto& section : sections_) {
        const auto& hdr = section.header;

        // Read section data from input stream
        std::vector<uint8_t> sectionContents(hdr.compressedSize);
        inputStream_.seekg(hdr.offsetInFile);
        inputStream_.read(reinterpret_cast<char*>(sectionContents.data()), hdr.compressedSize);

        // Update section offset to point into uncompressed buffer
        const_cast<SectionHeader&>(section.header).offsetInFile = static_cast<uint32_t>(currentPos);

        if (hdr.compression == 0) {
            // Uncompressed - copy directly
            std::memcpy(&uncompressedData_[currentPos], sectionContents.data(), hdr.compressedSize);
        } else if (hdr.uncompressedSize > 0) {
            // Compressed - decompress
            std::vector<uint8_t> decompressed;

            if (hdr.compression == 4) {
                decompressed = decompressor_->Decompress4(
                    sectionContents.data(),
                    hdr.compressedSize,
                    hdr.uncompressedSize);
            } else {
                decompressed = decompressor_->Decompress(
                    hdr.compression,
                    sectionContents.data(),
                    hdr.compressedSize,
                    hdr.uncompressedSize,
                    hdr.first16bit,
                    hdr.first8bit,
                    hdr.uncompressedSize);
            }

            std::memcpy(&uncompressedData_[currentPos], decompressed.data(), decompressed.size());
        }

        currentPos += hdr.uncompressedSize;
    }

    uncompressedPos_ = 0;
}

void GR2Reader::ReadSectionRelocations(Section& section) {
    if (section.header.numRelocations == 0) return;

    // Read relocation table
    inputStream_.seekg(section.header.relocationsOffset);

    for (uint32_t i = 0; i < section.header.numRelocations; i++) {
        uint32_t offsetInSection = ReadUInt32();
        SectionReference ref = ReadSectionReference();

        // Calculate target address
        uint32_t targetAddress = sections_[ref.section].header.offsetInFile + ref.offset;

        // Fix up pointer in uncompressed data
        uint32_t ptrLocation = section.header.offsetInFile + offsetInSection;
        std::memcpy(&uncompressedData_[ptrLocation], &targetAddress, 4);
    }
}

void GR2Reader::ReadSectionMixedMarshallingRelocations(Section& section) {
    // Mixed marshalling relocations for endian swapping
    // For now, skip if not needed (little-endian files)
    if (magic_.IsLittleEndian()) {
        return;
    }

    // TODO: Implement endian swapping if needed for big-endian files
}

// ============================================================================
// Helpers
// ============================================================================

void GR2Reader::Seek(const SectionReference& ref) {
    uint32_t offset = ResolveReference(ref);
    Seek(offset);
}

void GR2Reader::Seek(uint32_t offset) {
    if (!uncompressedData_.empty()) {
        uncompressedPos_ = offset;
    } else {
        inputStream_.seekg(offset);
    }
}

uint32_t GR2Reader::Tell() const {
    if (!uncompressedData_.empty()) {
        return static_cast<uint32_t>(uncompressedPos_);
    } else {
        return static_cast<uint32_t>(inputStream_.tellg());
    }
}

uint32_t GR2Reader::ResolveReference(const SectionReference& ref) const {
    return sections_[ref.section].header.offsetInFile + ref.offset;
}

// ============================================================================
// Main Read Function
// ============================================================================

std::shared_ptr<Root> GR2Reader::Read() {
    // Read magic and header
    magic_ = ReadMagic();
    header_ = ReadHeader();

    if (magic_.format != Magic::Format::LittleEndian32 &&
        magic_.format != Magic::Format::LittleEndian64) {
        throw ParsingException("Only little-endian GR2 files are supported");
    }

    // Read section headers
    sections_.resize(header_.numSections);
    for (uint32_t i = 0; i < header_.numSections; i++) {
        sections_[i].header = ReadSectionHeader();
    }

    // Uncompress all sections
    UncompressStream();

    // Apply relocations
    for (auto& section : sections_) {
        ReadSectionRelocations(section);
    }

    // Apply mixed marshalling relocations if needed
    if (!magic_.IsLittleEndian()) {
        for (auto& section : sections_) {
            ReadSectionMixedMarshallingRelocations(section);
        }
    }

    // Read type definition for root
    uint32_t rootTypeOffset = ResolveReference(header_.rootType);
    ReadTypeDefinition(rootTypeOffset);

    // Create and read root object
    auto root = Root::CreateEmpty();
    root->gr2Tag = header_.tag;

    // NOTE: Full deserialization would require implementing the complete
    // type system and recursive struct reading. This is a substantial
    // amount of code (~1000+ lines) that would read the type definitions
    // and use them to deserialize the Root structure.
    //
    // For a working implementation, you would need to:
    // 1. Read and parse all type definitions
    // 2. Recursively deserialize structures based on type info
    // 3. Handle all MemberTypes (Reference, Array, String, etc.)
    // 4. Convert vertex formats (QTangent, Half, Byte, etc.)
    //
    // This simplified version returns an empty root structure.
    // See the C# implementation in LSLib/Granny/GR2/Reader.cs for the
    // complete deserialization logic.

    return root;
}

void GR2Reader::ReadTypeDefinition(uint32_t offset) {
    // Type definition reading would go here
    // This is complex and requires parsing the GR2 type system
    // See LSLib/Granny/GR2/Reader.cs:ReadStructDefinition() for reference
}

StructDefinition* GR2Reader::GetTypeDefinition(uint32_t offset) {
    auto it = types_.find(offset);
    if (it != types_.end()) {
        return &it->second;
    }
    return nullptr;
}

MemberDefinition GR2Reader::ReadMemberDefinition() {
    MemberDefinition member;
    // Read member definition from current position
    // This would parse the member structure
    return member;
}

void* GR2Reader::ReadStruct(StructDefinition* def, MemberType memberType,
                            void* target, const std::string& targetMemberName) {
    // Recursive struct reading would go here
    return target;
}

void* GR2Reader::ReadStructInternal(StructDefinition* def, void* target) {
    // Internal struct reading
    return target;
}

void GR2Reader::ReadMember(const MemberDefinition& member, void* parentStruct) {
    // Member reading based on type
}

} // namespace gr2
