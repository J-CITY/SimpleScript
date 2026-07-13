# IkigaiScript — документация по языку

> **English:** [LANGUAGE.md](LANGUAGE.md)

IkigaiScript (внутреннее имя движка: **IkigaiScript**) — императивный скриптовый язык с выражениями, функциями первого класса, ООП, корутинами и статической типизацией с опциональным `dynamic`.

---

## Содержание

1. [Лексика и синтаксис](#лексика-и-синтаксис)
2. [Типы](#типы)
3. [Переменные](#переменные)
4. [Выражения и операторы](#выражения-и-операторы)
5. [Функции](#функции)
6. [Классы](#классы)
7. [Управление потоком](#управление-потоком)
8. [Коллекции](#коллекции)
9. [Корутины](#корутины)
10. [Конкурентность](#конкурентность)
11. [Модули](#модули)
12. [Defer](#defer)
13. [Транзакции](#транзакции)
14. [Live-переменные](#live-переменные)
15. [Result](#result)
16. [Generics](#generics)
17. [Декораторы и метаданные](#декораторы-и-метаданные)
18. [Встраивание в C++](#встраивание-в-c)
19. [Байткод и IKBC](#байткод-и-ikbc)
20. [Ограничения](#ограничения)

---

## Лексика и синтаксис

- Инструкции разделяются `;`
- Блоки — `{ ... }`
- Комментарии: `// однострочный`, `/* многострочный */`
- Строки: `"обычная"`, `"""многострочная"""`, интерполяция `` `hello ${name}` ``
- Символы: `'x'` (тип `Char`)
- Числа: десятичные, `0x` hex, `0b` binary, float с точкой

### Ключевые слова

`var`, `const`, `dynamic`, `fun`, `coro`, `class`, `if`, `else`, `for`, `while`, `match`, `case`, `default`, `return`, `yield`, `break`, `continue`, `defer`, `live`, `import`, `using`, `module`, `export`, `await`, `sync`, `race`, `spawn`

---

## Типы

| Тип | Пример | Описание |
|-----|--------|----------|
| `Null` | `null` | Отсутствие значения |
| `Bool` | `true`, `false` | Логический |
| `Char` | `'a'` | Unicode-символ |
| `Int` | `42`, `0xFF` | Целое |
| `Float` | `3.14` | Число с плавающей точкой |
| `String` | `"hello"` | UTF-8 строка |
| `Array` | `[1, 2, 3]` | Однородный массив |
| `List` | `list(1, 2)` | Список значений |
| `Set` | `set(...)` | Множество |
| `Map` | `map(...)` | Словарь |
| `Tuple` | `(1, "a")` | Кортеж фиксированной длины |
| `Range` | `1..5`, `1..=5` | Диапазон (исключительный / включительный) |
| `Optional` | `Int?` | Nullable-обёртка |
| `Result` | `Result<Int, String>` | Ok / Err |
| `Class` | экземпляр класса | Объект с полями и методами |
| `Function` | `fun(...) { }` | Функция |
| `Coro` | результат `coro` | Корутина (задача) |
| `Dynamic` | `dynamic x = ...` | Любой тип |

### Nullable

```js
var a: Int? = null;
var b: Int? = 5;
```

### Dynamic

```js
dynamic x = 10;
x = "string";  // разрешено
var y = 10;    // тип Int зафиксирован
// y = "oops"; // ошибка типа
```

---

## Переменные

```js
var name = expr;              // вывод типа
var count: Int = 0;           // явный тип
const MAX = 100;              // константа
dynamic buffer = "";          // динамический тип
live var sum = a + b;         // реактивная переменная (см. ниже)
```

Деструктуризация кортежей:

```js
var (x, y) = (10, 20);
var (first, _) = (1, 2);  // _ пропускает элемент
```

---

## Выражения и операторы

### Арифметика

`+`, `-`, `*`, `/`, `%` — для чисел; `+` также конкатенирует строки.

### Сравнение и логика

`==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`

`&&` и `||` — с **short-circuit** семантикой.

### Присваивание

```js
x = 10;
x += 1;
```

### Формы выражений

Многие конструкции можно использовать справа от `var name = ...`. Последнее выражение в блоке — его значение (с `;` в конце).

#### Блочное выражение

```js
var a = {
    var b = 40;
    b + 2;   // 42 — последнее выражение даёт результат
};

var nested = {
    var inner = { 10 + 20; };
    inner + 12;   // 42
};
```

#### if как выражение

```js
var a = if (true) { 42; } else { 10; };   // 42
var b = if (false) { 42; } else { 10; };  // 10

var x = 5;
var result = if (x > 3) { 100; } else { 0; };   // 100

var label = if (n > 0) { "pos"; } else { "neg"; };
```

Обе ветки — блоки `{ ... }`. Значение — последнее выражение выбранной ветки.

#### for-comprehension

`for` в выражении обходит коллекцию и строит `Array` из результатов тела:

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

// Итерация по диапазону
var squares = for (i : 1..=5) { i * i; };   // [1, 4, 9, 16, 25]
```

Формы привязки: `(x : coll)`, `(idx, x : arr)`, `(key, val : map)`, `(idx, key, val : map)`.

#### match как выражение

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

Тело каждой ветки — блок; результат — последнее выражение совпавшей ветки. Без совпадения и без `default` — исключение.

### Диапазоны

```js
var r1 = 1..5;    // 1, 2, 3, 4  (конец исключён)
var r2 = 1..=5;   // 1, 2, 3, 4, 5
```

---

## Функции

### Объявление

```js
fun add(a: Int, b: Int): Int {
    return a + b;
}
```

### Лямбды

```js
var mul = fun(a, b) { return a * b; };
print(mul(3, 4));
```

### Захват (closures)

```js
var offset = 10;
var add = fun[offset](x) { return offset + x; };

// ref-захват: [name]
// copy-захват: [local=outerName]
var snap = fun[local=outer](x) { return local + x; };
```

### Именованные аргументы

```js
fun f(a, b, c) { return a + b + c; }
print(f(c=3, a=1, b=2));       // 6
print(f(10, c=5, b=2));        // 17
```

### Значения по умолчанию

```js
fun greet(name, greeting = "Hello") {
    return greeting + ", " + name;
}
print(greet("World"));
```

### Вариативные аргументы

Поддерживается синтаксис с `...` в объявлении функции (см. тесты `FunctionsTests.cpp`).

---

## Классы

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

Доступ к полям: `obj.field`, вызов методов: `obj.method(args)`.

Наследование через `class Derived, Base { ... }` и `super(...)` — см. тесты `ClassesTests.cpp`.

---

## Управление потоком

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

for (i < 10) {   // с ручным инкрементом внутри
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

`match` как statement:

```js
match (n) {
    case 1 => { print("one"); }
    case 2 => { print("two"); }
    default => { print("other"); }
}
```

Если ни одна ветка не подошла и нет `default` — исключение.

---

## Коллекции

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

### Срезы (slicing)

```js
var lst = [10, 20, 30, 40, 50];
var s = lst[1..3];    // элементы с индексами 1, 2
var t = lst[1..=3];   // индексы 1, 2, 3

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

### Set и Map

```js
var s = set(1, 2, 3);
var m = map();
// операции через builtins — см. BuiltinsTests
```

---

## Корутины

Корутина объявляется ключевым словом `coro`. Вызов `coroName(args)` создаёт **задачу** (не выполняет тело сразу). Продолжение — через `task()` или `await task`.

```js
coro gen(a, b) {
    yield a + b + 2;
    yield a + b + 4;
    return a + b;
}

var cr = gen(13, 2);
print(cr());  // 17  (первый yield)
print(cr());  // 19  (второй yield)
print(cr());  // 15  (return)
```

`await` дожидается завершения задачи:

```js
coro answer() {
    return 42;
}
var t = answer();
var result = await t;
print(result);
```

---

## Конкурентность

### await

Блокирует текущую корутину до завершения задачи.

### sync

Выполняет несколько `await` и собирает результаты:

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

Возвращает результат первой завершившейся задачи:

```js
coro fast() { return 1; }
coro slow() { yield 0; return 2; }
var winner = race {
    fast();
    slow();
};
```

### spawn

Запуск анонимной задачи из блока или выражения:

```js
var t = spawn {
    return 99;
};
var result = await t;
```

Также поддерживается форма `spawn expr;` для одиночного выражения.

### Native jobs (C++)

Из хоста можно включить пул потоков:

```cpp
interp.enableNativePool(4);
interp.registerNativeJob("myJob", [](const List& args) { /* ... */ });
```

В скрипте: `nativeJob("myJob", arg1, arg2)`.

### Планировщик

```cpp
interp.pump(maxSteps);  // выполнить готовые задачи
```

---

## Модули

### Определение модуля

Файл `math.ik`:

```js
module Math;

export fun add(a, b) { return a + b; }
export var PI = 3.14;

fun helper() { return 100; }  // не экспортируется
```

### Импорт

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

Неквалифицированный импорт по имени модуля:

```js
import Math;
```

---

## Defer

Отложенное выполнение при выходе из scope (LIFO):

```js
fun demo() {
    defer { print("second"); }
    defer { print("first"); }
    print("body");
}
// Вывод: body → first → second
```

Defer выполняется при `return`, выходе из блока и раскрутке стека.

---

## Транзакции

Синтаксис `>>>{ ... }` — транзакционный блок с commit/rollback.

```js
var x = 1;

// Успех → true, изменения сохраняются
print(>>>{ x = 2; });

// Ошибка → false, откат
print(>>>{ x = 99; var err = 1 / 0; });
print(x);  // 2
```

### Optional-результат `>>>`

```js
var res = >>>{
    var a = 40;
    return a + 2;
};
print(optionalHas(res));  // true
print(optionalGet(res));    // 42
```

При ошибке внутри блока возвращается «пустой» optional.

---

## Live-переменные

Реактивные переменные пересчитываются при изменении зависимостей (через `DependencyManager`):

```js
var a = 1;
var b = 2;
live var c = a + b;
print(c);  // 3

a = 5;
print(c);  // 7

live var x;
live x = b + 1;  // отложенная привязка
```

Прямое присваивание live-переменной запрещено — используйте `live x = expr`.

---

## Result

Тип для явной обработки ошибок:

```js
var ok  = resultOk(42);
var err = resultErr("failure");

print(resultIsOk(ok));     // true
print(resultGet(ok));      // 42
print(resultGetErr(err));  // failure

print(ok);   // ok(42)
print(err);  // err(failure)
```

Типизация:

```js
var r: Result<Int, String> = resultOk(10);
r = resultErr("oops");
```

---

## Generics

Мономорфизация при вызове (специализация создаётся на лету):

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

> Generic-функции **не компилируются в байткод** (остаются на AST-пути).

---

## Декораторы и метаданные

Синтаксис `@name` / `@name(args)` для привязки метаданных к переменным, классам, функциям:

```js
@test(min=0, max=100, widget="slider")
var volume = 50;

@editable
class Entity {
    @range(min=0, max=1)
    var health = 1.0;
}
```

Из C++ метаданные читаются через `interp.getMetadata(scope, "memberName")`.

Декораторы `@interpret` и `@bytecode` на функциях управляют путём исполнения (AST vs VM).

---

## Встраивание в C++

### Базовый API

```cpp
#include "Library/ikigaiScript.h"

IkigaiScript::IkigaiScriptInterpreter interp;

// Выполнение
interp.evaluate("var x = 1;");
interp.evaluateFile("script.ik");

// Чтение переменных
auto val = interp.resolveVariable("x");

// Регистрация C++-функции
interp.newFunction("myFn", [](const IkigaiScript::List& args) {
    return std::make_shared<IkigaiScript::Value>(42);
});
```

### Режимы исполнения

```cpp
interp.setExecutionMode(IkigaiScript::ExecutionMode::Interpreter); // default
interp.setExecutionMode(IkigaiScript::ExecutionMode::Bytecode);
```

---

## Байткод и IKBC

### Компиляция (без выполнения top-level)

```cpp
auto chunk = interp.compileScript(R"(
    fun square(n: Int): Int { return n * n; }
    print(square(5));
)");
```

```cpp
auto chunk = interp.compileScriptFile("script.ik");
```

### Сериализация (формат IKBC v1)

```cpp
std::string blob = interp.serializeCompiledScript(*chunk);
interp.saveCompiledScript(*chunk, "script.ikbc");

auto loaded = interp.loadCompiledScriptFile("script.ikbc");
auto fromStr = interp.deserializeCompiledScript(blob);
```

Формат: бинарный blob с magic `"IKBC"`, little-endian. Содержит `Chunk` (именованные функции + `main`).

### Выполнение

```cpp
interp.runCompiledScript(chunk);
interp.runCompiledScriptFile("script.ikbc");
interp.runCompiledScriptString(blob);
```

### Ограничения байткод-пути

- C++ builtins (`print`, операторы `+` и т.д.) не сериализуются — подставляются из stdlib при запуске
- `import` при `compileScript` может загружать модули через интерпретатор
- generic-функции остаются на AST-пути
- `@interpret` / `forceInterpret` — принудительно AST

---

## Ограничения

| Область | Статус |
|---------|--------|
| Tuple-паттерны в `match` | в планах |
| Destructuring в параметрах функций | в планах |
| Полная сериализация модулей в IKBC | нет |
| GUI (`IkigaiScriptApp`) | требует GLFW/OpenGL |

Актуальный список задач: [ARCHITECTURE_PLAN.md](../ARCHITECTURE_PLAN.md).

---

## См. также

- [README.ru.md](../README.ru.md) — обзор и быстрый старт
- [README.md](../README.md) — overview (English)
- [`.agents/skills/architecture/SKILL.md`](../.agents/skills/architecture/SKILL.md) — архитектура движка
- `IkigaiScript/Tests/` — исполняемая спецификация языка (Catch2)
