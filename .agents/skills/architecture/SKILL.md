---
name: SimpleScript Architecture Guidelines
description: Rules and guidelines for adding or modifying code based on SimpleScript's architecture, including Memory Management, Values, AST, Type Systems, Parser, Scopes, and known gotchas.
---

# SimpleScript Architecture Guidelines

Workspace path: `D:\projects\pets\SimpleScript\SimpleScript`

When working with the codebase, always adhere strictly to the following rules.

---

## 1. Memory Management & AST (Arena Allocator)

**DO NOT** use `std::shared_ptr` or `std::unique_ptr` for AST nodes.

- All AST nodes (`Expression*`, `ExpressionPtr`) are allocated via `arena.make<T>()` — raw pointers owned by the arena.
- `ExpressionPtr = Expression*` throughout the codebase.
- The arena is cleared between interpreter resets; do not hold raw AST pointers across `clearState()` calls.
- Runtime **values** are `ValuePtr = std::shared_ptr<Value>` — smart pointers are fine and expected for values.

---

## 2. Runtime Values (`Value`, `ValuePtr`)

- `Value` uses a **tagged union** (`union Payload`) — NOT `std::variant`. Access via `getInt()`, `getString()`, `getList()`, etc.
- `getType()` returns `typeDescriptor.type` (a `Type` enum). Always check type before accessing payload.
- Every `Value` constructor initialises `typeDescriptor` inline: `explicit Value(Int a) { typeDescriptor = TypeDescriptor{Type::Int, true, false, true, false}; ... }` — fields are `{type, isInit, isNullable, isConst, isDynamic}`.
- **Default constructor** `Value()` creates a Null value with `isInit=true, isNullable=true`.

---

## 3. Type System (`TypeDescriptor`)

```cpp
struct TypeDescriptor {
    Type type     = Type::Null;
    bool isInit   = false;
    bool isNullable = true;
    bool isConst  = false;
    bool isDynamic = false;   // ← false by default since the static-typing refactor
};
```

### Semantics (post-refactor)

| Declaration | `isDynamic` | Behaviour |
|---|---|---|
| `var a = 10` | `false` | Type inferred as `Int`; reassignment to other types → `TypeConvertError` |
| `var a: Int = 10` | `false` | Explicit static type |
| `var a: Float = 1` | `false` | Upcast Int literal → Float at definition |
| `var a: Dynamic = 10` | `true` | Any type allowed |
| `dynamic a = 10` | `true` | Equivalent to `var a: Dynamic = 10` |
| `const a = 10` | `false` | `isConst=true`, `isDynamic=false` |

### `checkConvertTypes(t1, t2)` — declaration-time compatibility
- `t1.isDynamic = true` → always `true`
- `Float ← Int` → `true`
- `Int ← Bool` → `true` (declaration only)
- Exact match → `true`

### Assignment type check (strict for locked vars)
In the `=` operator, when `lhs.isInit && !lhs.isDynamic`:
- Allowed: exact match, or `Float ← Int`
- **Not** allowed: `Int ← Bool`, cross-type coercions

After assigning, owner flags (`isDynamic`, `isConst`, `isNullable`) are **preserved** from the LHS. Only `type` is preserved for static vars.

### `DefineVar` AST node
`DefineVar` carries a `TypeDescriptor typeDescriptor` (filled at parse time). In `consolidated()`, the runtime enforces the declared type and sets `isInit=true` on the stored value.

---

## 4. Parser (`parser.cpp`) — State Machine

The parser is a state machine driven by `ParseState` enum (in `enums.h`). Key states:

| State | Triggered by |
|---|---|
| `DefineVar` | `var` keyword |
| `DefineConst` | `const` keyword |
| `DefineDynamic` | `dynamic` keyword |
| `DefineFunc` / `FuncArgs` / `FuncResult` | `fun` keyword |
| `DefineCoro` / `CoroArgs` | `coro` keyword |
| `DefineClass` / `ClassArgs` | `class` keyword |
| `GenericBody` | generic function `<T>` |
| `ReadLine` | any non-keyword expression |
| `ReturnLine` / `YeldLine` | `return` / `yeld` |
| `LoopCall` / `ForEach` | `for` / `while` / `foreach` |
| `IfCall` / `ExpectIfEnd` | `if` / `else` |
| `Decorator` / `DecoratorArgs` | `@` |
| `ImportModule` | `import` |

### `ReadLine` state — brace tracking
`ReadLine` accumulates all tokens until `;`. It now tracks `{`/`}` nesting via `parseLambdaBrakets` so that `;` inside a lambda body does **not** prematurely fire.

### `DefineVar` / `DefineDynamic` lambda tracking
`parseLambda` flag (1 = `fun` seen, 2 = body `}` closed). The lambda path only activates when `parseStrings[0] == "fun"` **after** stripping the name and `=` — i.e., only when the **entire** RHS is a lambda. For nested lambdas like `map(arr, fun(...){})`, `getExpression` handles them via the inline-lambda detector.

### Adding a new keyword / feature
1. Add a `ParseState::MyNewState` to `enums.h`.
2. Add `token == "mykeyword"` → `parseState = ParseState::MyNewState` in `BeginExpression`.
3. Add `case ParseState::MyNewState:` handler in `parse()`.
4. Replicate changes in `parsingImpl.hpp` (legacy path used in some contexts).

---

## 5. Inline Lambdas (`Parser::parseInlineLambda`)

`parseInlineLambda(tokens, scope)` registers an anonymous function as `__anon_lambda__N` in the **global scope** (not in the current block scope, which may be closed before the lambda is called). Returns `ResolveVar("__anon_lambda__N")`.

Called from `getExpression` when `strings[i] == "fun"` and `strings[i+1] == "("` or `"["`.

Handles optional capture list `fun[a, cLocal=c](params) { body }`:
- `[a]` → ref-capture: pins the `ValuePtr` shared_ptr from the enclosing scope.
- `[local=outer]` → copy-capture: snapshot of value at creation time.

Captures are stored in `Function::captures` (`map<string, CaptureEntry>`) and injected into the call frame in `callFunction` before arg binding.

---

## 6. Scopes (`Scope`, `ScopePtr`)

```
globalScope
  ├── scopes["ClassName"]   ← class scope (isClassScope=true, persists)
  ├── scopes["__anon"]      ← anonymous block (closed when } is seen)
  └── scopes["fun__call_N"] ← unique call frame (closed after return)
```

### Key rules
- `newScope(name, parent)` reuses an existing scope if name already exists in `parent->scopes`. For function call frames, use **unique names** (`name + "__call_" + counter`) to avoid reusing stale frames.
- `closeScope(scope)` clears `functions`, `variables`, `scopes` and erases the scope from its parent. Class scopes (`isClassScope=true`) just walk back to parent without clearing.
- `resolveScope(name, scope)` searches up the chain and may re-parent; fixed to not create cycles.
- `resolveVariable(name, scope)` walks `scope->parent` chain, then searches `modules`. If not found, inserts a new null variable in `initialScope`.
- `resolveVariable(name, classs, scope)` — checks `classs->variables` first, then falls back to scope chain. Used when executing inside a class instance context.

### Class instances vs. class scope
- The **class scope** (`globalScope->scopes["Point"]`) stores default variable values and function definitions. It **persists** for the life of the interpreter.
- A **class instance** (`Class` object) copies variables from the class scope at construction time (`Class::Class(ScopePtr)`). Each instance has its own `variables` map.
- Methods look up instance fields via `classs->variables`, not via the scope chain.

### `MemberFunctionCall` args parsing — zero-arg gotcha

`p.method()` with **zero arguments**: the inner args-loop only set `addedArgs = true` when a non-empty arg was pushed. For zero-arg calls `)` was reached with empty buffer → `addedArgs = false` → `i` was reset to the method name → next `++i` landed on `(` again → `(` was re-processed, injecting a spurious `identity()` call into `subexpressions`. This made `callFunction` see 1 unexpected arg and throw.

**Fix (parser.cpp ~line 775):** set `addedArgs = true` whenever the closing `)` is reached at `nestLayers == 0`, regardless of whether any args were collected.

---

## 7. Function Calls (`callFunction`)

`callFunction(fnc, scope, args, namedArgs, classs)` — all 5 params. Key variants:
```cpp
callFunction(fnc, scope, args, classs)           // no named args
callFunction(fnc, scope, args, ClassRef classs)  // class instance via shared_ptr
callFunction(fnc, const List& args)              // uses globalScope, no classs
```

### Body types (`FunctionBodyType`)

| Type | Used for |
|---|---|
| `Subexpressions` | Script-defined functions/methods |
| `Lambda` | C++ stdlib lambdas `[this](List)` |
| `ScopedLambda` | C++ lambdas with scope arg |
| `ClassLambda` | C++ class lambdas |

For `Subexpressions` body:
1. Create unique call frame scope.
2. Inject explicit captures (`fnc->captures`).
3. Bind positional / named / default args.
4. For constructors: use `resolveScope(fnc->name, scope)` instead of new frame.
5. Execute subexpressions. Return `returnVal`.
6. `closeScope(scope)`.

### Short-circuit operators
`&&` and `||` are handled **before** arg evaluation in `consolidated()` FunctionCall case — the RHS is only evaluated if necessary.

---

## 8. Numeric Literals

`parseNumericLiteral(token)` in `stringUtils.hpp` handles:
- `0x`/`0X` prefix → base-16 via `from_chars`
- `0b`/`0B` prefix → base-2 via `from_chars`
- Otherwise → decimal/float via `toDouble`

Use `parseNumericLiteral` (not `toDouble`) anywhere a raw token is converted to a number.

---

## 9. `!` Operator and `fact(n)`

- `"!"` stdlib handler: **logical NOT** — `1 arg → Int(!truthy)`, `0 args → resolve ref`.
- **Factorial** is now `fact(n)` builtin.
- The unary `!x` expression parses as a 1-arg FunctionCall to `"!"`.

---

## 10. Known Limitations / Open Issues

| Issue | Status |
|---|---|
| `parsingImpl.hpp` is a legacy duplicate of some parser logic — NOT INCLUDED | Dead code; never `#include`-d. Kept for reference; changes to `parser.cpp` do NOT need to be synced there. |
| `var x;` (declaration without assignment) may crash in `consolidated()` DefineVar if `defineExpression == nullptr` | Not reproduced, but potential null-deref on `val->getType()` |
| `ReadLine` implicit statement termination via `}` | Fixed; `}` in ReadLine with no pending `{` triggers implicit `;` then closes scope inline, no recursion |
| Match expression form in getExpression inner loop — double evaluation | Fixed via `getExpressionDepth` counter + `closeCurrentExpression()` only updates `previousExpression` when currentExpression is non-null |
| `t.0` member access tokenization | Fixed; lexer only treats `.<digit>` as float continuation when the PRECEDING char is also a digit, not an identifier |

---

## 11. Match Expression (Rust-like)

**Syntax:**
```simplescript
match (n) {
    case 1 => { "one"; }
    case 1..5 => { "small"; }   // range pattern
    case _ => { "other"; }       // wildcard (same as default)
    default => { "other"; }      // also valid
};
var r = match (n) { case 1 => { 100; } default => { 0; } };  // expression form
```

**AST:** `MatchExpression { target, arms: Vec<MatchArm { pattern, body }>}` in `expressions.hpp`. `MatchArm.pattern == nullptr` = default/wildcard arm.

**Parser states:** `ParseState::MatchCall` (collect scrutinee), `ParseState::MatchCasePattern` (collect pattern until `=>`), `ParseState::MatchDefault` (wait for `=>`).

**Parser gotcha — `getExpressionDepth`:** When `match` is inside `getExpression`'s inner parse loop (e.g. `var r = match ...`), the outer `}` of the match body must NOT evaluate the match early. The counter `parser->getExpressionDepth` is incremented before the inner parse loop and decremented after. The `BeginExpression` top-of-turn check skips evaluation for `}` tokens when `getExpressionDepth > 0 && previousExpression->type == Match`.

**`closeCurrentExpression()` gotcha:** This method now only sets `previousExpression = currentExpression` when `currentExpression != nullptr`. This prevents `previousExpression` from being cleared to null when called with a nil currentExpression.

**Patterns:** literals, Range values (`..`/`..=`), wildcard `_`.

---

## 12. Range Expressions

**Syntax:** `1..5` (exclusive [1,5)), `1..=5` (inclusive [1,5]). Used in for-in loops and as array/list index for slicing.

**Type:** `Type::Range`. Stored in `Value::value.asRange` as `RangeValue { Int start, Int end_, bool inclusive }`. `getRange()` accessor.

**Lexer fix:** `..=` is checked before `..` in `lexer.cpp` (and `parsingImpl.hpp`'s `Tokenize`) to avoid partial match. `1..5` tokenizes as `1`, `..`, `5` — lexer stops decimal-float continuation at `..` (when preceding char is a digit AND next char is `.`).

**Operators:** `".."` and `"..="` stdlib handlers create `RangeValue`. `OperatorPrecedence::range` (between `compare` and `addsub`).

**ForEach:** Range values are iterable — the `ForEach` consolidated case handles `Type::Range` by iterating `start..(inclusive?end_:end_-1)`.

**Slicing:** `arr[1..3]` uses `listindex` which detects `Type::Range` index and slices the container. String slicing uses byte indices (Unicode-safe for multi-byte chars via `utf8Slice`).

---

## 13. Tuples

**Syntax:** `(a, b, c)` — detected in `getExpression` by pre-scanning for top-level commas inside `(...)`. Returns `TupleLiteralExpression { elements }`. `Type::Tuple` stored as `asList` (reuses List payload).

**Access:** `t.0`, `t.1` etc. — `MemberVariable` consolidated case checks `Type::Tuple`, parses member name as digit index, returns `getTuple()[idx]`.

**Lexer gotcha:** `t.0` was being tokenized as `t` + `.0` (float). Fixed: `Lexer::Tokenize` only treats `.<digit>` as decimal continuation when the preceding CHARACTER is also a digit, not an identifier letter.

**`Value::makeTuple`:** Static factory — creates Value with `Type::Tuple` and places items in `asList`. Use `getTuple()` to access the list.

**`for` over tuple:** `for (x : t)` iterates each element in order; `for (idx, x : t)` gives index + element — same as List. No multi-bind unpack in `for`.

### Tuple Destructuring

**Phase 1 — declaration with pattern:**
```simplescript
var (a, b) = (1, 2);
const (x, y) = (1, 2);
dynamic (p, q) = (1, "hi");
var (first, _) = (42, "skip");   // _ skips a slot
```
- `DefineVar::patternNames` (non-empty vector) drives this path in `consolidated()`.
- Each bound variable gets its `TypeDescriptor` inferred from the corresponding tuple element (same rules as plain `var`). `isConst`/`isDynamic` from the declaration apply to all variables.
- `_` slots are silently skipped — no variable is created.
- Errors: non-tuple RHS, size mismatch, duplicate names → runtime exception.
- Parser: `ParseState::DefineVar/DefineConst/DefineDynamic` — when `parseStrings[0] == "("`, scans the pattern before `=`.

**Phase 2 — reassignment:**
```simplescript
var a = 0; var b = 0;
(a, b) = (1, 2);       // assign into existing vars
(a, b) = (b, a);       // swap: RHS fully evaluated before any binding
(a, _) = (99, 0);     // _ skips a slot
```
- `DestructuringAssign { patternNames, valueExpression }` AST node (`ExpressionType::DestructuringAssign`).
- Variables must already exist in scope. Type-check and owner-flag rules mirror the `=` operator.
- `isConst` variables throw on reassign. Static-typed vars throw `TypeConvertError` on type mismatch.
- Parser: `ReadLine` / `}` implicit-semicolon path — checks `line[0] == "("` followed by pattern + `=` before falling through to `getExpression`.

**Wildcard `_`:** reuses the existing `_` keyword. In destructuring contexts it means "ignore this element" — no variable is bound.

---

## 14. Unicode

**`utf8Utils.hpp`:** Core utilities — all inline, no dependencies.

| Function | Purpose |
|----------|---------|
| `utf8DecodeOne(s, pos)` | Decode one code point, advance `pos` |
| `utf8Encode(cp, result)` | Encode code point to UTF-8 |
| `utf8Length(s)` | Count code points (not bytes) |
| `utf8At(s, n)` | `n`-th code point |
| `utf8Slice(s, start, end)` | Substring by code point indices `[start, end]` inclusive |
| `utf8Substr(s, start, count)` | Substring from code point `start`, `count` code points |
| `utf8Reverse(s)` | Reverse by code points |
| `utf8SplitCodePoints(s)` | Split into `vector<string>`, one code point each |
| `utf8Find(haystack, needle, startCp)` | First occurrence as **code point index**, or -1 |

### String operation semantics

| Operation | Index unit | Notes |
|-----------|-----------|-------|
| `length(s)` | code points | `utf8Length` |
| `s[i]` → `Char` | code points | `utf8At` |
| `s[a..b]` / `s[a..=b]` | code points | `utf8Slice` — matches List/Array slice semantics |
| `range(str, start, count)` | code points | `utf8Substr`; `count` is number of code points, not bytes |
| `reverse(s)` | code points | `utf8Reverse` |
| `split(s)` without delim | code points | `utf8SplitCodePoints` |
| `split(s, delim)` with delim | bytes | `std::string` split; correct for ASCII/valid UTF-8 delimiters |
| `find(s, needle)` → Int or null | code points | `utf8Find`; also accepts optional 3rd arg `startCp` |
| `replace(s, from, to)` | bytes | `std::string::find/replace`; correct for valid UTF-8 substrings |
| `startswith(s, p)` / `endswith(s, p)` | bytes | correct for valid UTF-8 |
| `String + String`, `+=` | bytes | UTF-8 concatenation is byte-correct |
| `Char + String` | — | Non-ASCII `Char` correctly encoded via `utf8Encode` |

**Char literals:** Parser uses `decodeCharLiteral(val)` (from `utf8Utils.hpp`) instead of `val[0]`, correctly decoding multi-byte UTF-8 and `\u{XXXX}` escapes.

**`getPrintString()`:** Chars are UTF-8 encoded via `utf8Encode`. String passthrough. Ranges/tuples have dedicated formats.

**Char→String conversion:** `upconvert`/`hardconvert`/`operator+` all use `utf8Encode` — non-ASCII chars no longer produce `?`.

**Escape sequences:** `replaceWhitespaceLiterals` handles `\\`, `\"`, `\'`, `\n`, `\r`, `\t`, `\u{XXXX}`.

**`Char` type annotation:** `checkTypeInScope("Char") → Type::Char`.

**Out of scope:** grapheme clusters, Unicode normalization, locale-aware collation.

---

## 15. Generics (current state)

**Syntax:** `fun identity<T>(x: T): T { return x; }`. On call: monomorphizes via text substitution + re-parse.

**Limitation:** Only bare `T` type parameters are inferred (not `List<T>` etc.). No generic classes. No explicit instantiation `f<Int>(x)`.

**Tests:** `GenericsTests.cpp` — 6 smoke tests for basic monomorphization.

---

## 16. Decorator System

Variables, functions, class scopes, and their members can be decorated with `@name` or `@name(args)`. Decorators are stored in:
- `scope->scopeMetadata` (class-level)
- `scope->membersMetadata[memberName]` (member-level)

Decorators are **metadata only** — they do not affect runtime semantics; they serve external tools like the VisualEditor.

---

## 17. Visitor Pattern (External Consumers)

For bytecode compilers or visual editors, use the **Visitor** pattern — do NOT embed code-gen or UI logic into AST nodes. AST nodes must expose data through `accept(Visitor&)` interfaces for analytical passes.

---

## 18. Module System (Verse-like)

SimpleScript implements a modular system inspired by the Verse programming language.

### Syntax & Keywords
- **`module Name;`**: Declares that the current file defines a module named `Name`. It updates `parseScope->name` and registers the module in the interpreter registry.
- **`export <statement>`**: Used before variable, constant, function, or class definitions. Marks the defined symbol as exported, registering it in `mod.exports`.
- **`import "path";`**: Loads and evaluates a script file as a module. Paths are resolved relatively and canonicalized.
- **`import Math;`**: Imports a built-in module.
- **`import/using { a, b as c } from Math;`**: Selectively imports exported symbols from module `Math` into the current scope (with optional renaming via `as`).
- **`using a = Math.b;`**: Binds a single symbol from `Math.b` as `a` in the current scope.
- **Qualified access (`Module.symbol` / `Module.func()`)**: Allows calling or reading exported symbols of a module using dot-notation.

### Compiler & Runtime Architecture
1. **Isolated Module Scopes**: Each script module is parsed and evaluated in an isolated scope (`parent = nullptr`).
2. **Path Canonicalization & Cache**: Modules are cached in `moduleIndexByPath` and `moduleIndexByName`. Circular imports are checked via `moduleLoadStack` before cache hits.
3. **Parser State Preservation (`ParserStateSaver`)**: Since imports trigger recursive `evaluate()` calls mid-parse, the `ParserStateSaver` RAII structure preserves the parser's state, stacks, and current expressions, ensuring re-entrant safety.
4. **Symbol Binding (`bindImportedSymbol`)**: Copies references of functions, variables, and class scopes from the module's scope into the importing scope. For functions and class constructors, it also populates `scope->variables` with `Value(FunctionRef)` to make them callable by name.
5. **Qualified Resolving**: `MemberVariable` and `MemberFunctionCall` expressions check if the LHS is a module name. If so, they redirect resolution to the module's scope and validate that the symbol is exported (`isExported`).

---

## 19. Defer Blocks (Verse-like)

SimpleScript implements scope-based deferred execution using the `defer` keyword.

### Syntax & Semantics
- **`defer { statements; }`**: Registers the block of statements to be executed when the current runtime scope is popped.
- **Execution Order**: Multiple `defer` statements in the same scope are executed in strict **LIFO** (Last In, First Out) order.
- **Scope-based**: `defer` statements inside functions, loops, or `{ }` blocks are tied to their respective scopes.
- **Loop Watermarks**: For loops (`Loop`) and iteration blocks (`ForEach`), defers registered inside an iteration are run at the end of that iteration (via watermarking `scope->deferred.size()`), preventing defer leakages across iterations.
- **Restrictions**: Transferring control out of a defer block (using `return`, `break`, or `continue`) is strictly forbidden and triggers a runtime `Exception`.
- **Exception Guarantee**: If a function exits via throwing an exception, all registered defers in its call frame scope are still guaranteed to run during unwinding (via try-catch guard in `callFunction` calling `closeScope`).

### Technical Architecture
1. **Scope Registration**: Registered defers are stored in `scope->deferred` (a vector of `ExpressionPtr`).
2. **`closeScope(scope, runDeferred = true)`**: Performs scope teardown. If `runDeferred` is `true`, it triggers `runDefers()`. Note that during parse-time (compilation), `closeScope` is called with `runDeferred = false` to prevent executing defers during syntax analysis.
3. **`runDefers(scope, classs, fromIndex)`**: Drains and executes deferred statements from the end of `scope->deferred` (LIFO) down to `fromIndex` using `executeDeferBody`.
4. **`executeDeferBody`**: Resolves the defer body expressions and validates that they did not trigger any control flow interrupt (`explicitReturn` or `interrupt` in the `BlockResult` returned by `needsToReturn`).
5. **Return Safety**: The return value of a function is copied (`std::make_shared<Value>(*returnVal)`) before `closeScope` runs, guaranteeing that deferred actions modifying local variables do not affect the already calculated return value of the function.

---

## 20. SafeBlocks and Transactions (`>>> { }`)

SimpleScript supports atomic transaction blocks via safe blocks.

### Syntax & Semantics
- **`>>>{ statements; }`**: Defines a transaction block.
- **Atomicity**: Changes to existing outer variables inside the block are captured using a **Copy-on-Write (COW)** overlay scope.
- **Commit & Rollback**:
  - If the block executes successfully (no exceptions are thrown), changes are committed (propagated to outer variables via `commitTransaction`) and defers registered inside the transaction scope are executed.
  - If any exception is thrown, changes are discarded via `rollbackTransaction` and defers registered inside the transaction scope are ignored.
- **Evaluation Values**:
  - If the block contains a `return <expr>` or produces a value from its last statement, it yields a `Type::Optional` (either `some(val)` upon success or `empty` upon failure).
  - If the block does not produce a value, it yields a `Type::Bool` (`true` upon success or `false` upon failure).

### Technical Architecture
1. **Overlay Scope**: Evaluated in a dedicated scope where `isTransactionScope = true`.
2. **COW Resolving**: Writing to a variable (LHS of `=`) via `resolveVariableForWrite` checks up the scope chain. If a transaction scope is crossed before finding the declaration, it copies the value using COW (`std::make_shared<Value>(*original_val)`) and registers the copy in the transaction scope.
3. **Assignment Handling**: The parser catches the LHS `ResolveVar` inside expressions being assigned to, enforcing write resolution.

---

## 21. Result Type (`Result<T, E>`)

SimpleScript implements a native `Result<T, E>` type representing either a successful computation `Ok(T)` or an error `Err(E)`.

### Syntax & Type Annotations
- **Type Syntax**: `Result<T, E>` where `T` is the success type and `E` is the error type.
- **Optional/Nullable Syntax**: `Result<T, E>?` represents a nullable Result.
- **Container Checks**: `Result` is registered as a container type, enabling recursive static type parameter checks (e.g. `Result<Int, String>` is incompatible with `Result<String, String>`). `Dynamic` is compatible with any type parameter.

### Stdlib API & Usage
- **`resultOk(val)`**: Constructs an `Ok` result (`Value::makeResultOk(val)`). If no explicit type parameters are provided, it infers the success type `T` from `val` and defaults the error type `E` to `Dynamic`.
- **`resultErr(err)`**: Constructs an `Err` result (`Value::makeResultErr(err)`). Infers `E` from `err` and defaults `T` to `Dynamic`.
- **`resultIsOk(r)`**: Returns a `Bool` indicating if `r` is an `Ok` variant.
- **`resultGet(r)`**: Unwraps the `Ok` value of `r`. Throws a runtime `Exception` if `r` is `Err`.
- **`resultGetErr(r)`**: Unwraps the `Err` value of `r`. Throws a runtime `Exception` if `r` is `Ok`.
- **Print Formatting**: Prints as `ok(value)` or `err(value)`.

---

## 22. Live Variables (Reactivity)

SimpleScript supports spreadsheet-like reactivity using `live` variables.

### Syntax & Semantics

```ss
live var a = b + c;       // live var with guard — recomputes when b or c change
live dynamic d = a;       // live dynamic — accepts any type on recompute
live var x;               // Pending live slot — guard set later via rebind
live dynamic label;       // Pending dynamic live slot
live x = y + 1;           // Live Rebind — attach/replace guard on existing var
live var n: Int;           // Typed pending slot (uninit until rebind)
```

### Two states of a live slot

| State | When | Behaviour |
|-------|------|-----------|
| **Pending** | `live var x;` / `live dynamic x;` | Slot registered in `DependencyManager`, guard = null, no subscriptions, value is uninit |
| **Active** | `live var x = expr;` or `live x = expr` | Guard evaluated, runtime deps tracked, recomputes on upstream change |

**Direct write to an Active live target** (`a = 99`) **throws** `Exception("Cannot assign directly to live variable...")`; use `live a = ...` to rebind.

**Reading a Pending slot** before rebind follows the normal uninit-variable rules.

### Technical Architecture
1. **`DependencyManager`** (`dependencyManager.hpp/.cpp`): Manages all live bindings, dependency tracking, and notification. Owned by `IkigaiScriptInterpreter`; cleared via `clear()` on `clearState()`.
2. **Identification (`VarSlot`)**: Variables are identified by their `Value*` address, which remains stable across normal assignments (`*lhs = *rhs`), making it perfect for tracking.
3. **Read Tracking**: When a live variable evaluates its guard expression, `isCollecting` is true. Any variable read via `resolveVariable` triggers `DependencyManager::recordRead(VarSlot)`. Deps are deduplicated per recompute cycle.
4. **Write Notification**: After a successful `operator=` assignment (and after `commitTransaction`), `DependencyManager::onVariableChanged(VarSlot)` is called. It notifies all subscribed live bindings to re-evaluate their guards. Notification is suppressed during recompute of the same binding to prevent trivial recursion.
5. **Transaction integration**: `onVariableChanged` is fired only after `commitTransaction` succeeds. Rolled-back writes never reach the live graph.
6. **Cycle Detection**: `internalRecompute` tracks recursion depth; exceeding 64 levels throws `Exception("Live dependency cycle detected")`. Mutual cycles (`live a = b+1; live b = a+1`) are caught on the second activation.
7. **Dynamic recompute**: For `isDynamic` bindings, the computed type is preserved as-is; only owner flags (`isConst`, `isNullable`, `isDynamic`) are carried forward. For static bindings, the declared type is enforced with the same rules as `operator=` (Float←Int upcast allowed).
8. **`registerLiveSlot`** (Pending path): called from `DefineVar` when `isLive && !defineExpression`. Stores a binding in Pending state with no deps or guard.
9. **`activateBinding` / `rebindBinding`**: transition a slot to Active, set the guard expression, and trigger the first recompute. `rebindBinding` also works on plain (non-live) vars, converting them to live on the fly.

---

## 23. Concurrency (Cooperative Async)

SimpleScript implements Verse-like cooperative async via a single-threaded scheduler.

### Architecture overview

```
Script layer (primary)                 Host layer (optional)
──────────────────────────             ──────────────────────
coro f() { yeld …; return …; }         interp.pump(N)
var t = f();      // → TaskRef         interp.scheduler.clear()
await t;          // drive to done
spawn { body }    // fire-and-forget
sync { … }        // await all
race  { … }       // first wins
branch{ … }       // background child
```

### New files

| File | Purpose |
|------|---------|
| `Library/concurrency/task.hpp` | `Task` struct, `TaskState` enum, `TaskRef`, `Coro`/`CoroRef` aliases |
| `Library/concurrency/cancellation.hpp` | `CancellationToken` — cooperative cancel tree |
| `Library/concurrency/scheduler.hpp/.cpp` | `ConcurrencyScheduler` — ready queue, `pump()`, `spawn()`, `suspendFor()` |

### Key types

```cpp
enum class TaskState { Created, Running, Suspended, Completed, Cancelled };

struct Task {
    FunctionRef func;
    ScopePtr    scope;           // unique coro call-frame scope
    Class*      classs = nullptr;
    int         pc = 0;         // program counter (expressionId)
    TaskState   state = TaskState::Created;
    ValuePtr    result;          // final return value
    ValuePtr    suspendPayload;  // last yeld value
    std::weak_ptr<Task> awaitingTask;   // child we're waiting for
    std::weak_ptr<Task> parent;
    std::vector<std::shared_ptr<Task>> children;
    CancellationToken::Ptr token;
    std::string id;              // unique scope name ("fnc__coro_N")
};

using Coro = Task;    // backward-compat alias
using CoroRef = TaskRef;
```

### coro / yeld (existing API — upgraded)

- **Unique scopes**: `callFunction` for `isCoro` now uses `fnc->name + "__coro_" + counter` (not `fnc->name`) to avoid stale frame reuse.
- **Exception safety**: `callCoro` wraps body execution in try/catch; on exception sets `state = Cancelled` and closes scope before re-throwing.
- **`task->result`** is stored on every `return` (and on body-exhaustion). `await` uses `task->result` to extract the completed value.
- **Yeld propagation**: `consolidated(Yeld)` now returns `ValueNode(val, ExpressionType::Yeld)`. `needsToReturn` has a `yeldReturn` field and propagates it through `IfElse`, `Block`, `Loop`, `ForEach`, `Match` cases.

### await

```ss
var t = f();           // create Task (Coro) from coro function
var result = await t;  // run t to completion synchronously; return final value
```

- Parser: `await` keyword → `ParseState::AwaitLine`. Reads tokens until `;`, creates `AwaitExpression { taskExpr }`.
- In `getExpression`: `await` is in the re-parse keyword list, so `var x = await t;` works.
- `consolidated(Await)`: if task already Completed → return `task->result`. Otherwise run `callCoro(task)` until `!task->isActive()`.

### spawn

```ss
spawn { body }     // block form: execute body, return last value
spawn f(args)      // expression form (via callExpr)
```

- Parser: `spawn` keyword → `ParseState::SpawnBlock`. Expects `{` then body.
- `consolidated(Spawn)`: block form runs body in a transient scope; expression form evaluates the call expression directly.
- Returns the result value (MVP: no background scheduling yet; executes synchronously).

### sync

```ss
var results = sync { await a; await b; };  // returns Tuple of results
```

- Parser: `sync` keyword → `ParseState::SyncBlock`. Body is a sequence of statements.
- `consolidated(SyncBlock)`: executes each statement; if result is a Task, runs it to completion. Collects all values into a Tuple.

### race

```ss
var winner = race { tf; ts; };  // first Task to complete wins
```

- Parser: `race` keyword → `ParseState::RaceBlock`.
- `consolidated(RaceBlock)`: collects Tasks from body expressions, runs them round-robin via `callCoro` until one completes. Cancels remaining tasks. Returns winner's result.

### branch

```ss
branch { body }  // fire-and-forget; body runs synchronously (MVP)
```

- MVP: runs body synchronously in a detached scope. Background scheduling is Phase 5+.

### CancellationToken

```cpp
auto token  = CancellationToken::make();
auto child  = token->makeChild();    // cancelled when parent cancels
token->onCancel([&](){ … });         // callback (sync)
token->cancel();
token->isCancelled();
```

- Parent cancellation cascades to all children.
- `Task::token` is automatically created (or inherited from parent) at spawn time.
- Phase 4 wires token cancellation to `runDefers`.

### `Function::isSuspending`

`Function` has a new field `bool isSuspending = false`. Currently always equals `isCoro`. Future phases will use it for static checks: `await`/`yeld` inside non-suspending function → Exception.

### `IkigaiScriptInterpreter` API

| Member | Purpose |
|--------|---------|
| `scheduler` (public) | `ConcurrencyScheduler` instance |
| `pump(maxSteps)` | drive up to `maxSteps` ready tasks; returns count taken |
| `callTask(TaskRef)` | called by scheduler to execute one task step |
| `clearState()` | also calls `scheduler.clear()` |

### Invariants

- **Single-threaded**: all script execution on the calling thread. No mutex on `Scope`/`Value`.
- `coroScopes` in `Scope` is **dead code** — kept for ABI; do not use.
- `Type::Task == Type::Coro` (enum alias, same numeric value).

### Phase 5: NativeJobPool — C++ thread pool

**Files:** `Library/concurrency/native_job_pool.hpp/.cpp`

```cpp
// Host setup (once, before script runs):
interp.enableNativePool(4);          // start 4 worker threads
interp.registerNativeJob("pathfind", [&map](const List& args) -> ValuePtr {
    // Runs in a worker thread — NO interpreter calls here!
    return computePath(map, args[0]->getInt(), args[1]->getInt());
});

// Script side:
//   var t = nativeJob("pathfind", x, y);
//   var path = await t;

// Or C++ API directly:
auto task = interp.submitNativeJob("pathfind", {makeInt(10), makeInt(20)});
while (task->isActive()) interp.pump(0);
```

**Data flow:**

```
Main thread                          Worker threads
────────────────────────────         ────────────────────────────
interp.pump()
  → nativePool->drainCompletions()   [worker 0] executes NativeJob lambda
  → scheduler.pump()                 [worker 1] executes NativeJob lambda
                                     ↓
                                     completionQueue_.push({task, result})
                                     ↑── mutex ──┘
```

**`Task::isNativeJob = true`**: `callTask()` skips native tasks; they are completed by workers. `drainCompletions()` sets `task->result`, `task->state = Completed`, and re-enqueues any parent awaiting the task.

**Thread safety invariant:** `Value`/`Scope`/`Arena` are NEVER touched from worker threads. The only cross-thread data structure is `completionQueue_` (protected by `completionMutex_`).

**`nativeJob("name", arg1, arg2, ...)`**: stdlib function. Submits the named job with args; returns a `TaskRef` (Type::Coro/Task) that can be `await`-ed.

### Known limitations (MVP, to be resolved later)

- `spawn { }` and `branch { }` execute **synchronously** (no true background scheduling).
- `await` inside loops blocks until completion (no interleaving with scheduler).
- No `sleep()` builtin.
- `thread.newThread` optional module is still unsafe (data race on shared interpreter); do not use for concurrency.

