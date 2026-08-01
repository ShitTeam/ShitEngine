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
# 修改 LIBCLANG_PREFIX 指向你的 libClang 安装目录
set(LIBCLANG_PREFIX "D:/LLVM" CACHE PATH "libClang 安装根目录")

# 头文件：先搜系统路径，再回退到 LIBCLANG_PREFIX
find_path(LIBCLANG_INCLUDE_DIR
    NAMES clang-c/Index.h
    PATHS ${LIBCLANG_PREFIX}/include
    REQUIRED
)
message(STATUS "libclang include: ${LIBCLANG_INCLUDE_DIR}")

# 库文件：先搜系统路径，再回退到 LIBCLANG_PREFIX
find_library(LIBCLANG_LIBRARY
    NAMES libclang clang
    PATHS ${LIBCLANG_PREFIX}/lib
    REQUIRED
)
message(STATUS "libclang lib:     ${LIBCLANG_LIBRARY}")

# 推断 libclang.dll 所在目录（优先 LIBCLANG_PREFIX/bin，否则从库文件位置推导）
set(LIBCLANG_DLL_PATH "${LIBCLANG_PREFIX}/bin/libclang.dll")
if(NOT EXISTS "${LIBCLANG_DLL_PATH}")
    get_filename_component(_LIBCLANG_LIB_DIR "${LIBCLANG_LIBRARY}" DIRECTORY)
    get_filename_component(_LIBCLANG_ROOT "${_LIBCLANG_LIB_DIR}/.." ABSOLUTE)
    set(LIBCLANG_DLL_PATH "${_LIBCLANG_ROOT}/bin/libclang.dll")
endif()

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

# 运行时需要 libclang.dll 在 PATH 中或 exe 同目录
# 仅当 DLL 确实存在时才拷贝（静态链接 / 非 Windows 环境会跳过）
if(EXISTS "${LIBCLANG_DLL_PATH}")
    add_custom_command(TARGET ReflectionScanner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBCLANG_DLL_PATH}"
            "$<TARGET_FILE_DIR:ReflectionScanner>/libclang.dll"
        COMMENT "Copying libclang.dll to output directory"
    )
else()
    message(WARNING
        "libclang.dll 未在 ${LIBCLANG_DLL_PATH} 找到，已跳过拷贝。\n"
        "  运行 ReflectionScanner 时请确保 libclang.dll 在 PATH 中。")
endif()
