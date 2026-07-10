#pragma once
#include "../Core/GraphContext.hpp"
#include <string_view>

namespace Visual {

    // Loads a graph from a JSON file into GraphContext.
    // Does NOT call any ImGui/NodeEditor APIs — position data is stored in
    // node.layoutPosition. The UI layer applies layout after loading via
    // NodeEditorSession::applyLayout().
    struct GraphLoader {
        // Returns the maximum ID found (so IdGenerator can be advanced past it).
        static int load(GraphContext& ctx, std::string_view path);
    };

} // namespace Visual
