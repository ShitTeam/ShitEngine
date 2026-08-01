#pragma once

#include <cstddef>
#include <ShitEngine/UI/UICanvas.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UICanvas() {
    Shit::ReflectType("UICanvas", sizeof(UICanvas))
        .Base("Component")
        .Field("m_sortOrder",
            &Shit::UICanvas::m_sortOrder, "int")
        .Meta(Shit::FieldMeta{.displayName = "Sort Order", .tooltip = "Canvas 渲染排序"})
        .Factory<UICanvas>()
        .Register<UICanvas>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(UICanvas) == 24,
        "UICanvas: size mismatch - regenerate reflection data");
    static_assert(offsetof(UICanvas, m_sortOrder) == 20,
        "UICanvas::m_sortOrder: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
