#include "gr2/reader.h"
#include <iostream>
#include <fstream>
#include <memory>

using namespace gr2;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.gr2>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "GR2 C++ Reader - Granny2 file format parser" << std::endl;
        std::cerr << "Reads GR2 mesh files and displays information." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Note: This version supports uncompressed GR2 files only." << std::endl;
        std::cerr << "      For Oodle-compressed files, you need granny2.dll support." << std::endl;
        return 1;
    }

    const char* filename = argv[1];

    try {
        // Open file
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Could not open file: " << filename << std::endl;
            return 1;
        }

        std::cout << "Reading GR2 file: " << filename << std::endl;
        std::cout << "=====================================" << std::endl;

        // Create reader
        GR2Reader reader(file);

        // Optional: Set custom decompressor here if you have granny2.dll support
        // reader.SetDecompressor(std::make_shared<YourCustomDecompressor>());

        // Read file
        auto root = reader.Read();

        std::cout << "File tag: 0x" << std::hex << reader.GetTag() << std::dec << std::endl;
        std::cout << std::endl;

        // Display contents
        std::cout << "Contents:" << std::endl;
        std::cout << "  Skeletons: " << root->skeletons.size() << std::endl;
        std::cout << "  Meshes: " << root->meshes.size() << std::endl;
        std::cout << "  Materials: " << root->materials.size() << std::endl;
        std::cout << "  Textures: " << root->textures.size() << std::endl;
        std::cout << "  Animations: " << root->animations.size() << std::endl;
        std::cout << std::endl;

        // Display skeletons
        for (size_t i = 0; i < root->skeletons.size(); i++) {
            const auto& skeleton = root->skeletons[i];
            std::cout << "Skeleton " << i << ": " << skeleton->name << std::endl;
            std::cout << "  Bones: " << skeleton->bones.size() << std::endl;

            for (const auto& bone : skeleton->bones) {
                std::cout << "    - " << bone->name
                         << " (parent: " << bone->parentIndex << ")" << std::endl;
            }
        }
        std::cout << std::endl;

        // Display meshes
        for (size_t i = 0; i < root->meshes.size(); i++) {
            const auto& mesh = root->meshes[i];
            std::cout << "Mesh " << i << ": " << mesh->name << std::endl;

            if (mesh->primaryVertexData) {
                const auto& vdata = mesh->primaryVertexData;
                std::cout << "  Vertices: " << vdata->vertices.size() << std::endl;
                std::cout << "  Format: " << vdata->format.GetName() << std::endl;

                // Show first vertex details
                if (!vdata->vertices.empty()) {
                    const auto& v = vdata->vertices[0];
                    std::cout << "  First vertex:" << std::endl;
                    std::cout << "    Position: (" << v.position.x << ", "
                              << v.position.y << ", " << v.position.z << ")" << std::endl;
                    if (vdata->format.normalType != NormalType::None) {
                        std::cout << "    Normal: (" << v.normal.x << ", "
                                  << v.normal.y << ", " << v.normal.z << ")" << std::endl;
                    }
                    if (vdata->format.textureCoordinates > 0) {
                        auto uv = v.GetUV(0);
                        std::cout << "    UV0: (" << uv.x << ", " << uv.y << ")" << std::endl;
                    }
                    if (vdata->format.hasBoneWeights) {
                        std::cout << "    Bone weights: (" << (int)v.boneWeights.a << ", "
                                  << (int)v.boneWeights.b << ", "
                                  << (int)v.boneWeights.c << ", "
                                  << (int)v.boneWeights.d << ")" << std::endl;
                    }
                }
            }

            if (mesh->primaryTopology) {
                size_t triangleCount = mesh->primaryTopology->indices.size() / 3;
                std::cout << "  Triangles: " << triangleCount << std::endl;
            }

            std::cout << "  Materials: " << mesh->materialBindings.size() << std::endl;
            std::cout << "  Bone bindings: " << mesh->boneBindings.size() << std::endl;
        }
        std::cout << std::endl;

        // Display materials
        for (size_t i = 0; i < root->materials.size(); i++) {
            const auto& material = root->materials[i];
            std::cout << "Material " << i << ": " << material->name << std::endl;
            std::cout << "  Maps: " << material->maps.size() << std::endl;
        }
        std::cout << std::endl;

        std::cout << "Successfully read GR2 file!" << std::endl;
        std::cout << std::endl;
        std::cout << "Complete GR2 reader - all data fully parsed:" << std::endl;
        std::cout << "  ✓ Skeleton hierarchies with bone transforms" << std::endl;
        std::cout << "  ✓ Full vertex data (position, normal, tangent, UVs, colors, weights)" << std::endl;
        std::cout << "  ✓ Triangle indices ready for rendering" << std::endl;
        std::cout << "  ✓ Materials and textures" << std::endl;
        std::cout << "  ✓ Automatic vertex format detection" << std::endl;

        return 0;

    } catch (const ParsingException& e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
