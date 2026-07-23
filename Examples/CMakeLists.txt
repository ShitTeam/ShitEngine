# CMakeList.txt: Example 的 CMake 项目

cmake_minimum_required (VERSION 3.20)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

project ("Example")

add_executable (Example
    "src/main.cpp"
    "src/Player.cpp"
    "src/Player.h"
    "src/UITestScene.cpp"
    "src/UITestScene.h"
    "src/InputMappingTestScene.cpp"
    "src/InputMappingTestScene.h"
)

# ── 方式 1：fetch_content 拉取源码（外部用户推荐）─────────────────
# include(FetchContent)
# FetchContent_Declare(
#     ShitEngine
#     GIT_REPOSITORY https://github.com/ShitTeam/ShitEngine.git
#     GIT_TAG main
#     GIT_SHALLOW TRUE
# )
# FetchContent_MakeAvailable(ShitEngine)
#
# ── 方式 2：add_subdirectory 源码编译（开发时用）─────────────────
add_subdirectory("../Engine" "${CMAKE_BINARY_DIR}/Engine_Build")

# ── 方式 3：find_package 预编译库 ──────────────────
# find_package(ShitEngine REQUIRED
#     PATHS /path/to/ShitEngine/lib/cmake
#     NO_DEFAULT_PATH)
# target_link_libraries(Example PRIVATE ShitEngine::ShitEngine)

target_link_libraries(Example PRIVATE ShitEngine)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Example PROPERTY CXX_STANDARD 20)
endif()
