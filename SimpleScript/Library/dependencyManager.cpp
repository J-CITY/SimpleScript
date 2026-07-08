#include "dependencyManager.hpp"
#include "ikigaiScript.h"

namespace IkigaiScript {

	void DependencyManager::registerLiveSlot(const VarSlot& target, ScopePtr scope, const TypeDescriptor& ownerType, bool isDynamic) {
		if (targetToBindingId.find(target) != targetToBindingId.end()) {
			return; // Already registered
		}

		size_t id = nextBindingId++;
		LiveBinding binding;
		binding.id = id;
		binding.target = target;
		binding.definitionScope = scope;
		binding.guardExpr = nullptr;
		binding.ownerType = ownerType;
		binding.isDynamic = isDynamic;
		binding.state = LiveBindingState::Pending;

		bindings[id] = binding;
		targetToBindingId[target] = id;
	}

	void DependencyManager::activateBinding(const VarSlot& target, ExpressionPtr guardExpr, ScopePtr scope, const TypeDescriptor& ownerType, bool isDynamic) {
		size_t id = 0;
		auto it = targetToBindingId.find(target);
		if (it != targetToBindingId.end()) {
			id = it->second;
			bindings[id].guardExpr = guardExpr;
			bindings[id].state = LiveBindingState::Active;
			bindings[id].ownerType = ownerType;
			bindings[id].isDynamic = isDynamic;
		}
		else {
			id = nextBindingId++;
			LiveBinding binding;
			binding.id = id;
			binding.target = target;
			binding.definitionScope = scope;
			binding.guardExpr = guardExpr;
			binding.ownerType = ownerType;
			binding.isDynamic = isDynamic;
			binding.state = LiveBindingState::Active;
			bindings[id] = binding;
			targetToBindingId[target] = id;
		}
		recompute(id);
	}

	void DependencyManager::rebindBinding(const VarSlot& target, ExpressionPtr guardExpr, ScopePtr scope) {
		auto it = targetToBindingId.find(target);
		if (it == targetToBindingId.end()) {
			// If the variable was not live, make it live!
			activateBinding(target, guardExpr, scope, target.valuePtr->typeDescriptor, target.valuePtr->typeDescriptor.isDynamic);
			return;
		}
		size_t id = it->second;
		bindings[id].guardExpr = guardExpr;
		bindings[id].state = LiveBindingState::Active;
		recompute(id);
	}

	void DependencyManager::recordRead(const VarSlot& slot) {
		if (!collecting) {
			return;
		}
		// Don't record a self-read (cycle)
		auto it = targetToBindingId.find(slot);
		if (it != targetToBindingId.end() && it->second == collectingBindingId) {
			return;
		}

		collectedReads.push_back(slot);
	}

	void DependencyManager::beginCollectReads(size_t bindingId) {
		collecting = true;
		collectingBindingId = bindingId;
		collectedReads.clear();
	}

	void DependencyManager::endCollectReads(size_t bindingId) {
		collecting = false;
		// Deduplicate while preserving first-seen order (guard `a + a` → single dep).
		std::vector<VarSlot> uniqueDeps;
		std::unordered_set<VarSlot> seen;
		for (const auto& slot : collectedReads) {
			if (seen.insert(slot).second) {
				uniqueDeps.push_back(slot);
			}
		}
		updateDependencies(bindingId, uniqueDeps);
		collectedReads.clear();
		collectingBindingId = 0;
	}

	void DependencyManager::updateDependencies(size_t bindingId, const std::vector<VarSlot>& newDeps) {
		auto& binding = bindings[bindingId];

		// Remove old subscriptions
		for (const auto& dep : binding.deps) {
			auto& subs = subscribers[dep];
			subs.erase(std::remove(subs.begin(), subs.end(), bindingId), subs.end());
		}

		binding.deps = newDeps;

		// Add new subscriptions
		for (const auto& dep : binding.deps) {
			subscribers[dep].push_back(bindingId);
		}
	}

	void DependencyManager::onVariableChanged(const VarSlot& slot) {
		if (notifySuppressed) {
			return;
		}
		auto it = subscribers.find(slot);
		if (it == subscribers.end()) {
			return;
		}
		std::unordered_set<size_t> recomputingSet;
		std::vector<size_t> subsToNotify = it->second; // copy in case it changes

		for (size_t id : subsToNotify) {
			internalRecompute(id, recomputingSet, 0);
		}
	}

	void DependencyManager::recompute(size_t bindingId) {
		std::unordered_set<size_t> recomputingSet;
		internalRecompute(bindingId, recomputingSet, 0);
	}

	void DependencyManager::internalRecompute(size_t bindingId, std::unordered_set<size_t>& recomputingSet, int iterationDepth) {
		if (iterationDepth > 64) {
			throw Exception("Live dependency cycle detected");
		}

		// Snapshot the ownerType before potential rehash (getValue may insert new bindings).
		LiveBinding localBinding = bindings[bindingId];
		if (localBinding.state != LiveBindingState::Active || !localBinding.guardExpr) {
			return;
		}
		if (localBinding.state != LiveBindingState::Active || !localBinding.guardExpr) {
			return;
		}
		recomputingSet.insert(bindingId);

		// Evaluate guard and collect deps
		beginCollectReads(bindingId);
		ValuePtr newVal = nullptr;
		try {
			if (interpreter) {
				newVal = interpreter->getValue(localBinding.guardExpr, localBinding.definitionScope, nullptr);
			}
		}
		catch (...) {
			endCollectReads(bindingId);
			throw Exception("Live dependency cycle detected");
		}
		endCollectReads(bindingId);

		if (!newVal) {
			newVal = std::make_shared<Value>();
		}

		// Re-acquire reference after potential rehash from getValue inserting new bindings.
		auto& binding = bindings[bindingId];

		// Type conversion/checking + owner flag preservation
		if (localBinding.isDynamic) {
			// Dynamic: keep the actual computed type, just apply owner flags.
			TypeDescriptor td = newVal->typeDescriptor;
			td.isDynamic = true;
			td.isConst = binding.ownerType.isConst;
			td.isNullable = binding.ownerType.isNullable;
			td.isInit = true;
			newVal->typeDescriptor = td;
		}
		else {
			// Static: if ownerType.type==Null (pending slot, no declared type), infer from value.
			if (binding.ownerType.type == Type::Null) {
				binding.ownerType.type = newVal->getType();
				if (!binding.ownerType.subtype && newVal->typeDescriptor.subtype) {
					binding.ownerType.subtype = newVal->typeDescriptor.subtype;
				}
				if (!binding.ownerType.subtype2 && newVal->typeDescriptor.subtype2) {
					binding.ownerType.subtype2 = newVal->typeDescriptor.subtype2;
				}
			}
			else if (newVal->getType() != binding.ownerType.type) {
				if (binding.ownerType.type == Type::Float && newVal->getType() == Type::Int) {
					newVal = std::make_shared<Value>(static_cast<double>(newVal->getInt()));
				}
				else if (newVal->getType() != Type::Null || !binding.ownerType.isNullable) {
					throw TypeConvertError(binding.ownerType, newVal->typeDescriptor);
				}
			}
			newVal->typeDescriptor = binding.ownerType;
			newVal->typeDescriptor.isDynamic = false;
			newVal->typeDescriptor.isInit = true;
		}

		bool valueChanged = true;
		// Write to target variable without triggering notification for self
		if (binding.target.valuePtr && newVal) {
			if (binding.target.valuePtr == newVal.get()) {
				valueChanged = false;
			}

			bool wasSuppressed = notifySuppressed;
			notifySuppressed = true;
			try {
				if (valueChanged) {
					*binding.target.valuePtr = *newVal;
					binding.target.valuePtr->typeDescriptor = newVal->typeDescriptor;
				}
			}
			catch (...) {
				notifySuppressed = wasSuppressed;
				throw;
			}
			notifySuppressed = wasSuppressed;
		}

		// Propagate changes to subscribers of this live binding
		if (valueChanged) {
			auto it = subscribers.find(binding.target);
			if (it != subscribers.end()) {
				std::vector<size_t> subsToNotify = it->second;
				for (size_t subId : subsToNotify) {
					internalRecompute(subId, recomputingSet, iterationDepth + 1);
				}
			}
		}
	}

	void DependencyManager::clear() {
		bindings.clear();
		targetToBindingId.clear();
		subscribers.clear();
		nextBindingId = 1;
		collecting = false;
		collectingBindingId = 0;
		collectedReads.clear();
		notifySuppressed = false;
	}

} // namespace IkigaiScript
