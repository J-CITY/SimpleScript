#include "TestHelpers.hpp"

// Tests for the ValuePool slab allocator backing makeValue/copyValue.
// These tests verify correctness (values survive pool allocation), pool reuse
// (freed blocks come back through the free list), and that the full scripting
// engine continues to work correctly through the pool path.

using namespace TestHelpers;
using namespace IkigaiScript;

// ---------------------------------------------------------------------------
// Correctness: makeValue/copyValue produce proper Values
// ---------------------------------------------------------------------------

TEST_CASE("ValuePool: makeValue() produces null Value", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	auto v = interp.makeValue();
	REQUIRE(v != nullptr);
	REQUIRE(v->getType() == Type::Null);
}

TEST_CASE("ValuePool: makeValue(Int) produces correct Int", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	auto v = interp.makeValue(Int(42));
	REQUIRE(v != nullptr);
	REQUIRE(v->getType() == Type::Int);
	REQUIRE(v->getInt() == 42);
}

TEST_CASE("ValuePool: makeValue(Float) produces correct Float", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	auto v = interp.makeValue(Float(3.14));
	REQUIRE(v != nullptr);
	REQUIRE(v->getType() == Type::Float);
	REQUIRE(v->getFloat() == Approx(3.14));
}

TEST_CASE("ValuePool: makeValue(string) produces correct String", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	auto v = interp.makeValue(std::string("hello"));
	REQUIRE(v != nullptr);
	REQUIRE(v->getType() == Type::String);
	REQUIRE(v->getString() == "hello");
}

TEST_CASE("ValuePool: copyValue deep-copies the Value", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	Value original(Int(99));
	auto copy = interp.copyValue(original);
	REQUIRE(copy != nullptr);
	REQUIRE(copy->getType() == Type::Int);
	REQUIRE(copy->getInt() == 99);
	// Mutating original does not affect the copy.
	original = Value(Int(0));
	REQUIRE(copy->getInt() == 99);
}

TEST_CASE("ValuePool: multiple allocations return distinct pointers", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	auto a = interp.makeValue(Int(1));
	auto b = interp.makeValue(Int(2));
	auto c = interp.makeValue(Int(3));
	REQUIRE(a.get() != b.get());
	REQUIRE(b.get() != c.get());
	REQUIRE(a->getInt() == 1);
	REQUIRE(b->getInt() == 2);
	REQUIRE(c->getInt() == 3);
}

// ---------------------------------------------------------------------------
// Pool reuse: freed blocks are returned to the free list
// ---------------------------------------------------------------------------

TEST_CASE("ValuePool: freed block is reused on next allocation", "[valuepool]") {
	IkigaiScriptInterpreter interp;

	// Allocate one Value, record its raw address, then drop it.
	Value* rawPtr = nullptr;
	{
		auto v = interp.makeValue(Int(7));
		rawPtr = v.get();
		// v is destroyed here → block returned to free list
	}

	// The next allocation should reuse the freed block.
	auto v2 = interp.makeValue(Int(8));
	REQUIRE(v2.get() == rawPtr);
	REQUIRE(v2->getInt() == 8);
}

TEST_CASE("ValuePool: free list depth grows after drops", "[valuepool]") {
	IkigaiScriptInterpreter interp;
	constexpr int N = 10;
	// Hold all N values alive simultaneously, then drop them all at once.
	{
		std::vector<ValuePtr> held;
		held.reserve(N);
		for (int i = 0; i < N; ++i) {
			held.push_back(interp.makeValue(Int(i)));
		}
		// All N drop here when held goes out of scope.
	}
	REQUIRE(interp.valuePool.freeListDepth() >= static_cast<size_t>(N));
}

TEST_CASE("ValuePool: slab grows only once for repeated alloc/free cycles", "[valuepool]") {
	IkigaiScriptInterpreter interp;

	// Prime the pool: allocate 20 values, drop them all → 20 blocks on free list.
	constexpr int N = 20;
	for (int i = 0; i < N; ++i) {
		auto v = interp.makeValue(Int(i));
		(void)v;
	}

	size_t slabsAfterPrime = interp.valuePool.slabCount();
	REQUIRE(slabsAfterPrime >= 1);

	// Allocate the same N values again — they should all come from the free list.
	{
		std::vector<ValuePtr> held;
		held.reserve(N);
		for (int i = 0; i < N; ++i) {
			held.push_back(interp.makeValue(Int(i)));
		}
		// Drop all at once.
	}

	// No new slab should have been allocated.
	REQUIRE(interp.valuePool.slabCount() == slabsAfterPrime);
}

// ---------------------------------------------------------------------------
// Integration: the scripting engine works correctly through the pool
// ---------------------------------------------------------------------------

TEST_CASE("ValuePool: interpreter evaluates arithmetic via pool", "[valuepool]") {
	REQUIRE(runFresh("var x = 1 + 2; print(x);") == "3");
}

TEST_CASE("ValuePool: strings and collections work via pool", "[valuepool]") {
	REQUIRE(runFresh(R"(
		var s = "hello" + " " + "world";
		print(s);
	)") == "hello world");
}

TEST_CASE("ValuePool: class instances work via pool", "[valuepool]") {
	REQUIRE(runFresh(R"(
		class Point { var x = 0; var y = 0; }
		var p = Point();
		p.x = 10; p.y = 20;
		print(p.x + p.y);
	)") == "30");
}

TEST_CASE("ValuePool: live variables recompute via pool", "[valuepool]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 1; var b = 2; live var c = a + b;");
	REQUIRE(run(interp, "print(c);") == "3");
	interp.evaluate("a = 10;");
	REQUIRE(run(interp, "print(c);") == "12");
}
