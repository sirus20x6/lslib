# Granny2 Reader - Minimum C++ Port Guide

## Executive Summary

This document outlines the minimum code needed to read Granny2 (GR2) files for 3D viewing from C++. The current LSLib implementation is written in **C#** (approximately 26,000 lines). For basic mesh viewing, you'll need to port approximately **4,600 lines** of core functionality.

## What You Get

A GR2 reader that can extract:
- **Meshes** - Vertex positions, normals, tangents, UVs, colors
- **Skeletons** - Bone hierarchies with transforms
- **Skinning** - Bone weights and indices for skeletal animation
- **Materials** - Material bindings and texture references
- **Topology** - Triangle indices

## Compression Support

The GR2 format supports different compression types:
- **Type 0**: Uncompressed (no dependencies)
- **Type 1-3, 4**: Oodle compression (requires granny2.dll from RAD Game Tools)

Since you have LZ4, note that **GR2 files don't use LZ4 compression natively**. The LZ4 code in LSLib is used for other game file formats (PAK, LSV), not GR2 files. For GR2, you'll either need:
1. Uncompressed GR2 files only, OR
2. Access to granny2.dll for Oodle decompression

## Minimum File Set for C++ Port

### Core Reading Engine (~2,500 lines C#)

| File | Lines | Purpose |
|------|-------|---------|
| `LSLib/Granny/GR2/Format.cs` | 1,236 | File format structures, serialization attributes, type system |
| `LSLib/Granny/GR2/Reader.cs` | 1,102 | Main GR2 parser, section decompression, relocation handling |
| `LSLib/Granny/GR2/Helpers.cs` | 160 | Utility functions for reading/writing primitives |

### Data Model (~2,100 lines C#)

| File | Lines | Purpose |
|------|-------|---------|
| `LSLib/Granny/Model/Root.cs` | 177 | Root container for all GR2 data |
| `LSLib/Granny/Model/Mesh.cs` | 824 | Mesh data structures, vertex/index data |
| `LSLib/Granny/Model/Vertex.cs` | 719 | Vertex formats and data types |
| `LSLib/Granny/Model/Skeleton.cs` | 382 | Bone hierarchy and transforms |

### Supporting Code (~800 lines C#)

| File | Lines | Purpose |
|------|-------|---------|
| `LSLib/Granny/Model/VertexSerialization.cs` | 605 | Vertex format serialization |
| `LSLib/Granny/Model/VertexHelpers.cs` | 249 | Vertex conversion utilities |

**Total: ~4,600 lines**

### Optional: Animation Support (+1,100 lines)

Add these for animation playback:
| File | Lines | Purpose |
|------|-------|---------|
| `LSLib/Granny/Model/Animation.cs` | 839 | Animation tracks and curves |
| `LSLib/Granny/Model/CurveData/*.cs` | ~17 files | Animation curve compression formats |

## Key Data Structures

### Root Container
```csharp
public class Root {
    public List<Skeleton> Skeletons;
    public List<VertexData> VertexDatas;
    public List<TriTopology> TriTopologies;
    public List<Mesh> Meshes;
    public List<Model> Models;
    public List<TrackGroup> TrackGroups;      // Animations
    public List<Animation> Animations;
    public List<Material> Materials;
    public List<Texture> Textures;
}
```

### Mesh Structure
```csharp
public class Mesh {
    public string Name;
    public VertexData PrimaryVertexData;      // Vertex buffer
    public TriTopology PrimaryTopology;       // Index buffer
    public List<MaterialBinding> MaterialBindings;
    public List<BoneBinding> BoneBindings;    // For skinned meshes
}
```

### Vertex Data
```csharp
public class Vertex {
    public Vector3 Position;
    public Vector3 Normal, Tangent, Binormal;
    public Vector2[] TextureCoordinates;      // Up to 8 UV channels
    public Vector4[] Colors;                  // Up to 2 color channels
    public BoneWeight BoneWeights;            // 4 influences max
    public BoneWeight BoneIndices;
}
```

Vertex formats supported:
- **Position**: Float3, Word4 (quantized)
- **Normals/Tangents**: Float3, Half4, Byte4, QTangent (quaternion compressed)
- **UVs**: Float2, Half2
- **Colors**: Float4, Byte4

### Skeleton Structure
```csharp
public class Skeleton {
    public string Name;
    public List<Bone> Bones;
    public Int32 LODType;
}

public class Bone {
    public string Name;
    public int ParentIndex;
    public Transform Transform;               // Local transform
    public float[] InverseWorldTransform;     // 4x4 matrix for skinning
}
```

### Transform Structure
```csharp
public class Transform {
    public Vector3 Translation;
    public Quaternion Rotation;
    public Matrix3 ScaleShear;               // 3x3 matrix
    public UInt32 Flags;                     // Indicates which components are present
}
```

## GR2 File Format Overview

### File Structure
```
[Magic Header]        32 bytes - Format signature
[File Header]         Variable - Version, CRC, sections
[Section Headers]     64 bytes each
[Section Data]        Compressed/uncompressed data
[Relocations]         Pointer fixup tables
```

### Magic Header (32 bytes)
```cpp
struct Magic {
    uint8_t signature[16];    // Format identifier
    uint32_t headersSize;     // Offset where sections begin
    uint32_t headerFormat;    // 0 = uncompressed headers
    uint32_t reserved[2];
};
```

Supported formats:
- Little-endian 32-bit (most common)
- Little-endian 64-bit
- Big-endian 32/64-bit

### File Header
```cpp
struct Header {
    uint32_t version;              // 6 or 7
    uint32_t fileSize;
    uint32_t crc;
    uint32_t sectionsOffset;
    uint32_t numSections;
    SectionReference rootType;     // Points to type definition
    SectionReference rootNode;     // Points to Root object
    uint32_t tag;                  // Game identifier
    uint32_t extraTags[4];

    // Version 7 only:
    uint32_t stringTableCrc;
    uint32_t reserved[3];
};
```

### Section Header (64 bytes)
```cpp
struct SectionHeader {
    uint32_t compression;                 // 0=none, 4=Oodle
    uint32_t offsetInFile;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint32_t alignment;
    uint32_t first16bit;                  // For endian swapping
    uint32_t first8bit;
    uint32_t relocationsOffset;           // Pointer fixup table
    uint32_t numRelocations;
    uint32_t mixedMarshallingDataOffset;
    uint32_t numMixedMarshallingData;
};
```

### Section Types
```cpp
enum SectionType {
    Main = 0,              // Main data structures
    RigidVertex = 1,       // Non-skinned vertices
    DeformableVertex = 2,  // Skinned vertices
    RigidIndex = 3,        // Rigid mesh indices
    DeformableIndex = 4,   // Deformable mesh indices
    Skeleton = 5,          // Bone data
    Mesh = 6,              // Mesh structures
    TrackGroup = 7,        // Animation data
    DiscardablePadding = 8 // Unused
};
```

## Reading Algorithm

### High-Level Flow
```cpp
1. Read Magic header → Detect format (endianness, 32/64-bit)
2. Read File header
3. Read Section headers (N sections)
4. Decompress sections
   - If compression == 0: Copy as-is
   - If compression == 4: Oodle decompress (needs granny2.dll)
   - Else: Oodle decompress with parameters
5. Apply relocations
   - Fix up all pointers in decompressed data
6. Parse type definitions
7. Deserialize Root object using type system
8. Access mesh data from Root
```

### Detailed Steps

**Step 1-3: Read Headers**
```cpp
Magic magic = ReadMagic();
Header header = ReadHeader();
vector<SectionHeader> sections(header.numSections);
for (int i = 0; i < header.numSections; i++) {
    sections[i] = ReadSectionHeader();
}
```

**Step 4: Decompress Sections**
```cpp
vector<uint8_t> uncompressed;
for (auto& section : sections) {
    if (section.compression == 0) {
        // Copy uncompressed
        copy(input + section.offsetInFile,
             input + section.offsetInFile + section.compressedSize,
             back_inserter(uncompressed));
    } else {
        // Oodle decompress (requires granny2.dll)
        auto decompressed = OodleDecompress(
            input + section.offsetInFile,
            section.compressedSize,
            section.uncompressedSize);
        copy(decompressed.begin(), decompressed.end(),
             back_inserter(uncompressed));
    }
}
```

**Step 5: Apply Relocations**

Relocations convert file offsets to memory addresses:
```cpp
for (auto& section : sections) {
    for (int i = 0; i < section.numRelocations; i++) {
        uint32_t offsetInSection = ReadUInt32();
        SectionReference ref = ReadSectionReference();

        // Calculate target address
        uint32_t targetAddress = sections[ref.section].offsetInFile + ref.offset;

        // Fix up pointer
        WriteUInt32At(section.offsetInFile + offsetInSection, targetAddress);
    }
}
```

**Step 6-7: Type System**

GR2 uses a reflection-based type system. Each structure has a type definition:
```cpp
struct StructDefinition {
    vector<MemberDefinition> members;
};

struct MemberDefinition {
    string name;
    MemberType type;           // Inline, Reference, Array, String, etc.
    uint32_t offset;
    StructDefinition* structDef;  // For nested structures
};
```

The reader uses these type definitions to recursively deserialize objects.

**Step 8: Access Data**
```cpp
Root* root = ParseRoot(header.rootNode);

// Extract mesh data
for (auto& mesh : root->meshes) {
    vector<Vertex>& vertices = mesh->primaryVertexData->vertices;
    vector<uint32_t>& indices = mesh->primaryTopology->indices;

    // vertices and indices are now ready for rendering
}
```

## Dependencies

### Math Library
You'll need a C++ math library for:
- `Vector2`, `Vector3`, `Vector4`
- `Quaternion`
- `Matrix3`, `Matrix4`

Recommendations:
- **GLM** (OpenGL Mathematics) - Header-only, similar to OpenTK
- **Eigen** - High-performance linear algebra
- **Your own** - Implement basic types

### Binary I/O
Standard C++ streams are sufficient:
- `std::ifstream`
- `std::vector<uint8_t>` for buffers
- Endianness conversion (if needed)

### Compression (Optional)
For Oodle compression support:
- **granny2.dll** from RAD Game Tools (proprietary)
- Or stick to uncompressed GR2 files

## C++ Port Architecture

### Recommended Structure
```
gr2/
├── format.h              // Format structures (Magic, Header, Section)
├── format.cpp
├── reader.h              // Main GR2Reader class
├── reader.cpp
├── types.h               // Type system (StructDefinition, MemberDefinition)
├── types.cpp
├── model/
│   ├── root.h            // Root container
│   ├── mesh.h            // Mesh structures
│   ├── vertex.h          // Vertex formats
│   ├── skeleton.h        // Bone hierarchy
│   └── animation.h       // Animation data (optional)
└── compression.h         // Oodle wrapper (optional)
```

### Key Implementation Challenges

**1. Serialization Attributes**

C# uses attributes for serialization:
```csharp
[Serialization(Type = MemberType.ArrayOfReferences)]
public List<Mesh> Meshes;
```

In C++, you'll need to implement a reflection system or manually specify type info:
```cpp
class Root {
public:
    vector<Mesh*> meshes;

    static TypeInfo GetTypeInfo() {
        return TypeInfo {
            {"meshes", MemberType::ArrayOfReferences, offsetof(Root, meshes)}
        };
    }
};
```

**2. Polymorphic Deserialization**

The type system reads unknown types at runtime. You'll need:
- Dynamic type creation
- Recursive structure parsing
- Pointer fixup tracking

**3. Vertex Format Handling**

GR2 supports many vertex formats. The reader needs to:
- Detect format from type definition
- Convert compressed formats (Half, Byte, QTangent) to Float
- Handle variable number of UV channels and color maps

**4. Endianness**

Big-endian files require byte swapping. Implement:
```cpp
template<typename T>
T ByteSwap(T value) {
    if constexpr (sizeof(T) == 2) {
        return (value << 8) | (value >> 8);
    } else if constexpr (sizeof(T) == 4) {
        return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) |
               ((value & 0xFF0000) >> 8) | ((value >> 24) & 0xFF);
    }
    // etc.
}
```

## Simplified Approach: Format Documentation

If porting 4,600 lines seems daunting, you have alternatives:

### Option 1: Use C# Library via Interop
Create a C++/CLI wrapper around LSLib:
```cpp
// C++/CLI wrapper
public ref class GR2Wrapper {
public:
    static array<float>^ ReadVertices(String^ filename) {
        auto reader = gcnew GR2Reader(File::OpenRead(filename));
        auto root = gcnew Root();
        reader->Read(root);
        // Convert to flat array and return
    }
};
```

Then call from native C++:
```cpp
// Native C++
auto vertices = GR2Wrapper::ReadVertices("model.gr2");
```

### Option 2: Export to Intermediate Format
Use the existing C# tools to convert GR2 → GLTF:
```bash
# Use Divine CLI tool
divine.exe -s "model.gr2" -g "model.glb" -l info
```

Then load GLTF in your C++ viewer (many libraries available).

### Option 3: Minimal C++ Reader
Implement a simplified reader that:
- Only handles uncompressed files
- Only supports specific vertex formats you need
- Skips animations/advanced features

This could reduce the port to ~1,500 lines.

## Usage Example (C#)

Here's how the existing C# library is used:
```csharp
using var fs = File.OpenRead("model.gr2");
var reader = new GR2Reader(fs);
var root = new Root();
reader.Read(root);

// Access mesh data
foreach (var mesh in root.Meshes) {
    Console.WriteLine($"Mesh: {mesh.Name}");

    var vertices = mesh.PrimaryVertexData.Vertices;
    var indices = mesh.PrimaryTopology.Indices;

    Console.WriteLine($"  Vertices: {vertices.Count}");
    Console.WriteLine($"  Triangles: {indices.Length / 3}");

    // Extract position data
    foreach (var vert in vertices) {
        float x = vert.Position.X;
        float y = vert.Position.Y;
        float z = vert.Position.Z;
        // Use vertex data...
    }
}

// Access skeleton
if (root.Skeletons.Count > 0) {
    var skeleton = root.Skeletons[0];
    foreach (var bone in skeleton.Bones) {
        Console.WriteLine($"Bone: {bone.Name}, Parent: {bone.ParentIndex}");
    }
}
```

## Next Steps

### Recommended Approach

1. **Prototype with C# first**
   - Use LSLib directly to understand GR2 structure
   - Export test files to GLTF to verify correctness

2. **Define your requirements**
   - Which vertex formats do you need?
   - Do you need animations?
   - Can you use uncompressed GR2 files only?

3. **Choose implementation strategy**
   - Full port (~2-3 weeks)
   - C++/CLI wrapper (~1-2 days)
   - Intermediate format (immediate)

4. **Start with core types**
   - Implement math types (Vector3, Matrix4, etc.)
   - Port Format.cs structures
   - Port Reader.cs parsing logic

5. **Test incrementally**
   - Start with simple uncompressed files
   - Verify magic/header reading
   - Test section decompression
   - Validate data extraction

## Resources

- **LSLib Repository**: https://github.com/Norbyte/lslib
- **License**: MIT (free to use, modify, redistribute)
- **Granny2 SDK**: Contact RAD Game Tools for official C++ SDK
- **GLTF Alternative**: https://github.com/KhronosGroup/glTF

## File Manifest

Here are the exact files you'd need to port from LSLib:

### Critical (Must have)
```
LSLib/Granny/GR2/Format.cs          - 1,236 lines
LSLib/Granny/GR2/Reader.cs          - 1,102 lines
LSLib/Granny/GR2/Helpers.cs         - 160 lines
LSLib/Granny/Model/Root.cs          - 177 lines
LSLib/Granny/Model/Mesh.cs          - 824 lines
LSLib/Granny/Model/Vertex.cs        - 719 lines
LSLib/Granny/Model/Skeleton.cs      - 382 lines
```

### Important (Highly recommended)
```
LSLib/Granny/Model/VertexSerialization.cs  - 605 lines
LSLib/Granny/Model/VertexHelpers.cs        - 249 lines
```

### Optional (For animations)
```
LSLib/Granny/Model/Animation.cs            - 839 lines
LSLib/Granny/Model/CurveData/*.cs          - 17 files
```

### Skip (Export only, not needed for reading)
```
LSLib/Granny/GR2/Writer.cs          - Writing GR2 files
LSLib/Granny/Model/*Exporter.cs     - GLTF/Collada export
LSLib/Granny/Model/*Importer.cs     - Format import
LSLib/Granny/Collada*.cs            - Collada conversion
```

## Conclusion

You have three realistic options:

1. **Full C++ Port** - 4,600 lines, 2-3 weeks, maximum control
2. **C++/CLI Wrapper** - Minimal code, 1-2 days, requires .NET
3. **GLTF Pipeline** - No coding, immediate, adds conversion step

For a 3D viewer specifically, I'd recommend **Option 3** (GLTF) unless you have specific requirements that prevent it. The existing LSLib tools can batch-convert GR2 → GLTF, and there are excellent C++ GLTF loaders available.

If you must read GR2 directly in C++, the full port is well-documented here and the MIT license allows it freely.
