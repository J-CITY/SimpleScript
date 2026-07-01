#include "VisualVisitors.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <fstream>
#include <json.hpp>
#include <set>
#include <stack>

#include "VisualNodes.h"
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "Context.h"
#include "../imgui/imgui_internal.h"

using namespace Visual;


void NodeDrawer::run(ValueNode& node) {
    auto& ce = CodeEditorContext::Instance();
    ImGui::PushItemWidth(150.0f);
    std::visit(overloaded{
        [&](int& arg) {
            ImGui::Text("Int");
            ImGui::InputInt("##int_val", &arg);
        },
        [](float& arg) {
            ImGui::Text("Float");
            ImGui::InputFloat("##float_val", &arg);
        },
        [](std::string& arg) {
            ImGui::Text("String");
            ImGui::InputText("##string_val", &arg);
        },
        [](bool& arg) {
            ImGui::Text("Bool");
            ImGui::Checkbox("##bool_val", &arg);
        },
        [&node, &ce](ArrayType& arg) {
            std::visit(overloaded{
                [&node, &ce](std::vector<bool>& arg) {
                    ImGui::Text("Array Bool");
                    if (ImGui::Button("Add")) {
                        auto pin = ce.createPin(IdGenerator::GenId(), "Value", { PinType::Bool }, &node, PinKind::Input);
                    }
                    ImGui::SameLine();
                    //TODO: add support delete each element
                    if (ImGui::Button("Del")) {
                        ce.deletePin(node.inputs.back()->id.Get());
                        node.inputs.pop_back();
                    }
                },
                [&node, &ce](std::vector<int>& arg) {
                    ImGui::Text("Array Int");
                    if (ImGui::Button("Add")) {
                        auto pin = ce.createPin(IdGenerator::GenId(), "Value", { PinType::Int }, &node, PinKind::Input);
                    }
                    ImGui::SameLine();
                    //TODO: add support delete each element
                    if (ImGui::Button("Del")) {
                        ce.deletePin(node.inputs.back()->id.Get());
                        node.inputs.pop_back();
                    }
                },
                [&node, &ce](std::vector<float>& arg) {
                    ImGui::Text("Array Float");
                    if (ImGui::Button("Add")) {
                        auto pin = ce.createPin(IdGenerator::GenId(), "Value", { PinType::Float }, &node, PinKind::Input);
                    }
                    ImGui::SameLine();
                    //TODO: add support delete each element
                    if (ImGui::Button("Del")) {
                        ce.deletePin(node.inputs.back()->id.Get());
                        node.inputs.pop_back();
                    }
                }
            }, arg);
        },
        [&node, &ce](std::vector<std::shared_ptr<Value>>& arg) {
            ImGui::Text("List");
            if (ImGui::Button("Add")) {
                auto pin = ce.createPin(IdGenerator::GenId(), "Value", { PinType::Any }, &node, PinKind::Input);
            }
            ImGui::SameLine();
            //TODO: add support delete each element
            if (ImGui::Button("Del")) {
                ce.deletePin(node.inputs.back()->id.Get());
                node.inputs.pop_back();
            }
        },
        [&node, &ce](std::unordered_map<int, std::shared_ptr<Value>>& arg) {
            ImGui::Text("Map");
            if (ImGui::Button("Add")) {
                ce.createPin(IdGenerator::GenId(), "Key", { PinType::Any } , &node, PinKind::Input);
                ce.createPin(IdGenerator::GenId(), "Value", { PinType::Any }, &node, PinKind::Input);
            }
            ImGui::SameLine();
            //TODO: add support delete each element
            if (ImGui::Button("Del")) {
                ce.deletePin(node.inputs.back()->id.Get());
                node.inputs.pop_back();
                ce.deletePin(node.inputs.back()->id.Get());
                node.inputs.pop_back();
            }
        },
        [&node, &ce](std::shared_ptr<Class>& arg) {
            ImGui::Text((std::string("Class ") + arg->name).c_str());
        },
        [](auto& arg) {
            
        }
    }, node.value->value.value());
    ImGui::PopItemWidth();
}

void NodeDrawer::run(VariableNode& node) {
    ImGui::PushItemWidth(150.0f);
    ImGui::Text("Variable");
    ImGui::InputText("##variable_node_value", &node.value);
    ImGui::PopItemWidth();
}

void NodeDrawer::run(Node& node) {
    ImGui::Text(node.name.c_str());
}

bool drawCombo(std::string id, int& current, std::vector<std::string> items)
{
    static bool do_popup = false;
    if (ImGui::Button(items[current].c_str())) {
        do_popup = true;
    }
	ax::NodeEditor::Suspend();

    if (do_popup) {
        ImGui::OpenPopup(id.c_str());
        do_popup = false;
    }

    if (ImGui::BeginPopup(id.c_str())) {
        ImGui::TextDisabled("Pick One:");
        ImGui::BeginChild("popup_scroller", ImVec2(100, 100), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        int i = 0;
    	for (auto& e : items) {
            if (ImGui::Button(e.c_str())) {
                current = i;
                ImGui::CloseCurrentPopup();
                ImGui::EndChild();
                ImGui::EndPopup();
                ax::NodeEditor::Resume();
                return true;
            }
            i++;
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ax::NodeEditor::Resume();
    return false;
}

std::vector<std::string> getBaseTypes() {
    return { "Bool", "Int", "Float", "String" };
}

std::vector<std::string> getContainersTypes() {
    return { "ArrayBool", "ArrayInt", "ArrayFloat", "List", "Map" };
}

std::vector<std::string> getClassTypes() {
    auto& ce = CodeEditorContext::Instance();
    std::vector<std::string> res;
    for (auto e : ce.classNodes) {
        res.push_back(e.second->className);
    }
    return res;
}

std::vector<std::string> getAllTypesStrs(bool withNone) {
    std::vector<std::string> res;
	if (withNone) {
        res.push_back("none");
	}
    {
        auto v = getBaseTypes();
        res.insert(res.end(), v.begin(), v.end());
    }
    {
        auto v = getContainersTypes();
        res.insert(res.end(), v.begin(), v.end());
    }
    {
        auto v = getClassTypes();
        res.insert(res.end(), v.begin(), v.end());
    }
    return res;
}

PinTypeDescriptor StringToPinTypeDescriptor(const std::string& str) {
    if (str == "None") {
        return { PinType::None };
    }

	if (str == "Bool") {
        return { PinType::Bool };
    }
	if (str == "Int") {
        return { PinType::Int };
	}
	if (str == "Float") {
        return { PinType::Float };
	}
	if (str == "String") {
        return { PinType::String };
	}
	if (str == "ArrayBool") {
        return { PinType::ArrayBool };
	}
	if (str == "ArrayInt") {
        return { PinType::ArrayInt };
	}
	if (str == "ArrayFloat") {
        return { PinType::ArrayFloat };
	}
	if (str == "List") {
        return { PinType::List };
	}
    if (str == "Map") {
        return { PinType::Map };
    }

    if (str == "Any") {
        return { PinType::Any };
    }
    if (str == "Container") {
        return { PinType::Container };
    }

    if (str.rfind("Class", 0) == 0) {
        return { PinType::Object, str.substr(5) };
    }
}

int PinTypeDescriptorToInt(PinTypeDescriptor& type) {
    auto& ce = CodeEditorContext::Instance();
    if ((int)type.type >= (int)PinType::Object) {
        int i = 0;
        for (auto e : ce.classNodes) {
	        if (e.second->className == std::get<std::string>(type.subType.value())) {
                break;
	        }
            i++;
        }
        return (int)type.type + i;
    }
    return (int)type.type;
}

void NodeDrawer::run(FunNode& node) {
    auto& ce = CodeEditorContext::Instance();
    ImGui::Text(node.name.c_str());
    ImGui::PushItemWidth(150.0f);
    ImGui::InputText("##function_name", &node.funName);

    //Return type
    auto items = getAllTypesStrs(true);
    auto item = PinTypeDescriptorToInt(node.outputType);
    if (drawCombo("##ComboResType", item, items)) {
        node.outputType = StringToPinTypeDescriptor(items[item]);
    }

    if (ImGui::Button("Add")) {
        auto id = IdGenerator::GenId();
        node.inputVars[id] = (std::make_pair("", PinTypeDescriptor{ PinType::Bool } ));
        auto pin = ce.createPin(id, "", { PinType::Bool }, &node, PinKind::Output);
    }
    ImGui::PopItemWidth();
    int i = 0;
    for (auto e = node.inputVars.begin(); e != node.inputVars.end();) {
        auto id = "inputVars" + std::to_string(i);
        ImGui::PushID(id.c_str());
        bool needDelete = false;
	    if (ImGui::Button("X")) {
            needDelete = true;
	    }
        ImGui::SameLine();
        auto vname = e->second.first;

        ImGui::PushItemWidth(150.0f);
        if (ImGui::InputText("##variable_node_value", &vname)) {
            for (auto& outpin : node.outputs) {
	            if (outpin->id.Get() == e->first) {
                    outpin->name = vname;
	            }
            }
            e->second.first = vname;
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        //TODO: custom types
        std::vector items = getAllTypesStrs(false);
        auto item = PinTypeDescriptorToInt(e->second.second) - 1;
        if (drawCombo("##Combo", item, items)) {
            for (auto& outpin : node.outputs) {
                if (outpin->id.Get() == e->first) {
                    outpin->type = StringToPinTypeDescriptor(items[item]);
                }
            }
            e->second.second = StringToPinTypeDescriptor(items[item]);
        }

        //if (ImGui::Combo("##Combo", &item, "bool\0int\0float\0string\0\0")) {
        //    e->second = static_cast<PinType>(item + 1);
        //}

        if (needDelete) {
            std::erase_if(node.outputs, [_id = e->first](const auto& a) {
                return a->id.Get() == _id;
            });
            ce.deletePin(e->first);
            e = node.inputVars.erase(e);
        }
        else {
            e++;
        }
        ImGui::PopID();
        i++;
    }
}

void NodeDrawer::run(ClassNode& node) {
    auto& ce = CodeEditorContext::Instance();
    ImGui::Text(node.name.c_str());
    ImGui::PushItemWidth(150.0f);

    ImGui::InputText("##class_name", &node.className);

    if (ImGui::Button("Add Member")) {
        auto id = IdGenerator::GenId();
        node.memberVars[id] = { "", PinType::Bool };
        auto pin = ce.createPin(id, "", { PinType::Variable, std::make_shared<PinTypeDescriptor>(PinType::Bool) }, &node, PinKind::Input);
    }
    ImGui::PopItemWidth();
    int i = 0;
    for (auto e = node.memberVars.begin(); e != node.memberVars.end();) {
        auto id = "inputVars" + std::to_string(i);
        ImGui::PushID(id.c_str());
        bool needDelete = false;
        if (ImGui::Button("X")) {
            needDelete = true;
        }
        ImGui::SameLine();
        auto vname = e->second.name;

        ImGui::PushItemWidth(150.0f);
        if (ImGui::InputText("##variable_node_value", &vname)) {
            for (auto& outpin : node.inputs) {
                if (outpin->id.Get() == e->first) {
                    outpin->name = vname;
                }
            }
            e->second.name = vname;
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        //TODO: custom class types
        std::vector items = getAllTypesStrs(false);
        auto item = PinTypeDescriptorToInt(e->second.type) - 1;
        if (drawCombo("##Combo", item, items)) {
            for (auto& outpin : node.inputs) {
                if (outpin->id.Get() == e->first) {
                    outpin->type.subType = std::make_shared<PinTypeDescriptor>(StringToPinTypeDescriptor(items[item]));
                }
            }
            e->second.type.subType = std::make_shared<PinTypeDescriptor>(StringToPinTypeDescriptor(items[item]));
        }

        if (needDelete) {
            std::erase_if(node.outputs, [_id = e->first](const auto& a) {
                return a->id.Get() == _id;
                });
            ce.deletePin(e->first);
            e = node.memberVars.erase(e);
        }
        else {
            e++;
        }
        ImGui::PopID();
        i++;
    }
}

void NodeDrawer::run(MethodNode& node) {
    auto& ce = CodeEditorContext::Instance();
    ImGui::Text(node.name.c_str());
    ImGui::PushItemWidth(150.0f);
    ImGui::InputText("##method_name", &node.funName);

    //Return type
    auto items = getAllTypesStrs(true);
    auto item = PinTypeDescriptorToInt(node.outputType);
    if (drawCombo("##ComboResType", item, items)) {
        node.outputType = StringToPinTypeDescriptor(items[item]);
    }

    if (ImGui::Button("Add")) {
        auto id = IdGenerator::GenId();
        node.inputVars[id] = (std::make_pair("", PinTypeDescriptor{ PinType::Bool }));
        auto pin = ce.createPin(id, "", { PinType::Bool }, &node, PinKind::Output);
    }
    ImGui::PopItemWidth();
    int i = 0;
    for (auto e = node.inputVars.begin(); e != node.inputVars.end();) {
        auto id = "inputVars" + std::to_string(i);
        ImGui::PushID(id.c_str());
        bool needDelete = false;
        if (ImGui::Button("X")) {
            needDelete = true;
        }
        ImGui::SameLine();
        auto vname = e->second.first;

        ImGui::PushItemWidth(150.0f);
        if (ImGui::InputText("##variable_node_value", &vname)) {
            for (auto& outpin : node.outputs) {
                if (outpin->id.Get() == e->first) {
                    outpin->name = vname;
                }
            }
            e->second.first = vname;
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        
        std::vector items = getAllTypesStrs(false);
        auto item = PinTypeDescriptorToInt(e->second.second) - 1;
        if (drawCombo("##Combo", item, items)) {
            for (auto& outpin : node.outputs) {
                if (outpin->id.Get() == e->first) {
                    outpin->type = StringToPinTypeDescriptor(items[item]);
                }
            }
            e->second.second = StringToPinTypeDescriptor(items[item]);
        }

        if (needDelete) {
            std::erase_if(node.outputs, [_id = e->first](const auto& a) {
                return a->id.Get() == _id;
                });
            ce.deletePin(e->first);
            e = node.inputVars.erase(e);
        }
        else {
            ++e;
        }
        ImGui::PopID();
        i++;
    }
}

void NodeDrawer::run(MethodCall& node) {
    ImGui::Text(node.name.c_str());
}

void NodeDrawer::run(ThisNode& node) {
    ImGui::Text(node.name.c_str());
}

void NodeDrawer::run(EqualNode& node) {
    ImGui::Text(node.name.c_str());
}

void NodeDrawer::run(StartNode& node) {
    ImGui::Text(node.name.c_str());
}

void NodeDrawer::run(FunCall& node) {
    auto funcNode = CodeEditorContext::Instance().getNodeById(node.funDeclarationId);
    ImGui::Text(std::static_pointer_cast<FunNode>(funcNode.value())->funName.c_str());
}

void NodeDrawer::run(ImportNode& node) {
    ImGui::PushItemWidth(150.0f);
    ImGui::Text("Import");
    ImGui::InputText("##import_node_value", &node.value);
    ImGui::PopItemWidth();
}

void NodeDrawer::run(CommentNode& node) {
	const auto sz = ax::NodeEditor::GetGroupBoundsSize();
    ImGui::PushItemWidth(sz.x);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImVec4(255, 255, 255, 0)));
    ImGui::InputText("##comment_node_value", &node.value);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();
    ax::NodeEditor::Group(node.size);
}

void NodeDrawer::run(OperatorNode& node) {
    ImGui::Text(node.name.c_str());
}

void CodeGenerator::run(ValueNode& node) {
    std::visit(overloaded{
        [this](int arg) {
            appendToken(std::to_string(arg));
        },
        [this](float arg) {
            appendToken(std::to_string(arg));
        },
        [this](bool arg) {
            appendToken((arg ? "true" : "false"));
        },
        [this](std::string& arg) {
            appendToken("\"");
            appendToken(arg);
            appendToken("\"");
        },
        [this, node](std::vector<std::shared_ptr<Value>>& arg) {
            appendToken("list(");
			int i = 0;
            for (auto& e : node.inputs) {
                auto n = CodeEditorContext::Instance().getNextNodeByPin(*e);
                if (!n) {
                    throw;
                }
                n->run(*this);
                if (i != node.inputs.size() - 1) {
                    appendToken(",");
                }
                i++;
            }
            appendToken(")");
        },
        [this, node](std::unordered_map<int, std::shared_ptr<Value>>& arg) {
            appendToken("map(");
            int i = 0;
            for (auto& e : node.inputs) {
                auto n = CodeEditorContext::Instance().getNextNodeByPin(*e);
                if (!n) {
                    throw;
                }
                n->run(*this);
                if (i != node.inputs.size() - 1) {
                    appendToken(",");
                }
                i++;
            }
            appendToken(")");
            endToken();
        },
        [this](std::shared_ptr<Class>& arg) {
            
        },
        [this](auto& arg) {
           
        }
    }, node.value->value.value());
}

void CodeGenerator::run(VariableNode& node) {
    //if (!node.isInit) {
    //    appendToken("var");
    //}
    appendToken(node.value);
    //if (!node.isInit) {
    //    node.isInit = true;
    //}
}

void CodeGenerator::setVar(VariableNode& node) {
    if (!CodeEditorContext::Instance().initVariables.contains(node.id.Get())) {
        appendToken("var");
        CodeEditorContext::Instance().addInScope(node.value, std::make_shared<Value>());
    }
    else {
        CodeEditorContext::Instance().updateInScope(node.value, std::make_shared<Value>());
    }
    appendToken(node.value);
    if (!CodeEditorContext::Instance().initVariables.contains(node.id.Get())) {
        CodeEditorContext::Instance().initVariables.insert(node.id.Get());
    }
}

void CodeGenerator::setVar(ThisNode& node, PinPtr pin) {
    if (!node.inputs[0]->node) {
        throw;
    }
    auto varNode = std::static_pointer_cast<VariableNode>(CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[0]));

    if (!CodeEditorContext::Instance().initVariables.contains(varNode->id.Get())) {
        appendToken("var");
        appendToken(varNode->value);
        appendToken("=");
        appendToken(varNode->outputs[0]->type.getPinDescr()->getStr());
        appendToken("()");
        endToken();
        CodeEditorContext::Instance().addInScope(varNode->value, std::make_shared<Value>());
    }
    else {
        //CodeEditorContext::Instance().updateInScope(node.value, std::make_shared<Value>());
    }

    appendToken(varNode->value);
    appendToken(".");
    appendToken(CodeEditorContext::Instance().getNextPin(*pin)->name);
    if (!CodeEditorContext::Instance().initVariables.contains(varNode->id.Get())) {
        CodeEditorContext::Instance().initVariables.insert(varNode->id.Get());
    }
}

void CodeGenerator::run(CommentNode& node) {
}

void CodeGenerator::run(OperatorNode& node) {
    auto leftNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[0]);
    auto rightNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);

    genCodeForExpression(leftNode, *node.inputs[0]);
    appendToken(node.name);
    genCodeForExpression(rightNode, *node.inputs[1]);
}

void CodeGenerator::appendToken(std::string token) {
    code += token;
    code += " ";
}

void CodeGenerator::endToken() {
    code += ";\n";
}

void CodeGenerator::endLineToken() {
    code += "\n";
}

void CodeGenerator::addStartScopeToken() {
    code += "{";
    tabsCount++;
}

void CodeGenerator::addEndScopeToken() {
    code += "}";
    tabsCount--;
}

void CodeGenerator::addTabsToken() {
    code += std::string(tabsCount, '\t');
}

void CodeGenerator::run(ClassNode& node) {
    appendToken("class");
    appendToken(node.className);
    addStartScopeToken();
    endLineToken();
    for (auto e : node.inputs) {
        addTabsToken();
        appendToken("var");
        appendToken(e->name);
        appendToken("=");
        auto pinNode = CodeEditorContext::Instance().getNextNodeByPin(*e);
        if (pinNode != nullptr) {
            pinNode->run(*this);
        }
        endToken();
    }
    addEndScopeToken();
    endToken();
}

void CodeGenerator::run(MethodNode& node)
{
}

void CodeGenerator::run(MethodCall& node)
{
}

void CodeGenerator::run(ThisNode& node)
{
    auto classNode = std::static_pointer_cast<VariableNode>(CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[0]));
    if (!classNode) {
        throw;
    }
    //if (classNode->computedType.type != PinType::Object) {
    //    throw;
    //}
    appendToken(classNode->value + ".");
}

void CodeGenerator::genCodeForFuncInputVar(FunNode& node, Pin& pin) {
	for (auto& e : node.outputs) {
		if (e->id == pin.id) {
            code += node.inputVars[e->id.Get()].first;
		}
	}
}

void CodeGenerator::genCodeForLoopIndexVar(Node& node) {
    code += "i_" + std::to_string(node.id.Get());
}

void CodeGenerator::genCodeForExpression(NodePtr node, Pin& pin)
{
    if (!node) {
        return;
    }
    if (node->type == NodeType::FunctionDeclaration) {
        genCodeForFuncInputVar(*static_cast<FunNode*>(node.get()), *CodeEditorContext::Instance().getNextPin(pin));
    }
    else if (node->type == NodeType::For) {
        genCodeForLoopIndexVar(*node);
    }
    else {
        node->run(*this);
    }
}


void CodeGenerator::run(Node& node) {
    auto& ce = CodeEditorContext::Instance();
    if (node.type == NodeType::If) {
        code += "if (";

        auto condPin = node.inputs[1];
    	auto condNode = CodeEditorContext::Instance().getNextNodeByPin(*condPin);
        //if (condNode) condNode->genCode(*this);
        genCodeForExpression(condNode, *condPin);

        code += ")";
        code += "{";
        CodeEditorContext::Instance().pushScope();
        auto trueNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
        if (trueNode) trueNode->run(*this);
        CodeEditorContext::Instance().popScope();
        code += "}";
        code += "else";
        code += "{";
        CodeEditorContext::Instance().pushScope();
        auto falseNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[1]);
        if (falseNode) falseNode->run(*this);
        CodeEditorContext::Instance().popScope();
        code += "}";

        auto nextNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[2]);
        if (nextNode) nextNode->run(*this);
    }
    else if (node.type == NodeType::For) {
        CodeEditorContext::Instance().pushScope();
        //TODO: add support >= == !=
        //for(i=0;i<n;i++) {
        auto indexStr = "i_" + std::to_string(node.id.Get());

        code += "for (" + indexStr + "=";
		auto startNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);
        //if (startNode) startNode->genCode(*this);
        genCodeForExpression(startNode, *node.inputs[1]);
    	code += ";" + indexStr + " < ";
        auto endNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[2]);
       // if (endNode) endNode->genCode(*this);
        genCodeForExpression(endNode, *node.inputs[2]);
        code += ";" + indexStr + "+=";
        auto stepNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[3]);
       // if (stepNode) stepNode->genCode(*this);
        genCodeForExpression(stepNode, *node.inputs[3]);
        code += ")";
        code += "{";
        auto bodyNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
        if (bodyNode) bodyNode->run(*this);
        code += "}";

        CodeEditorContext::Instance().popScope();
        auto nextNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[1]);
        if (nextNode) nextNode->run(*this);
    }
    //else if (node.type == NodeType::Operator)
    //{
    //    auto leftNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[0]);
    //    auto rightNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);
    //
    //    genCodeForExpression(leftNode, *node.inputs[0]);
    //    code += node.name;
    //    genCodeForExpression(rightNode, *node.inputs[1]);
    //}
    //else if (node.type == NodeType::Equal)
    //{
    //    auto varNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);
    //    setVar(*std::static_pointer_cast<VariableNode>(varNode));
    //    appendToken("=");
    //    auto exprNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[2]);
    //    genCodeForExpression(exprNode, *node.inputs[2]);
    //    endToken();
    //
    //    auto nextNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
    //    if (nextNode) nextNode->run(*this);
    //}
    else if (node.type == NodeType::FunctionCall)
    {
        CodeEditorContext::Instance().pushScope();


        CodeEditorContext::Instance().popScope();
        
    }
    else if (node.type == NodeType::Return)
    {
        appendToken("return");
        auto exprNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);
        genCodeForExpression(exprNode, *node.inputs[1]);
        endToken();
    }
    else if (node.type == NodeType::Continue)
    {
        addTabsToken();
        appendToken("continue");
        endToken();
    }
    else if (node.type == NodeType::Break)
    {
        addTabsToken();
        appendToken("break");
        endToken();
    }
    //else if (node.type == NodeType::Start)
    //{
    //    auto n = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
    //    if (n) n->run(*this);
    //}
}

void CodeGenerator::run(EqualNode& node) {
    auto varNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);
    if (varNode->type == NodeType::Variable) {
        setVar(*std::static_pointer_cast<VariableNode>(varNode));
    }
    else if (varNode->type == NodeType::This) {
        setVar(*std::static_pointer_cast<ThisNode>(varNode), node.inputs[1]);
    }
	appendToken("=");
    auto exprNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[2]);
    genCodeForExpression(exprNode, *node.inputs[2]);
    endToken();

    auto nextNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
    if (nextNode) nextNode->run(*this);
}

void CodeGenerator::run(StartNode& node) {
    auto n = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
    if (n) n->run(*this);
}

void CodeGenerator::run(FunNode& node) {
    CodeEditorContext::Instance().pushScope();
    code += "fun " + node.funName + "(";
    for (auto& e : node.inputVars) {
        code += e.second.first + ", ";
    }
    if (node.inputVars.empty())
    {
        code += ")";
     }
    else {
        code.pop_back();
        code.back() = ')';
    }
    code += " {";

    auto& context = CodeEditorContext::Instance();
    auto bodyLink = context.findLinkByPinId(node.outputs[0]->id);
    if (bodyLink) {
        auto bodyNode = context.findPin(bodyLink->endPinID)->node;
        bodyNode->run(*this);
    }

    code += "}";
    CodeEditorContext::Instance().popScope();
}

void CodeGenerator::run(FunCall& node) {
    auto funcNode = CodeEditorContext::Instance().getNodeById(node.funDeclarationId);
    appendToken(std::static_pointer_cast<FunNode>(funcNode.value())->funName);
    appendToken("(");
    auto link = CodeEditorContext::Instance().getLinkByPinId(node.inputs[0]->id.Get());
    for (int i = 1; i < node.inputs.size(); i++) {
        auto n = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[i]);
        if (n) n->run(*this);
        if (i != node.inputs.size()-1) {
            appendToken(",");
        }
    }
    appendToken(")");
    if (link) {
        endToken();
    }

}

void CodeGenerator::run(ImportNode& node) {
    code += "import \"" + node.value + "\"" + ENDL;
}

//TODO: save calculatedType
SaveNode::SaveNode(std::string_view path) {
    data["nodes"] = {};
    data["links"] = {};
}

void saveType(nlohmann::json& pn, PinTypeDescriptor& type) {
    pn["type"]["type"] = type.type;
    if (type.subType) {
        if (std::holds_alternative<std::string>(type.subType.value())) {
            pn["type"]["subtype"]["type"] = "string"; //subtype if type class
            pn["type"]["subtype"]["val"] = type.getStr();
        }
        else {
            pn["type"]["subtype"]["type"]["type"] = "int"; //subtype if type variable
            pn["type"]["subtype"]["type"]["val"] = (int)type.getPinDescr()->type;

            auto& sst = type.getPinDescr()->subType;
            if (sst) {
                if (std::holds_alternative<std::string>(type.subType.value())) { // if subtype class
                    pn["type"]["subtype"]["subtype"]["type"] = "string";
                    pn["type"]["subtype"]["subtype"]["val"] = type.getStr();
                }
                //else {
                //    pn["computedType"]["subtype"]["subtype"]["type"]["type"] = "int";
                //    pn["computedType"]["subtype"]["subtype"]["type"]["val"] = (int)e->type.getPinDescr()->type;
                //}
            }
        }
    }
}

void SaveNode::saveBase(nlohmann::json& nd, Node& node) {
    auto pos = ax::NodeEditor::GetNodePosition(node.id);
    nd["pos"]["x"] = pos.x;
    nd["pos"]["y"] = pos.y;
    nd["type"] = node.type;
    //nd["computedType"]["type"] = (int)node.computedType.type;
    //if (node.computedType.subType) {
    //    if (std::holds_alternative<std::string>(node.computedType.subType.value())) {
    //        nd["computedType"]["subtype"]["type"] = "string";
    //        nd["computedType"]["subtype"]["val"] = std::get<std::string>(node.computedType.subType.value());
    //    }
    //    else {
    //        nd["computedType"]["subtype"]["type"] = "int";
    //        nd["computedType"]["subtype"]["val"] = (int)std::get<PinType>(node.computedType.subType.value());
    //    }
    //}
    nd["id"] = node.id.Get();
    nd["name"] = node.name;
    nd["color"] = { node.color.Value.x, node.color.Value.y, node.color.Value.z, node.color.Value.w };
    nd["inputs"] = {};
    nd["outputs"] = {};
    for (auto& e : node.inputs) {
        nlohmann::json pn;
        pn["id"] = e->id.Get();
        pn["name"] = e->name;
        saveType(pn, e->type);
        //pn["type"]["type"] = e->type.type;
        //if (e->type.subType) {
        //    if (std::holds_alternative<std::string>(e->type.subType.value())) {
        //        nd["type"]["subtype"]["type"] = "string"; //subtype if type class
        //        nd["type"]["subtype"]["val"] = e->type.getStr();
        //    }
        //    else {
        //        nd["type"]["subtype"]["type"]["type"] = "int"; //subtype if type variable
        //        nd["type"]["subtype"]["type"]["val"] = (int)e->type.getPinDescr()->type;
        //
        //        auto& sst = e->type.getPinDescr()->subType;
        //        if (sst) {
        //            if (std::holds_alternative<std::string>(e->type.subType.value())) { // if subtype class
        //                nd["type"]["subtype"]["subtype"]["type"] = "string";
        //                nd["type"]["subtype"]["subtype"]["val"] = e->type.getStr();
        //            }
        //            //else {
        //            //    nd["computedType"]["subtype"]["subtype"]["type"]["type"] = "int";
        //            //    nd["computedType"]["subtype"]["subtype"]["type"]["val"] = (int)e->type.getPinDescr()->type;
        //            //}
        //        }
        //    }
        //}
        nd["inputs"].push_back(pn);
    }
    for (auto& e : node.outputs) {
        nlohmann::json pn;
        pn["id"] = e->id.Get();
        pn["name"] = e->name;
        saveType(pn, e->type);
        //pn["type"]["type"] = e->type.type;
        //if (e->type.subType) {
        //    if (std::holds_alternative<std::string>(e->type.subType.value())) {
        //        nd["computedType"]["subtype"]["type"] = "string";
        //        nd["computedType"]["subtype"]["val"] = std::get<std::string>(e->type.subType.value());
        //    }
        //    else {
        //        nd["computedType"]["subtype"]["type"] = "int";
        //        nd["computedType"]["subtype"]["val"] = (int)std::get<PinType>(e->type.subType.value());
        //    }
        //}
        nd["outputs"].push_back(pn);
    }
}
void SaveNode::run(ValueNode& node) {
    auto& ce = CodeEditorContext::Instance();

    nlohmann::json nd;
    saveBase(nd, node);

    std::visit(overloaded{
        [&nd](int arg) {
            nd["value"] = arg;
        },
        [&nd](float arg) {
            nd["value"] = arg;
        },
        [&nd](bool arg) {
            nd["value"] = arg;
        },
        [&nd](std::string& arg) {
            nd["value"] = arg;
        },

        //[&node, &ce](ArrayType& arg) {
        //    std::visit(overloaded{
        //        [&node, &ce](std::vector<bool>& arg) {
        //            
        //            
        //        },
        //        [&node, &ce](std::vector<int>& arg) {
        //            
        //        },
        //        [&node, &ce](std::vector<float>& arg) {
        //            
        //        }
        //    }, arg);
        //},
        //[&node, &ce](std::vector<std::shared_ptr<Value>>& arg) {
        //    
        //},
        //[&node, &ce](std::unordered_map<int, std::shared_ptr<Value>>& arg) {
        //    
        //},
        //[&node, &ce](std::shared_ptr<Class>& arg) {
        //    ImGui::Text((std::string("Class ") + arg->name).c_str());
        //},
        [](auto& arg) {

        }
    }, node.value->value.value());

    data["nodes"].push_back(nd);
}

void SaveNode::run(VariableNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    
    nd["value"] = node.value;
    data["nodes"].push_back(nd);
}

void SaveNode::run(Node& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    data["nodes"].push_back(nd);
}

void SaveNode::run(FunNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);

    nd["funName"] = node.funName;
    nd["inputVars"] = {};
    for (auto& e : node.inputVars)
    {
        nlohmann::json pn;
        pn["id"] = e.first;
        pn["name"] = e.second.first;
        saveType(pn, e.second.second);
        nd["inputVars"].push_back(pn);
    }
    saveType(nd["outputType"], node.outputType);
    data["nodes"].push_back(nd);
}

void SaveNode::run(FunCall& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    nd["funDeclarationId"] = node.funDeclarationId;
    data["nodes"].push_back(nd);
}

void SaveNode::run(ImportNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    nd["value"] = node.value;
    data["nodes"].push_back(nd);
}

void SaveNode::run(CommentNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    nd["value"] = node.value;
    data["nodes"].push_back(nd);
}

void SaveNode::run(OperatorNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    nd["opType"] = node.opType;
    data["nodes"].push_back(nd);
}

void SaveNode::run(ClassNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);

    nd["className"] = node.className;
    nd["memberVars"] = {};
    for (auto& e : node.memberVars)
    {
        nlohmann::json pn;
        pn["id"] = e.first;
        pn["name"] = e.second.name;
        saveType(pn, e.second.type);
        //pn["type"]["type"] = (int)e.second.type.type;
        //if (e.second.type.subType) {
        //    if (std::holds_alternative<std::string>(e.second.type.subType.value())) {
        //        nd["computedType"]["subtype"]["type"] = "string";
        //        nd["computedType"]["subtype"]["val"] = std::get<std::string>(e.second.type.subType.value());
        //    }
        //    else {
        //        nd["computedType"]["subtype"]["type"] = "int";
        //        nd["computedType"]["subtype"]["val"] = (int)std::get<PinType>(e.second.type.subType.value());
        //    }
        //}
        nd["inputVars"].push_back(pn);
    }
    data["nodes"].push_back(nd);
}

void SaveNode::run(MethodNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);

    nd["funName"] = node.funName;
    nd["classNodeId"] = node.classNodeId;
    nd["inputVars"] = {};
    for (auto& e : node.inputVars)
    {
        nlohmann::json pn;
        pn["id"] = e.first;
        pn["name"] = e.second.first;
        pn["type"]["type"] = (int)e.second.second.type;
        //if (e.second.second.subType) {
        //    if (std::holds_alternative<std::string>(e.second.second.subType.value())) {
        //        nd["computedType"]["subtype"]["type"] = "string";
        //        nd["computedType"]["subtype"]["val"] = std::get<std::string>(e.second.second.subType.value());
        //    }
        //    else {
        //        nd["computedType"]["subtype"]["type"] = "int";
        //        nd["computedType"]["subtype"]["val"] = (int)e.second.second.getPinDescr()->type;
        //    }
        //}
        nd["inputVars"].push_back(pn);
    }
    data["nodes"].push_back(nd);
}

void SaveNode::run(MethodCall& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    nd["methodeDeclarationId"] = node.methodeDeclarationId;
    data["nodes"].push_back(nd);
}

void SaveNode::run(ThisNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    nd["classId"] = node.classId;
    data["nodes"].push_back(nd);
}

void SaveNode::run(EqualNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    data["nodes"].push_back(nd);
}

void SaveNode::run(StartNode& node) {
    nlohmann::json nd;
    saveBase(nd, node);
    data["nodes"].push_back(nd);
}

void SaveNode::saveLinks() {
    auto& ce = CodeEditorContext::Instance();

    for (auto& e : ce.links) {
        nlohmann::json lk;
        lk["id"] = e.second->id.Get();
        lk["start"] = e.second->startPinID.Get();
        lk["end"] = e.second->endPinID.Get();
        data["links"].push_back(lk);
    }
}


void CodeExecutor::run(ValueNode& node) {
    expressionValues.push(node.value);
}
void CodeExecutor::run(VariableNode& node) {
    if (!CodeEditorContext::Instance().initVariables.contains(node.id.Get())) {
        throw;
    }
    auto val = CodeEditorContext::Instance().findInScope(node.value);
    if (!val) {
        throw;
    }
    expressionValues.push(val);
};
void CodeExecutor::run(Node& node) {
    static std::stack<bool> isBreak;
    auto& ce = CodeEditorContext::Instance();
    if (node.type == NodeType::If) {
        auto condPin = node.inputs[1];
        auto condNode = CodeEditorContext::Instance().getNextNodeByPin(*condPin);
    	condNode->run(*this);

        auto condRes = std::get<bool>(expressionValues.top()->value.value());
        expressionValues.pop();

        if (condRes) {
            CodeEditorContext::Instance().pushScope();
            auto trueNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
            if (trueNode) trueNode->run(*this);
            CodeEditorContext::Instance().popScope();
        }
        else {
            CodeEditorContext::Instance().pushScope();
            auto falseNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[1]);
            if (falseNode) falseNode->run(*this);
            CodeEditorContext::Instance().popScope();
        }

        auto nextNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[2]);
        if (nextNode) nextNode->run(*this);
    }
    else if (node.type == NodeType::For) {
        CodeEditorContext::Instance().pushScope();
        //TODO: add support >= == !=
        //for(i=0;i<n;i++) {
        auto indexStr = "i_" + std::to_string(node.id.Get());
        
        auto startNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]);
        if (startNode) startNode->run(*this);
        
        auto endNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[2]);
        if (endNode) endNode->run(*this);

    	auto stepNode = CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[3]);
        if (stepNode) stepNode->run(*this);

        isBreak.push(false);
        while (true) {
            auto bodyNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
            if (bodyNode) bodyNode->run(*this);

            if (isBreak.top())
            {
                break;
            }
        }
        isBreak.pop();
        CodeEditorContext::Instance().popScope();

        auto nextNode = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[1]);
        if (nextNode) nextNode->run(*this);
    }
    else if (node.type == NodeType::Continue)
    {
    }
    else if (node.type == NodeType::Break)
    {
        isBreak.top() = true;
    }
};
void CodeExecutor::run(FunNode& node) {

};
void CodeExecutor::run(FunCall& node) {

};
void CodeExecutor::run(ImportNode& node) {
    //pass
};
void CodeExecutor::run(CommentNode& node) {
    //pass
};
void CodeExecutor::run(OperatorNode& node) {
    auto n1 = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
    auto n2 = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[1]);

    n1->run(*this);
    n2->run(*this);

    auto top1 = expressionValues.top();
    expressionValues.pop();
    auto top2 = expressionValues.top();
    expressionValues.pop();

    //cast
    auto id1 = top1->value.value().index();
    auto id2 = top2->value.value().index();

    if (id1 != id2)
    {
        if (id1 > 2 || id2 > 2)
        {
            throw;
        }
        if (id1 > id2)
        {
            std::swap(id1, id2);
            std::swap(top1, top2);

            if (id2 == 1)
            {
                top1->value.value() = (int)std::get<bool>(top1->value.value());
            }
            if (id2 == 2)
            {
                if (id1 == 0) {
                    top1->value.value() = (float)std::get<bool>(top1->value.value());
                }
                else
                {
                    top1->value.value() = (float)std::get<int>(top1->value.value());
                }
            }
        }
    }


    switch (node.opType)
    {
    case OperatorType::Plus:
	    {
			

            
	    }
    	break;
    case OperatorType::Minus: break;
    case OperatorType::Multiply: break;
    case OperatorType::Divided: break;
    case OperatorType::Less: break;
    case OperatorType::EqualLess: break;
    case OperatorType::Great: break;
    case OperatorType::EqualGreat: break;
    case OperatorType::Inequality: break;
    case OperatorType::Equality: break;
    case OperatorType::LogicalAnd: break;
    case OperatorType::LogicalOr: break;
    default: ;
    }
};
void CodeExecutor::run(ClassNode& node) {

};
void CodeExecutor::run(MethodNode& node) {
    //pass
};
void CodeExecutor::run(MethodCall& node) {
    //pass
};
void CodeExecutor::run(ThisNode& node) {

};
void CodeExecutor::run(EqualNode& node) {
    //TODO: add support for containers
    auto var = static_cast<VariableNode*>(CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[1]).get());
    CodeEditorContext::Instance().getNextNodeByPin(*node.inputs[2])->run(*this);
    auto val = expressionValues.top();
    expressionValues.pop();

    if (CodeEditorContext::Instance().findInScope(var->value)) {
        CodeEditorContext::Instance().updateInScope(var->value, val);
    }
    else {
        CodeEditorContext::Instance().addInScope(var->value, val);
    }
}

void CodeExecutor::run(StartNode& node) {
    auto n = CodeEditorContext::Instance().getNextNodeByPin(*node.outputs[0]);
    if (n) n->run(*this);
};
