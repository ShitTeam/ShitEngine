#pragma once

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
    return true;
}

} // namespace Shit
