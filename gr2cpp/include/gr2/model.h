#pragma once

#include "format.h"
#include <string>
#include <vector>
#include <memory>

namespace gr2 {

// Forward declarations
struct Root;
struct Skeleton;
struct Bone;
struct Mesh;
struct VertexData;
struct TriTopology;
struct Material;
struct Texture;
struct Animation;
struct TrackGroup;
struct Model;

// ============================================================================
// Bone Weight
// ============================================================================

struct BoneWeight {
    uint8_t a, b, c, d;

    BoneWeight() : a(0), b(0), c(0), d(0) {}
    BoneWeight(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : a(a), b(b), c(c), d(d) {}

    uint8_t& operator[](int index) {
        switch (index) {
            case 0: return a;
            case 1: return b;
            case 2: return c;
            case 3: return d;
            default: throw std::out_of_range("BoneWeight index out of range");
        }
    }

    const uint8_t& operator[](int index) const {
        switch (index) {
            case 0: return a;
            case 1: return b;
            case 2: return c;
            case 3: return d;
            default: throw std::out_of_range("BoneWeight index out of range");
        }
    }

    bool operator==(const BoneWeight& other) const {
        return a == other.a && b == other.b && c == other.c && d == other.d;
    }
};

// ============================================================================
// Vertex Types and Formats
// ============================================================================

enum class PositionType {
    None,
    Float3,
    Word4
};

enum class NormalType {
    None,
    Float3,
    Half4,
    Byte4,
    QTangent
};

enum class ColorMapType {
    None,
    Float4,
    Byte4
};

enum class TextureCoordinateType {
    None,
    Float2,
    Half2
};

struct VertexDescriptor {
    bool hasBoneWeights = false;
    int numBoneInfluences = 4;
    PositionType positionType = PositionType::None;
    NormalType normalType = NormalType::None;
    NormalType tangentType = NormalType::None;
    NormalType binormalType = NormalType::None;
    ColorMapType colorMapType = ColorMapType::None;
    int colorMaps = 0;
    TextureCoordinateType textureCoordinateType = TextureCoordinateType::None;
    int textureCoordinates = 0;

    std::string GetName() const;
};

struct Vertex {
    static constexpr int MaxBoneInfluences = 4;
    static constexpr int MaxTextureCoordinates = 8;
    static constexpr int MaxColorMaps = 2;

    VertexDescriptor format;

    Vector3 position = Vector3::Zero();
    Vector3 normal = Vector3::Zero();
    Vector3 tangent = Vector3::Zero();
    Vector3 binormal = Vector3::Zero();

    Vector2 textureCoordinates[MaxTextureCoordinates];
    Vector4 colors[MaxColorMaps];

    BoneWeight boneWeights;
    BoneWeight boneIndices;

    Vector2 GetUV(int index) const {
        if (index < 0 || index >= MaxTextureCoordinates)
            return Vector2();
        return textureCoordinates[index];
    }

    void SetUV(int index, const Vector2& uv) {
        if (index >= 0 && index < MaxTextureCoordinates)
            textureCoordinates[index] = uv;
    }

    Vector4 GetColor(int index) const {
        if (index < 0 || index >= MaxColorMaps)
            return Vector4();
        return colors[index];
    }

    void SetColor(int index, const Vector4& color) {
        if (index >= 0 && index < MaxColorMaps)
            colors[index] = color;
    }
};

// ============================================================================
// Vertex Data
// ============================================================================

struct VertexData {
    std::vector<Vertex> vertices;
    VertexDescriptor format;
};

// ============================================================================
// Topology
// ============================================================================

struct TriTopology {
    std::vector<uint32_t> indices;
    std::vector<uint32_t> groups;   // Triangle groups
    int bytesPerIndex = 2;          // 2 or 4
};

// ============================================================================
// Material and Texture
// ============================================================================

struct Texture {
    std::string fileName;
    std::string name;
};

struct MaterialMap {
    std::string usage;
    std::shared_ptr<Texture> texture;
};

struct Material {
    std::string name;
    std::vector<MaterialMap> maps;
};

struct MaterialBinding {
    std::shared_ptr<Material> material;
};

// ============================================================================
// Bone and Skeleton
// ============================================================================

struct Bone {
    std::string name;
    int32_t parentIndex = -1;
    Transform transform;
    float inverseWorldTransform[16];  // 4x4 matrix
    int32_t lodError = 0;
};

struct Skeleton {
    std::string name;
    std::vector<std::shared_ptr<Bone>> bones;
    int32_t lodType = 0;
};

struct BoneBinding {
    std::string boneName;
    Vector3 obb_min;
    Vector3 obb_max;
    std::vector<int32_t> triangleIndices;
};

// ============================================================================
// Mesh
// ============================================================================

struct Mesh {
    std::string name;
    std::shared_ptr<VertexData> primaryVertexData;
    std::shared_ptr<TriTopology> primaryTopology;
    std::vector<MaterialBinding> materialBindings;
    std::vector<BoneBinding> boneBindings;
    int32_t vertexType = 0;
    int32_t triangleType = 0;
};

// ============================================================================
// Model
// ============================================================================

struct Model {
    std::string name;
    std::shared_ptr<Skeleton> skeleton;
    Transform initialPlacement;
    std::vector<std::shared_ptr<Mesh>> meshBindings;
};

// ============================================================================
// Animation (Simplified - full implementation would need curve data)
// ============================================================================

struct TransformTrack {
    std::string name;
    // Curve data would go here
    // For now we just store the name for basic support
};

struct TrackGroup {
    std::string name;
    std::vector<TransformTrack> transformTracks;
    // Additional fields omitted for simplicity
};

struct Animation {
    std::string name;
    float duration = 0.0f;
    float timeStep = 0.0f;
    // Full animation support would need curve data structures
};

// ============================================================================
// Art Tool Info
// ============================================================================

struct ArtToolInfo {
    std::string fromArtToolName;
    int32_t artToolMajorRevision = 0;
    int32_t artToolMinorRevision = 0;
    float unitsPerMeter = 1.0f;
    Vector3 origin = Vector3::Zero();
    Vector3 rightVector = Vector3(1, 0, 0);
    Vector3 upVector = Vector3(0, 1, 0);
    Vector3 backVector = Vector3(0, 0, 1);
};

struct ExporterInfo {
    std::string exporterName;
    int32_t exporterMajorRevision = 0;
    int32_t exporterMinorRevision = 0;
    int32_t exporterCustomization = 0;
    int32_t exporterBuildNumber = 0;
};

// ============================================================================
// Root Container
// ============================================================================

struct Root {
    std::shared_ptr<ArtToolInfo> artToolInfo;
    std::shared_ptr<ExporterInfo> exporterInfo;
    std::string fromFileName;

    std::vector<std::shared_ptr<Texture>> textures;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Skeleton>> skeletons;
    std::vector<std::shared_ptr<VertexData>> vertexDatas;
    std::vector<std::shared_ptr<TriTopology>> triTopologies;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Model>> models;
    std::vector<std::shared_ptr<TrackGroup>> trackGroups;
    std::vector<std::shared_ptr<Animation>> animations;

    void* extendedData = nullptr;

    bool zUp = false;
    uint32_t gr2Tag = 0;

    static std::shared_ptr<Root> CreateEmpty();
};

} // namespace gr2
