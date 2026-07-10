#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "GraphTypes.hpp"

// Overloaded helper for std::visit
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace Visual {

    struct Pin;
    struct Link;
    struct Node;
    struct Value;

    using NodePtr  = std::shared_ptr<Visual::Node>;
    using NodePtrOpt = std::optional<NodePtr>;
    using LinkPtr  = std::shared_ptr<Visual::Link>;
    using LinkPtrOpt = std::optional<LinkPtr>;
    using PinPtr   = std::shared_ptr<Visual::Pin>;
    using PinPtrOpt = std::optional<PinPtr>;

    // -----------------------------------------------------------------
    // Visual-layer runtime value (used only for ValueNode storage)
    // -----------------------------------------------------------------

    using ArrayType = std::variant<std::vector<bool>, std::vector<int>, std::vector<float>>;

    class Class {
    public:
        std::string name;
        std::map<std::string, Value> members;
        Id classDeclarationId = 0;
    };

    using ValueType = std::variant<bool, int, float, std::string,
        ArrayType,
        std::vector<std::shared_ptr<Value>>,
        std::unordered_map<int, std::shared_ptr<Value>>,
        std::shared_ptr<Class>,
        void*>;

    struct Value {
        std::optional<ValueType> value;
    };

    using ValuePtr = std::shared_ptr<Value>;

    // -----------------------------------------------------------------
    // Scope (used in GraphContext for codegen variable tracking)
    // -----------------------------------------------------------------

    struct Scope;
    using ScopePtr = std::shared_ptr<Visual::Scope>;

    struct Scope {
        ScopePtr parent;
        std::unordered_map<std::string, ValuePtr> variables;
    };

    // -----------------------------------------------------------------
    // Pin / Node type enumerations
    // -----------------------------------------------------------------

    enum class PinType {
        None = 0,
        Bool,
        Int,
        Float,
        String,
        ArrayBool,
        ArrayInt,
        ArrayFloat,
        List,
        Map,
        Object,
        Function,
        Container,
        Any,
        Variable,
        Flow,
    };

    struct PinTypeDescriptor;
    using PinSubtype = std::optional<std::variant<std::string, std::shared_ptr<PinTypeDescriptor>>>;

    struct PinTypeDescriptor {
        PinType type = PinType::None;
        PinSubtype subType;

        std::string& getStr() {
            return std::get<std::string>(subType.value());
        }
        std::shared_ptr<PinTypeDescriptor> getPinDescr() const {
            if (!subType) return std::make_shared<PinTypeDescriptor>();
            return std::get<std::shared_ptr<PinTypeDescriptor>>(subType.value());
        }
    };

    enum class PinKind {
        Output,
        Input
    };

    enum class NodeType {
        ValueInt,
        ValueFloat,
        ValueBool,
        ValueString,
        ValueArrayInt,
        ValueArrayFloat,
        ValueArrayBool,
        ValueList,
        ValueMap,
        ValueClass,

        Variable,

        Operator,
        Equal,
        FunctionCall,
        FunctionDeclaration,

        ClassDeclaration,
        ClassMethodCall,
        ClassMethodDeclaration,

        If,
        While,
        For,
        ForEach,

        Return,
        Continue,
        Break,
        Import,

        This,

        Comment,

        Start,
    };

    enum class OperatorType {
        Plus,
        Minus,
        Multiply,
        Divided,
        Less,
        EqualLess,
        Great,
        EqualGreat,
        Inequality,
        Equality,
        LogicalAnd,
        LogicalOr,
    };

} // namespace Visual
