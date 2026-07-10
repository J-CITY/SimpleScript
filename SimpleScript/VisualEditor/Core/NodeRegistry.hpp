#pragma once
#include <string>
#include <vector>

#include "PinTypes.hpp"

namespace Visual {

    // Whether a node type can be used in visual blueprints.
    enum class BlueprintCapability {
        Supported,    // fully supported; appears in palette
        Unsupported,  // language feature not mappable to nodes; hidden from palette
    };

    struct NodePaletteEntry {
        std::string   group;
        std::string   label;
        NodeType      type;
        BlueprintCapability capability;
        // Factory info stored separately in VisualCodeEditor
    };

    // Registry of all node types with their capability flags.
    // Used by the UI to populate the palette and by GraphValidator to reject
    // unsupported nodes before code generation.
    struct NodeRegistry {
        static const std::vector<NodePaletteEntry>& entries();
        static BlueprintCapability capability(NodeType t);
        static bool isSupported(NodeType t);
    };

} // namespace Visual
