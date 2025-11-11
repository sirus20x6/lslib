#pragma once

#include "format.h"
#include "model.h"
#include <istream>
#include <memory>
#include <map>
#include <functional>

namespace gr2 {

// ============================================================================
// Exceptions
// ============================================================================

class ParsingException : public std::runtime_error {
public:
    explicit ParsingException(const std::string& message)
        : std::runtime_error(message) {}
};

// ============================================================================
// Compression Interface
// ============================================================================

class IDecompressor {
public:
    virtual ~IDecompressor() = default;

    // Decompress data using Oodle/Granny compression
    virtual std::vector<uint8_t> Decompress(
        int format,
        const uint8_t* compressed,
        size_t compressedSize,
        size_t decompressedSize,
        int stop0, int stop1, int stop2) = 0;

    // Decompress using format 4 (incremental)
    virtual std::vector<uint8_t> Decompress4(
        const uint8_t* compressed,
        size_t compressedSize,
        size_t decompressedSize) = 0;
};

// Null decompressor (throws error if compression is encountered)
class NullDecompressor : public IDecompressor {
public:
    std::vector<uint8_t> Decompress(
        int format,
        const uint8_t* compressed,
        size_t compressedSize,
        size_t decompressedSize,
        int stop0, int stop1, int stop2) override;

    std::vector<uint8_t> Decompress4(
        const uint8_t* compressed,
        size_t compressedSize,
        size_t decompressedSize) override;
};

// ============================================================================
// GR2 Reader
// ============================================================================

class GR2Reader {
public:
    explicit GR2Reader(std::istream& stream);
    ~GR2Reader();

    // Set custom decompressor (e.g., one that calls granny2.dll)
    void SetDecompressor(std::shared_ptr<IDecompressor> decompressor);

    // Read GR2 file into Root object
    std::shared_ptr<Root> Read();

    // Get file tag (game identifier)
    uint32_t GetTag() const { return header_.tag; }

private:
    // Reading primitives
    void ReadBytes(uint8_t* buffer, size_t count);
    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    float ReadFloat();
    std::string ReadString();

    // File structure reading
    Magic ReadMagic();
    Header ReadHeader();
    SectionHeader ReadSectionHeader();
    SectionReference ReadSectionReference();

    // Section processing
    void UncompressStream();
    void ReadSectionRelocations(Section& section);
    void ReadSectionMixedMarshallingRelocations(Section& section);

    // Type system
    void ReadTypeDefinition(uint32_t offset);
    StructDefinition* GetTypeDefinition(uint32_t offset);
    MemberDefinition ReadMemberDefinition();

    // Deserialization
    void* ReadStruct(StructDefinition* def, MemberType memberType, void* target, const std::string& targetMemberName);
    void* ReadStructInternal(StructDefinition* def, void* target);
    void ReadMember(const MemberDefinition& member, void* parentStruct);

    // Helpers
    void Seek(const SectionReference& ref);
    void Seek(uint32_t offset);
    uint32_t Tell() const;
    uint32_t ResolveReference(const SectionReference& ref) const;

    // Stream management
    std::istream& inputStream_;
    std::vector<uint8_t> uncompressedData_;
    size_t uncompressedPos_ = 0;

    // File structure
    Magic magic_;
    Header header_;
    std::vector<Section> sections_;

    // Type system
    std::map<uint32_t, StructDefinition> types_;
    std::map<uint32_t, void*> cachedStructs_;

    // Decompression
    std::shared_ptr<IDecompressor> decompressor_;
};

// ============================================================================
// Helper Functions
// ============================================================================

// Half-precision float conversion
float HalfToFloat(uint16_t half);
uint16_t FloatToHalf(float f);

// Byte/normalized conversions
Vector3 ByteToNormal(uint8_t x, uint8_t y, uint8_t z);
Vector3 NormalToByte(const Vector3& normal);

// QTangent (quaternion compressed tangent frame) conversion
void QTangentToTBN(uint16_t qx, uint16_t qy, uint16_t qz, uint16_t qw,
                   Vector3& tangent, Vector3& binormal, Vector3& normal);

} // namespace gr2
