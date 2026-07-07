#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <stdexcept>

#include "scope.hpp"
#include "expressions.hpp"
#include "types.hpp"

namespace IkigaiScript {

    struct VarSlot {
        Value* valuePtr = nullptr;

        bool operator==(const VarSlot& other) const {
            return valuePtr == other.valuePtr;
        }
    };

} // namespace IkigaiScript

namespace std {
    template <>
    struct hash<IkigaiScript::VarSlot> {
        size_t operator()(const IkigaiScript::VarSlot& slot) const {
            return std::hash<IkigaiScript::Value*>{}(slot.valuePtr);
        }
    };
} // namespace std

namespace IkigaiScript {

    class IkigaiScriptInterpreter;

    enum class LiveBindingState {
        Pending,
        Active
    };

    struct LiveBinding {
        size_t id = 0;
        VarSlot target;
        ScopePtr definitionScope;
        ExpressionPtr guardExpr = nullptr;
        TypeDescriptor ownerType;
        bool isDynamic = false;
        LiveBindingState state = LiveBindingState::Pending;
        std::vector<VarSlot> deps;
    };

    class DependencyManager {
    private:
        IkigaiScriptInterpreter* interpreter = nullptr;
        
        size_t nextBindingId = 1;
        std::unordered_map<size_t, LiveBinding> bindings;
        std::unordered_map<VarSlot, size_t> targetToBindingId;
        std::unordered_map<VarSlot, std::vector<size_t>> subscribers; // dep -> bindings

        bool collecting = false;
        size_t collectingBindingId = 0;
        std::vector<VarSlot> collectedReads;

        bool notifySuppressed = false;

        void internalRecompute(size_t bindingId, std::unordered_set<size_t>& recomputingSet, int iterationDepth);
        void updateDependencies(size_t bindingId, const std::vector<VarSlot>& newDeps);

    public:
        explicit DependencyManager(IkigaiScriptInterpreter* interp) : interpreter(interp) {}

        void setInterpreter(IkigaiScriptInterpreter* interp) { interpreter = interp; }

        void registerLiveSlot(const VarSlot& target, ScopePtr scope, const TypeDescriptor& ownerType, bool isDynamic);
        void activateBinding(const VarSlot& target, ExpressionPtr guardExpr, ScopePtr scope, const TypeDescriptor& ownerType, bool isDynamic);
        void rebindBinding(const VarSlot& target, ExpressionPtr guardExpr, ScopePtr scope);

        bool isCollecting() const { return collecting; }
        void recordRead(const VarSlot& slot);
        
        void beginCollectReads(size_t bindingId);
        void endCollectReads(size_t bindingId);

        bool isSuppressed() const { return notifySuppressed; }
        void onVariableChanged(const VarSlot& slot);

        void recompute(size_t bindingId);

        void clear();

        bool hasLiveBinding(const VarSlot& slot) const {
            return targetToBindingId.find(slot) != targetToBindingId.end();
        }

        LiveBinding* getLiveBinding(const VarSlot& slot) {
            auto it = targetToBindingId.find(slot);
            if (it != targetToBindingId.end()) {
                return &bindings[it->second];
            }
            return nullptr;
        }
    };

} // namespace IkigaiScript
