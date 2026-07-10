#include "BpSourceMap.hpp"
#include <sstream>

// Minimal JSON support without nlohmann dependency
// We use a very simple hand-rolled format for the header comment.

using namespace Visual;

void BpSourceMap::add(int nodeId, int startLine, int endLine, NodeType kind) {
    entries.push_back({nodeId, startLine, endLine, kind});
}

std::optional<BpSourceEntry> BpSourceMap::findByNodeId(int nodeId) const {
    for (auto& e : entries)
        if (e.nodeId == nodeId) return e;
    return std::nullopt;
}

std::optional<BpSourceEntry> BpSourceMap::findByLine(int line) const {
    for (auto& e : entries)
        if (line >= e.startLine && line <= e.endLine) return e;
    return std::nullopt;
}

std::string BpSourceMap::toHeaderComment() const {
    std::ostringstream ss;
    ss << "// @bp-map {\"v\":1,\"graph\":\"" << graphName << "\",\"entries\":[";
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& e = entries[i];
        ss << "{\"node\":" << e.nodeId
           << ",\"sl\":"   << e.startLine
           << ",\"el\":"   << e.endLine
           << ",\"kind\":" << (int)e.kind << "}";
    }
    ss << "]}";
    return ss.str();
}

BpSourceMap BpSourceMap::fromHeaderComment(const std::string& firstLine) {
    BpSourceMap map;
    // Minimal parser: find "graph":"..." and "entries":[...]
    auto gPos = firstLine.find("\"graph\":\"");
    if (gPos != std::string::npos) {
        gPos += 9;
        auto gEnd = firstLine.find('"', gPos);
        if (gEnd != std::string::npos)
            map.graphName = firstLine.substr(gPos, gEnd - gPos);
    }
    // Parse entries array
    auto ePos = firstLine.find("\"entries\":[");
    while (ePos != std::string::npos) {
        auto nodePos = firstLine.find("\"node\":", ePos);
        if (nodePos == std::string::npos) break;
        nodePos += 7;
        int nodeId   = std::stoi(firstLine.substr(nodePos));
        auto slPos   = firstLine.find("\"sl\":", nodePos);
        auto elPos   = firstLine.find("\"el\":", nodePos);
        auto kindPos = firstLine.find("\"kind\":", nodePos);
        if (slPos == std::string::npos || elPos == std::string::npos || kindPos == std::string::npos) break;
        int startLine = std::stoi(firstLine.substr(slPos + 5));
        int endLine   = std::stoi(firstLine.substr(elPos + 5));
        int kind      = std::stoi(firstLine.substr(kindPos + 7));
        map.entries.push_back({nodeId, startLine, endLine, static_cast<NodeType>(kind)});
        ePos = firstLine.find("\"node\":", nodePos + 1);
    }
    return map;
}

// -----------------------------------------------------------------
// CodeEmitter
// -----------------------------------------------------------------

void CodeEmitter::emitRaw(std::string_view text) {
    code += text;
    for (char c : text)
        if (c == '\n') ++currentLine;
}

void CodeEmitter::emitDecorator(Id nodeId) {
    std::string dec = "@bp(node=" + std::to_string(nodeId) + ")\n";
    emitRaw(dec);
}

int CodeEmitter::beginNode(Id nodeId, NodeType kind) {
    return currentLine;
}

void CodeEmitter::endNode(Id nodeId, NodeType kind, int startLine) {
    map.add(nodeId, startLine, currentLine - 1, kind);
}

void CodeEmitter::appendToken(std::string_view token) {
    code += token;
    code += ' ';
}

void CodeEmitter::endToken() {
    code += ";\n";
    ++currentLine;
}

void CodeEmitter::endLineToken() {
    code += '\n';
    ++currentLine;
}

void CodeEmitter::addStartScopeToken() {
    code += '{';
    ++tabsCount;
}

void CodeEmitter::addEndScopeToken() {
    code += '}';
    --tabsCount;
}

void CodeEmitter::addTabsToken() {
    code += std::string(tabsCount, '\t');
}
