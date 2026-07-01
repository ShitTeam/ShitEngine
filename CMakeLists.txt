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

option(BUILD_SHARED_LIBS "依赖库默认编译为动态库" OFF)

include(cmake/DependencyManager.cmake)

# 配置和构建依赖项
setup_project_dependencies()

# GLM
add_definitions(-DGLM_FORCE_RADIANS)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

# 将源代码添加到此项目的可执行文件。
add_library(ShitEngine SHARED
    # Core
    "src/ShitEngine/Core/Game.cpp"
    "src/ShitEngine/Core/Log.cpp"
    "src/ShitEngine/Core/Time.cpp"
    "src/ShitEngine/Core/Window.cpp"
    "src/ShitEngine/Core/Config.cpp"
    "include/ShitEngine/Core/Game.h"
    "include/ShitEngine/Core/Core.h"
    "include/ShitEngine/Core/Log.h"
    "include/ShitEngine/Core/pch.h"
    "include/ShitEngine/Core/Time.h"
    "include/ShitEngine/Core/Window.h"
    "include/ShitEngine/Core/Config.h"

    # Math
    "include/ShitEngine/Math.h"

    # Input
    "src/ShitEngine/Input/Input.cpp"
    "include/ShitEngine/Input/Input.h"
    "include/ShitEngine/Input/KeyCode.h"

    # Scene
    "src/ShitEngine/Scene/Scene.cpp"
    "src/ShitEngine/Scene/SceneManager.cpp"
    "include/ShitEngine/Scene/Scene.h"
    "include/ShitEngine/Scene/SceneManager.h"

    # GameObject
    "src/ShitEngine/GameObject/GameObject.cpp"
    "include/ShitEngine/GameObject/GameObject.h"

    # Component
    "src/ShitEngine/Component/Component.cpp"
    "src/ShitEngine/Component/Behavior.cpp"
    "src/ShitEngine/Component/TransformComponent.cpp"
    "src/ShitEngine/Component/RendererComponent.cpp"
    "src/ShitEngine/Component/SpriteRenderer.cpp"
    "src/ShitEngine/Component/CameraComponent.cpp"
    "src/ShitEngine/Component/AnimationComponent.cpp"
    "include/ShitEngine/Component/Component.h"
    "include/ShitEngine/Component/Behavior.h"
    "include/ShitEngine/Component/TransformComponent.h"
    "include/ShitEngine/Component/RendererComponent.h"
    "include/ShitEngine/Component/SpriteRenderer.h"
    "include/ShitEngine/Component/CameraComponent.h"
    "include/ShitEngine/Component/AnimationComponent.h"

    # Render
    "src/ShitEngine/Render/RenderSystem.cpp"
    "src/ShitEngine/Render/Renderer.cpp"
    "src/ShitEngine/Render/Sprite.cpp"
    "src/ShitEngine/Render/Animation.cpp"
    "include/ShitEngine/Render/RenderSystem.h"
    "include/ShitEngine/Render/Renderer.h"
    "include/ShitEngine/Render/Sprite.h"
    "include/ShitEngine/Render/Animation.h"

    # Resource
    "src/ShitEngine/Resource/ResourceManager.cpp"
    "src/ShitEngine/Resource/TextureManager.cpp"
    "include/ShitEngine/Resource/ResourceManager.h"
    "include/ShitEngine/Resource/TextureManager.h"
    "include/ShitEngine/Resource/AudioManager.h"
    "include/ShitEngine/Resource/FontManager.h"

    # System
    "src/ShitEngine/System/System.cpp"
    "src/ShitEngine/System/BehaviorSystem.cpp"
    "include/ShitEngine/System/System.h"
    "include/ShitEngine/System/BehaviorSystem.h"

    # Umbrella
    "include/ShitEngine.h"
)

target_precompile_headers(ShitEngine PUBLIC "include/ShitEngine/Core/pch.h")
target_compile_definitions(ShitEngine PRIVATE SHIT_EXPORTS)
target_include_directories(ShitEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ShitEngine PUBLIC
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    spdlog::spdlog
    glm::glm
    nlohmann_json::nlohmann_json)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ShitEngine PROPERTY CXX_STANDARD 20)
endif()

# ── Install ──────────────────────────────────────────────
include(GNUInstallDirs)

# 安装头文件（保留目录结构）
install(DIRECTORY include/ShitEngine
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN "pch.h" EXCLUDE)

# 安装 umbrella header
install(FILES include/ShitEngine.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 安装库文件
install(TARGETS ShitEngine
        EXPORT ShitEngineTargets
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR})

# 安装 CMake config，方便他人 find_package
install(EXPORT ShitEngineTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine)

# 生成并安装 ShitEngineConfig.cmake
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ShitEngineConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ShitEngineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ShitEngineConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine)

# 生成版本文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ShitEngineConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ShitEngineConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine)
