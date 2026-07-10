#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "GraphTypes.hpp"
#include "PinTypes.hpp"

namespace Visual {

    struct Visitor;

    // -----------------------------------------------------------------
    // Pin
    // -----------------------------------------------------------------

    struct Pin {
        Id   id    = 0;
        Node* node = nullptr;
        std::string name;
        PinTypeDescriptor type;
        PinKind kind;
        bool isUniqueConnection = false;
        int  availablePinType   = 0;

        Pin(Id id, std::string name, PinTypeDescriptor type, Node* node, PinKind kind,
            int availablePinType = 0)
            : id(id), name(std::move(name)), type(type), kind(kind),
              node(node), availablePinType(availablePinType) {}
    };

    // -----------------------------------------------------------------
    // Base Node
    // -----------------------------------------------------------------

    struct Node {
        Id       id    = 0;
        std::string  name;
        std::vector<PinPtr> inputs;
        std::vector<PinPtr> outputs;
        ColorRGBA color;
        NodeType  type;
        Vec2      layoutPosition;   // stored in model; UI syncs on drag
        Vec2      size;
        bool      isHighlighted = false; // set by ExecutionObserver for debug UI

        std::string state;
        std::string savedState;

        Node(Id id, std::string name, NodeType type,
             ColorRGBA color = ColorRGBA(255, 255, 255))
            : id(id), name(std::move(name)), color(color), type(type) {}

        void addPin(PinPtr pin, PinKind k) {
            k == PinKind::Input ? inputs.push_back(pin) : outputs.push_back(pin);
        }

        virtual void checkInputPinType(PinTypeDescriptor) {}
        virtual ~Node() = default;
        virtual void accept(Visitor& v);
    };

    // -----------------------------------------------------------------
    // Concrete node types
    // -----------------------------------------------------------------

    struct StartNode : Node {
        StartNode(Id id, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, "StartNode", NodeType::Start, color) {}
        void accept(Visitor& v) override;
    };

    struct ValueNode : Node {
        ValuePtr value;

        ValueNode(Id id, std::string name, NodeType type, Value val,
                  ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), type, color) {
            value = std::make_shared<Value>();
            value->value = val.value;
        }
        void accept(Visitor& v) override;
    };

    struct OperatorNode : Node {
        OperatorType opType;

        OperatorNode(Id id, std::string name, OperatorType opType,
                     ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), NodeType::Operator, color), opType(opType) {}
        void accept(Visitor& v) override;
    };

    struct VariableNode : Node {
        std::string value; // variable name

        VariableNode(Id id, std::string name, std::string val,
                     ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), NodeType::Variable, color), value(std::move(val)) {}
        void accept(Visitor& v) override;
    };

    struct ImportNode : Node {
        std::string value;

        ImportNode(Id id, std::string name, std::string val,
                   ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), NodeType::Import, color), value(std::move(val)) {}
        void accept(Visitor& v) override;
    };

    struct CommentNode : Node {
        std::string value;

        CommentNode(Id id, std::string val, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, "Comment", NodeType::Comment, color), value(std::move(val)) {}
        void accept(Visitor& v) override;
    };

    struct FunNode : Node {
        std::string funName = "FunName";
        std::map<int, std::pair<std::string, PinTypeDescriptor>> inputVars;
        PinTypeDescriptor outputType = {PinType::None};

        FunNode(Id id, std::string name, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), NodeType::FunctionDeclaration, color) {}
        void accept(Visitor& v) override;
    };

    struct FunCall : Node {
        Id funDeclarationId = 0;

        FunCall(Id id, Id funDeclarationId, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, "FunCall", NodeType::FunctionCall, color),
              funDeclarationId(funDeclarationId) {}
        void accept(Visitor& v) override;
    };

    struct EqualNode : Node {
        EqualNode(Id id, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, "=", NodeType::Equal, color) {}
        void accept(Visitor& v) override;
    };

    struct ClassNode : Node {
        std::string className = "ClassName";

        struct Member {
            std::string name;
            PinTypeDescriptor type;
        };

        std::map<int, Member> memberVars;
        std::map<std::string, int> membersToPin;

        ClassNode(Id id, std::string name, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), NodeType::ClassDeclaration, color) {}
        void accept(Visitor& v) override;
    };

    struct MethodNode : Node {
        std::string funName = "FunName";
        std::map<int, std::pair<std::string, PinTypeDescriptor>> inputVars;
        PinTypeDescriptor outputType = {PinType::None};
        Id classNodeId = 0;

        MethodNode(Id id, std::string name, Id classNodeId,
                   ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, std::move(name), NodeType::ClassMethodDeclaration, color),
              classNodeId(classNodeId) {}
        void accept(Visitor& v) override;
    };

    struct MethodCall : Node {
        Id methodeDeclarationId = 0;

        MethodCall(Id id, Id methodeDeclarationId, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, "MemberCall", NodeType::ClassMethodCall, color),
              methodeDeclarationId(methodeDeclarationId) {}
        void accept(Visitor& v) override;
    };

    struct ThisNode : Node {
        Id classId = 0;

        ThisNode(Id id, Id classId, ColorRGBA color = ColorRGBA(255, 255, 255))
            : Node(id, "This", NodeType::This, color), classId(classId) {}
        void accept(Visitor& v) override;
    };

    // -----------------------------------------------------------------
    // Link
    // -----------------------------------------------------------------

    struct Link {
        Id id          = 0;
        Id startPinID  = 0;
        Id endPinID    = 0;
        ColorRGBA color;

        Link(Id id, Id startPinId, Id endPinId)
            : id(id), startPinID(startPinId), endPinID(endPinId),
              color(255, 255, 255) {}
    };

} // namespace Visual
