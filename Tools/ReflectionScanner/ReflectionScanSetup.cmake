# ReflectionScanSetup.cmake
# 反射扫描的配置期检测与 setup_reflection_scan() 函数定义。
# 由顶层 CMakeLists.txt 在 add_subdirectory(Tools/ReflectionScanner) 之后 include，
# 以便 Engine / Examples 子目录（在第三方依赖配置完成后）能调用 setup_reflection_scan()，
# 并把 glm/SDL3 等依赖的 include 路径传给 Scanner（否则反射头里的 Vector2=glm::vec2、
# SDL_FRect 等类型会解析失败，导致 typeName 退化、字段信息错误）。

# ── SDK 工具配置（可选）──
# 独立 SDK（P14：cmake --install 产物）会在同目录附带 ShitEngineToolsConfig.cmake，
# 其中记录了 ReflectionScanner 可执行路径与 libclang 根目录；仓库内开发构建不存在该文件，
# 走默认逻辑（ReflectionScanner 为 build 目标名、LIBCLANG_PREFIX 为缓存变量）。
set(_SDK_TOOLS_CONFIG "${CMAKE_CURRENT_LIST_DIR}/../ShitEngineToolsConfig.cmake")
if(EXISTS "${_SDK_TOOLS_CONFIG}")
    include("${_SDK_TOOLS_CONFIG}")
endif()

# 扫描器可执行：仓库模式直接用目标名（CMake 自动替换为可执行路径并添加依赖）；
# SDK 模式由 ShitEngineToolsConfig.cmake 提供绝对路径。
if(NOT DEFINED REFLECTION_SCANNER_EXE)
    set(REFLECTION_SCANNER_EXE "ReflectionScanner")
endif()

# ── 自动检测编译器系统 include 路径（所有作用域共用）──
# detect_system_includes.cmake 通过 message() 输出路径（写入 stderr），
# 因此用 ERROR_VARIABLE 捕获。
if(NOT DEFINED _REFLECT_SYSTEM_INCLUDES)
    set(_REFLECT_SYSTEM_INCLUDES "")
    if(CMAKE_CXX_COMPILER)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -P
                "${CMAKE_CURRENT_LIST_DIR}/detect_system_includes.cmake"
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            ERROR_VARIABLE _REFLECT_SYSTEM_INCLUDES
            ERROR_STRIP_TRAILING_WHITESPACE
            TIMEOUT 30
        )
        message(STATUS "Detected system includes: ${_REFLECT_SYSTEM_INCLUDES}")
    endif()
    set(_REFLECT_SYSTEM_INCLUDES "${_REFLECT_SYSTEM_INCLUDES}" CACHE INTERNAL
        "ReflectionScanner: compiler system include paths")
endif()

# ── 检测 libclang resource 目录（含 builtin headers: stddef.h / stdarg.h 等）──
# 优先级：SDK 模式由 ShitEngineToolsConfig.cmake 提供 REFLECTION_CLANG_RESOURCE_DIR
# （绝对路径，SDK 自带拷贝）；仓库模式从 LIBCLANG_PREFIX 推导（--resource-dir 传入，
# 避免 Scanner.cpp 硬编码版本号——LLVM 升级或换机器后不再失效）。
if(NOT DEFINED _REFLECT_CLANG_RESOURCE_DIR)
    set(_REFLECT_CLANG_RESOURCE_DIR "${REFLECTION_CLANG_RESOURCE_DIR}")
endif()
if(NOT _REFLECT_CLANG_RESOURCE_DIR AND EXISTS "${LIBCLANG_PREFIX}/lib/clang")
    file(GLOB _CLANG_RES_ENTRIES LIST_DIRECTORIES true "${LIBCLANG_PREFIX}/lib/clang/*")
    foreach(_entry IN LISTS _CLANG_RES_ENTRIES)
        if(IS_DIRECTORY "${_entry}")
            set(_REFLECT_CLANG_RESOURCE_DIR "${_entry}")
        endif()
    endforeach()
endif()

if(_REFLECT_CLANG_RESOURCE_DIR)
    message(STATUS "libclang resource dir: ${_REFLECT_CLANG_RESOURCE_DIR}")
else()
    message(WARNING
        "未在 ${LIBCLANG_PREFIX}/lib/clang 下找到 resource 目录。\n"
        "  ReflectionScanner 可能无法解析 builtin headers（stddef.h 等），\n"
        "  导致 CXError_ASTReadError。请检查 LIBCLANG_PREFIX 是否正确。")
endif()
set(_REFLECT_CLANG_RESOURCE_DIR "${_REFLECT_CLANG_RESOURCE_DIR}" CACHE INTERNAL
    "ReflectionScanner: libclang resource dir")

# ── setup_reflection_scan() ──
# 封装扫描器调用配置：
#   - 自动发现输入头文件 (GLOB_RECURSE CONFIGURE_DEPENDS)
#   - 增量生成目标 (reflect-<scope>)
#   - 强制运行目标 (run-reflectionscanner-<scope>)
#
# 参数:
#   SCOPE_NAME   作用域名称 (engine / examples)
#   INPUT_DIR    扫描输入目录
#   OUTPUT_DIR   生成代码输出目录
#   INCLUDE_ROOT include 路径裁剪前缀（决定生成代码中 #include <...> 的路径）
#   ARGN         额外的 --include 路径（如第三方库头路径 glm/SDL3，须在依赖配置后传入）
function(setup_reflection_scan SCOPE_NAME INPUT_DIR OUTPUT_DIR INCLUDE_ROOT)
    file(GLOB_RECURSE REFLECTION_HEADERS CONFIGURE_DEPENDS
        "${INPUT_DIR}/*.h"
    )

    # 构建 --system-include 参数
    set(_SYS_INC_ARGS "")
    foreach(_path IN LISTS _REFLECT_SYSTEM_INCLUDES)
        list(APPEND _SYS_INC_ARGS "--system-include" "${_path}")
    endforeach()

    set(SCAN_COMMAND
        ${REFLECTION_SCANNER_EXE}
            --input "${INPUT_DIR}"
            --output "${OUTPUT_DIR}"
            --include "${INCLUDE_ROOT}"
            --include-root "${INCLUDE_ROOT}"
            ${_SYS_INC_ARGS}
    )
    if(_REFLECT_CLANG_RESOURCE_DIR)
        list(APPEND SCAN_COMMAND "--resource-dir" "${_REFLECT_CLANG_RESOURCE_DIR}")
    endif()

    foreach(extra_inc IN LISTS ARGN)
        list(APPEND SCAN_COMMAND "--include" "${extra_inc}")
    endforeach()

    set(STAMP_FILE "${OUTPUT_DIR}/.reflect-${SCOPE_NAME}-stamp")

    # 仓库模式依赖扫描器目标（自动重建）；SDK 模式用已安装的独立 exe，无目标依赖
    set(_SCAN_DEPENDS "")
    if(TARGET ReflectionScanner)
        list(APPEND _SCAN_DEPENDS ReflectionScanner)
    endif()

    add_custom_command(
        OUTPUT "${STAMP_FILE}"
        COMMAND ${SCAN_COMMAND}
        COMMAND ${CMAKE_COMMAND} -E touch "${STAMP_FILE}"
        COMMENT "Running ReflectionScanner on ${SCOPE_NAME} headers (incremental)..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${_SCAN_DEPENDS} ${REFLECTION_HEADERS}
    )

    add_custom_target(reflect-${SCOPE_NAME} DEPENDS "${STAMP_FILE}")

    add_custom_target(run-reflectionscanner-${SCOPE_NAME}
        COMMAND ${SCAN_COMMAND}
        COMMENT "Force-running ReflectionScanner on ${SCOPE_NAME} headers..."
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${_SCAN_DEPENDS}
    )
endfunction()
