# ReflectionScanner — libClang 源码扫描工具
# 解析 SHIT_CLASS / SHIT_STRUCT / SHIT_META 标记（通过 AST 的 annotate 属性）
# 并生成反射注册代码（.gen.h）
#
# 注意：system include / resource dir 检测与 setup_reflection_scan() 函数定义
# 已移至 ReflectionScanSetup.cmake，由顶层 CMakeLists.txt include，
# 以便 Engine/Examples 在第三方依赖配置后调用并传入 glm/SDL3 等 include 路径。

cmake_minimum_required(VERSION 3.20)
project(ReflectionScanner VERSION 1.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 配置 libClang 路径 ──────────────────────────────
# 默认空值：优先走系统路径（Linux: apt, macOS: brew, Windows: vcpkg 或手动指定）
set(LIBCLANG_PREFIX "" CACHE PATH "libClang 安装根目录（空=系统默认路径）")

# Linux/macOS: clang 头文件不在标准路径（如 Ubuntu 的 /usr/lib/llvm-18/include），
# 尝试用 llvm-config 探测实际安装目录（libclang-dev / llvm-dev 包提供）
if(NOT LIBCLANG_PREFIX)
    find_program(_LLVM_CONFIG
        NAMES llvm-config llvm-config-19 llvm-config-18 llvm-config-17 llvm-config-16
    )
    if(_LLVM_CONFIG)
        execute_process(COMMAND ${_LLVM_CONFIG} --includedir
            OUTPUT_VARIABLE _LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${_LLVM_CONFIG} --libdir
            OUTPUT_VARIABLE _LLVM_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "llvm-config found: ${_LLVM_CONFIG} (inc=${_LLVM_INCLUDE_DIR} lib=${_LLVM_LIB_DIR})")
    endif()
endif()

# 头文件：先搜系统路径 / llvm-config 路径，再回退到 LIBCLANG_PREFIX
find_path(LIBCLANG_INCLUDE_DIR
    NAMES clang-c/Index.h
    PATHS ${LIBCLANG_PREFIX}/include ${_LLVM_INCLUDE_DIR}
    REQUIRED
)
message(STATUS "libclang include: ${LIBCLANG_INCLUDE_DIR}")

# 库文件：Linux → libclang.so，macOS → libclang.dylib，Windows → libclang.dll/.lib
# find_library 自动处理各平台后缀
find_library(LIBCLANG_LIBRARY
    NAMES libclang clang
    PATHS ${LIBCLANG_PREFIX}/lib ${_LLVM_LIB_DIR}
    REQUIRED
)
message(STATUS "libclang lib:     ${LIBCLANG_LIBRARY}")

# ── 目标 ────────────────────────────────────────────
add_executable(ReflectionScanner
    src/main.cpp
    src/Scanner.cpp
    src/Generator.cpp
)

target_include_directories(ReflectionScanner PRIVATE
    src
    ${LIBCLANG_INCLUDE_DIR}
)

target_link_libraries(ReflectionScanner PRIVATE ${LIBCLANG_LIBRARY})

# 运行时库拷贝：仅 Windows 需要 libclang.dll 随 exe 分发
# Linux/macOS 上链接时已嵌入 rpath 或系统路径可找到
if(WIN32)
    # 从 LIBCLANG_PREFIX 或库文件位置推断 DLL 路径
    set(LIBCLANG_DLL_PATH "${LIBCLANG_PREFIX}/bin/libclang.dll")
    if(NOT EXISTS "${LIBCLANG_DLL_PATH}")
        get_filename_component(_LIBCLANG_LIB_DIR "${LIBCLANG_LIBRARY}" DIRECTORY)
        get_filename_component(_LIBCLANG_ROOT "${_LIBCLANG_LIB_DIR}/.." ABSOLUTE)
        set(LIBCLANG_DLL_PATH "${_LIBCLANG_ROOT}/bin/libclang.dll")
    endif()
    if(EXISTS "${LIBCLANG_DLL_PATH}")
        add_custom_command(TARGET ReflectionScanner POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIBCLANG_DLL_PATH}"
                "$<TARGET_FILE_DIR:ReflectionScanner>/libclang.dll"
            COMMENT "Copying libclang.dll to output directory"
        )
    else()
        message(WARNING
            "libclang.dll 未找到，已跳过拷贝。\n"
            "  运行 ReflectionScanner 时请确保 libclang.dll 在 PATH 中。")
    endif()
endif()

# ── Install（P14 SDK：编辑器外部脚本工程需要随 SDK 安装扫描器与构建脚本）──
include(GNUInstallDirs)

# 扫描器 exe
install(TARGETS ReflectionScanner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Windows: 额外安装 libclang.dll（SDK 自包含，不依赖本机 LLVM 安装）
if(WIN32)
    set(LIBCLANG_DLL_PATH "${LIBCLANG_PREFIX}/bin/libclang.dll")
    if(NOT EXISTS "${LIBCLANG_DLL_PATH}")
        get_filename_component(_LIBCLANG_LIB_DIR "${LIBCLANG_LIBRARY}" DIRECTORY)
        get_filename_component(_LIBCLANG_ROOT "${_LIBCLANG_LIB_DIR}/.." ABSOLUTE)
        set(LIBCLANG_DLL_PATH "${_LIBCLANG_ROOT}/bin/libclang.dll")
    endif()
    if(EXISTS "${LIBCLANG_DLL_PATH}")
        install(FILES "${LIBCLANG_DLL_PATH}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

# 反射扫描 CMake 脚本（脚本工程的 CMakeLists 从 ${ShitEngine_DIR}/tools/ 引用）
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ReflectionScanSetup.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/detect_system_includes.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine/tools)

# ── libclang resource 目录（builtin headers：stddef.h/stdarg.h 等）──
# 解析反射头必须有；SDK 自带拷贝 → 整个 SDK 可搬移，不依赖本机 LLVM 安装。
# 跨平台：Windows → LIBCLANG_PREFIX/lib/clang/；Linux → /usr/lib/llvm-NN/lib/clang/（apt）；
# macOS → $(brew --prefix llvm)/lib/clang/ 或 $(xcrun --show-sdk-path)/usr/lib/clang/
set(_SDK_CLANG_RES_NAME "")
set(_CLANG_RES_CANDIDATES "")
if(LIBCLANG_PREFIX AND EXISTS "${LIBCLANG_PREFIX}/lib/clang")
    set(_CLANG_RES_CANDIDATES "${LIBCLANG_PREFIX}/lib/clang")
else()
    # 从库文件路径 / llvm-config 路径推导 clang resource 目录
    get_filename_component(_LIBCLANG_LIB_DIR "${LIBCLANG_LIBRARY}" DIRECTORY)
    get_filename_component(_LIBCLANG_ROOT "${_LIBCLANG_LIB_DIR}/.." ABSOLUTE)
    foreach(_candidate
            "${_LIBCLANG_ROOT}/lib/clang"
            "${_LIBCLANG_ROOT}/../lib/clang"
            "${_LLVM_LIB_DIR}/clang"
            "${_LLVM_LIB_DIR}/../lib/clang"
            "${_LLVM_INCLUDE_DIR}/../lib/clang")
        if(EXISTS "${_candidate}")
            set(_CLANG_RES_CANDIDATES "${_candidate}")
            break()
        endif()
    endforeach()
endif()
if(_CLANG_RES_CANDIDATES)
    file(GLOB _CLANG_RES_ENTRIES LIST_DIRECTORIES true "${_CLANG_RES_CANDIDATES}/*")
    foreach(_dir IN LISTS _CLANG_RES_ENTRIES)
        if(IS_DIRECTORY "${_dir}")
            get_filename_component(_dir_name "${_dir}" NAME)
            if(_dir_name MATCHES "^[0-9][0-9.]*$")
                set(_SDK_CLANG_RES_NAME "${_dir_name}")
                set(_SDK_CLANG_RES_DIR "${_dir}")
            endif()
        endif()
    endforeach()
endif()
if(_SDK_CLANG_RES_DIR)
    install(DIRECTORY "${_SDK_CLANG_RES_DIR}"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/ShitEngine/clang-resource)
    message(STATUS "SDK clang resource: ${_SDK_CLANG_RES_DIR}")
else()
    message(WARNING
        "未找到 libclang resource 目录，SDK 反射扫描可能失败。\n"
        "  Linux: apt install libclang-dev\n"
        "  macOS: brew install llvm\n"
        "  Windows: 设置 LIBCLANG_PREFIX")
endif()

# ── 工具信息配置（SDK 模式下由 ReflectionScanSetup.cmake include）──
# 采用相对 CMAKE_CURRENT_LIST_DIR 的路径，SDK 整体搬移后仍有效。
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ShitEngineToolsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ShitEngineToolsConfig.cmake
    @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ShitEngineToolsConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ShitEngine)