# Runtime — 宿主程序，动态加载插件 DLL

add_executable(ShitRuntime
    "src/main.cpp"
    "src/PluginManager.cpp"
    "src/PluginManager.h"
)

target_link_libraries(ShitRuntime PRIVATE ShitEngine)

add_dependencies(ShitRuntime ShitEngine)

set_target_properties(ShitRuntime PROPERTIES CXX_STANDARD 20)

# 输出到根级 bin/（与 ShitEngine.dll 和 ExamplePlugin.dll 在一起）
set_target_properties(ShitRuntime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 拷贝 config.json / settings.json / resource/ / Scenes/*.scene 到 exe 所在目录
add_custom_command(TARGET ShitRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:ShitRuntime>/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/Examples/settings.json"
        "$<TARGET_FILE_DIR:ShitRuntime>/settings.json"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Scenes"
        "$<TARGET_FILE_DIR:ShitRuntime>/Scenes"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Examples/resource"
        "$<TARGET_FILE_DIR:ShitRuntime>/resource"
    COMMENT "Copying config and resources to output directory"
)
