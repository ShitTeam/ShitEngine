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
add_subdirectory("vendor/glm")

# GLM
# 强制 GLM 使用弧度制
add_definitions(-DGLM_FORCE_RADIANS)
# 强制使用 0 到 1 的深度范围
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

# 将源代码添加到此项目的可执行文件。
add_library(ShitEngine SHARED
    "src/ShitEngine/Core/Game.cpp"
    "include/ShitEngine/Core/Game.h"
  "include/ShitEngine/Core/Config.h" "include/ShitEngine.h" "include/ShitEngine/Core/Log.h" "src/ShitEngine/Core/Log.cpp" "include/ShitEngine/Core/pch.h" "include/ShitEngine/Resource/ResourceManager.h" "src/ShitEngine/Resource/ResourceManager.cpp" "include/ShitEngine/Resource/TextureManager.h" "include/ShitEngine/Resource/AudioManager.h" "include/ShitEngine/Resource/FontManager.h" "src/ShitEngine/Resource/TextureManager.cpp" "include/ShitEngine/Math.h" "src/ShitEngine/Core/Time.cpp")

target_precompile_headers(ShitEngine PUBLIC "include/ShitEngine/Core/pch.h")

target_compile_definitions(ShitEngine PRIVATE SHIT_EXPORTS)

target_include_directories(ShitEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ShitEngine PUBLIC SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network spdlog::spdlog glm::glm)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ShitEngine PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
