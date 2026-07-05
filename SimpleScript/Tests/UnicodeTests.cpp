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
