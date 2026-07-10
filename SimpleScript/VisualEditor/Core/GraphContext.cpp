#include "GraphContext.hpp"
#include <stdexcept>

using namespace Visual;

GraphContext::GraphContext() {
    global  = std::make_shared<Scope>();
    current = global;
}

void GraphContext::clear() {
    nodes.clear();
    funNodes.clear();
    methodNodes.clear();
    classNodes.clear();
    links.clear();
    pins.clear();
    startNode.reset();
}

ScopePtr GraphContext::pushScope() {
    auto newScope   = std::make_shared<Scope>();
    newScope->parent = current;
    current          = newScope;
    return current;
}

void GraphContext::resetScope() {
    current = global;
    global->variables.clear();
    initVariables.clear();
}

ScopePtr GraphContext::popScope() {
    current = current->parent;
    return current;
}

ValuePtr GraphContext::findInScope(const std::string& id) {
    auto cur = current;
    if (cur->variables.count(id)) return cur->variables[id];
    while (cur->parent) {
        cur = cur->parent;
        if (cur->variables.count(id)) return cur->variables[id];
    }
    return nullptr;
}

void GraphContext::addInScope(const std::string& id, ValuePtr val) {
    current->variables[id] = val;
}

void GraphContext::updateInScope(const std::string& id, ValuePtr val) {
    current->variables[id] = val;
}

PinPtrOpt GraphContext::getPinById(Id id) {
    auto it = pins.find(id);
    if (it != pins.end()) return it->second;
    return std::nullopt;
}

LinkPtrOpt GraphContext::getLinkById(Id id) {
    auto it = links.find(id);
    if (it != links.end()) return it->second;
    return std::nullopt;
}

LinkPtrOpt GraphContext::getLinkByPinId(Id id) {
    for (auto& [lid, link] : links)
        if (link->startPinID == id || link->endPinID == id) return link;
    return std::nullopt;
}

Id GraphContext::getLinkIdByPinId(Id id) {
    for (auto& [lid, link] : links)
        if (link->startPinID == id || link->endPinID == id) return link->id;
    return -1;
}

NodePtrOpt GraphContext::getNodeById(Id id) {
    auto it = nodes.find(id);
    if (it != nodes.end()) return it->second;
    return std::nullopt;
}

LinkPtr GraphContext::createLink(Id id, Id startPinId, Id endPinId) {
    auto link    = std::make_shared<Link>(id, startPinId, endPinId);
    links[link->id] = link;
    return link;
}

PinPtr GraphContext::createPin(Id id, std::string_view name, PinTypeDescriptor type,
                               Node* node, PinKind kind) {
    auto pin = std::make_shared<Pin>(id, std::string(name), type, node, kind);
    pins[pin->id] = pin;
    if (kind == PinKind::Input)
        node->inputs.push_back(pin);
    else
        node->outputs.push_back(pin);
    return pin;
}

void GraphContext::registerFunctionCall(NodePtr node) {
    auto fn = std::static_pointer_cast<FunNode>(node);
    funNodes[fn->id] = fn;
}

void GraphContext::registerMethodCall(NodePtr node) {
    auto mn = std::static_pointer_cast<MethodNode>(node);
    methodNodes[mn->id] = mn;
}

void GraphContext::registerClassCall(NodePtr node) {
    auto cn = std::static_pointer_cast<ClassNode>(node);
    classNodes[cn->id] = cn;
}

void GraphContext::deletePin(Id id) {
    pins.erase(id);
    deleteAllLinksWithPinId(id);
}

void GraphContext::deleteLink(Id id) {
    links.erase(id);
}

void GraphContext::deleteNode(Id id) {
    auto opt = getNodeById(id);
    if (!opt) return;
    auto node = opt.value();

    if (node->type == NodeType::FunctionDeclaration)  funNodes.erase(id);
    if (node->type == NodeType::ClassDeclaration)     classNodes.erase(id);
    if (node->type == NodeType::ClassMethodDeclaration) methodNodes.erase(id);

    for (auto& pin : node->inputs)  deletePin(pin->id);
    for (auto& pin : node->outputs) deletePin(pin->id);

    nodes.erase(id);
}

LinkPtr GraphContext::findLinkByPinId(Id pinId) {
    for (auto& [id, link] : links)
        if (link->startPinID == pinId || link->endPinID == pinId) return link;
    return nullptr;
}

PinPtr GraphContext::findPin(Id pinId) {
    for (auto& [nid, node] : nodes) {
        for (auto& p : node->inputs)  if (p->id == pinId) return p;
        for (auto& p : node->outputs) if (p->id == pinId) return p;
    }
    return nullptr;
}

bool GraphContext::isPinLinked(Id pinId) {
    for (auto& [id, link] : links)
        if (link->startPinID == pinId || link->endPinID == pinId) return true;
    return false;
}

NodePtr GraphContext::getNextNodeByPin(Pin& pin) {
    auto link = findLinkByPinId(pin.id);
    if (!link) return nullptr;
    Id otherId = (pin.id == link->endPinID) ? link->startPinID : link->endPinID;
    auto otherPin = findPin(otherId);
    if (!otherPin || !otherPin->node) return nullptr;
    auto opt = getNodeById(otherPin->node->id);
    return opt ? opt.value() : nullptr;
}

PinPtr GraphContext::getNextPin(Pin& pin) {
    auto link = findLinkByPinId(pin.id);
    if (!link) return nullptr;
    Id otherId = (pin.id == link->endPinID) ? link->startPinID : link->endPinID;
    return findPin(otherId);
}
