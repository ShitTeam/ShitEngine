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

# ── Install ──────────────────────────────────────────────
include(GNUInstallDirs)

# 头文件
install(DIRECTORY include/ShitEngine
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN "pch.h" EXCLUDE)
install(FILES include/ShitEngine.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 安装第三方依赖头文件（ShitEngine 内部头文件需要这些）
if(spdlog_SOURCE_DIR)
    install(DIRECTORY "${spdlog_SOURCE_DIR}/include/spdlog"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h")
endif()

# SDL3 头文件
if(SDL3_SOURCE_DIR)
    install(DIRECTORY "${SDL3_SOURCE_DIR}/include/SDL3"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h")
endif()
if(SDL3_mixer_SOURCE_DIR)
    install(DIRECTORY "${SDL3_mixer_SOURCE_DIR}/include/SDL3_mixer"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h")
endif()
if(SDL3_image_SOURCE_DIR)
    install(DIRECTORY "${SDL3_image_SOURCE_DIR}/include/SDL3_image"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h")
endif()
if(SDL3_ttf_SOURCE_DIR)
    install(DIRECTORY "${SDL3_ttf_SOURCE_DIR}/include/SDL3_ttf"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h")
endif()

# glm（header-only）
if(glm_SOURCE_DIR)
    install(DIRECTORY "${glm_SOURCE_DIR}/glm"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp")
endif()

# nlohmann_json（header-only）
if(json_SOURCE_DIR)
    install(DIRECTORY "${json_SOURCE_DIR}/include/nlohmann"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.hpp")
endif()

# 库文件（Debug 自动加 -d 后缀）
install(TARGETS ShitEngine
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# CMake config
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/ShitEngineConfig.cmake.in
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
