---
name: SimpleScript Architecture Guidelines
description: Rules and guidelines for adding or modifying code based on SimpleScript's architecture, including Memory Management, Values, AST, and Type Systems.
---

# SimpleScript Architecture Guidelines

When working with the SimpleScript codebase (`f:\projects\SimpleScript\SimpleScript`), always adhere strictly to the following core architectural rules:

## 1. Memory Management & AST (Arena Allocator)
**DO NOT** use `std::shared_ptr` or `std::unique_ptr` for AST nodes (`Expression`, `Statement`, `Block`, etc.).
- The project uses an **Arena Allocator** (`Library/arena.h`) to drastically reduce allocation overhead.
- All AST node allocations must be performed using `arena.make<T>()` and passed as raw pointers (`Expression*`).
- **Reason:** Parsing large scripts creates a massive AST tree. Smart pointers would introduce severe performance and memory fragmentation bottlenecks.

## 2. Dynamic Types & Values (Tagged Unions)
**DO NOT** use deep OOP hierarchies for runtime values.
- The `Value` struct uses a `std::variant` (Tagged Union) internally to represent the payload (Int, Float, String, Function, Map, etc.).
- Runtime values are stored as `ValuePtr` (`std::shared_ptr<Value>`).
- Values avoid full C++ RTTI overhead. Always rely on the `Type` enum descriptor (`Type::Int`, `Type::String`, etc.) embedded within `TypeDescriptor` to identify the current type.
- **Dynamic Flag:** Use `typeDescriptor.isDynamic = true` for variables that explicitly skip strong static typing rules and can accept any type assignment at runtime (like dynamically inserted map values).

## 3. Decorator System
Variables, functions, and properties can have decorators prefixed by `@`.
- Decorators must be collected during the Lexing and Parsing phases and bound directly to the AST nodes.
- They are intended to provide metadata for external tools like the `VisualEditor`, rather than directly affecting core runtime logic.

## 4. Visitor Pattern for External Consumers
The Interpreter acts as the direct executor (`Lexer` -> `Parser` -> `execute(AST)`). 
For external tools (like bytecode compilers or visual editors), **Visitor patterns** must be used.
- Do not pollute core AST nodes with logic specific to Code Generation or UI rendering.
- Ensure that AST nodes expose necessary data through clean `accept(Visitor&)` interfaces if you are implementing new analytical passes.

## 5. Lexer & Parser Separation
- **Lexer** (`lexer.cpp`): Responsible strictly for chunking strings into tokens and inferring basic classifications.
- **Parser** (`parser.cpp`): State machine-based processor parsing tokens into AST via `ParseState`.
- Any new language feature must introduce distinct states in the `Parser` (e.g., `ParseState::DefineClass`, `ParseState::DefineEnum`) instead of mixing structural logic randomly.
