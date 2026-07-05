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

---

## 14. Unicode

**`utf8Utils.hpp`:** Provides `decodeCodePoint`, `encodeCodePoint`, `utf8Length`, `utf8At`, `utf8Slice`.

**Char literals:** Parser now uses `decodeCharLiteral(val)` (from `utf8Utils.hpp`) instead of `val[0]`, correctly decoding multi-byte UTF-8 and `\u{XXXX}` escapes.

**`getPrintString()`:** Chars are UTF-8 encoded (not `?` for non-ASCII). `string` passthrough. Ranges/tuples have dedicated formats.

**`length(s)`:** Returns Unicode code point count via `utf8Length`, not byte count.

**`s[i]`:** Returns `Char` at code point index `i` via `utf8At` (string indexing in `listindex`).

**Escape sequences:** `replaceWhitespaceLiterals` handles `\\`, `\"`, `\'`, `\n`, `\r`, `\t`, `\u{XXXX}`.

**`Char` type annotation:** `checkTypeInScope("Char") → Type::Char`.

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
