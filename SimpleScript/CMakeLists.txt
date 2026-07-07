cmake_minimum_required(VERSION 3.15)
project(IkigaiScript VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Library (Core Interpreter) ---
file(GLOB CORE_SOURCES "Library/*.cpp")
file(GLOB CONCURRENCY_SOURCES "Library/concurrency/*.cpp")
add_library(IkigaiScriptCore ${CORE_SOURCES} ${CONCURRENCY_SOURCES})
target_include_directories(IkigaiScriptCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# --- Tests ---
file(GLOB TEST_SOURCES
    "Tests/*Tests.cpp"
    "Tests/TestMain.cpp"
)
add_executable(SimpleScriptTests ${TEST_SOURCES})
target_link_libraries(SimpleScriptTests PRIVATE IkigaiScriptCore)

# --- Main Application (Visual Editor & Interpreter) ---
# Опционально: собирать GUI только если найдены зависимости
find_package(glfw3 QUIET)
find_package(OpenGL QUIET)

# Все исходники приложения (включая ImGui)
file(GLOB APP_SOURCES 
    "SimpleScript.cpp"
    "VisualEditor/*.cpp"
    "imgui/*.cpp"
    "imgui/misc/cpp/*.cpp"
    "imgui/imgui-node-editor/*.cpp"
)

add_executable(SimpleScriptApp ${APP_SOURCES})
target_link_libraries(SimpleScriptApp PRIVATE IkigaiScriptCore)

if (glfw3_FOUND AND OPENGL_FOUND)
    target_link_libraries(SimpleScriptApp PRIVATE glfw OpenGL::GL)
else()
    message(WARNING "GLFW or OpenGL not found! GUI compilation might fail. Make sure to provide glfw3.")
    if(WIN32)
        # На Windows можно попробовать слинковать opengl32 напрямую
        target_link_libraries(SimpleScriptApp PRIVATE opengl32)
    endif()
endif()

# Опции для тестов
enable_testing()
add_test(NAME RunSimpleScriptTests COMMAND SimpleScriptTests)
