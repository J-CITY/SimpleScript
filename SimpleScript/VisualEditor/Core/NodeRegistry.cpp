#include "NodeRegistry.hpp"

using namespace Visual;

static const std::vector<NodePaletteEntry> s_entries = {
    // -- Constants / values --
    {"Types", "Int",    NodeType::ValueInt,    BlueprintCapability::Supported},
    {"Types", "Float",  NodeType::ValueFloat,  BlueprintCapability::Supported},
    {"Types", "Bool",   NodeType::ValueBool,   BlueprintCapability::Supported},
    {"Types", "String", NodeType::ValueString, BlueprintCapability::Supported},
    {"Types", "List",   NodeType::ValueList,   BlueprintCapability::Supported},
    {"Types", "Map",    NodeType::ValueMap,    BlueprintCapability::Supported},

    // -- Variables --
    {"Variables", "Variable", NodeType::Variable, BlueprintCapability::Supported},

    // -- Assignment --
    {"Set", "Set value", NodeType::Equal, BlueprintCapability::Supported},

    // -- Arithmetic operators --
    {"Binary ops", "+",  NodeType::Operator, BlueprintCapability::Supported},
    {"Binary ops", "-",  NodeType::Operator, BlueprintCapability::Supported},
    {"Binary ops", "*",  NodeType::Operator, BlueprintCapability::Supported},
    {"Binary ops", "/",  NodeType::Operator, BlueprintCapability::Supported},

    // -- Logic operators --
    {"Logic", "==", NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", "!=", NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", ">=", NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", "<=", NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", "<",  NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", ">",  NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", "||", NodeType::Operator, BlueprintCapability::Supported},
    {"Logic", "&&", NodeType::Operator, BlueprintCapability::Supported},

    // -- Control flow --
    {"Flow", "If / Else",   NodeType::If,       BlueprintCapability::Supported},
    {"Flow", "For",         NodeType::For,      BlueprintCapability::Supported},
    {"Flow", "While",       NodeType::While,    BlueprintCapability::Supported},
    {"Flow", "Return",      NodeType::Return,   BlueprintCapability::Supported},
    {"Flow", "Continue",    NodeType::Continue, BlueprintCapability::Supported},
    {"Flow", "Break",       NodeType::Break,    BlueprintCapability::Supported},

    // -- Functions --
    {"Functions", "Function Declaration", NodeType::FunctionDeclaration, BlueprintCapability::Supported},
    {"Functions", "Function Call",        NodeType::FunctionCall,        BlueprintCapability::Supported},

    // -- Classes --
    {"Classes", "Class Declaration",  NodeType::ClassDeclaration,      BlueprintCapability::Supported},
    {"Classes", "Method Declaration", NodeType::ClassMethodDeclaration, BlueprintCapability::Supported},
    {"Classes", "Method Call",        NodeType::ClassMethodCall,        BlueprintCapability::Supported},
    {"Classes", "This",               NodeType::This,                   BlueprintCapability::Supported},

    // -- Misc --
    {"Misc", "Import",  NodeType::Import,  BlueprintCapability::Supported},
    {"Misc", "Comment", NodeType::Comment, BlueprintCapability::Supported},

    // -- Unsupported (not shown in palette, rejected by validator) --
    // ForEach is deferred to Tier 2
    {"Flow", "ForEach", NodeType::ForEach, BlueprintCapability::Unsupported},
};

const std::vector<NodePaletteEntry>& NodeRegistry::entries() {
    return s_entries;
}

BlueprintCapability NodeRegistry::capability(NodeType t) {
    for (auto& e : s_entries)
        if (e.type == t) return e.capability;
    return BlueprintCapability::Unsupported;
}

bool NodeRegistry::isSupported(NodeType t) {
    return capability(t) == BlueprintCapability::Supported;
}
