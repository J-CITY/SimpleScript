#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../Core/GraphContext.hpp"
#include "../Core/NodeRegistry.hpp"
#include "../Mapping/BpSourceMap.hpp"
#include "NodeEditorSession.hpp"
#include "NodeDrawer.hpp"

namespace Visual {

    struct VisualCodeEditor {
        VisualCodeEditor();
        ~VisualCodeEditor() = default;

        void draw();

    private:
        GraphContext&     ctx_;
        NodeEditorSession session_;
        NodeDrawer        drawer_;

        // Last generated script + source map
        std::string  lastScript_;
        BpSourceMap  lastSourceMap_;

        // Highlighted node from ExecutionObserver
        Id           highlightedNodeId_ = 0;

        //         group →  [ (name_fn, callback) ]
        std::map<std::string,
            std::vector<std::pair<std::function<const std::string&()>,
                                  std::function<void()>>>> menuButtons_;

        void addButton(const std::string& group, const std::string& name,
                       std::function<void()> cb);
        void addButton(const std::string& group,
                       std::function<const std::string&()> name,
                       std::function<void()> cb);
        void initSideBar();

        // Drawing helpers
        void drawLinks();
        void drawPinIcon(const Pin& pin, bool connected, int alpha);
        void drawNode(Node& node);

        // Code generation & execution
        void generateCode();
        void runScript();

        // Save / load
        void saveScene(std::string_view path);
        void loadScene(std::string_view path);

        // Node factory helpers
        void createValueNode(Value val);
        void createVariableNode(PinTypeDescriptor type);
        void createStartNode();
        void createImportNode();
        void createThisNode();
        void createEqualNode();
        void createBinaryOperatorNode(const std::string& name);
        void createBinaryOperatorLogicNode(const std::string& name);
        void createFunctionNode();
        void createClassNode();
        void createClassMethodNode(std::shared_ptr<ClassNode> classNode);
        void createMethodCall(std::shared_ptr<MethodNode> fnode);
        void createClassInstanceNode(std::shared_ptr<ClassNode> classNode);
        void createIfElseNode();
        void createForNode();
        void createWhileNode();
        void createForEachNode();
        void createReturnNode();
        void createContinueNode();
        void createBreakNode();
        void createFunCall(std::shared_ptr<FunNode> fnode);
        void createCommentNode();
    };

} // namespace Visual
