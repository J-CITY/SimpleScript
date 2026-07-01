#pragma once


#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "../imgui/imgui-node-editor/imgui_node_editor.h"

#include "VisualTypes.h"

namespace Visual
{
	struct Visitor;
	struct CodeGenerator;

	struct NodeDrawer;
    struct Node;

    //TODO:: remove node from pin
    struct Pin {
        ax::NodeEditor::PinId   id;
        Node* node = nullptr;
        std::string name;
        PinTypeDescriptor type;
        PinKind kind;
        bool isUniqueConnection = false;

        int avaliablePinType = 0;

        Pin(int id, std::string name, PinTypeDescriptor type, Node* node, PinKind kind, int avaliablePinType = 0) :
            id(id), name(std::move(name)), type(type), kind(kind), node(node), avaliablePinType(avaliablePinType)
        {
        }
    };

    struct Node {
        ax::NodeEditor::NodeId id;
        std::string name;
        std::vector<PinPtr> inputs;
        std::vector<PinPtr> outputs;
        ImColor color;
        NodeType type;
        ImVec2 size;

        std::string State;
        std::string SavedState;

        //PinTypeDescriptor computedType = PinTypeDescriptor{PinType::None};

        Node(int id, std::string name, NodeType type, ImColor color = ImColor(255, 255, 255)) :
            id(id), name(std::move(name)), color(color), type(type), size(0, 0) {
        }

        void addPin(PinPtr pin, PinKind type) {
            type == PinKind::Input ?
                inputs.push_back(pin) : outputs.push_back(pin);
        }

        //virtual void setInputPinsType(PinTypeDescriptor type) {
	    //    for (auto& e : inputs) {
        //        e->type = type;
	    //    }
        //}

        virtual void checkInputPinType(PinTypeDescriptor type) {};

        virtual ~Node() {}

        virtual void run(Visitor& drawer);

       // virtual void updateComputedType() {}
    };

    struct StartNode : public Node {
        StartNode(int id, ImColor color = ImColor(255, 255, 255)) :
            Node(id, "StartNode", NodeType::Start, color) {
        }

        void run(Visitor& drawer) override;
    };

    struct ValueNode : public Node {
        ValuePtr value;

        ValueNode(int id, std::string name, NodeType type, Value val, ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), type, color) {
            value = std::make_shared<Value>();
        	value->value = val.value;
        }

        void run(Visitor& drawer) override;
    };

    
    struct OperatorNode : public Node {
        OperatorType opType;

        OperatorNode(int id, std::string name, OperatorType opType, ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), NodeType::Operator, color), opType(opType){
        }

        void run(Visitor& drawer) override;

        //void updateComputedType() override;
        //void setInputPinsType(PinTypeDescriptor type) override {
        //    Node::setInputPinsType(type);
        //
        //    static std::set<OperatorType> types = { OperatorType::Plus, OperatorType::Minus, OperatorType::Multiply, OperatorType::Divided };
        //    if (types.contains(opType)) {
        //        outputs[0]->type = type;
        //    }
        //}
    };

    struct VariableNode : public Node {
        std::string value; //var name
        //bool isInit = false;

        VariableNode(int id, std::string name, std::string val, ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), NodeType::Variable, color) {
            value = val;
            //computedType = varType;
        }

        void run(Visitor& drawer) override;
    };

    struct ImportNode : public Node {
        std::string value;

        ImportNode(int id, std::string name, std::string val, ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), NodeType::Import, color) {
            value = val;
        }

        void run(Visitor& drawer) override;
    };

    struct CommentNode : public Node {
        std::string value;

        CommentNode(int id, std::string val, ImColor color = ImColor(255, 255, 255)) :
            Node(id, "Comment", NodeType::Comment, color) {
            value = val;
        }

        void run(Visitor& drawer) override;
    };

    struct FunNode : public Node {
        std::string funName = "FunName";

        std::map<int, std::pair<std::string, PinTypeDescriptor>> inputVars;
        PinTypeDescriptor outputType = { PinType::None };

        FunNode(int id, std::string name,  ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), NodeType::FunctionDeclaration, color) {
        }

        void run(Visitor& drawer) override;
    };

    struct FunCall : public Node {
        Id funDeclarationId;

        FunCall(int id, Id funDeclarationId, ImColor color = ImColor(255, 255, 255)) :
            Node(id, "FunCall", NodeType::FunctionCall, color), funDeclarationId(funDeclarationId) {
        }

        void run(Visitor& drawer) override;
    };

    struct EqualNode : public Node {
        EqualNode(int id, ImColor color = ImColor(255, 255, 255)) :
            Node(id, "=", NodeType::Equal, color) {
        }

        void run(Visitor& drawer) override;
    };

    struct ClassNode : public Node {
        std::string className = "ClassName";

        struct Member {
            std::string name;
            PinTypeDescriptor type;
        };

        std::map<int, Member> memberVars;
        std::map<std::string, int> membersToPin;

        ClassNode(int id, std::string name, ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), NodeType::ClassDeclaration, color) {
        }

        void run(Visitor& drawer) override;
    };

    struct MethodNode : public Node {
        std::string funName = "FunName";

        std::map<int, std::pair<std::string, PinTypeDescriptor>> inputVars;
        PinTypeDescriptor outputType = { PinType::None };
        Id classNodeId = 0;

        MethodNode(int id, std::string name, Id classNodeId, ImColor color = ImColor(255, 255, 255)) :
            Node(id, std::move(name), NodeType::ClassMethodDeclaration, color), classNodeId(classNodeId){
        }

        void run(Visitor& drawer) override;
    };

    struct MethodCall : public Node {
        Id methodeDeclarationId = 0;

        MethodCall(int id, Id methodeDeclarationId, ImColor color = ImColor(255, 255, 255)) :
            Node(id, "MemberCall", NodeType::ClassMethodCall, color), methodeDeclarationId(methodeDeclarationId) {
        }

        void run(Visitor& drawer) override;
    };

    struct ThisNode: public Node {
        Id classId = 0;
        ThisNode(int id, Id classId, ImColor color = ImColor(255, 255, 255)) :
            Node(id, "This", NodeType::This, color), classId(classId){
        }

        void run(Visitor& drawer) override;
    };

    struct Link {
	    ax::NodeEditor::LinkId id;

        ax::NodeEditor::PinId startPinID;
        ax::NodeEditor::PinId endPinID;

        ImColor color;

        Link(int id, ax::NodeEditor::PinId startPinId, ax::NodeEditor::PinId endPinId) :
            id(id), startPinID(startPinId), endPinID(endPinId), color(255, 255, 255)
        {
        }
    };
}