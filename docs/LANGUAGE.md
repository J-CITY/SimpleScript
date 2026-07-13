# IkigaiScript — Language Reference

> **Русский:** [LANGUAGE.ru.md](LANGUAGE.ru.md)

IkigaiScript (engine name: **IkigaiScript**) is an imperative scripting language with expressions, first-class functions, OOP, coroutines, and static typing with optional `dynamic`.

---

## Table of contents

1. [Lexical structure and syntax](#lexical-structure-and-syntax)
2. [Types](#types)
3. [Variables](#variables)
4. [Expressions and operators](#expressions-and-operators)
5. [Functions](#functions)
6. [Classes](#classes)
7. [Control flow](#control-flow)
8. [Collections](#collections)
9. [Coroutines](#coroutines)
10. [Concurrency](#concurrency)
11. [Modules](#modules)
12. [Defer](#defer)
13. [Transactions](#transactions)
14. [Live variables](#live-variables)
15. [Result](#result)
16. [Generics](#generics)
17. [Decorators and metadata](#decorators-and-metadata)
18. [C++ embedding](#c-embedding)
19. [Bytecode and IKBC](#bytecode-and-ikbc)
20. [Limitations](#limitations)

---

## Lexical structure and syntax

- Statements are separated by `;`
- Blocks use `{ ... }`
- Comments: `// line`, `/* block */`
- Strings: `"plain"`, `"""multiline"""`, interpolation `` `hello ${name}` ``
- Characters: `'x'` (type `Char`)
- Numbers: decimal, `0x` hex, `0b` binary, floating-point with a dot

### Keywords

`var`, `const`, `dynamic`, `fun`, `coro`, `class`, `if`, `else`, `for`, `while`, `match`, `case`, `default`, `return`, `yield`, `break`, `continue`, `defer`, `live`, `import`, `using`, `module`, `export`, `await`, `sync`, `race`, `spawn`

---

## Types

| Type | Example | Description |
|------|---------|-------------|
| `Null` | `null` | Absence of value |
| `Bool` | `true`, `false` | Boolean |
| `Char` | `'a'` | Unicode character |
| `Int` | `42`, `0xFF` | Integer |
| `Float` | `3.14` | Floating-point |
| `String` | `"hello"` | UTF-8 string |
| `Array` | `[1, 2, 3]` | Homogeneous array |
| `List` | `list(1, 2)` | Value list |
| `Set` | `set(...)` | Set |
| `Map` | `map(...)` | Dictionary |
| `Tuple` | `(1, "a")` | Fixed-length tuple |
| `Range` | `1..5`, `1..=5` | Range (exclusive / inclusive end) |
| `Optional` | `Int?` | Nullable wrapper |
| `Result` | `Result<Int, String>` | Ok / Err |
| `Class` | class instance | Object with fields and methods |
| `Function` | `fun(...) { }` | Function |
| `Coro` | result of `coro` | Coroutine (task) |
| `Dynamic` | `dynamic x = ...` | Any type |

### Nullable

```js
var a: Int? = null;
var b: Int? = 5;
```

### Dynamic

```js
dynamic x = 10;
x = "string";  // allowed
var y = 10;    // type Int is fixed
// y = "oops"; // type error
```

---

## Variables

```js
var name = expr;              // type inference
var count: Int = 0;           // explicit type
const MAX = 100;              // constant
dynamic buffer = "";          // dynamic type
live var sum = a + b;         // reactive variable (see below)
```

Tuple destructuring:

```js
var (x, y) = (10, 20);
var (first, _) = (1, 2);  // _ skips an element
```

---

## Expressions and operators

### Arithmetic

`+`, `-`, `*`, `/`, `%` — for numbers; `+` also concatenates strings.

### Comparison and logic

`==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`

`&&` and `||` use **short-circuit** evaluation.

### Assignment

```js
x = 10;
x += 1;
```

### Expression forms

Many constructs can be used on the right-hand side of `var name = ...`. The last expression in a block is its value (semicolon-terminated).

#### Block expression

```js
var a = {
    var b = 40;
    b + 2;   // 42 — last expression is the result
};

var nested = {
    var inner = { 10 + 20; };
    inner + 12;   // 42
};
```

#### if expression

```js
var a = if (true) { 42; } else { 10; };   // 42
var b = if (false) { 42; } else { 10; };  // 10

var x = 5;
var result = if (x > 3) { 100; } else { 0; };   // 100

var label = if (n > 0) { "pos"; } else { "neg"; };
```

Both branches must be blocks `{ ... }`. The last expression in the chosen branch becomes the value.

#### for-comprehension

`for` in an expression iterates a collection and builds an `Array` from the body results:

```js
var arr = [1, 2, 3];
var doubled = for (x : arr) { x * 2; };   // [2, 4, 6]

var indexed = for (idx, x : arr) { idx + x; };   // [1, 3, 5]

var map = dictionary();
map["a"] = 100;
map["b"] = 200;
var values = for (key, val : map) { val * 2; };

var nums = [1, 2, 3, 4, 5, 6];
var scaled = for (x : nums) { x * 2; };

// Range iteration
var squares = for (i : 1..=5) { i * i; };   // [1, 4, 9, 16, 25]
```

Binding forms: `(x : coll)`, `(idx, x : arr)`, `(key, val : map)`, `(idx, key, val : map)`.

#### match expression

```js
var n = 99;
var r = match (n) {
    case 1 => { "one"; }
    default => { "other"; }
};   // "other"

var s = "hello";
var code = match (s) {
    case "hi"    => { 1; }
    case "hello" => { 2; }
    default      => { 0; }
};   // 2

var b = true;
var flag = match (b) {
    case false => { 0; }
    case true  => { 1; }
};   // 1
```

Each arm body is a block; the matched arm's last expression is the result. Without a matching arm and without `default`, an exception is thrown.

### Ranges

```js
var r1 = 1..5;    // 1, 2, 3, 4  (end excluded)
var r2 = 1..=5;   // 1, 2, 3, 4, 5
```

---

## Functions

### Declaration

```js
fun add(a: Int, b: Int): Int {
    return a + b;
}
```

### Lambdas

```js
var mul = fun(a, b) { return a * b; };
print(mul(3, 4));
```

### Capture (closures)

```js
var offset = 10;
var add = fun[offset](x) { return offset + x; };

// ref capture: [name]
// copy capture: [local=outerName]
var snap = fun[local=outer](x) { return local + x; };
```

### Named arguments

```js
fun f(a, b, c) { return a + b + c; }
print(f(c=3, a=1, b=2));       // 6
print(f(10, c=5, b=2));        // 17
```

### Default values

```js
fun greet(name, greeting = "Hello") {
    return greeting + ", " + name;
}
print(greet("World"));
```

### Variadic arguments

`...` syntax in function declarations is supported (see `FunctionsTests.cpp`).

---

## Classes

```js
class Counter {
    var value = 0;

    fun Counter(start) {
        value = start;
    }

    fun inc() {
        value = value + 1;
    }

    fun get() {
        return value;
    }
}

var c = Counter(10);
c.inc();
print(c.get());  // 11
```

Field access: `obj.field`, method calls: `obj.method(args)`.

Inheritance via `class Derived, Base { ... }` and `super(...)` — see `ClassesTests.cpp`.

---

## Control flow

### if / else

```js
if (x > 0) {
    print("positive");
} else {
    print("non-positive");
}
```

### for (C-style)

```js
for (i = 0; i < 10; i++) {
    print(i);
}

for (i < 10) {   // manual increment inside
    print(i);
    i++;
}
```

### for-each

```js
for (item : myList) {
    print(item);
}

for (i : 1..=5) {
    print(i);
}
```

### while

```js
while (n > 0) {
    print(n);
    n = n - 1;
}
```

### break / continue

```js
for (i = 0; ; i++) {
    if (i >= 10) { break; }
    if (i % 2 == 0) { continue; }
    print(i);
}
```

### match

```js
var result = match (value) {
    case 1 => { "one"; }
    case 2 => { "two"; }
    case "hello" => { "greeting"; }
    default => { "unknown"; }
};
```

`match` as a statement:

```js
match (n) {
    case 1 => { print("one"); }
    case 2 => { print("two"); }
    default => { print("other"); }
}
```

If no branch matches and there is no `default`, an exception is thrown.

---

## Collections

### Array

```js
var a = [1, 2, 3];
var empty = array();
print(a[0]);
pushback(a, 4);
print(length(a));
```

### List

```js
var lst = list(10, 20, 30);
print(lst[1]);
```

### Slicing

```js
var lst = [10, 20, 30, 40, 50];
var s = lst[1..3];    // indices 1, 2
var t = lst[1..=3];   // indices 1, 2, 3

var str = "hello";
print(str[1..3]);     // "el"
```

### Tuple

```js
var t = (1, "hello", true);
print(t.0);
print(t.1);

for (x : t) {
    print(x);
}
```

### Set and Map

```js
var s = set(1, 2, 3);
var m = map();
// operations via builtins — see BuiltinsTests
```

---

## Coroutines

A coroutine is declared with `coro`. Calling `coroName(args)` creates a **task** (the body does not run immediately). Resume with `task()` or `await task`.

```js
coro gen(a, b) {
    yield a + b + 2;
    yield a + b + 4;
    return a + b;
}

var cr = gen(13, 2);
print(cr());  // 17  (first yield)
print(cr());  // 19  (second yield)
print(cr());  // 15  (return)
```

`await` waits for task completion:

```js
coro answer() {
    return 42;
}
var t = answer();
var result = await t;
print(result);
```

---

## Concurrency

### await

Blocks the current coroutine until the task completes.

### sync

Runs multiple `await` expressions and collects results:

```js
coro a() { return 1; }
coro b() { return 2; }
var ta = a();
var tb = b();
var results = sync {
    await ta;
    await tb;
};
```

### race

Returns the result of the first completed task:

```js
coro fast() { return 1; }
coro slow() { yield 0; return 2; }
var winner = race {
    fast();
    slow();
};
```

### spawn

Launch an anonymous task from a block or expression:

```js
var t = spawn {
    return 99;
};
var result = await t;
```

The form `spawn expr;` is also supported for a single expression.

### Native jobs (C++)

Enable a thread pool from the host:

```cpp
interp.enableNativePool(4);
interp.registerNativeJob("myJob", [](const List& args) { /* ... */ });
```

In script: `nativeJob("myJob", arg1, arg2)`.

### Scheduler

```cpp
interp.pump(maxSteps);  // run ready tasks
```

---

## Modules

### Module definition

File `math.ik`:

```js
module Math;

export fun add(a, b) { return a + b; }
export var PI = 3.14;

fun helper() { return 100; }  // not exported
```

### Import

```js
import "path/to/math.ik";
print(Math.add(2, 3));
print(Math.PI);
```

### Selective import

```js
using { add, PI } from Math;
print(add(1, 2));
```

Unqualified import by module name:

```js
import Math;
```

---

## Defer

Deferred execution on scope exit (LIFO):

```js
fun demo() {
    defer { print("second"); }
    defer { print("first"); }
    print("body");
}
// Output: body → first → second
```

Defer runs on `return`, block exit, and stack unwinding.

---

## Transactions

Syntax `>>>{ ... }` — transactional block with commit/rollback.

```js
var x = 1;

// Success → true, changes are kept
print(>>>{ x = 2; });

// Error → false, rollback
print(>>>{ x = 99; var err = 1 / 0; });
print(x);  // 2
```

### Optional result `>>>`

```js
var res = >>>{
    var a = 40;
    return a + 2;
};
print(optionalHas(res));  // true
print(optionalGet(res));  // 42
```

On error inside the block, an empty optional is returned.

---

## Live variables

Reactive variables are recomputed when dependencies change (via `DependencyManager`):

```js
var a = 1;
var b = 2;
live var c = a + b;
print(c);  // 3

a = 5;
print(c);  // 7

live var x;
live x = b + 1;  // deferred binding
```

Direct assignment to a live variable is not allowed — use `live x = expr`.

---

## Result

Type for explicit error handling:

```js
var ok  = resultOk(42);
var err = resultErr("failure");

print(resultIsOk(ok));     // true
print(resultGet(ok));      // 42
print(resultGetErr(err));  // failure

print(ok);   // ok(42)
print(err);  // err(failure)
```

Typing:

```js
var r: Result<Int, String> = resultOk(10);
r = resultErr("oops");
```

---

## Generics

Monomorphization at call site (specialization is created on the fly):

```js
fun identity<T>(x: T): T {
    return x;
}

print(identity(42));
print(identity("hello"));

fun wrap<T>(x: T): T { return x; }
var a = wrap(1);
var b = wrap("world");
```

> Generic functions are **not compiled to bytecode** (they stay on the AST path).

---

## Decorators and metadata

Syntax `@name` / `@name(args)` attaches metadata to variables, classes, and functions:

```js
@test(min=0, max=100, widget="slider")
var volume = 50;

@editable
class Entity {
    @range(min=0, max=1)
    var health = 1.0;
}
```

From C++, metadata is read via `interp.getMetadata(scope, "memberName")`.

Decorators `@interpret` and `@bytecode` on functions control the execution path (AST vs VM).

---

## C++ embedding

### Basic API

```cpp
#include "Library/ikigaiScript.h"

IkigaiScript::IkigaiScriptInterpreter interp;

// Execution
interp.evaluate("var x = 1;");
interp.evaluateFile("script.ik");

// Read variables
auto val = interp.resolveVariable("x");

// Register a C++ function
interp.newFunction("myFn", [](const IkigaiScript::List& args) {
    return std::make_shared<IkigaiScript::Value>(42);
});
```

### Execution modes

```cpp
interp.setExecutionMode(IkigaiScript::ExecutionMode::Interpreter); // default
interp.setExecutionMode(IkigaiScript::ExecutionMode::Bytecode);
```

---

## Bytecode and IKBC

### Compilation (without running top-level code)

```cpp
auto chunk = interp.compileScript(R"(
    fun square(n: Int): Int { return n * n; }
    print(square(5));
)");
```

```cpp
auto chunk = interp.compileScriptFile("script.ik");
```

### Serialization (IKBC v1 format)

```cpp
std::string blob = interp.serializeCompiledScript(*chunk);
interp.saveCompiledScript(*chunk, "script.ikbc");

auto loaded = interp.loadCompiledScriptFile("script.ikbc");
auto fromStr = interp.deserializeCompiledScript(blob);
```

Format: binary blob with magic `"IKBC"`, little-endian. Contains a `Chunk` (named functions + `main`).

### Execution

```cpp
interp.runCompiledScript(chunk);
interp.runCompiledScriptFile("script.ikbc");
interp.runCompiledScriptString(blob);
```

### Bytecode path limitations

- C++ builtins (`print`, `+` operators, etc.) are not serialized — resolved from stdlib at runtime
- `import` during `compileScript` may load modules via the interpreter
- generic functions stay on the AST path
- `@interpret` / `forceInterpret` — force AST execution

---

## Limitations

| Area | Status |
|------|--------|
| Tuple patterns in `match` | planned |
| Destructuring in function parameters | planned |
| Full module serialization in IKBC | not supported |
| GUI (`IkigaiScriptApp`) | requires GLFW/OpenGL |

Current task list: [ARCHITECTURE_PLAN.md](../ARCHITECTURE_PLAN.md).

---

## See also

- [README.md](../README.md) — overview and quick start
- [README.ru.md](../README.ru.md) — обзор (Русский)
- [`.agents/skills/architecture/SKILL.md`](../.agents/skills/architecture/SKILL.md) — engine architecture
- `IkigaiScript/Tests/` — executable language specification (Catch2)
