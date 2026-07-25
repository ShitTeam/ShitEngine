# ══════════════════════════════════════════════════════
# detect_system_includes.cmake
# 检测编译器的系统 include 路径（用于 ReflectionScanner）
# ══════════════════════════════════════════════════════

# 用法: cmake -P detect_system_includes.cmake
#        -DCMAKE_CXX_COMPILER=g++
#        -DOUTPUT_VAR=name
# 输出: 打印以分号分隔的路径列表

if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "g++")
endif()

# 平台空设备（Windows: NUL, Unix: /dev/null）
if(CMAKE_HOST_WIN32)
    set(NULL_DEVICE "NUL")
else()
    set(NULL_DEVICE "/dev/null")
endif()

# GCC/MinGW: 运行 -E -v 从 stderr 提取 include 路径
# 输出格式：
#   ...
#   #include "..." search starts here:
#   #include <...> search starts here:
#     /path/1
#     /path/2
#   End of search list.
#   ...

execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -E -x c++ - -v
    INPUT_FILE ${NULL_DEVICE}
    ERROR_VARIABLE gcc_stderr
    OUTPUT_QUIET
    TIMEOUT 30
)

# 解析 "search starts here" 到 "End of search list" 之间的行
string(REGEX MATCH "#include[^#]*End of search list\\." section "${gcc_stderr}")

set(paths "")
if(section)
    string(REPLACE "\n" ";" lines "${section}")
    set(in_section FALSE)
    foreach(line IN LISTS lines)
        string(STRIP "${line}" line)
        if(line MATCHES "search starts here:")
            set(in_section TRUE)
            continue()
        endif()
        if(line MATCHES "End of search list")
            set(in_section FALSE)
            continue()
        endif()
        if(in_section AND line)
            # 去掉前导空格/制表符
            string(REGEX REPLACE "^[ \t]+" "" line "${line}")
            list(APPEND paths "${line}")
        endif()
    endforeach()
endif()

# 输出路径列表（CMake 分号分隔）
message("${paths}")
