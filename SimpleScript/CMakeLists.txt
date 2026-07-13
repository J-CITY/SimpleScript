cmake_minimum_required(VERSION 3.15)
project(IkigaiScript VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Core interpreter library ---
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "Library/*.cpp")
file(GLOB CONCURRENCY_SOURCES CONFIGURE_DEPENDS "Library/concurrency/*.cpp")
file(GLOB BYTECODE_SOURCES CONFIGURE_DEPENDS "Library/bytecode/*.cpp")
file(GLOB VISITOR_SOURCES CONFIGURE_DEPENDS "Library/visitor/*.cpp")

add_library(IkigaiScriptCore
    ${CORE_SOURCES}
    ${CONCURRENCY_SOURCES}
    ${BYTECODE_SOURCES}
    ${VISITOR_SOURCES}
)
target_include_directories(IkigaiScriptCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Visual-editor core library (headless, no ImGui) ---
file(GLOB VISUAL_CORE_SOURCES CONFIGURE_DEPENDS
    "VisualEditor/Core/*.cpp"
    "VisualEditor/Mapping/*.cpp"
    "VisualEditor/Visitors/*.cpp"
)

add_library(IkigaiVisualCore ${VISUAL_CORE_SOURCES})
target_include_directories(IkigaiVisualCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(IkigaiVisualCore PUBLIC IkigaiScriptCore)

# --- Tests ---
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    "Tests/*Tests.cpp"
    "Tests/TestMain.cpp"
)
add_executable(IkigaiScriptTests ${TEST_SOURCES})
target_link_libraries(IkigaiScriptTests PRIVATE IkigaiScriptCore IkigaiVisualCore)

# --- Main Application (Visual Editor + Interpreter, requires GLFW/OpenGL) ---
find_package(glfw3 QUIET)
find_package(OpenGL QUIET)

file(GLOB APP_SOURCES CONFIGURE_DEPENDS
    "SimpleScript.cpp"
    "VisualEditor/UI/*.cpp"
    "imgui/*.cpp"
    "imgui/misc/cpp/*.cpp"
    "imgui/imgui-node-editor/*.cpp"
)

add_executable(IkigaiScriptApp ${APP_SOURCES})
target_compile_definitions(IkigaiScriptApp PRIVATE IKIGAI_VISUAL_EDITOR_UI)
target_link_libraries(IkigaiScriptApp PRIVATE IkigaiVisualCore)

if (glfw3_FOUND AND OPENGL_FOUND)
    target_link_libraries(IkigaiScriptApp PRIVATE glfw OpenGL::GL)
else()
    message(WARNING "GLFW or OpenGL not found — GUI will not link without them.")
    if(WIN32)
        target_link_libraries(IkigaiScriptApp PRIVATE opengl32)
    endif()
endif()

# --- Tests runner ---
enable_testing()
add_test(NAME RunIkigaiScriptTests COMMAND IkigaiScriptTests)
