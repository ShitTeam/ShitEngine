# ExamplePlugin — 示例插件（SHARED 库，供 Runtime 动态加载）
# 需要根 CMakeLists.txt 开启 BUILD_EXAMPLES

add_library(ExamplePlugin SHARED
    "src/plugin_export.cpp"
    "src/Behaviors.h"
    "src/CoinDemo.h"
    "src/ReflectionBehavior.h"
    "src/Player.cpp"
    "src/Player.h"
    "src/ReflectionTestTypes.h"
)

target_link_libraries(ExamplePlugin PRIVATE ShitEngine)
add_dependencies(ExamplePlugin ShitEngine)
set_target_properties(ExamplePlugin PROPERTIES CXX_STANDARD 20)
set_target_properties(ExamplePlugin PROPERTIES
    PREFIX ""
    DEBUG_POSTFIX "")

# 插件自身的头文件和生成代码
target_include_directories(ExamplePlugin PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/generated")

# ── 反射扫描（须在依赖配置后，传入 Engine/glm/SDL3 include 路径）──
if(BUILD_TOOLS)
    FetchContent_GetProperties(glm)
    FetchContent_GetProperties(SDL3)
    set(_EX_REFLECT_INCS "${CMAKE_SOURCE_DIR}/Engine/include")
    if(glm_SOURCE_DIR)
        list(APPEND _EX_REFLECT_INCS "${glm_SOURCE_DIR}")
    endif()
    if(SDL3_SOURCE_DIR)
        list(APPEND _EX_REFLECT_INCS "${SDL3_SOURCE_DIR}/include")
    endif()
    setup_reflection_scan(examples
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/generated/reflection"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        ${_EX_REFLECT_INCS}
    )
endif()

# 当 BUILD_TOOLS=ON 时，自动运行 Scanner 生成反射代码
if(BUILD_TOOLS AND TARGET reflect-examples)
    add_dependencies(ExamplePlugin reflect-examples)
    message(STATUS "ReflectionScanner integration enabled (examples) — auto-generating .gen.h files")
endif()
