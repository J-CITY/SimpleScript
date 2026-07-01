#pragma once
#include <map>
#include <memory>

#include "VisualCodeEditor.h"
#include "VisualNodes.h"
#include "VisualTypes.h"

namespace Visual
{
	struct FunNode;
	struct Link;
    struct Pin;
    struct Node;

    struct CodeEditorContext {
        static inline CodeEditorContext& Instance() {
            static CodeEditorContext instance;
            return instance;
        }

        CodeEditorContext();

        ~CodeEditorContext();
        ScopePtr pushScope();
        void resetScope();
        ScopePtr popScope();
        ValuePtr findInScope(const std::string& id);
        void addInScope(const std::string& id, ValuePtr val);
        void updateInScope(const std::string& id, ValuePtr val);

        std::unique_ptr<ax::NodeEditor::EditorContext, void(*)(ax::NodeEditor::EditorContext*)> context;

        ScopePtr global;
        ScopePtr current;
        std::unordered_set<Id> initVariables;

        std::map<Id, NodePtr> nodes;
        std::map<Id, std::shared_ptr<FunNode>> funNodes;
        std::map<Id, std::shared_ptr<MethodNode>> methodNodes;
        std::map<Id, std::shared_ptr<ClassNode>> classNodes;
        std::map<Id, LinkPtr> links;
        std::map<Id, PinPtr> pins;

        std::shared_ptr<StartNode> startNode;

        void clear() {
            nodes.clear();
            funNodes.clear();
            classNodes.clear();
            links.clear();
            pins.clear();
        }

        PinPtrOpt getPinById(Id id);
        LinkPtrOpt getLinkById(Id id);
        LinkPtrOpt getLinkByPinId(Id id);
        Id getLinkIdByPinId(Id id);
        NodePtrOpt getNodeById(Id id);

        template<class T, class... Args>
        std::shared_ptr<T> createNode(Id id, Args... args) {
            auto node = std::make_shared<T>(id, args...);
            nodes[node->id.Get()] = node;
            return node;
        }

        LinkPtr createLink(Id id, ax::NodeEditor::PinId idStart, ax::NodeEditor::PinId idEnd);
        PinPtr createPin(Id id, std::string_view name, PinTypeDescriptor type, Node* node, PinKind kind);

        void registerFunctionCall(NodePtr node);
        void registerMethodCall(NodePtr node);
        void registerClassCall(NodePtr node);

        void deletePin(Id id);
        void deleteLink(Id id);
        void deleteNode(Id id);

        LinkPtr findLinkByPinId(ax::NodeEditor::PinId id);
        PinPtr findPin(ax::NodeEditor::PinId id);
        bool isPinLinked(ax::NodeEditor::PinId id);

        void deleteAllLinksWithPinId(ax::NodeEditor::PinId id) {
	        for (auto it = links.begin(); it != links.end(); ) {
                if (it->second->startPinID == id || it->second->endPinID == id) {
                    it = links.erase(it);
                }
                ++it;
	        }
        }

        NodePtr getNextNodeByPin(Pin& pin);
        PinPtr getNextPin(Pin& pin);
    };
}
