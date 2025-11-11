#include "gr2/model.h"
#include <sstream>

namespace gr2 {

// ============================================================================
// VertexDescriptor Implementation
// ============================================================================

std::string VertexDescriptor::GetName() const {
    std::ostringstream vertexFormat;
    std::ostringstream attributeCounts;

    switch (positionType) {
        case PositionType::Float3:
            vertexFormat << "P";
            attributeCounts << "3";
            break;
        case PositionType::Word4:
            vertexFormat << "PW";
            attributeCounts << "4";
            break;
        case PositionType::None:
            break;
    }

    if (hasBoneWeights) {
        vertexFormat << "W";
        attributeCounts << numBoneInfluences;
    }

    switch (normalType) {
        case NormalType::Float3:
            vertexFormat << "N";
            attributeCounts << "3";
            break;
        case NormalType::Half4:
            vertexFormat << "HN";
            attributeCounts << "4";
            break;
        case NormalType::QTangent:
            vertexFormat << "QN";
            attributeCounts << "4";
            break;
        case NormalType::Byte4:
            vertexFormat << "BN";
            attributeCounts << "4";
            break;
        case NormalType::None:
            break;
    }

    switch (tangentType) {
        case NormalType::Float3:
            vertexFormat << "G";
            attributeCounts << "3";
            break;
        case NormalType::Half4:
            vertexFormat << "HG";
            attributeCounts << "4";
            break;
        case NormalType::None:
        default:
            break;
    }

    switch (binormalType) {
        case NormalType::Float3:
            vertexFormat << "B";
            attributeCounts << "3";
            break;
        case NormalType::Half4:
            vertexFormat << "HB";
            attributeCounts << "4";
            break;
        case NormalType::None:
        default:
            break;
    }

    for (int i = 0; i < colorMaps; i++) {
        switch (colorMapType) {
            case ColorMapType::Float4:
                vertexFormat << "D";
                attributeCounts << "4";
                break;
            case ColorMapType::Byte4:
                vertexFormat << "CD";
                attributeCounts << "4";
                break;
            case ColorMapType::None:
                break;
        }
    }

    for (int i = 0; i < textureCoordinates; i++) {
        switch (textureCoordinateType) {
            case TextureCoordinateType::Float2:
                vertexFormat << "T";
                attributeCounts << "2";
                break;
            case TextureCoordinateType::Half2:
                vertexFormat << "HT";
                attributeCounts << "2";
                break;
            case TextureCoordinateType::None:
                break;
        }
    }

    return vertexFormat.str() + attributeCounts.str();
}

// ============================================================================
// Root Implementation
// ============================================================================

std::shared_ptr<Root> Root::CreateEmpty() {
    auto root = std::make_shared<Root>();
    root->zUp = false;
    root->gr2Tag = 0;
    return root;
}

} // namespace gr2
