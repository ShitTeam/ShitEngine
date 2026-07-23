#pragma once
#include "../Core/Core.h"

namespace Shit {

/**
 * @brief 事件基类，所有自定义事件须继承此结构体
 */
struct Event {
    virtual ~Event() = default;
};

} // namespace Shit
