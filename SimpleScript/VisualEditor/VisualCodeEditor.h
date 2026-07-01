#pragma once
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "Utils.h"
#include "VisualNodes.h"
#include "VisualTypes.h"
#include "../imgui/imgui-node-editor/imgui_node_editor.h"


namespace Visual
{
	struct FunNode;
	struct Link;
	struct Pin;
    struct Node;
    
    struct VisualCodeEditor {
        VisualCodeEditor();
        ~VisualCodeEditor() = default;

        void draw();
    private:
    	//Side panel buttons
		//       group                  name         cb
        std::map<std::string, std::vector<std::pair<std::function<const std::string&()>, std::function<void()>>>> menuButtons;

        void addButton(const std::string& group, const std::string& name, std::function<void()> cb);
        void addButton(const std::string& group, std::function<const std::string& ()> name, std::function<void()> cb);
        void initSideBar();

        void drawLinks();
        void drawPinIcon(const Visual::Pin& pin, bool connected, int alpha);
        void drawNode(Node& node);

        
        void generateCode();
        

        void createValueNode(Visual::Value val);
        void createVariableNode(PinType type);
        void createStartNode();
        void createVariableNode(PinTypeDescriptor type);
        void createImportNode();
        void createThisNode();
        void createEqualNode(PinType type);
        void createBinaryOperatorNode(const std::string& name, PinType type);
        void createBinaryOperatorLogicNode(const std::string& name, PinType type);
        void createFunctionNode();
        void createClassNode();
        void createClassMethodNode(std::shared_ptr<ClassNode> classNode);
        void createMethodCall(std::shared_ptr<MethodNode> fnode);
        void createClassInstanceNode(std::shared_ptr<ClassNode> fnode);
        void createIfElseNode();
        void createForNode();
        void createWhileNode();
        void createForEachNode();
        void createReturn(PinType type);
        void createContinue();
        void createBreak();
        void createFunCall(std::shared_ptr<FunNode> fnode);
        void createComment();
    };
}
