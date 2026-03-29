# CMakeList.txt: ShitEngine 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.20)

# UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

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
add_subdirectory("vendor/nlohmann_json")

# GLM
# 强制 GLM 使用弧度制
add_definitions(-DGLM_FORCE_RADIANS)
# 强制使用 0 到 1 的深度范围
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

# 将源代码添加到此项目的可执行文件。
add_library(ShitEngine SHARED
    "src/ShitEngine/Core/Game.cpp"
    "include/ShitEngine/Core/Game.h"
  "include/ShitEngine/Core/Core.h" "include/ShitEngine.h" "include/ShitEngine/Core/Log.h" "src/ShitEngine/Core/Log.cpp" "include/ShitEngine/Core/pch.h" "include/ShitEngine/Resource/ResourceManager.h" "src/ShitEngine/Resource/ResourceManager.cpp" "include/ShitEngine/Resource/TextureManager.h" "include/ShitEngine/Resource/AudioManager.h" "include/ShitEngine/Resource/FontManager.h" "src/ShitEngine/Resource/TextureManager.cpp" "include/ShitEngine/Math.h" "src/ShitEngine/Core/Time.cpp" "include/ShitEngine/Core/Window.h" "src/ShitEngine/Core/Window.cpp" "include/ShitEngine/Core/Input.h" "src/ShitEngine/Core/Input.cpp" "include/ShitEngine/Core/Config.h" "src/ShitEngine/Core/Config.cpp" "include/ShitEngine/GameObject/GameObject.h" "include/ShitEngine/Component/Component.h" "include/ShitEngine/Component/Behavior.h"  "src/ShitEngine/Component/Behavior.cpp" "src/ShitEngine/GameObject/GameObject.cpp" "include/ShitEngine/Component/TransformComponent.h" "src/ShitEngine/Component/TransformComponent.cpp" "include/ShitEngine/Render/RenderSystem.h" "src/ShitEngine/Render/RenderSystem.cpp" "include/ShitEngine/Component/SpriteRenderer.h" "src/ShitEngine/Component/SpriteRenderer.cpp" "include/ShitEngine/Render/Sprite.h" "src/ShitEngine/Render/Sprite.cpp" "include/ShitEngine/Scene/Scene.h" "src/ShitEngine/Scene/Scene.cpp" "include/ShitEngine/Scene/SceneManager.h" "src/ShitEngine/Scene/SceneManager.cpp" "include/ShitEngine/Component/RendererComponent.h" "src/ShitEngine/Component/RendererComponent.cpp" "src/ShitEngine/Component/Component.cpp" "include/ShitEngine/Component/CameraComponent.h" "src/ShitEngine/Component/CameraComponent.cpp"
)

target_precompile_headers(ShitEngine PUBLIC "include/ShitEngine/Core/pch.h")

target_compile_definitions(ShitEngine PRIVATE SHIT_EXPORTS)

target_include_directories(ShitEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ShitEngine PUBLIC SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network spdlog::spdlog glm::glm nlohmann_json::nlohmann_json)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ShitEngine PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
