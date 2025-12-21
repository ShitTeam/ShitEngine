# CMakeList.txt: ShitEngine 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.10)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

project ("ShitEngine")

# 将源代码添加到此项目的可执行文件。
add_executable (ShitEngine
	"src/main.cpp"
	"include/ShitEngine/Game.h"
	"src/ShitEngine/Game.cpp"
)

# 告诉 CMake 头文件在 include 目录里
target_include_directories(ShitEngine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(ShitEngine PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ShitEngine PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
