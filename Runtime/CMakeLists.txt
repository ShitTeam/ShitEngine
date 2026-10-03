# Runtime — 宿主程序，动态加载插件 DLL（PluginManager 由引擎提供）

add_executable(ShitRuntime
    "src/main.cpp"
)

target_link_libraries(ShitRuntime PRIVATE ShitEngine)

add_dependencies(ShitRuntime ShitEngine)

set_target_properties(ShitRuntime PROPERTIES CXX_STANDARD 20)

# 输出到根级 bin/（与 ShitEngine.dll 在一起）
set_target_properties(ShitRuntime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 拷贝 config.json 与 Scenes/ 到 exe 所在目录
# （settings.json 不存在时由 Config::Init 以安全默认回退，无需拷贝；
#   resource/ 资产按需自备——导出游戏的资源装配走编辑器「导出游戏」流程）
add_custom_command(TARGET ShitRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:ShitRuntime>/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Scenes"
        "$<TARGET_FILE_DIR:ShitRuntime>/Scenes"
    COMMENT "Copying config and resources to output directory"
)

# P18：SDK 自包含运行时 —— 编辑器「导出游戏」从 SDK bin/ 取 ShitRuntime.exe
include(GNUInstallDirs)
install(TARGETS ShitRuntime RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
