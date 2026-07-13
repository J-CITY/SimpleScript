# IkigaiScript (IkigaiScript)

![IkigaiScript Logo](docs/logo.png)

> **English:** [README.md](README.md)

IkigaiScript — встраиваемый скриптовый язык на C++20 с статической типизацией, классами, корутинами, модулями и двойной архитектурой исполнения: **интерпретатор AST** (по умолчанию) и **стековая VM с байткодом**.

Полная справка по синтаксису: [docs/LANGUAGE.ru.md](docs/LANGUAGE.ru.md).

## Быстрый старт

### Сборка (Windows / CMake)

По умолчанию собираются только **ядро + headless VisualCore + тесты** (без GUI-зависимостей):

```powershell
cd IkigaiScript
mkdir build
cd build
cmake ..
cmake --build . --config Debug --target IkigaiScriptTests
.\Debug\IkigaiScriptTests.exe
```

### Окно визуального редактора (опционально)

GUI-приложение (`IkigaiScriptApp`) **выключено по умолчанию**. Зависимости **не** git-сабмодули — укажите локальные пути в CMake.

| Опция / путь CMake | Назначение |
|--------------------|------------|
| `IKIGAI_BUILD_VISUAL_EDITOR` | `ON` — собрать окно редактора (по умолчанию `OFF`) |
| `IKIGAI_LIBS_ROOT` | Корень с `glfw` / `glew` (например `C:/libs`) |
| `IKIGAI_IMGUI_DIR` | Дерево Dear ImGui (`imgui.h`, backends, `imgui-node-editor/`, `TextEditor.*`) |
| `IKIGAI_GLFW_INCLUDE_DIR` / `IKIGAI_GLFW_LIB_DIR` | Явные пути к GLFW |
| `IKIGAI_GLEW_INCLUDE_DIR` / `IKIGAI_GLEW_LIB_DIR` / `IKIGAI_GLEW_BIN_DIR` | Явные пути к GLEW |

```powershell
cmake .. -DIKIGAI_BUILD_VISUAL_EDITOR=ON `
  -DIKIGAI_LIBS_ROOT=C:/libs `
  -DIKIGAI_IMGUI_DIR=D:/path/to/imgui
cmake --build . --config Release --target IkigaiScriptApp
.\Release\IkigaiScriptApp.exe
```

`IKIGAI_LIBS_ROOT` задаёт дефолты для GLFW/GLEW (`glfw-3.4` или `glfw`, `glew`). Точные path-переменные их переопределяют.

### Запуск скрипта из C++

```cpp
#include "Library/ikigaiScript.h"

IkigaiScript::IkigaiScriptInterpreter interp;
interp.evaluate(R"(
    var msg = "Hello, IkigaiScript!";
    print(msg);
)");
```

### Запуск файла

```cpp
interp.evaluateFile("script.ik");
```

## Пример: «Hello World»

```js
print("Hello, IkigaiScript!");
```

## Основные возможности

### Типы и переменные

Статические типы, nullable-аннотации, `const`, `dynamic`, вывод типа:

```js
var n = 42;
var s: String = "text";
var x: Int? = null;
const PI = 3.14;
dynamic any = 10;
any = "now a string";
```

### Функции и лямбды

```js
fun add(a, b) { return a + b; }
print(add(3, 4));  // 7

var double = fun(n) { return n * 2; };
print(double(5));  // 10

// Захват переменных
var base = 10;
var addBase = fun[base](x) { return base + x; };
print(addBase(5));  // 15
```

Именованные аргументы и значения по умолчанию:

```js
fun greet(name, prefix = "Hello") { return prefix + ", " + name; }
print(greet(name = "World"));  // Hello, World
```

### Классы

```js
class Point {
    var x = 0;
    var y = 0;
    fun Point(a, b) { x = a; y = b; }
    fun sum() { return x + y; }
}
var p = Point(3, 4);
print(p.sum());  // 7
```

### Управление потоком

Инструкции:

```js
if (n > 0) { print("positive"); } else { print("non-positive"); }

for (i = 0; i < 5; i++) { print(i); }

for (x : 1..=5) { print(x); }  // range: 1..5 включительно
```

**Выражения** — присваивание из `if`, `for`, `match` или блока:

```js
var a = if (x > 3) { 100; } else { 0; };

var doubled = for (x : [1, 2, 3]) { x * 2; };   // [2, 4, 6]

var r = match (n) {
    case 1 => { "one"; }
    case 2 => { "two"; }
    default => { "other"; }
};

var sum = {
    var a = 40;
    a + 2;   // 42
};
```

Подробнее: [docs/LANGUAGE.ru.md](docs/LANGUAGE.ru.md#формы-выражений).

### Коллекции

```js
var arr = [1, 2, 3];
print(arr[0]);        // 1
print(length(arr));   // 3

var lst = list(1, 2, 3);
var slice = lst[1..3];  // [20, 30] при lst = [10,20,30,40]

var t = (1, "hello", true);
print(t.0);  // 1
var (a, b) = (10, 20);
```

### Корутины и конкурентность

```js
coro counter(n) {
    yield n;
    return n + 1;
}
var c = counter(10);
print(c());   // 10
print(c());   // 11

var t = counter(5);
var result = await t;
```

Блоки `sync`, `race`, `spawn` — см. [docs/LANGUAGE.ru.md](docs/LANGUAGE.ru.md#конкурентность).

### Модули

```js
// math.ik
module Math;
export fun add(a, b) { return a + b; }
export var PI = 3.14;

// main.ik
import "math.ik";
print(Math.add(2, 3));
using { add } from Math;
```

### Defer, транзакции, live-переменные

```js
fun demo() {
    defer { print("cleanup"); }
    print("work");
}

var x = 1;
print(>>>{ x = 2; });  // транзакция: true при успехе
print(x);              // 2

var a = 1;
live var b = a + 1;
a = 5;
print(b);  // 6 — пересчитывается автоматически
```

### Result и generics

```js
var ok = resultOk(42);
print(resultGet(ok));  // 42

fun identity<T>(x: T): T { return x; }
print(identity("hello"));
```

### Метаданные (декораторы)

Для интеграции с визуальным редактором и инструментами:

```js
@test(min=0, max=100)
var volume = 50;
```

### Байткод и IKBC

Компиляция без выполнения top-level кода, сохранение в бинарный формат `.ikbc`:

```cpp
auto chunk = interp.compileScript(R"(
    fun square(n: Int): Int { return n * n; }
    print(square(5));
)");
interp.saveCompiledScript(*chunk, "script.ikbc");

// На другом инстансе:
interp.runCompiledScriptFile("script.ikbc");
```

Подробнее: [docs/LANGUAGE.ru.md](docs/LANGUAGE.ru.md#байткод-и-ikbc).

## Архитектура

```
Исходник → Lexer → Parser → AST
                              ├─ Interpreter (tree-walk, по умолчанию)
                              └─ BytecodeCompiler → Chunk → VM
```

- **Интерпретатор** — полная семантика, режим по умолчанию.
- **VM** — `interp.setExecutionMode(ExecutionMode::Bytecode)` или `compileScript` + `runCompiledScript`.
- **IKBC** — бинарный формат сериализации `Chunk` (magic `IKBC`).

## Структура репозитория

| Путь | Описание |
|------|----------|
| `IkigaiScript/Library/` | Ядро: парсер, интерпретатор, VM, байткод |
| `IkigaiScript/Tests/` | Тесты Catch2 (~480+ кейсов) |
| `IkigaiScript/VisualEditor/` | Ядро blueprint-графа + опциональный ImGui UI (`IKIGAI_BUILD_VISUAL_EDITOR`) |
| `docs/LANGUAGE.md` | Документация по языку (English) |
| `docs/LANGUAGE.ru.md` | Документация по языку (Русский) |
| `.agents/skills/architecture/SKILL.md` | Архитектурные правила для разработчиков |

## Тесты

```powershell
cd IkigaiScript\build\Debug
.\IkigaiScriptTests.exe                    # все тесты
.\IkigaiScriptTests.exe "[bytecode]"       # только байткод
.\IkigaiScriptTests.exe "[coroutines]"     # корутины
```

## Лицензия

Уточните лицензию в репозитории при публикации.
