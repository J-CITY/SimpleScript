#pragma once
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>

#include "GraphModel.hpp"

namespace Visual {

    struct FunNode;
    struct MethodNode;
    struct ClassNode;

    // -----------------------------------------------------------------
    // IdGenerator — simple monotonic integer ID pool
    // -----------------------------------------------------------------

    struct IdGenerator {
        static Id GenId() { return id++; }
        static void SetStart(Id start) { id = start; }
    private:
        inline static Id id = 1;
    };

    // -----------------------------------------------------------------
    // GraphContext — graph data and query helpers (no ImGui dependency)
    // -----------------------------------------------------------------

    struct GraphContext {
        static GraphContext& Instance() {
            static GraphContext instance;
            return instance;
        }

        GraphContext();
        ~GraphContext() = default;

        // Core data maps
        std::map<Id, NodePtr>   nodes;
        std::map<Id, LinkPtr>   links;
        std::map<Id, PinPtr>    pins;
        std::map<Id, std::shared_ptr<FunNode>>    funNodes;
        std::map<Id, std::shared_ptr<MethodNode>> methodNodes;
        std::map<Id, std::shared_ptr<ClassNode>>  classNodes;

        std::shared_ptr<StartNode> startNode;

        // Codegen variable-init tracking
        std::unordered_set<Id> initVariables;
        ScopePtr global;
        ScopePtr current;

        void clear();

        // Scope management
        ScopePtr pushScope();
        void     resetScope();
        ScopePtr popScope();
        ValuePtr findInScope(const std::string& id);
        void     addInScope(const std::string& id, ValuePtr val);
        void     updateInScope(const std::string& id, ValuePtr val);

        // Queries
        PinPtrOpt  getPinById(Id id);
        LinkPtrOpt getLinkById(Id id);
        LinkPtrOpt getLinkByPinId(Id id);
        Id         getLinkIdByPinId(Id id);
        NodePtrOpt getNodeById(Id id);

        // Factory helpers
        template<class T, class... Args>
        std::shared_ptr<T> createNode(Id id, Args&&... args) {
            auto node = std::make_shared<T>(id, std::forward<Args>(args)...);
            nodes[node->id] = node;
            return node;
        }

        LinkPtr createLink(Id id, Id startPinId, Id endPinId);
        PinPtr  createPin(Id id, std::string_view name, PinTypeDescriptor type,
                          Node* node, PinKind kind);

        void registerFunctionCall(NodePtr node);
        void registerMethodCall(NodePtr node);
        void registerClassCall(NodePtr node);

        // Deletion
        void deletePin(Id id);
        void deleteLink(Id id);
        void deleteNode(Id id);

        void deleteAllLinksWithPinId(Id pinId) {
            for (auto it = links.begin(); it != links.end();) {
                if (it->second->startPinID == pinId || it->second->endPinID == pinId)
                    it = links.erase(it);
                else
                    ++it;
            }
        }

        // Graph traversal
        NodePtr getNextNodeByPin(Pin& pin);
        PinPtr  getNextPin(Pin& pin);
        LinkPtr findLinkByPinId(Id pinId);
        PinPtr  findPin(Id pinId);
        bool    isPinLinked(Id pinId);
    };

} // namespace Visual
