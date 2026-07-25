# CMakeLists.txt — ShitEngine 工作区根
cmake_minimum_required(VERSION 3.20)
project(ShitEngine VERSION 1.0.0 LANGUAGES C CXX)

# UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# MSVC 热重载
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# MinGW: 允许 COFF 目标文件超过 65535 段
if(MINGW)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wa,-mbig-obj")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

# 输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Tools 可选（默认 OFF：libClang 路径硬编码，强制 ON 会导致无该依赖的环境配置失败）
# 必须在 Engine 之前：Engine 会检测 reflect 目标并建立依赖
option(BUILD_TOOLS "构建工具项目（ReflectionScanner 等）" OFF)
if(BUILD_TOOLS)
    add_subdirectory(Tools/ReflectionScanner)
    # 加载反射扫描配置（system include/resource dir 检测 + setup_reflection_scan 函数）
    # 在此 include 以便 Engine/Examples 子目录能调用 setup_reflection_scan()
    include(${CMAKE_SOURCE_DIR}/Tools/ReflectionScanner/ReflectionScanSetup.cmake)
elseif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # 仅在顶层项目且未启用工具时提醒，FetchContent 消费者不打印
    message(WARNING
        "BUILD_TOOLS=OFF：反射 .gen.h 不会自动重新生成。\n"
        "  修改带 SHIT_REFLECT 的头文件后，请手动运行:\n"
        "    cmake --build . --target run-reflectionscanner\n"
        "  或配置时加 -DBUILD_TOOLS=ON 启用自动生成。")
endif()

# Engine 库（检测 BUILD_TOOLS 决定是否自动运行 ReflectionScanner）
add_subdirectory(Engine)

# Examples 可选（外部用户 FetchContent 时跳过）
set(BUILD_EXAMPLES ON CACHE BOOL "构建示例项目（本地开发默认开启）")
if(BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

# Runtime 宿主程序（动态加载插件 DLL）
set(BUILD_RUNTIME ON CACHE BOOL "构建 Runtime 宿主程序（本地开发默认开启）")
if(BUILD_RUNTIME)
    add_subdirectory(Runtime)
endif()

# ── 反射扫描伞形目标（须在 Engine/Examples 之后，reflect-engine/reflect-examples 已创建）──
if(BUILD_TOOLS)
    set(_REFLECT_DEPS "")
    set(_REFLECT_FORCE_DEPS "")
    foreach(_t reflect-engine reflect-examples)
        if(TARGET ${_t})
            list(APPEND _REFLECT_DEPS ${_t})
        endif()
    endforeach()
    foreach(_t run-reflectionscanner-engine run-reflectionscanner-examples)
        if(TARGET ${_t})
            list(APPEND _REFLECT_FORCE_DEPS ${_t})
        endif()
    endforeach()
    if(_REFLECT_DEPS)
        add_custom_target(reflect DEPENDS ${_REFLECT_DEPS})
    endif()
    if(_REFLECT_FORCE_DEPS)
        add_custom_target(run-reflectionscanner DEPENDS ${_REFLECT_FORCE_DEPS})
    endif()
endif()
