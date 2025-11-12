#include "gr2/vertex_format.h"
#include "gr2/reader.h"
#include "gr2/format.h"
#include <cmath>

namespace gr2 {

// ============================================================================
// Vertex Format Detection
// ============================================================================

VertexDescriptor VertexFormatDetector::DetectFormat(StructDefinition* def) {
    VertexDescriptor desc;
    int colorIndex = 0;
    int uvIndex = 0;

    for (const auto& member : def->members) {
        if (member.name == "Position") {
            DetectPositionFormat(member, desc);
        }
        else if (member.name == "BoneWeights") {
            DetectBoneWeightsFormat(member, desc);
        }
        else if (member.name == "BoneIndices") {
            // BoneIndices uses the same format as BoneWeights
            // Just note that we have bone data
        }
        else if (member.name == "Normal") {
            DetectNormalFormat(member, desc);
        }
        else if (member.name == "Tangent") {
            DetectTangentFormat(member, desc);
        }
        else if (member.name == "Binormal") {
            DetectBinormalFormat(member, desc);
        }
        else if (member.name == "QTangent") {
            // QTangent encodes Normal, Tangent, and Binormal
            desc.normalType = NormalType::QTangent;
            desc.tangentType = NormalType::QTangent;
            desc.binormalType = NormalType::QTangent;
        }
        else if (member.name.find("DiffuseColor") != std::string::npos) {
            DetectColorFormat(member, desc, colorIndex);
        }
        else if (member.name.find("TextureCoordinate") != std::string::npos) {
            DetectUVFormat(member, desc, uvIndex);
        }
    }

    return desc;
}

void VertexFormatDetector::DetectPositionFormat(const MemberDefinition& member, VertexDescriptor& desc) {
    if (member.type == MemberType::Real32 && member.arraySize == 3) {
        desc.positionType = PositionType::Float3;
    }
    else if ((member.type == MemberType::BinormalInt16 || member.type == MemberType::UInt16)
             && member.arraySize == 4) {
        desc.positionType = PositionType::Word4;
    }
}

void VertexFormatDetector::DetectBoneWeightsFormat(const MemberDefinition& member, VertexDescriptor& desc) {
    if (member.type == MemberType::NormalUInt8) {
        desc.hasBoneWeights = true;
        desc.numBoneInfluences = static_cast<int>(member.arraySize);
    }
}

void VertexFormatDetector::DetectNormalFormat(const MemberDefinition& member, VertexDescriptor& desc) {
    if (member.type == MemberType::Real32 && member.arraySize == 3) {
        desc.normalType = NormalType::Float3;
    }
    else if (member.type == MemberType::Real16 && member.arraySize == 4) {
        desc.normalType = NormalType::Half4;
    }
    else if (member.type == MemberType::BinormalInt8 && member.arraySize == 4) {
        desc.normalType = NormalType::Byte4;
    }
    else if (member.type == MemberType::BinormalInt16 && member.arraySize == 4) {
        desc.normalType = NormalType::QTangent;
    }
}

void VertexFormatDetector::DetectTangentFormat(const MemberDefinition& member, VertexDescriptor& desc) {
    if (member.type == MemberType::Real32 && member.arraySize == 3) {
        desc.tangentType = NormalType::Float3;
    }
    else if (member.type == MemberType::Real16 && member.arraySize == 4) {
        desc.tangentType = NormalType::Half4;
    }
    else if (member.type == MemberType::BinormalInt8 && member.arraySize == 4) {
        desc.tangentType = NormalType::Byte4;
    }
}

void VertexFormatDetector::DetectBinormalFormat(const MemberDefinition& member, VertexDescriptor& desc) {
    if (member.type == MemberType::Real32 && member.arraySize == 3) {
        desc.binormalType = NormalType::Float3;
    }
    else if (member.type == MemberType::Real16 && member.arraySize == 4) {
        desc.binormalType = NormalType::Half4;
    }
    else if (member.type == MemberType::BinormalInt8 && member.arraySize == 4) {
        desc.binormalType = NormalType::Byte4;
    }
}

void VertexFormatDetector::DetectColorFormat(const MemberDefinition& member, VertexDescriptor& desc, int& colorIndex) {
    if (member.type == MemberType::Real32 && member.arraySize == 4) {
        desc.colorMapType = ColorMapType::Float4;
        desc.colorMaps = std::max(desc.colorMaps, colorIndex + 1);
        colorIndex++;
    }
    else if (member.type == MemberType::NormalUInt8 && member.arraySize == 4) {
        desc.colorMapType = ColorMapType::Byte4;
        desc.colorMaps = std::max(desc.colorMaps, colorIndex + 1);
        colorIndex++;
    }
}

void VertexFormatDetector::DetectUVFormat(const MemberDefinition& member, VertexDescriptor& desc, int& uvIndex) {
    if (member.type == MemberType::Real32 && member.arraySize == 2) {
        desc.textureCoordinateType = TextureCoordinateType::Float2;
        desc.textureCoordinates = std::max(desc.textureCoordinates, uvIndex + 1);
        uvIndex++;
    }
    else if (member.type == MemberType::Real16 && member.arraySize == 2) {
        desc.textureCoordinateType = TextureCoordinateType::Half2;
        desc.textureCoordinates = std::max(desc.textureCoordinates, uvIndex + 1);
        uvIndex++;
    }
}

// ============================================================================
// Vertex Reading
// ============================================================================

Vertex VertexReader::ReadVertex(const VertexDescriptor& format) {
    Vertex vertex;
    vertex.format = format;

    // Read components in the order they appear in the file format
    // Position
    if (format.positionType != PositionType::None) {
        vertex.position = ReadPosition(format.positionType);
    }

    // Bone weights and indices
    if (format.hasBoneWeights) {
        vertex.boneWeights = ReadBoneWeights(format.numBoneInfluences);
        vertex.boneIndices = ReadBoneIndices(format.numBoneInfluences);
    }

    // Normal/Tangent/Binormal (or QTangent)
    if (format.normalType == NormalType::QTangent) {
        ReadQTangent(vertex.tangent, vertex.binormal, vertex.normal);
    } else {
        if (format.normalType != NormalType::None) {
            vertex.normal = ReadNormal(format.normalType);
        }
        if (format.tangentType != NormalType::None) {
            vertex.tangent = ReadTangent(format.tangentType);
        }
        if (format.binormalType != NormalType::None) {
            vertex.binormal = ReadBinormal(format.binormalType);
        }
    }

    // Colors
    for (int i = 0; i < format.colorMaps; i++) {
        vertex.SetColor(i, ReadColor(format.colorMapType));
    }

    // UVs
    for (int i = 0; i < format.textureCoordinates; i++) {
        vertex.SetUV(i, ReadUV(format.textureCoordinateType));
    }

    return vertex;
}

// ============================================================================
// Component Readers
// ============================================================================

Vector3 VertexReader::ReadPosition(PositionType type) {
    switch (type) {
        case PositionType::Float3: {
            float x = reader_->ReadFloat();
            float y = reader_->ReadFloat();
            float z = reader_->ReadFloat();
            return Vector3(x, y, z);
        }
        case PositionType::Word4: {
            uint16_t x = reader_->ReadUInt16();
            uint16_t y = reader_->ReadUInt16();
            uint16_t z = reader_->ReadUInt16();
            uint16_t w = reader_->ReadUInt16(); // Unused
            (void)w;
            // Convert from quantized to float
            return Vector3(
                static_cast<float>(x) / 65535.0f,
                static_cast<float>(y) / 65535.0f,
                static_cast<float>(z) / 65535.0f
            );
        }
        default:
            return Vector3::Zero();
    }
}

BoneWeight VertexReader::ReadBoneWeights(int numInfluences) {
    BoneWeight weight;
    weight.a = (numInfluences > 0) ? reader_->ReadUInt8() : 0;
    weight.b = (numInfluences > 1) ? reader_->ReadUInt8() : 0;
    weight.c = (numInfluences > 2) ? reader_->ReadUInt8() : 0;
    weight.d = (numInfluences > 3) ? reader_->ReadUInt8() : 0;
    return weight;
}

BoneWeight VertexReader::ReadBoneIndices(int numInfluences) {
    return ReadBoneWeights(numInfluences); // Same format
}

Vector3 VertexReader::ReadNormal(NormalType type) {
    switch (type) {
        case NormalType::Float3: {
            float x = reader_->ReadFloat();
            float y = reader_->ReadFloat();
            float z = reader_->ReadFloat();
            return Vector3(x, y, z);
        }
        case NormalType::Half4: {
            uint16_t x = reader_->ReadUInt16();
            uint16_t y = reader_->ReadUInt16();
            uint16_t z = reader_->ReadUInt16();
            reader_->ReadUInt16(); // Unused
            return HalfVector3ToFloat(x, y, z);
        }
        case NormalType::Byte4: {
            int8_t x = reader_->ReadInt8();
            int8_t y = reader_->ReadInt8();
            int8_t z = reader_->ReadInt8();
            reader_->ReadInt8(); // Unused
            return NormalSByteVector4ToFloat(x, y, z);
        }
        default:
            return Vector3::Zero();
    }
}

Vector3 VertexReader::ReadTangent(NormalType type) {
    return ReadNormal(type); // Same format
}

Vector3 VertexReader::ReadBinormal(NormalType type) {
    return ReadNormal(type); // Same format
}

void VertexReader::ReadQTangent(Vector3& tangent, Vector3& binormal, Vector3& normal) {
    // Read quaternion as 4x int16
    int16_t qx = reader_->ReadInt16();
    int16_t qy = reader_->ReadInt16();
    int16_t qz = reader_->ReadInt16();
    int16_t qw = reader_->ReadInt16();

    // Convert to float quaternion
    float x = qx / 32767.0f;
    float y = qy / 32767.0f;
    float z = qz / 32767.0f;
    float w = qw / 32767.0f;

    // Convert quaternion to TBN matrix
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

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

    // Normal is cross product of tangent and binormal
    // Flip based on quaternion W sign
    float crossX = tangent.y * binormal.z - tangent.z * binormal.y;
    float crossY = tangent.z * binormal.x - tangent.x * binormal.z;
    float crossZ = tangent.x * binormal.y - tangent.y * binormal.x;

    normal = Vector3(crossX, crossY, crossZ);
    if (w < 0.0f) {
        normal = normal * -1.0f;
    }
}

Vector4 VertexReader::ReadColor(ColorMapType type) {
    switch (type) {
        case ColorMapType::Float4: {
            float x = reader_->ReadFloat();
            float y = reader_->ReadFloat();
            float z = reader_->ReadFloat();
            float w = reader_->ReadFloat();
            return Vector4(x, y, z, w);
        }
        case ColorMapType::Byte4: {
            uint8_t x = reader_->ReadUInt8();
            uint8_t y = reader_->ReadUInt8();
            uint8_t z = reader_->ReadUInt8();
            uint8_t w = reader_->ReadUInt8();
            return NormalByteVector4ToFloat(x, y, z, w);
        }
        default:
            return Vector4();
    }
}

Vector2 VertexReader::ReadUV(TextureCoordinateType type) {
    switch (type) {
        case TextureCoordinateType::Float2: {
            float x = reader_->ReadFloat();
            float y = reader_->ReadFloat();
            return Vector2(x, y);
        }
        case TextureCoordinateType::Half2: {
            uint16_t x = reader_->ReadUInt16();
            uint16_t y = reader_->ReadUInt16();
            return HalfVector2ToFloat(x, y);
        }
        default:
            return Vector2();
    }
}

// ============================================================================
// Format Conversion Helpers
// ============================================================================

Vector2 VertexReader::HalfVector2ToFloat(uint16_t x, uint16_t y) {
    return Vector2(HalfToFloat(x), HalfToFloat(y));
}

Vector3 VertexReader::HalfVector3ToFloat(uint16_t x, uint16_t y, uint16_t z) {
    return Vector3(HalfToFloat(x), HalfToFloat(y), HalfToFloat(z));
}

Vector4 VertexReader::HalfVector4ToFloat(uint16_t x, uint16_t y, uint16_t z, uint16_t w) {
    return Vector4(HalfToFloat(x), HalfToFloat(y), HalfToFloat(z), HalfToFloat(w));
}

Vector3 VertexReader::NormalSByteVector4ToFloat(int8_t x, int8_t y, int8_t z) {
    return Vector3(
        x / 127.0f,
        y / 127.0f,
        z / 127.0f
    );
}

Vector3 VertexReader::NormalSWordVector4ToFloat(int16_t x, int16_t y, int16_t z) {
    return Vector3(
        x / 32767.0f,
        y / 32767.0f,
        z / 32767.0f
    );
}

Vector4 VertexReader::NormalByteVector4ToFloat(uint8_t x, uint8_t y, uint8_t z, uint8_t w) {
    return Vector4(
        x / 255.0f,
        y / 255.0f,
        z / 255.0f,
        w / 255.0f
    );
}

} // namespace gr2
