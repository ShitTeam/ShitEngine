#pragma once

#ifdef SHIT_EXPORTS
    // 如果正在构建引擎本身，导出符号
    #define SHIT_API __declspec(dllexport)
#else
    // 如果别人在使用引擎，导入符号
    #define SHIT_API __declspec(dllimport)
#endif

#define BIT(x) (1 << x)