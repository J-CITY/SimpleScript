cmake_minimum_required(VERSION 3.15)
project(IkigaiScript VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# =============================================================================
# Options & dependency paths
# =============================================================================
#
# Default build = core + headless VisualCore + tests (no window / no ImGui).
# Enable the GUI app with:
#   cmake .. -DIKIGAI_BUILD_VISUAL_EDITOR=ON
#
# External deps are NOT git submodules. Point CMake at local trees, e.g.:
#   -DIKIGAI_LIBS_ROOT=C:/libs
#   -DIKIGAI_IMGUI_DIR=C:/libs/imgui
# or set the finer-grained path variables below.

option(IKIGAI_BUILD_VISUAL_EDITOR "Build IkigaiScriptApp (GLFW/ImGui visual editor window)" OFF)

set(IKIGAI_LIBS_ROOT "" CACHE PATH
    "Optional root with glfw / glew (e.g. C:/libs). Used to derive defaults for finer paths.")

set(IKIGAI_IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui" CACHE PATH
    "Dear ImGui tree (imgui.h, backends, misc/cpp, imgui-node-editor/, TextEditor.*)")

set(IKIGAI_GLFW_INCLUDE_DIR "" CACHE PATH "GLFW include dir (contains GLFW/glfw3.h)")
set(IKIGAI_GLFW_LIB_DIR     "" CACHE PATH "GLFW lib dir (glfw3.lib / glfw3dll.lib)")
set(IKIGAI_GLEW_INCLUDE_DIR "" CACHE PATH "GLEW include dir (contains GL/glew.h)")
set(IKIGAI_GLEW_LIB_DIR     "" CACHE PATH "GLEW lib dir (glew32s.lib / glew32.lib)")
set(IKIGAI_GLEW_BIN_DIR     "" CACHE PATH "GLEW bin dir (glew32.dll), optional")

# =============================================================================
# Core interpreter
# =============================================================================
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

# =============================================================================
# Visual editor core (headless graph / codegen — no ImGui)
# =============================================================================
file(GLOB VISUAL_CORE_SOURCES CONFIGURE_DEPENDS
    "VisualEditor/Core/*.cpp"
    "VisualEditor/Mapping/*.cpp"
    "VisualEditor/Visitors/*.cpp"
)

add_library(IkigaiVisualCore ${VISUAL_CORE_SOURCES})
target_include_directories(IkigaiVisualCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(IkigaiVisualCore PUBLIC IkigaiScriptCore)

# =============================================================================
# Tests (always; do not require GUI deps)
# =============================================================================
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
    "Tests/*Tests.cpp"
    "Tests/TestMain.cpp"
)
add_executable(IkigaiScriptTests ${TEST_SOURCES})
target_link_libraries(IkigaiScriptTests PRIVATE IkigaiScriptCore IkigaiVisualCore)

enable_testing()
add_test(NAME RunIkigaiScriptTests COMMAND IkigaiScriptTests)

# =============================================================================
# Visual editor window (optional)
# =============================================================================
if(NOT IKIGAI_BUILD_VISUAL_EDITOR)
    message(STATUS "IkigaiScriptApp skipped (IKIGAI_BUILD_VISUAL_EDITOR=OFF)")
    return()
endif()

# --- Derive path defaults from IKIGAI_LIBS_ROOT when finer paths are empty ---
function(ikigai_default_path out_var value)
    if("${${out_var}}" STREQUAL "")
        set(${out_var} "${value}" PARENT_SCOPE)
    endif()
endfunction()

if(NOT IKIGAI_LIBS_ROOT STREQUAL "")
    ikigai_default_path(IKIGAI_GLFW_INCLUDE_DIR "${IKIGAI_LIBS_ROOT}/glfw-3.4/include")
    ikigai_default_path(IKIGAI_GLFW_LIB_DIR     "${IKIGAI_LIBS_ROOT}/glfw-3.4/build")
    if(NOT EXISTS "${IKIGAI_GLFW_INCLUDE_DIR}/GLFW/glfw3.h")
        set(IKIGAI_GLFW_INCLUDE_DIR "${IKIGAI_LIBS_ROOT}/glfw/include")
    endif()
    if(NOT EXISTS "${IKIGAI_GLFW_LIB_DIR}/glfw3.lib"
       AND NOT EXISTS "${IKIGAI_GLFW_LIB_DIR}/glfw3dll.lib")
        set(IKIGAI_GLFW_LIB_DIR "${IKIGAI_LIBS_ROOT}/glfw/build")
    endif()

    ikigai_default_path(IKIGAI_GLEW_INCLUDE_DIR "${IKIGAI_LIBS_ROOT}/glew/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        ikigai_default_path(IKIGAI_GLEW_LIB_DIR "${IKIGAI_LIBS_ROOT}/glew/lib/Release/x64")
        ikigai_default_path(IKIGAI_GLEW_BIN_DIR "${IKIGAI_LIBS_ROOT}/glew/bin/Release/x64")
    else()
        ikigai_default_path(IKIGAI_GLEW_LIB_DIR "${IKIGAI_LIBS_ROOT}/glew/lib/Release/Win32")
        ikigai_default_path(IKIGAI_GLEW_BIN_DIR "${IKIGAI_LIBS_ROOT}/glew/bin/Release/Win32")
    endif()
endif()

# --- Validate required paths ---
if(NOT EXISTS "${IKIGAI_IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR
        "Dear ImGui not found at IKIGAI_IMGUI_DIR=${IKIGAI_IMGUI_DIR}\n"
        "Place imgui there (no submodule) or pass -DIKIGAI_IMGUI_DIR=...")
endif()
if(NOT EXISTS "${IKIGAI_GLFW_INCLUDE_DIR}/GLFW/glfw3.h")
    message(FATAL_ERROR
        "GLFW headers not found at IKIGAI_GLFW_INCLUDE_DIR=${IKIGAI_GLFW_INCLUDE_DIR}\n"
        "Pass -DIKIGAI_LIBS_ROOT=... or -DIKIGAI_GLFW_INCLUDE_DIR=...")
endif()
if(NOT EXISTS "${IKIGAI_GLEW_INCLUDE_DIR}/GL/glew.h")
    message(FATAL_ERROR
        "GLEW headers not found at IKIGAI_GLEW_INCLUDE_DIR=${IKIGAI_GLEW_INCLUDE_DIR}\n"
        "Pass -DIKIGAI_LIBS_ROOT=... or -DIKIGAI_GLEW_INCLUDE_DIR=...")
endif()

find_package(OpenGL REQUIRED)

file(GLOB IMGUI_CORE_SOURCES CONFIGURE_DEPENDS
    "${IKIGAI_IMGUI_DIR}/imgui.cpp"
    "${IKIGAI_IMGUI_DIR}/imgui_draw.cpp"
    "${IKIGAI_IMGUI_DIR}/imgui_tables.cpp"
    "${IKIGAI_IMGUI_DIR}/imgui_widgets.cpp"
    "${IKIGAI_IMGUI_DIR}/TextEditor.cpp"
)
# Official imgui keeps backends in backends/; some local trees flatten them.
set(_imgui_backend_candidates
    "${IKIGAI_IMGUI_DIR}/imgui_impl_glfw.cpp"
    "${IKIGAI_IMGUI_DIR}/imgui_impl_opengl3.cpp"
    "${IKIGAI_IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IKIGAI_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
set(IMGUI_BACKEND_SOURCES "")
foreach(_f IN LISTS _imgui_backend_candidates)
    if(EXISTS "${_f}")
        list(APPEND IMGUI_BACKEND_SOURCES "${_f}")
    endif()
endforeach()
if(IMGUI_BACKEND_SOURCES STREQUAL "")
    message(FATAL_ERROR
        "ImGui GLFW/OpenGL3 backends not found under ${IKIGAI_IMGUI_DIR}\n"
        "Expected imgui_impl_glfw.cpp + imgui_impl_opengl3.cpp "
        "(root or backends/)")
endif()

file(GLOB IMGUI_MISC_SOURCES CONFIGURE_DEPENDS
    "${IKIGAI_IMGUI_DIR}/misc/cpp/*.cpp"
)
file(GLOB IMGUI_NODE_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${IKIGAI_IMGUI_DIR}/imgui-node-editor/*.cpp"
)
if(IMGUI_NODE_EDITOR_SOURCES STREQUAL "")
    message(FATAL_ERROR
        "imgui-node-editor sources not found at "
        "${IKIGAI_IMGUI_DIR}/imgui-node-editor/")
endif()

file(GLOB APP_UI_SOURCES CONFIGURE_DEPENDS
    "SimpleScript.cpp"
    "VisualEditor/UI/*.cpp"
)

add_executable(IkigaiScriptApp
    ${APP_UI_SOURCES}
    ${IMGUI_CORE_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
    ${IMGUI_MISC_SOURCES}
    ${IMGUI_NODE_EDITOR_SOURCES}
)

target_compile_definitions(IkigaiScriptApp PRIVATE IKIGAI_VISUAL_EDITOR_UI)

target_include_directories(IkigaiScriptApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IKIGAI_IMGUI_DIR}
    ${IKIGAI_IMGUI_DIR}/backends
    ${IKIGAI_IMGUI_DIR}/misc/cpp
    ${IKIGAI_IMGUI_DIR}/imgui-node-editor
    ${IKIGAI_GLFW_INCLUDE_DIR}
    ${IKIGAI_GLEW_INCLUDE_DIR}
)

target_link_libraries(IkigaiScriptApp PRIVATE
    IkigaiVisualCore
    OpenGL::GL
)

# --- GLFW ---
if(EXISTS "${IKIGAI_GLFW_LIB_DIR}/glfw3.lib")
    target_link_libraries(IkigaiScriptApp PRIVATE "${IKIGAI_GLFW_LIB_DIR}/glfw3.lib")
elseif(EXISTS "${IKIGAI_GLFW_LIB_DIR}/glfw3dll.lib")
    target_link_libraries(IkigaiScriptApp PRIVATE "${IKIGAI_GLFW_LIB_DIR}/glfw3dll.lib")
else()
    message(FATAL_ERROR
        "GLFW library not found in IKIGAI_GLFW_LIB_DIR=${IKIGAI_GLFW_LIB_DIR}\n"
        "Expected glfw3.lib or glfw3dll.lib")
endif()

# --- GLEW ---
if(EXISTS "${IKIGAI_GLEW_LIB_DIR}/glew32s.lib")
    target_compile_definitions(IkigaiScriptApp PRIVATE GLEW_STATIC)
    target_link_libraries(IkigaiScriptApp PRIVATE "${IKIGAI_GLEW_LIB_DIR}/glew32s.lib")
elseif(EXISTS "${IKIGAI_GLEW_LIB_DIR}/glew32.lib")
    target_link_libraries(IkigaiScriptApp PRIVATE "${IKIGAI_GLEW_LIB_DIR}/glew32.lib")
else()
    message(FATAL_ERROR
        "GLEW library not found in IKIGAI_GLEW_LIB_DIR=${IKIGAI_GLEW_LIB_DIR}\n"
        "Expected glew32s.lib or glew32.lib")
endif()

if(WIN32)
    target_link_libraries(IkigaiScriptApp PRIVATE opengl32 gdi32 user32 shell32)
endif()

# --- Copy runtime DLLs ---
if(EXISTS "${IKIGAI_GLFW_LIB_DIR}/glfw3.dll")
    add_custom_command(TARGET IkigaiScriptApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IKIGAI_GLFW_LIB_DIR}/glfw3.dll"
            $<TARGET_FILE_DIR:IkigaiScriptApp>)
endif()
if(NOT IKIGAI_GLEW_BIN_DIR STREQUAL "" AND EXISTS "${IKIGAI_GLEW_BIN_DIR}/glew32.dll")
    add_custom_command(TARGET IkigaiScriptApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IKIGAI_GLEW_BIN_DIR}/glew32.dll"
            $<TARGET_FILE_DIR:IkigaiScriptApp>)
endif()

message(STATUS "IkigaiScriptApp enabled")
message(STATUS "  IMGUI  : ${IKIGAI_IMGUI_DIR}")
message(STATUS "  GLFW   : ${IKIGAI_GLFW_INCLUDE_DIR} | ${IKIGAI_GLFW_LIB_DIR}")
message(STATUS "  GLEW   : ${IKIGAI_GLEW_INCLUDE_DIR} | ${IKIGAI_GLEW_LIB_DIR}")
