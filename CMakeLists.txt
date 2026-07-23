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

add_subdirectory(Engine)

# Examples 可选（外部用户 FetchContent 时跳过）
option(BUILD_EXAMPLES "构建示例项目" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()
