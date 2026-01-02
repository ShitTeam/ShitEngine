# CMakeList.txt: ShitEngine 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ShitEngine")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory("vendor/SFML")
add_subdirectory("vendor/spdlog")

# 将源代码添加到此项目的可执行文件。
add_library(ShitEngine SHARED
    "src/ShitEngine/Game.cpp"
    "include/ShitEngine/Game.h"
  "include/ShitEngine/Config.h" "include/ShitEngine.h" "include/ShitEngine/Log.h" "src/ShitEngine/Log.cpp" "include/ShitEngine/pch.h")

target_precompile_headers(ShitEngine PUBLIC "include/ShitEngine/pch.h")

target_compile_definitions(ShitEngine PRIVATE SHIT_EXPORTS)

target_include_directories(ShitEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ShitEngine PUBLIC SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network spdlog::spdlog)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ShitEngine PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
