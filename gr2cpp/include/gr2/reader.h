#pragma once

#include "format.h"
#include "model.h"
#include <istream>
#include <memory>
#include <map>
#include <functional>
#include <stack>

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
// Reference Types
// ============================================================================

struct RelocatableReference {
    uint64_t offset = 0;
    bool IsValid() const { return offset != 0; }
};

struct StringReference {
    uint64_t offset = 0;
    bool IsValid() const { return offset != 0; }
};

struct ArrayReference {
    uint32_t size = 0;
    uint64_t offset = 0;
    bool IsValid() const { return offset != 0; }
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
    int8_t ReadInt8();
    uint16_t ReadUInt16();
    int16_t ReadInt16();
    uint32_t ReadUInt32();
    int32_t ReadInt32();
    uint64_t ReadUInt64();
    float ReadFloat();
    std::string ReadStringDirect();

    // File structure reading
    Magic ReadMagic();
    Header ReadHeader();
    SectionHeader ReadSectionHeader();
    SectionReference ReadSectionReference();

    // Reference reading
    RelocatableReference ReadReference();
    StructReference ReadStructReference();
    StringReference ReadStringReference();
    ArrayReference ReadArrayReference();

    // Section processing
    void UncompressStream();
    void ReadSectionRelocations(Section& section);
    void ReadSectionMixedMarshallingRelocations(Section& section);

    // Type system
    MemberDefinition ReadMemberDefinition();
    StructDefinition* ReadStructDefinition();
    StructDefinition* GetOrReadStructDefinition(uint32_t offset);

    // Deserialization - Main entry points
    std::shared_ptr<Root> ReadRoot();
    std::shared_ptr<Skeleton> ReadSkeleton(StructDefinition* def);
    std::shared_ptr<Bone> ReadBone(StructDefinition* def);
    std::shared_ptr<Mesh> ReadMesh(StructDefinition* def);
    std::shared_ptr<VertexData> ReadVertexData(StructDefinition* def);
    std::shared_ptr<TriTopology> ReadTriTopology(StructDefinition* def);
    std::shared_ptr<Material> ReadMaterial(StructDefinition* def);
    std::shared_ptr<Texture> ReadTexture(StructDefinition* def);
    std::shared_ptr<Model> ReadModel(StructDefinition* def);

    // Generic member reading
    void ReadStructMembers(StructDefinition* def, const std::function<void(const MemberDefinition&)>& handler);

    // Member reading by type
    void* ReadMemberValue(const MemberDefinition& member);
    Transform ReadTransform();
    std::string ReadString(const StringReference& ref);

    // Array reading
    template<typename T>
    std::vector<std::shared_ptr<T>> ReadArrayOfReferences(
        const ArrayReference& arrayRef,
        StructDefinition* elementDef,
        std::function<std::shared_ptr<T>(StructDefinition*)> readFunc);

    template<typename T>
    std::vector<std::shared_ptr<T>> ReadReferenceToArray(
        const ArrayReference& arrayRef,
        StructDefinition* elementDef,
        std::function<std::shared_ptr<T>(StructDefinition*)> readFunc);

    template<typename T>
    std::vector<T> ReadInlineArray(uint32_t count, std::function<T()> readFunc);

    // Helpers
    void Seek(const SectionReference& ref);
    void Seek(uint32_t offset);
    void Seek(const RelocatableReference& ref);
    uint32_t Tell() const;
    uint32_t ResolveReference(const SectionReference& ref) const;
    void SavePosition();
    void RestorePosition();

    // Stream management
    std::istream& inputStream_;
    std::vector<uint8_t> uncompressedData_;
    size_t uncompressedPos_ = 0;
    std::stack<uint32_t> positionStack_;

    // File structure
    Magic magic_;
    Header header_;
    std::vector<Section> sections_;

    // Type system
    std::map<uint32_t, std::unique_ptr<StructDefinition>> types_;
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
