#pragma once
#include <optional>
#include <string>
#include <vector>

#include "../Core/PinTypes.hpp"

namespace Visual {

    // One entry: a node in the visual graph ↔ a range of lines in the generated script.
    struct BpSourceEntry {
        int      nodeId;
        int      startLine;
        int      endLine;
        NodeType kind;
    };

    // Maps blueprint node IDs ↔ line ranges in the generated .ik script.
    // Serialized as a single-line JSON comment at the top of the generated file so
    // the host can recover it without re-running the generator.
    struct BpSourceMap {
        std::string graphName;
        std::vector<BpSourceEntry> entries;

        void add(int nodeId, int startLine, int endLine, NodeType kind);

        // Look up by node ID (for UI: which lines to highlight given active node).
        std::optional<BpSourceEntry> findByNodeId(int nodeId) const;

        // Look up by line (for text editor: which node is active at this line).
        std::optional<BpSourceEntry> findByLine(int line) const;

        // Serialize to a single-line JSON comment for embedding in script header.
        // Format:  // @bp-map {"v":1,"graph":"...","entries":[...]}
        std::string toHeaderComment() const;

        // Parse the header comment back to a BpSourceMap.
        static BpSourceMap fromHeaderComment(const std::string& firstLine);
    };

    // -----------------------------------------------------------------
    // Helper used by CodeGenerator to track line counts while emitting.
    // -----------------------------------------------------------------

    struct CodeEmitter {
        std::string  code;
        BpSourceMap  map;
        int          currentLine = 1;

        // Append raw text, updating line counter.
        void emitRaw(std::string_view text);

        // Emit the @bp(node=N) decorator line (for var / fun / class statements).
        void emitDecorator(Id nodeId);

        // Mark start of a statement block for a given node.
        int  beginNode(Id nodeId, NodeType kind);

        // Mark end and record entry in the source map.
        void endNode(Id nodeId, NodeType kind, int startLine);

        // Convenience helpers that mirror the old CodeGenerator API.
        void appendToken(std::string_view token);
        void endToken();          // appends ";\n" and bumps line
        void endLineToken();      // appends "\n" and bumps line
        void addStartScopeToken();
        void addEndScopeToken();
        void addTabsToken();

        int  tabsCount = 0;
    };

} // namespace Visual
