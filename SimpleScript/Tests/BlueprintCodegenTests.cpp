#include "TestHelpers.hpp"
#include "../VisualEditor/Core/GraphContext.hpp"
#include "../VisualEditor/Core/GraphModel.hpp"
#include "../VisualEditor/Core/NodeRegistry.hpp"
#include "../VisualEditor/Mapping/BpSourceMap.hpp"
#include "../VisualEditor/Visitors/CodeGenerator.hpp"
#include "../VisualEditor/Visitors/GraphValidator.hpp"

// Note: use explicit Visual:: prefix throughout to avoid ambiguity with
// IkigaiScript::ValueNode / IkigaiScript::Value from TestHelpers.

using namespace TestHelpers;

namespace Vz = Visual; // short alias to keep lines readable

static Vz::GraphContext& resetCtx() {
    auto& ctx = Vz::GraphContext::Instance();
    ctx.clear();
    Vz::IdGenerator::SetStart(1);
    return ctx;
}

// =============================================================================
// NodeRegistry
// =============================================================================

TEST_CASE("NodeRegistry: basic types are supported", "[blueprint.registry]") {
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::ValueInt));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::ValueFloat));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::ValueBool));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::ValueString));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::Variable));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::Equal));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::If));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::For));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::While));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::FunctionDeclaration));
    REQUIRE(Vz::NodeRegistry::isSupported(Vz::NodeType::ClassDeclaration));
}

TEST_CASE("NodeRegistry: ForEach is unsupported", "[blueprint.registry]") {
    REQUIRE_FALSE(Vz::NodeRegistry::isSupported(Vz::NodeType::ForEach));
}

// =============================================================================
// BpSourceMap
// =============================================================================

TEST_CASE("BpSourceMap: add and find by nodeId", "[blueprint.sourcemap]") {
    Vz::BpSourceMap map;
    map.graphName = "test";
    map.add(10, 3, 5, Vz::NodeType::Equal);
    map.add(20, 7, 8, Vz::NodeType::If);

    auto e1 = map.findByNodeId(10);
    REQUIRE(e1.has_value());
    REQUIRE(e1->startLine == 3);
    REQUIRE(e1->endLine   == 5);
    REQUIRE(e1->kind      == Vz::NodeType::Equal);

    REQUIRE(map.findByNodeId(20).has_value());
    REQUIRE_FALSE(map.findByNodeId(99).has_value());
}

TEST_CASE("BpSourceMap: find by line", "[blueprint.sourcemap]") {
    Vz::BpSourceMap map;
    map.add(42, 4, 6, Vz::NodeType::Equal);

    REQUIRE(map.findByLine(4).has_value());
    REQUIRE(map.findByLine(5).has_value());
    REQUIRE(map.findByLine(6).has_value());
    REQUIRE_FALSE(map.findByLine(7).has_value());
}

TEST_CASE("BpSourceMap: header comment round-trip", "[blueprint.sourcemap]") {
    Vz::BpSourceMap map;
    map.graphName = "material_01";
    map.add(42, 3, 3, Vz::NodeType::Equal);
    map.add(17, 5, 8, Vz::NodeType::FunctionDeclaration);

    auto header  = map.toHeaderComment();
    auto rebuilt = Vz::BpSourceMap::fromHeaderComment(header);

    REQUIRE(rebuilt.graphName == "material_01");
    REQUIRE(rebuilt.entries.size() == 2);
    REQUIRE(rebuilt.entries[0].nodeId    == 42);
    REQUIRE(rebuilt.entries[0].startLine == 3);
    REQUIRE(rebuilt.entries[1].nodeId    == 17);
    REQUIRE(rebuilt.entries[1].startLine == 5);
}

// =============================================================================
// Code generation helpers
// =============================================================================

// Builds the minimal "Start -> Equal(var, value)" graph in ctx and returns it.
// Nodes: Start(id=1), Equal(id=10), VariableNode(id=11, name), ValueNode(id=12, intVal)
static void buildSimpleAssign(Vz::GraphContext& ctx, const std::string& varName, int intVal) {
    auto startNode = ctx.createNode<Vz::StartNode>(1);
    ctx.createPin(2, "", Vz::PinTypeDescriptor{Vz::PinType::Flow}, startNode.get(), Vz::PinKind::Output);
    ctx.startNode = startNode;

    auto equalNode = ctx.createNode<Vz::EqualNode>(10);
    ctx.createPin(3, "", Vz::PinTypeDescriptor{Vz::PinType::Flow}, equalNode.get(), Vz::PinKind::Input);
    ctx.createPin(4, "Var", Vz::PinTypeDescriptor{Vz::PinType::None}, equalNode.get(), Vz::PinKind::Input);
    ctx.createPin(5, "Val", Vz::PinTypeDescriptor{Vz::PinType::None}, equalNode.get(), Vz::PinKind::Input);
    ctx.createPin(6, "", Vz::PinTypeDescriptor{Vz::PinType::Flow}, equalNode.get(), Vz::PinKind::Output);

    auto varNode = ctx.createNode<Vz::VariableNode>(11, "Variable", varName);
    ctx.createPin(7, "", Vz::PinTypeDescriptor{Vz::PinType::Variable}, varNode.get(), Vz::PinKind::Output);

    Vz::Value val;
    val.value = intVal;
    auto valNode = ctx.createNode<Vz::ValueNode>(12, "Constant", Vz::NodeType::ValueInt, val);
    ctx.createPin(8, "Value", Vz::PinTypeDescriptor{Vz::PinType::Int}, valNode.get(), Vz::PinKind::Output);

    // Links
    ctx.createLink(20, startNode->outputs[0]->id, equalNode->inputs[0]->id);
    ctx.createLink(21, varNode->outputs[0]->id,   equalNode->inputs[1]->id);
    ctx.createLink(22, valNode->outputs[0]->id,   equalNode->inputs[2]->id);

    Vz::IdGenerator::SetStart(30);
}

// =============================================================================
// Code generation tests
// =============================================================================

TEST_CASE("Blueprint codegen: Start -> Equal(var=5) produces valid script",
          "[blueprint.codegen]") {
    auto& ctx = resetCtx();
    buildSimpleAssign(ctx, "x", 5);

    auto result = Vz::compileGraph(ctx, "test");

    INFO("Generated script:\n" << result.script);
    REQUIRE_FALSE(result.script.empty());
    REQUIRE(result.script.find("var") != std::string::npos);
    REQUIRE(result.script.find("x")   != std::string::npos);
    REQUIRE(result.script.find("5")   != std::string::npos);
}

TEST_CASE("Blueprint codegen: @bp(node=N) decorator appears in output",
          "[blueprint.codegen]") {
    auto& ctx = resetCtx();
    buildSimpleAssign(ctx, "roughness", 0);

    auto result = Vz::compileGraph(ctx, "material");

    INFO("Generated script:\n" << result.script);
    // EqualNode has id=10, so @bp(node=10) must appear
    REQUIRE(result.script.find("@bp(node=10)") != std::string::npos);
    REQUIRE(result.script.find("@bp-map")      != std::string::npos);

    auto entry = result.sourceMap.findByNodeId(10);
    REQUIRE(entry.has_value());
}

TEST_CASE("Blueprint codegen: FunNode produces fun declaration",
          "[blueprint.codegen]") {
    auto& ctx = resetCtx();

    auto startNode = ctx.createNode<Vz::StartNode>(1);
    ctx.createPin(2, "", Vz::PinTypeDescriptor{Vz::PinType::Flow}, startNode.get(), Vz::PinKind::Output);
    ctx.startNode = startNode;

    auto funNode = ctx.createNode<Vz::FunNode>(5, "Function");
    funNode->funName = "add";
    funNode->inputVars[10] = {"a", Vz::PinTypeDescriptor{Vz::PinType::Int}};
    funNode->inputVars[11] = {"b", Vz::PinTypeDescriptor{Vz::PinType::Int}};
    funNode->outputType    = Vz::PinTypeDescriptor{Vz::PinType::Int};
    ctx.createPin(12, "Body",   Vz::PinTypeDescriptor{Vz::PinType::Flow}, funNode.get(), Vz::PinKind::Output);
    ctx.createPin(13, "Result", Vz::PinTypeDescriptor{Vz::PinType::Int},  funNode.get(), Vz::PinKind::Output);
    ctx.registerFunctionCall(funNode);

    auto result = Vz::compileGraph(ctx, "funtest");

    INFO("Generated script:\n" << result.script);
    REQUIRE(result.script.find("fun add(") != std::string::npos);
    REQUIRE(result.script.find("a") != std::string::npos);
    REQUIRE(result.script.find("b") != std::string::npos);
}

// =============================================================================
// End-to-end: generated script runs correctly via IkigaiScript
// =============================================================================

TEST_CASE("Blueprint codegen: generated script evaluates correctly",
          "[blueprint.codegen.run]") {
    auto& ctx = resetCtx();
    buildSimpleAssign(ctx, "x", 42);

    auto result = Vz::compileGraph(ctx, "run_test");

    INFO("Generated script:\n" << result.script);
    REQUIRE_FALSE(result.script.empty());

    auto interp = makeInterp();
    REQUIRE_NOTHROW(interp.evaluate(result.script));

    auto x = interp.resolveVariable("x", interp.getGlobalScope());
    REQUIRE(x != nullptr);
    REQUIRE(x->getInt() == 42);
}

// =============================================================================
// GraphValidator
// =============================================================================

TEST_CASE("GraphValidator: passes valid graph", "[blueprint.validator]") {
    auto& ctx = resetCtx();
    ctx.startNode = ctx.createNode<Vz::StartNode>(1);
    ctx.createPin(2, "", Vz::PinTypeDescriptor{Vz::PinType::Flow}, ctx.startNode.get(), Vz::PinKind::Output);

    Vz::GraphValidator validator(ctx);
    REQUIRE(validator.validate().empty());
}

TEST_CASE("GraphValidator: fails without StartNode", "[blueprint.validator]") {
    auto& ctx = resetCtx();
    // Intentionally no start node

    Vz::GraphValidator validator(ctx);
    auto errors = validator.validate();
    REQUIRE_FALSE(errors.empty());
    REQUIRE(errors[0].message.find("StartNode") != std::string::npos);
}

TEST_CASE("GraphValidator: flags FunCall with missing declaration",
          "[blueprint.validator]") {
    auto& ctx = resetCtx();
    ctx.startNode = ctx.createNode<Vz::StartNode>(1);
    ctx.createPin(2, "", Vz::PinTypeDescriptor{Vz::PinType::Flow}, ctx.startNode.get(), Vz::PinKind::Output);
    ctx.createNode<Vz::FunCall>(3, 99); // id 99 doesn't exist

    Vz::GraphValidator validator(ctx);
    auto errors = validator.validate();
    REQUIRE_FALSE(errors.empty());
}

// =============================================================================
// ExecutionObserver
// =============================================================================

TEST_CASE("ExecutionObserver: onEnterNode is called for @bp-tagged statements",
          "[blueprint.observer]") {
    struct TestObserver : IkigaiScript::ExecutionObserver {
        std::vector<int> visitedNodes;
        void onEnterNode(int bpNodeId) override { visitedNodes.push_back(bpNodeId); }
    };

    TestObserver obs;
    auto interp = makeInterp();
    interp.setExecutionObserver(&obs);

    interp.evaluate(R"(
        @bp(node=42)
        var x = 10;
        @bp(node=43)
        var y = 20;
    )");

    REQUIRE(obs.visitedNodes.size() >= 2);
    REQUIRE(std::count(obs.visitedNodes.begin(), obs.visitedNodes.end(), 42) >= 1);
    REQUIRE(std::count(obs.visitedNodes.begin(), obs.visitedNodes.end(), 43) >= 1);
}
