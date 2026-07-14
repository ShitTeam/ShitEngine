# CMakeList.txt: ShitEngine 的 CMake 项目
cmake_minimum_required (VERSION 3.20)

project(ShitEngine VERSION 1.0.0 LANGUAGES C CXX)

# UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# MSVC 热重载
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# MinGW: 允许 COFF 目标文件超过 65535 段
if(MINGW)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wa,-mbig-obj")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_SHARED_LIBS "依赖库默认编译为动态库" ON)

include(cmake/DependencyManager.cmake)
setup_project_dependencies()

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
    # Resource
    "src/ShitEngine/Resource/ResourceManager.cpp"
    "src/ShitEngine/Resource/TextureManager.cpp"
    # System
    "src/ShitEngine/System/System.cpp"
    "src/ShitEngine/System/BehaviorSystem.cpp"
)

set_target_properties(ShitEngine PROPERTIES
    VERSION       ${PROJECT_VERSION}
    SOVERSION     ${PROJECT_VERSION_MAJOR}
    DEBUG_POSTFIX "-d"                              # Debug 版本加 -d 后缀
)

target_include_directories(ShitEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_precompile_headers(ShitEngine PUBLIC "include/ShitEngine/Core/pch.h")
target_compile_definitions(ShitEngine PRIVATE SHIT_EXPORTS)

target_link_libraries(ShitEngine PUBLIC
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    spdlog::spdlog
    glm::glm
    nlohmann_json::nlohmann_json)

set_target_properties(ShitEngine PROPERTIES CXX_STANDARD 20)

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
