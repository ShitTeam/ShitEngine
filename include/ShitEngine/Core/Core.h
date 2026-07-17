#pragma once

#include <cstdint>

#ifdef _WIN32
	#ifdef SHIT_EXPORTS
		// 如果正在构建引擎本身，导出符号
		#define SHIT_API __declspec(dllexport)
	#else
		// 如果别人在使用引擎，导入符号
		#define SHIT_API __declspec(dllimport)
	#endif
#else
	#define SHIT_API
#endif

#define BIT(x) (1 << x)

namespace Shit {
	/**
	 * @brief RGBA 颜色（每通道 8 位）
	 *
	 * 用于 UIImage 等 UI 控件着色与按钮状态过渡。
	 * 变量名不缩写。
	 */
	struct Color {
		std::uint8_t red   = 255;
		std::uint8_t green = 255;
		std::uint8_t blue  = 255;
		std::uint8_t alpha = 255;
	};
}