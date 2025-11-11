#include "gr2/reader.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <sstream>

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
            uint32_t result = sign << 31;
            return *reinterpret_cast<float*>(&result);
        } else {
            exponent = 1;
            while ((mantissa & 0x400) == 0) {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x3FF;
        }
    } else if (exponent == 31) {
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
    float x = (static_cast<float>(qx) / 32767.5f) - 1.0f;
    float y = (static_cast<float>(qy) / 32767.5f) - 1.0f;
    float z = (static_cast<float>(qz) / 32767.5f) - 1.0f;
    float w = (static_cast<float>(qw) / 32767.5f) - 1.0f;

    float len = std::sqrt(x * x + y * y + z * z + w * w);
    if (len > 0.0001f) {
        x /= len; y /= len; z /= len; w /= len;
    }

    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    tangent = Vector3(1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy));
    binormal = Vector3(2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx));
    normal = Vector3(2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy));
}

// ============================================================================
// Null Decompressor Implementation
// ============================================================================

std::vector<uint8_t> NullDecompressor::Decompress(
    int format, const uint8_t*, size_t, size_t, int, int, int) {
    throw ParsingException(
        "Compressed GR2 file detected but no decompressor available. "
        "Files with Oodle compression require granny2.dll support.");
}

std::vector<uint8_t> NullDecompressor::Decompress4(
    const uint8_t*, size_t, size_t) {
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

GR2Reader::~GR2Reader() = default;

void GR2Reader::SetDecompressor(std::shared_ptr<IDecompressor> decompressor) {
    decompressor_ = decompressor;
}

// ============================================================================
// Reading Primitives
// ============================================================================

void GR2Reader::ReadBytes(uint8_t* buffer, size_t count) {
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

int8_t GR2Reader::ReadInt8() {
    return static_cast<int8_t>(ReadUInt8());
}

uint16_t GR2Reader::ReadUInt16() {
    uint16_t value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 2);
    return value;
}

int16_t GR2Reader::ReadInt16() {
    return static_cast<int16_t>(ReadUInt16());
}

uint32_t GR2Reader::ReadUInt32() {
    uint32_t value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 4);
    return value;
}

int32_t GR2Reader::ReadInt32() {
    return static_cast<int32_t>(ReadUInt32());
}

uint64_t GR2Reader::ReadUInt64() {
    uint64_t value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 8);
    return value;
}

float GR2Reader::ReadFloat() {
    float value;
    ReadBytes(reinterpret_cast<uint8_t*>(&value), 4);
    return value;
}

std::string GR2Reader::ReadStringDirect() {
    std::string result;
    char ch;
    while ((ch = static_cast<char>(ReadUInt8())) != '\0') {
        result += ch;
    }
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
// Reference Reading
// ============================================================================

RelocatableReference GR2Reader::ReadReference() {
    RelocatableReference ref;
    if (magic_.Is32Bit()) {
        ref.offset = ReadUInt32();
    } else {
        ref.offset = ReadUInt64();
    }
    return ref;
}

StructReference GR2Reader::ReadStructReference() {
    StructReference ref;
    if (magic_.Is32Bit()) {
        ref.offset = ReadUInt32();
    } else {
        ref.offset = ReadUInt64();
    }
    return ref;
}

StringReference GR2Reader::ReadStringReference() {
    StringReference ref;
    if (magic_.Is32Bit()) {
        ref.offset = ReadUInt32();
    } else {
        ref.offset = ReadUInt64();
    }
    return ref;
}

ArrayReference GR2Reader::ReadArrayReference() {
    ArrayReference ref;
    ref.size = ReadUInt32();
    if (magic_.Is32Bit()) {
        ref.offset = ReadUInt32();
    } else {
        ref.offset = ReadUInt64();
    }
    return ref;
}

// ============================================================================
// Section Processing
// ============================================================================

void GR2Reader::UncompressStream() {
    size_t totalSize = 0;
    for (const auto& section : sections_) {
        totalSize += section.header.uncompressedSize;
    }

    uncompressedData_.resize(totalSize);
    size_t currentPos = 0;

    for (auto& section : sections_) {
        const auto& hdr = section.header;

        std::vector<uint8_t> sectionContents(hdr.compressedSize);
        inputStream_.seekg(hdr.offsetInFile);
        inputStream_.read(reinterpret_cast<char*>(sectionContents.data()), hdr.compressedSize);

        const_cast<SectionHeader&>(section.header).offsetInFile = static_cast<uint32_t>(currentPos);

        if (hdr.compression == 0) {
            std::memcpy(&uncompressedData_[currentPos], sectionContents.data(), hdr.compressedSize);
        } else if (hdr.uncompressedSize > 0) {
            std::vector<uint8_t> decompressed;

            if (hdr.compression == 4) {
                decompressed = decompressor_->Decompress4(
                    sectionContents.data(), hdr.compressedSize, hdr.uncompressedSize);
            } else {
                decompressed = decompressor_->Decompress(
                    hdr.compression, sectionContents.data(), hdr.compressedSize,
                    hdr.uncompressedSize, hdr.first16bit, hdr.first8bit, hdr.uncompressedSize);
            }

            std::memcpy(&uncompressedData_[currentPos], decompressed.data(), decompressed.size());
        }

        currentPos += hdr.uncompressedSize;
    }

    uncompressedPos_ = 0;
}

void GR2Reader::ReadSectionRelocations(Section& section) {
    if (section.header.numRelocations == 0) return;

    inputStream_.seekg(section.header.relocationsOffset);

    for (uint32_t i = 0; i < section.header.numRelocations; i++) {
        uint32_t offsetInSection = ReadUInt32();
        SectionReference ref = ReadSectionReference();

        uint32_t targetAddress = sections_[ref.section].header.offsetInFile + ref.offset;
        uint32_t ptrLocation = section.header.offsetInFile + offsetInSection;
        std::memcpy(&uncompressedData_[ptrLocation], &targetAddress, 4);
    }
}

void GR2Reader::ReadSectionMixedMarshallingRelocations(Section& section) {
    if (magic_.IsLittleEndian()) {
        return;
    }
    // TODO: Implement endian swapping if needed for big-endian files
}

// ============================================================================
// Type System
// ============================================================================

MemberDefinition GR2Reader::ReadMemberDefinition() {
    MemberDefinition member;

    int32_t typeId = ReadInt32();
    if (typeId < 0 || typeId > static_cast<int>(MemberType::Max)) {
        member.type = MemberType::None;
        return member;
    }

    member.type = static_cast<MemberType>(typeId);
    auto nameRef = ReadStringReference();
    if (nameRef.IsValid()) {
        member.name = ReadString(nameRef);
    }

    member.definition = ReadStructReference();
    member.arraySize = ReadUInt32();

    for (int i = 0; i < 3; i++) {
        member.extra[i] = ReadUInt32();
    }

    if (magic_.Is32Bit()) {
        member.unknown = ReadUInt32();
    } else {
        member.unknown = static_cast<uint32_t>(ReadUInt64());
    }

    return member;
}

StructDefinition* GR2Reader::ReadStructDefinition() {
    auto def = std::make_unique<StructDefinition>();

    while (true) {
        auto member = ReadMemberDefinition();
        if (member.IsValid()) {
            def->members.push_back(member);
        } else {
            break;
        }
    }

    return def.release();
}

StructDefinition* GR2Reader::GetOrReadStructDefinition(uint32_t offset) {
    auto it = types_.find(offset);
    if (it != types_.end()) {
        return it->second.get();
    }

    SavePosition();
    Seek(offset);
    auto def = ReadStructDefinition();
    types_[offset] = std::unique_ptr<StructDefinition>(def);
    RestorePosition();

    return def;
}

// ============================================================================
// String Reading
// ============================================================================

std::string GR2Reader::ReadString(const StringReference& ref) {
    if (!ref.IsValid()) return "";

    SavePosition();
    Seek(ref.offset);
    std::string result = ReadStringDirect();
    RestorePosition();

    return result;
}

// ============================================================================
// Transform Reading
// ============================================================================

Transform GR2Reader::ReadTransform() {
    Transform transform;

    transform.flags = ReadUInt32();

    transform.translation.x = ReadFloat();
    transform.translation.y = ReadFloat();
    transform.translation.z = ReadFloat();

    transform.rotation.x = ReadFloat();
    transform.rotation.y = ReadFloat();
    transform.rotation.z = ReadFloat();
    transform.rotation.w = ReadFloat();

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            transform.scaleShear[i][j] = ReadFloat();
        }
    }

    return transform;
}

// ============================================================================
// Generic Struct Member Reading
// ============================================================================

void GR2Reader::ReadStructMembers(
    StructDefinition* def,
    const std::function<void(const MemberDefinition&)>& handler) {

    for (const auto& member : def->members) {
        handler(member);
    }
}

// ============================================================================
// Main Read Function
// ============================================================================

std::shared_ptr<Root> GR2Reader::Read() {
    magic_ = ReadMagic();
    header_ = ReadHeader();

    if (magic_.format != Magic::Format::LittleEndian32 &&
        magic_.format != Magic::Format::LittleEndian64) {
        throw ParsingException("Only little-endian GR2 files are supported");
    }

    sections_.resize(header_.numSections);
    for (uint32_t i = 0; i < header_.numSections; i++) {
        sections_[i].header = ReadSectionHeader();
    }

    UncompressStream();

    for (auto& section : sections_) {
        ReadSectionRelocations(section);
    }

    if (!magic_.IsLittleEndian()) {
        for (auto& section : sections_) {
            ReadSectionMixedMarshallingRelocations(section);
        }
    }

    uint32_t rootTypeOffset = ResolveReference(header_.rootType);
    GetOrReadStructDefinition(rootTypeOffset);

    Seek(header_.rootNode);
    return ReadRoot();
}

// ============================================================================
// Root Deserialization
// ============================================================================

std::shared_ptr<Root> GR2Reader::ReadRoot() {
    auto root = Root::CreateEmpty();
    root->gr2Tag = header_.tag;

    uint32_t rootTypeOffset = ResolveReference(header_.rootType);
    auto rootDef = GetOrReadStructDefinition(rootTypeOffset);

    ReadStructMembers(rootDef, [&](const MemberDefinition& member) {
        if (member.name == "Skeletons") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->skeletons = ReadArrayOfReferences<Skeleton>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadSkeleton(def); });
            }
        }
        else if (member.name == "Meshes") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->meshes = ReadArrayOfReferences<Mesh>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadMesh(def); });
            }
        }
        else if (member.name == "Materials") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->materials = ReadArrayOfReferences<Material>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadMaterial(def); });
            }
        }
        else if (member.name == "Textures") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->textures = ReadArrayOfReferences<Texture>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadTexture(def); });
            }
        }
        else if (member.name == "VertexDatas") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->vertexDatas = ReadArrayOfReferences<VertexData>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadVertexData(def); });
            }
        }
        else if (member.name == "TriTopologies") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->triTopologies = ReadArrayOfReferences<TriTopology>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadTriTopology(def); });
            }
        }
        else if (member.name == "Models") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                root->models = ReadArrayOfReferences<Model>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadModel(def); });
            }
        }
        else {
            // Skip unknown members - read the data to advance position
            switch (member.type) {
                case MemberType::String: ReadStringReference(); break;
                case MemberType::Reference: ReadReference(); break;
                case MemberType::ArrayOfReferences: ReadArrayReference(); break;
                case MemberType::ReferenceToArray:
                case MemberType::ReferenceToVariantArray:
                    ReadArrayReference();
                    break;
                case MemberType::Int32: ReadInt32(); break;
                case MemberType::UInt32: ReadUInt32(); break;
                case MemberType::Real32: ReadFloat(); break;
                default: break;
            }
        }
    });

    return root;
}

// ============================================================================
// Skeleton Deserialization
// ============================================================================

std::shared_ptr<Skeleton> GR2Reader::ReadSkeleton(StructDefinition* def) {
    auto skeleton = std::make_shared<Skeleton>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "Name") {
            auto str = ReadStringReference();
            skeleton->name = ReadString(str);
        }
        else if (member.name == "Bones") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                skeleton->bones = ReadArrayOfReferences<Bone>(
                    arrayRef,
                    GetOrReadStructDefinition(member.definition.offset),
                    [this](StructDefinition* def) { return ReadBone(def); });
            }
        }
        else if (member.name == "LODType") {
            skeleton->lodType = ReadInt32();
        }
        else {
            // Skip unknown
            if (member.type == MemberType::String) ReadStringReference();
            else if (member.type == MemberType::Int32) ReadInt32();
        }
    });

    return skeleton;
}

// ============================================================================
// Bone Deserialization
// ============================================================================

std::shared_ptr<Bone> GR2Reader::ReadBone(StructDefinition* def) {
    auto bone = std::make_shared<Bone>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "Name") {
            auto str = ReadStringReference();
            bone->name = ReadString(str);
        }
        else if (member.name == "ParentIndex") {
            bone->parentIndex = ReadInt32();
        }
        else if (member.name == "LocalTransform") {
            bone->transform = ReadTransform();
        }
        else if (member.name == "InverseWorld4x4") {
            for (int i = 0; i < 16; i++) {
                bone->inverseWorldTransform[i] = ReadFloat();
            }
        }
        else if (member.name == "LODError") {
            bone->lodError = ReadInt32();
        }
        else {
            // Skip
            if (member.type == MemberType::String) ReadStringReference();
            else if (member.type == MemberType::Int32) ReadInt32();
            else if (member.type == MemberType::Transform) ReadTransform();
        }
    });

    return bone;
}

// ============================================================================
// Mesh, VertexData, TriTopology Deserialization (Simplified)
// ============================================================================

std::shared_ptr<Mesh> GR2Reader::ReadMesh(StructDefinition* def) {
    auto mesh = std::make_shared<Mesh>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "Name") {
            auto str = ReadStringReference();
            mesh->name = ReadString(str);
        }
        else if (member.name == "PrimaryVertexData") {
            auto ref = ReadReference();
            if (ref.IsValid()) {
                SavePosition();
                Seek(ref);
                mesh->primaryVertexData = ReadVertexData(
                    GetOrReadStructDefinition(member.definition.offset));
                RestorePosition();
            }
        }
        else if (member.name == "PrimaryTopology") {
            auto ref = ReadReference();
            if (ref.IsValid()) {
                SavePosition();
                Seek(ref);
                mesh->primaryTopology = ReadTriTopology(
                    GetOrReadStructDefinition(member.definition.offset));
                RestorePosition();
            }
        }
        else {
            // Skip other members for simplicity
            if (member.type == MemberType::String) ReadStringReference();
            else if (member.type == MemberType::Reference) ReadReference();
            else if (member.type == MemberType::ArrayOfReferences) ReadArrayReference();
        }
    });

    return mesh;
}

std::shared_ptr<VertexData> GR2Reader::ReadVertexData(StructDefinition* def) {
    auto vertexData = std::make_shared<VertexData>();

    // Simplified - just skip the data for now
    // Full implementation would parse vertex formats and data
    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.type == MemberType::Reference) ReadReference();
        else if (member.type == MemberType::ArrayOfReferences) ReadArrayReference();
        else if (member.type == MemberType::ReferenceToArray) ReadArrayReference();
        else if (member.type == MemberType::Int32) ReadInt32();
        else if (member.type == MemberType::UInt32) ReadUInt32();
    });

    return vertexData;
}

std::shared_ptr<TriTopology> GR2Reader::ReadTriTopology(StructDefinition* def) {
    auto topology = std::make_shared<TriTopology>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "Indices") {
            auto arrayRef = ReadArrayReference();
            if (arrayRef.IsValid() && arrayRef.size > 0) {
                SavePosition();
                Seek(arrayRef.offset);
                topology->indices.resize(arrayRef.size);
                for (uint32_t i = 0; i < arrayRef.size; i++) {
                    if (topology->bytesPerIndex == 2) {
                        topology->indices[i] = ReadUInt16();
                    } else {
                        topology->indices[i] = ReadUInt32();
                    }
                }
                RestorePosition();
            }
        }
        else if (member.name == "BytesPerIndex") {
            topology->bytesPerIndex = ReadInt32();
        }
        else {
            if (member.type == MemberType::ReferenceToArray) ReadArrayReference();
            else if (member.type == MemberType::Int32) ReadInt32();
        }
    });

    return topology;
}

// ============================================================================
// Material and Texture Deserialization
// ============================================================================

std::shared_ptr<Material> GR2Reader::ReadMaterial(StructDefinition* def) {
    auto material = std::make_shared<Material>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "Name") {
            auto str = ReadStringReference();
            material->name = ReadString(str);
        }
        else {
            if (member.type == MemberType::String) ReadStringReference();
            else if (member.type == MemberType::Reference) ReadReference();
            else if (member.type == MemberType::ArrayOfReferences) ReadArrayReference();
        }
    });

    return material;
}

std::shared_ptr<Texture> GR2Reader::ReadTexture(StructDefinition* def) {
    auto texture = std::make_shared<Texture>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "FileName") {
            auto str = ReadStringReference();
            texture->fileName = ReadString(str);
        }
        else if (member.name == "Name") {
            auto str = ReadStringReference();
            texture->name = ReadString(str);
        }
        else {
            if (member.type == MemberType::String) ReadStringReference();
        }
    });

    return texture;
}

// ============================================================================
// Model Deserialization
// ============================================================================

std::shared_ptr<Model> GR2Reader::ReadModel(StructDefinition* def) {
    auto model = std::make_shared<Model>();

    ReadStructMembers(def, [&](const MemberDefinition& member) {
        if (member.name == "Name") {
            auto str = ReadStringReference();
            model->name = ReadString(str);
        }
        else if (member.name == "InitialPlacement") {
            model->initialPlacement = ReadTransform();
        }
        else {
            if (member.type == MemberType::String) ReadStringReference();
            else if (member.type == MemberType::Transform) ReadTransform();
            else if (member.type == MemberType::Reference) ReadReference();
            else if (member.type == MemberType::ArrayOfReferences) ReadArrayReference();
        }
    });

    return model;
}

// ============================================================================
// Array Reading Templates
// ============================================================================

template<typename T>
std::vector<std::shared_ptr<T>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference& arrayRef,
    StructDefinition* elementDef,
    std::function<std::shared_ptr<T>(StructDefinition*)> readFunc) {

    std::vector<std::shared_ptr<T>> result;

    if (!arrayRef.IsValid() || arrayRef.size == 0) {
        return result;
    }

    SavePosition();
    Seek(arrayRef.offset);

    // Read array of pointers
    std::vector<RelocatableReference> refs(arrayRef.size);
    for (uint32_t i = 0; i < arrayRef.size; i++) {
        refs[i] = ReadReference();
    }

    // Follow each pointer and read the object
    for (const auto& ref : refs) {
        if (ref.IsValid()) {
            Seek(ref);
            result.push_back(readFunc(elementDef));
        }
    }

    RestorePosition();
    return result;
}

template<typename T>
std::vector<std::shared_ptr<T>> GR2Reader::ReadReferenceToArray(
    const ArrayReference& arrayRef,
    StructDefinition* elementDef,
    std::function<std::shared_ptr<T>(StructDefinition*)> readFunc) {

    std::vector<std::shared_ptr<T>> result;

    if (!arrayRef.IsValid() || arrayRef.size == 0) {
        return result;
    }

    SavePosition();
    Seek(arrayRef.offset);

    for (uint32_t i = 0; i < arrayRef.size; i++) {
        result.push_back(readFunc(elementDef));
    }

    RestorePosition();
    return result;
}

// ============================================================================
// Helpers
// ============================================================================

void GR2Reader::Seek(const SectionReference& ref) {
    uint32_t offset = ResolveReference(ref);
    Seek(offset);
}

void GR2Reader::Seek(uint32_t offset) {
    uncompressedPos_ = offset;
}

void GR2Reader::Seek(const RelocatableReference& ref) {
    uncompressedPos_ = static_cast<size_t>(ref.offset);
}

uint32_t GR2Reader::Tell() const {
    return static_cast<uint32_t>(uncompressedPos_);
}

uint32_t GR2Reader::ResolveReference(const SectionReference& ref) const {
    return sections_[ref.section].header.offsetInFile + ref.offset;
}

void GR2Reader::SavePosition() {
    positionStack_.push(Tell());
}

void GR2Reader::RestorePosition() {
    if (!positionStack_.empty()) {
        Seek(positionStack_.top());
        positionStack_.pop();
    }
}

// Explicit template instantiations
template std::vector<std::shared_ptr<Skeleton>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<Skeleton>(StructDefinition*)>);
template std::vector<std::shared_ptr<Bone>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<Bone>(StructDefinition*)>);
template std::vector<std::shared_ptr<Mesh>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<Mesh>(StructDefinition*)>);
template std::vector<std::shared_ptr<Material>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<Material>(StructDefinition*)>);
template std::vector<std::shared_ptr<Texture>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<Texture>(StructDefinition*)>);
template std::vector<std::shared_ptr<VertexData>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<VertexData>(StructDefinition*)>);
template std::vector<std::shared_ptr<TriTopology>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<TriTopology>(StructDefinition*)>);
template std::vector<std::shared_ptr<Model>> GR2Reader::ReadArrayOfReferences(
    const ArrayReference&, StructDefinition*, std::function<std::shared_ptr<Model>(StructDefinition*)>);

} // namespace gr2
