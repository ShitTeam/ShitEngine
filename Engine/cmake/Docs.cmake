# Docs.cmake — Doxygen API 文档生成
#
# 用法: cmake --build . --target docs
#       生成到 ${CMAKE_BINARY_DIR}/docs/api/

find_package(Doxygen QUIET)

if(Doxygen_FOUND)
    # 复制 Doxyfile 到构建目录，确保路径正确
    # 注意：用 CMAKE_CURRENT_SOURCE_DIR（引擎自身目录）而非 CMAKE_SOURCE_DIR（顶层），
    # 这样引擎被 add_subdirectory 引入 Example 等顶层项目时，仍能正确定位 Doxyfile.in / docs / include。
    set(SHIT_DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    configure_file(
        "${SHIT_DOXYGEN_SOURCE_DIR}/Doxyfile.in"
        "${CMAKE_BINARY_DIR}/Doxyfile"
        @ONLY
    )

    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/docs/api"
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${SHIT_DOXYGEN_SOURCE_DIR}"
        COMMENT "生成 Doxygen API 文档..."
        VERBATIM
    )
else()
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E echo
            "Doxygen 未安装。跳过 API 文档生成。"
        COMMENT "请安装 Doxygen: https://www.doxygen.nl/download.html"
    )
endif()
