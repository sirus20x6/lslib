#pragma once

#include "math_types.h"
#include "model.h"
#include <cstdint>
#include <string>

namespace gr2 {

// Forward declaration
class GR2Reader;
struct StructDefinition;
struct MemberDefinition;

// ============================================================================
// Vertex Format Detection
// ============================================================================

class VertexFormatDetector {
public:
    // Detect vertex format from GR2 struct definition
    static VertexDescriptor DetectFormat(StructDefinition* def);

private:
    static void DetectPositionFormat(const MemberDefinition& member, VertexDescriptor& desc);
    static void DetectBoneWeightsFormat(const MemberDefinition& member, VertexDescriptor& desc);
    static void DetectNormalFormat(const MemberDefinition& member, VertexDescriptor& desc);
    static void DetectTangentFormat(const MemberDefinition& member, VertexDescriptor& desc);
    static void DetectBinormalFormat(const MemberDefinition& member, VertexDescriptor& desc);
    static void DetectColorFormat(const MemberDefinition& member, VertexDescriptor& desc, int& colorIndex);
    static void DetectUVFormat(const MemberDefinition& member, VertexDescriptor& desc, int& uvIndex);
};

// ============================================================================
// Vertex Reading Helper Functions
// ============================================================================

class VertexReader {
public:
    explicit VertexReader(GR2Reader* reader) : reader_(reader) {}

    // Read vertex components based on format
    Vertex ReadVertex(const VertexDescriptor& format);

    // Component readers
    Vector3 ReadPosition(PositionType type);
    BoneWeight ReadBoneWeights(int numInfluences);
    BoneWeight ReadBoneIndices(int numInfluences);
    Vector3 ReadNormal(NormalType type);
    Vector3 ReadTangent(NormalType type);
    Vector3 ReadBinormal(NormalType type);
    void ReadQTangent(Vector3& tangent, Vector3& binormal, Vector3& normal);
    Vector4 ReadColor(ColorMapType type);
    Vector2 ReadUV(TextureCoordinateType type);

    // Format conversion helpers
    static Vector2 HalfVector2ToFloat(uint16_t x, uint16_t y);
    static Vector3 HalfVector3ToFloat(uint16_t x, uint16_t y, uint16_t z);
    static Vector4 HalfVector4ToFloat(uint16_t x, uint16_t y, uint16_t z, uint16_t w);
    static Vector3 NormalSByteVector4ToFloat(int8_t x, int8_t y, int8_t z);
    static Vector3 NormalSWordVector4ToFloat(int16_t x, int16_t y, int16_t z);
    static Vector4 NormalByteVector4ToFloat(uint8_t x, uint8_t y, uint8_t z, uint8_t w);

private:
    GR2Reader* reader_;
};

} // namespace gr2
