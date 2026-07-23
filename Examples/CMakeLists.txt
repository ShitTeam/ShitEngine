# Examples — 示例项目（需要根 CMakeLists.txt 开启 BUILD_EXAMPLES）

add_executable(Example
    "src/main.cpp"
    "src/Player.cpp"
    "src/Player.h"
    "src/UITestScene.cpp"
    "src/UITestScene.h"
    "src/InputMappingTestScene.cpp"
    "src/InputMappingTestScene.h"
)

target_link_libraries(Example PRIVATE ShitEngine)
set_target_properties(Example PROPERTIES CXX_STANDARD 20)

# 运行时输出到根级 bin/（DLL 和 exe 在一起方便调试）
set_target_properties(Example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
