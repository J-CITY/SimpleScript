#include "Context.h"

#include "VisualNodes.h"

using namespace Visual;

CodeEditorContext::CodeEditorContext():
	context(std::unique_ptr<ax::NodeEditor::EditorContext, void(*)(ax::NodeEditor::EditorContext*)>(
        ax::NodeEditor::CreateEditor(), ax::NodeEditor::DestroyEditor)) {

    global = std::make_shared<Scope>();
    current = global;
}

CodeEditorContext::~CodeEditorContext() {

}

ScopePtr CodeEditorContext::pushScope() {
	auto newScope = std::make_shared<Scope>();
    newScope->parent = current;
    current = newScope;
    return current;
}

void CodeEditorContext::resetScope() {
    current = global;
    global->variables.clear();
    initVariables.clear();
}

ScopePtr CodeEditorContext::popScope() {
    current = current->parent;
    return current;
}

ValuePtr CodeEditorContext::findInScope(const std::string& id) {
    auto _cur = current;

    if (_cur->variables.contains(id)) {
        return _cur->variables[id];
    }
    while (_cur->parent) {
        _cur = _cur->parent;
        if (_cur->variables.contains(id)) {
            return _cur->variables[id];
        }
    }
    return nullptr;
}

void CodeEditorContext::addInScope(const std::string& id, ValuePtr val) {
    if (current->variables.contains(id)) {
        throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    }
    current->variables[id] = val;
}

void CodeEditorContext::updateInScope(const std::string& id, ValuePtr val) {
    if (!current->variables.contains(id)) {
        throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    }
    current->variables[id] = val;
}

PinPtrOpt CodeEditorContext::getPinById(Id id) {
    if (pins.contains(id)) {
        return pins.at(id);
    }
    return std::nullopt;
}

LinkPtrOpt CodeEditorContext::getLinkById(Id id) {
    if (links.contains(id)) {
        return links.at(id);
    }
    return std::nullopt;
}

LinkPtrOpt CodeEditorContext::getLinkByPinId(Id id) {
    for (auto& link : links) {
	    if (link.second->startPinID.Get() == id || link.second->endPinID.Get() == id) {
            return link.second;
	    }
    }
    return std::nullopt;
}

Id CodeEditorContext::getLinkIdByPinId(Id id) {
    for (auto& link : links) {
        if (link.second->startPinID.Get() == id || link.second->endPinID.Get() == id) {
            return link.second->id.Get();
        }
    }
    return -1;
}

NodePtrOpt CodeEditorContext::getNodeById(Id id) {
    if (nodes.contains(id)) {
        return nodes.at(id);
    }
    return std::nullopt;
}

LinkPtr CodeEditorContext::createLink(Id id, ax::NodeEditor::PinId idStart, ax::NodeEditor::PinId idEnd) {
    auto link = std::make_shared<Link>(id, idStart, idEnd);
    links[link->id.Get()] = link;
    return link;
}

PinPtr CodeEditorContext::createPin(Id id, std::string_view name, PinTypeDescriptor type, Node* node, PinKind kind) {
    auto pin = std::make_shared<Pin>(id, std::string(name.begin(), name.end()), type, node, kind);
    pins[pin->id.Get()] = pin;
    if (kind == PinKind::Input) {
        node->inputs.push_back(pin);
    }
    else {
        node->outputs.push_back(pin);\
    }
    return pin;
}

void CodeEditorContext::registerFunctionCall(NodePtr node) {
    auto _node = std::static_pointer_cast<FunNode>(node);
    funNodes[_node->id.Get()] = _node;
}

void CodeEditorContext::registerMethodCall(NodePtr node) {
    auto _node = std::static_pointer_cast<MethodNode>(node);
    auto ptr = getNodeById(_node->classNodeId);
    auto classNode = std::static_pointer_cast<ClassNode>(ptr.value());
    methodNodes[_node->id.Get()] = _node;
}

void CodeEditorContext::registerClassCall(NodePtr node) {
    auto _node = std::static_pointer_cast<ClassNode>(node);
    classNodes[_node->id.Get()] = _node;
}

void CodeEditorContext::deletePin(Id id) {
    pins.erase(id);

    for (auto it = links.begin(); it != links.end(); ++it) {
		if ((it->second->startPinID.Get() == id) || (it->second->startPinID.Get() == id)) {
            deleteLink(it->first);
		}
    }
}

void CodeEditorContext::deleteLink(Id id) {
    links.erase(id);
}

void CodeEditorContext::deleteNode(Id id) {
    auto node = getNodeById(id).value();
	if (node && node->type == NodeType::FunctionDeclaration) {
        funNodes.erase(id);
	}
    if (node && node->type == NodeType::ClassDeclaration) {
        classNodes.erase(id);
    }
    if (node && node->type == NodeType::ClassMethodDeclaration) {
        methodNodes.erase(id);
    }
    for (auto e : node->inputs) {
        deletePin(e->id.Get());
    }
    for (auto e : node->outputs) {
        deletePin(e->id.Get());
    }
    nodes.erase(id);

}

LinkPtr CodeEditorContext::findLinkByPinId(ax::NodeEditor::PinId id) {
    for (auto& [_id, e] : CodeEditorContext::Instance().links) {
        if (e->startPinID == id || e->endPinID == id) {
            return e;
        }
    }
    return nullptr;
}

PinPtr CodeEditorContext::findPin(ax::NodeEditor::PinId id) {
    if (!id)
        return nullptr;

    for (auto& node : nodes) {
        for (auto& pin : node.second->inputs)
            if (pin->id == id)
                return pin;

        for (auto& pin : node.second->outputs)
            if (pin->id == id)
                return pin;
    }
    return nullptr;
}

bool CodeEditorContext::isPinLinked(ax::NodeEditor::PinId id) {
    if (!id)
        return false;
    
    for (auto& [_id, link] : links)
        if (link->startPinID == id || link->endPinID == id)
            return true;
    return false;
}

NodePtr CodeEditorContext::getNextNodeByPin(Pin& pin) {
    auto& context = CodeEditorContext::Instance();
    auto link = context.findLinkByPinId(pin.id);
    if (link) {
        auto otherpin = context.findPin(pin.id == link->endPinID ? link->startPinID : link->endPinID);
        if (!otherpin) {
            return nullptr;
        }
        return getNodeById(otherpin->node->id.Get()).value();
    }
    return nullptr;
}

PinPtr CodeEditorContext::getNextPin(Pin& pin) {
    auto link = findLinkByPinId(pin.id);
    if (link) {
        auto otherpin = findPin(pin.id == link->endPinID ? link->startPinID : link->endPinID);
        return otherpin;
    }
    return nullptr;
}
