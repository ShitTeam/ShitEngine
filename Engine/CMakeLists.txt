# Engine 库目标
# 根 CMakeLists.txt 通过 add_subdirectory(Engine) 引入本文件

# ── 依赖 ──
option(BUILD_SHARED_LIBS "依赖库默认编译为动态库" ON)
include(cmake/DependencyManager.cmake)
setup_project_dependencies()

# Doxygen API 文档 (cmake --build . --target docs)
include(cmake/Docs.cmake)

# GLM
add_definitions(-DGLM_FORCE_RADIANS)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

# ── 库目标 ──────────────────────────────────────────────
add_library(ShitEngine SHARED
    # Core
    "src/ShitEngine/Core/Game.cpp"
    "src/ShitEngine/Core/Log.cpp"
    "src/ShitEngine/Core/Time.cpp"
    "src/ShitEngine/Core/Window.cpp"
    "src/ShitEngine/Core/Config.cpp"
    "src/ShitEngine/Core/TextInputGate.cpp"
    "src/ShitEngine/Core/EngineContext.cpp"
    "src/ShitEngine/Event/EventBus.cpp"
    # Audio
    "src/ShitEngine/Audio/AudioTrack.cpp"
    "src/ShitEngine/Audio/AudioPlayer.cpp"
    # Input
    "src/ShitEngine/Input/Input.cpp"
    # Scene
    "src/ShitEngine/Scene/Scene.cpp"
    "src/ShitEngine/Scene/SceneManager.cpp"
    # GameObject
    "src/ShitEngine/GameObject/GameObject.cpp"
    # Component
    "src/ShitEngine/Component/Component.cpp"
    "src/ShitEngine/Component/Behavior.cpp"
    "src/ShitEngine/Component/TransformComponent.cpp"
    "src/ShitEngine/Component/RendererComponent.cpp"
    "src/ShitEngine/Component/SpriteRenderer.cpp"
    "src/ShitEngine/Component/CameraComponent.cpp"
    "src/ShitEngine/Component/AnimationComponent.cpp"
    # Render
    "src/ShitEngine/Render/RenderSystem.cpp"
    "src/ShitEngine/Render/Renderer.cpp"
    "src/ShitEngine/Render/Sprite.cpp"
    "src/ShitEngine/Render/Animation.cpp"
    "src/ShitEngine/Render/SpriteSheet.cpp"
    # Resource
    "src/ShitEngine/Resource/ResourceManager.cpp"
    "src/ShitEngine/Resource/TextureManager.cpp"
    "src/ShitEngine/Resource/AudioManager.cpp"
    "src/ShitEngine/Resource/FontManager.cpp"
    # System
    "src/ShitEngine/System/System.cpp"
    "src/ShitEngine/System/BehaviorSystem.cpp"
    # Reflection
    "src/ShitEngine/Reflection/TypeRegistry.cpp"
    # UI
    "src/ShitEngine/UI/UITransform.cpp"
    "src/ShitEngine/UI/UIRendererComponent.cpp"
    "src/ShitEngine/UI/UICanvas.cpp"
    "src/ShitEngine/UI/UIImage.cpp"
    "src/ShitEngine/UI/UIText.cpp"
    "src/ShitEngine/UI/UIButton.cpp"
    "src/ShitEngine/UI/UIRenderSystem.cpp"
    "src/ShitEngine/UI/UITextInput.cpp"
    "src/ShitEngine/UI/UITextBox.cpp"
    "src/ShitEngine/UI/UITextArea.cpp"
    # Physics
    "src/ShitEngine/Physics/PhysicsSystem2D.cpp"
    "src/ShitEngine/Physics/RigidBody2D.cpp"
    "src/ShitEngine/Physics/BoxCollider2D.cpp"
    "src/ShitEngine/Physics/CircleCollider2D.cpp"
)

set_target_properties(ShitEngine PROPERTIES
    OUTPUT_NAME   "ShitEngine"
    DEBUG_POSTFIX "-d"
)

target_include_directories(ShitEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# 生成代码只有 Engine 内部 (Game.cpp) 需要，不对消费者暴露
# 避免 Plugin 编译时 Engine/generated 和 Plugin/generated 的 ReflectionRegisterAll.h 冲突
target_include_directories(ShitEngine PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/generated>)

target_precompile_headers(ShitEngine PRIVATE "include/ShitEngine/Core/pch.h")
target_compile_definitions(ShitEngine PRIVATE SHIT_EXPORTS)

target_link_libraries(ShitEngine PUBLIC
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    spdlog::spdlog
    glm::glm
    nlohmann_json::nlohmann_json
    box2d::box2d)

set_target_properties(ShitEngine PROPERTIES CXX_STANDARD 20)

# ── 反射扫描（须在依赖配置后，以传入 glm/SDL3 等 include 路径，否则
# 反射头里的 Vector2=glm::vec2、SDL_FRect 等类型会解析失败）──
if(BUILD_TOOLS)
    FetchContent_GetProperties(glm)
    FetchContent_GetProperties(SDL3)
    set(_ENGINE_REFLECT_INCS "")
    if(glm_SOURCE_DIR)
        list(APPEND _ENGINE_REFLECT_INCS "${glm_SOURCE_DIR}")
    endif()
    if(SDL3_SOURCE_DIR)
        list(APPEND _ENGINE_REFLECT_INCS "${SDL3_SOURCE_DIR}/include")
    endif()
    setup_reflection_scan(engine
        "${CMAKE_CURRENT_SOURCE_DIR}/include/ShitEngine"
        "${CMAKE_CURRENT_SOURCE_DIR}/generated/reflection"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        ${_ENGINE_REFLECT_INCS}
    )
endif()

# MSVC 使用者也需要 /utf-8（spdlog 要求）
if(MSVC)
    target_compile_options(ShitEngine PUBLIC /utf-8)
endif()

# ── Install ──────────────────────────────────────────────
include(GNUInstallDirs)

# 头文件
install(DIRECTORY include/ShitEngine
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN "pch.h" EXCLUDE)
install(FILES include/ShitEngine.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 安装第三方依赖头文件（用 FetchContent 获取源路径）
macro(_install_fetch_headers _dep _subdir _pattern)
    FetchContent_GetProperties(${_dep})
    set(_dir "${${_dep}_SOURCE_DIR}")
    if(_dir AND EXISTS "${_dir}/${_subdir}")
        install(DIRECTORY "${_dir}/${_subdir}"
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                FILES_MATCHING PATTERN "${_pattern}")
    endif()
endmacro()

_install_fetch_headers(spdlog       "include/spdlog"      "*.h")
_install_fetch_headers(SDL3         "include/SDL3"        "*.h")
_install_fetch_headers(SDL3_mixer   "include/SDL3_mixer"  "*.h")
_install_fetch_headers(SDL3_image   "include/SDL3_image"  "*.h")
_install_fetch_headers(SDL3_ttf     "include/SDL3_ttf"    "*.h")
_install_fetch_headers(glm          "glm"                 "*.*")
_install_fetch_headers(json         "include/nlohmann"    "*.hpp")

# 库文件（Debug 自动加 -d 后缀）
install(TARGETS ShitEngine
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# CMake config
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ShitEngineConfig.cmake.in
    ${CMAKE_BINARY_DIR}/ShitEngineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/ShitEngineConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_BINARY_DIR}/ShitEngineConfig.cmake
    ${CMAKE_BINARY_DIR}/ShitEngineConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine)

# CPack
set(CPACK_PACKAGE_NAME ShitEngine)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight 2D game engine based on C++20 and SDL3")
include(CPack)

# 当 BUILD_TOOLS=ON 时，自动运行 Scanner 生成反射代码
if(BUILD_TOOLS AND TARGET reflect-engine)
    add_dependencies(ShitEngine reflect-engine)
    message(STATUS "ReflectionScanner integration enabled (engine) — auto-generating .gen.h files")
endif()
