import os
with open(r'F:\projects\SimpleScript\SimpleScript\Tests\SimpleScriptTests.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

new_test = '''
TEST_CASE("EXPRESSION - While as Expression", "[EXPRESSION]") {
    Interpreter interpreter;
    
    // while loop collecting values
    std::string testScript1 = 
        "var i = 0;\\n"
        "var arr = while (i < 3) {\\n"
        "  i = i + 1;\\n"
        "  i * 10;\\n"
        "};\\n"
        "assert(len(arr) == 3);\\n"
        "assert(arr[0] == 10);\\n"
        "assert(arr[1] == 20);\\n"
        "assert(arr[2] == 30);\\n";

    REQUIRE_NOTHROW(interpreter.run(testScript1));

    // for loop (standard) collecting values
    std::string testScript2 = 
        "var arr = for (var i = 1; i <= 3; i = i + 1) {\\n"
        "  i * 2;\\n"
        "};\\n"
        "assert(len(arr) == 3);\\n"
        "assert(arr[0] == 2);\\n"
        "assert(arr[1] == 4);\\n"
        "assert(arr[2] == 6);\\n";

    REQUIRE_NOTHROW(interpreter.run(testScript2));
}
'''
if 'EXPRESSION - While as Expression' not in content:
    content += new_test
    with open(r'F:\projects\SimpleScript\SimpleScript\Tests\SimpleScriptTests.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
