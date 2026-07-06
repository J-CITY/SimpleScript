#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("unicode: ASCII string length", "[unicode.length.ascii]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "print(length(\"hello\"));") == "5");
}

TEST_CASE("unicode: Cyrillic string length in code points", "[unicode.length.cyrillic]") {
    auto interp = makeInterp();
    // "Привет" = 6 Cyrillic chars, 12 UTF-8 bytes
    REQUIRE(run(interp, "print(length(\"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\"));") == "6");
}

TEST_CASE("unicode: emoji length is 1 code point", "[unicode.length.emoji]") {
    auto interp = makeInterp();
    // 🎮 = U+1F3AE, 4 UTF-8 bytes, 1 code point
    REQUIRE(run(interp, "print(length(\"\xF0\x9F\x8E\xAE\"));") == "1");
}

TEST_CASE("unicode: ASCII char literal", "[unicode.char.ascii]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var c = 'A'; print(c);") == "A");
}

TEST_CASE("unicode: Cyrillic char literal decoded correctly", "[unicode.char.cyrillic]") {
    auto interp = makeInterp();
    // 'А' = U+0410, UTF-8: 0xD0 0x90
    REQUIRE(run(interp, "var c = '\xD0\x90'; print(c);") == "\xD0\x90");
}

TEST_CASE("unicode: escape \\n produces 3-char string", "[unicode.escape.newline]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "print(length(\"a\\nb\"));") == "3");
}

TEST_CASE("unicode: escape \\u{41} produces A", "[unicode.escape.unicode]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "print(\"\\u{41}\");") == "A");
}

TEST_CASE("unicode: string index returns char", "[unicode.index.char]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var s = \"hello\"; var c = s[1]; print(c);") == "e");
}

TEST_CASE("unicode: mixed ASCII and emoji string print", "[unicode.print.mixed]") {
    auto interp = makeInterp();
    // "Hi 👋" — emoji U+1F44B = 0xF0 0x9F 0x91 0x8B
    std::string expected = "Hi \xF0\x9F\x91\x8B";
    REQUIRE(run(interp, "var s = \"Hi \xF0\x9F\x91\x8B\"; print(s);") == expected);
}

TEST_CASE("unicode: Char type annotation works", "[unicode.type.char]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var c: Char = 'X'; print(c);") == "X");
}

// ---------------------------------------------------------------------------
// Slicing — code point semantics
// ---------------------------------------------------------------------------

TEST_CASE("unicode: slice Cyrillic exclusive range", "[unicode.slice.cyrillic]") {
    auto interp = makeInterp();
    // "Привет"[0..2] exclusive → code points 0,1 = "Пр" (2 chars)
    interp.evaluate("var s = \"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\";");
    REQUIRE(run(interp, "print(s[0..2]);") == "\xD0\x9F\xD1\x80");
}

TEST_CASE("unicode: slice Cyrillic inclusive range", "[unicode.slice.cyrillic.incl]") {
    auto interp = makeInterp();
    // "Привет"[0..=2] inclusive → code points 0,1,2 = "При" (3 chars)
    interp.evaluate("var s = \"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\";");
    REQUIRE(run(interp, "print(s[0..=2]);") == "\xD0\x9F\xD1\x80\xD0\xB8");
}

TEST_CASE("unicode: slice emoji at code point index", "[unicode.slice.emoji]") {
    auto interp = makeInterp();
    // "Hi 🎮" — emoji is code point index 3
    // s[3..=3] should return just the emoji
    std::string emoji = "\xF0\x9F\x8E\xAE";
    interp.evaluate("var s = \"Hi " + emoji + "\";");
    REQUIRE(run(interp, "print(s[3..=3]);") == emoji);
}

TEST_CASE("unicode: slice length consistency with length()", "[unicode.slice.length]") {
    auto interp = makeInterp();
    // slice length should match code point count, not byte count
    interp.evaluate("var s = \"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\";"); // "Привет" 6 cp
    interp.evaluate("var sl = s[1..=3];"); // "рив" — inclusive, code points 1,2,3 = 3 elements
    REQUIRE(run(interp, "print(length(sl));") == "3");
}

// ---------------------------------------------------------------------------
// range(str, start, count) — code point semantics
// ---------------------------------------------------------------------------

TEST_CASE("unicode: range(str) ASCII baseline", "[unicode.range.ascii]") {
    auto interp = makeInterp();
    interp.evaluate("var s = \"hello\";");
    // range(str, start, count)
    REQUIRE(run(interp, "print(range(s, 0, 2));") == "he");
}

TEST_CASE("unicode: range(str) Cyrillic code points", "[unicode.range.cyrillic]") {
    auto interp = makeInterp();
    // "Привет", start=0, count=2 → "Пр" (not 2 bytes which would be partial)
    interp.evaluate("var s = \"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\";");
    REQUIRE(run(interp, "print(length(range(s, 0, 3)));") == "3");
}

// ---------------------------------------------------------------------------
// reverse — code point semantics
// ---------------------------------------------------------------------------

TEST_CASE("unicode: reverse ASCII string", "[unicode.reverse.ascii]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "print(reverse(\"abc\"));") == "cba");
}

TEST_CASE("unicode: reverse string with emoji", "[unicode.reverse.emoji]") {
    auto interp = makeInterp();
    // "🎮ab" reversed → "ba🎮"
    std::string emoji = "\xF0\x9F\x8E\xAE";
    interp.evaluate("var s = \"" + emoji + "ab\";");
    REQUIRE(run(interp, "print(reverse(s));") == "ba" + emoji);
}

// ---------------------------------------------------------------------------
// split(s) without delimiter — splits by code points
// ---------------------------------------------------------------------------

TEST_CASE("unicode: split ASCII string by code points", "[unicode.split.ascii]") {
    auto interp = makeInterp();
    interp.evaluate("var parts = split(\"ab\");");
    REQUIRE(run(interp, "print(length(parts));") == "2");
}

TEST_CASE("unicode: split Cyrillic string by code points", "[unicode.split.cyrillic]") {
    auto interp = makeInterp();
    // "Пр" = 2 Cyrillic chars (4 bytes) — should split into 2 elements, not 4
    interp.evaluate("var parts = split(\"\xD0\x9F\xD1\x80\");");
    REQUIRE(run(interp, "print(length(parts));") == "2");
}

// ---------------------------------------------------------------------------
// find — substring search returning code point index
// ---------------------------------------------------------------------------

TEST_CASE("unicode: find substring in ASCII string", "[unicode.find.ascii]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "print(find(\"hello\", \"ll\"));") == "2");
}

TEST_CASE("unicode: find returns null for missing substring", "[unicode.find.miss]") {
    auto interp = makeInterp();
    // find returns null value when not found — print produces empty or null output
    interp.evaluate("var r = find(\"hello\", \"xyz\");");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("unicode: find in Cyrillic string returns code point index", "[unicode.find.cyrillic]") {
    auto interp = makeInterp();
    // "Привет" — find "вет" starts at code point 3
    interp.evaluate("var s = \"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\";");
    // "вет" = \xD0\xB2\xD0\xB5\xD1\x82 starts at cp index 3
    REQUIRE(run(interp, "print(find(s, \"\xD0\xB2\xD0\xB5\xD1\x82\"));") == "3");
}

// ---------------------------------------------------------------------------
// Char + String concatenation — correct UTF-8 for non-ASCII
// ---------------------------------------------------------------------------

TEST_CASE("unicode: Cyrillic char + string concatenation", "[unicode.char.concat]") {
    auto interp = makeInterp();
    // 'А' (U+0410) + "BC" should produce valid UTF-8 "АBC"
    interp.evaluate("var c = '\xD0\x90';"); // А
    std::string expected = "\xD0\x90" "BC";
    REQUIRE(run(interp, "print(c + \"BC\");") == expected);
}

TEST_CASE("unicode: Char upconvert to String preserves non-ASCII", "[unicode.char.upconvert]") {
    auto interp = makeInterp();
    // Converting non-ASCII Char to String should not produce '?'
    interp.evaluate("var c = '\xD0\x90';"); // А
    // concat with empty string forces upconvert
    REQUIRE(run(interp, "print(c + \"\");") == "\xD0\x90");
}
