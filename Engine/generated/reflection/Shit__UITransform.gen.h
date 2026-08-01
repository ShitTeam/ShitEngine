#pragma once

#include <ShitEngine/UI/UITransform.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UITransform() {
    Shit::ReflectType("UITransform", sizeof(UITransform))
        .Base("Component")
        .Field("m_anchorMin",
            &Shit::UITransform::m_anchorMin, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Anchor Min", .tooltip = "锚点左上（归一化 0-1，相对父级）", .range = {0, 1}, .step = 0.01})
        .Field("m_anchorMax",
            &Shit::UITransform::m_anchorMax, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Anchor Max", .tooltip = "锚点右下（归一化 0-1，相对父级）", .range = {0, 1}, .step = 0.01})
        .Field("m_pivot",
            &Shit::UITransform::m_pivot, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Pivot", .tooltip = "轴心（归一化 0-1，相对自身）", .range = {0, 1}, .step = 0.01})
        .Field("m_anchoredPosition",
            &Shit::UITransform::m_anchoredPosition, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Anchored Position", .tooltip = "相对锚点的偏移（像素）"})
        .Field("m_width",
            &Shit::UITransform::m_width, "float")
        .Meta(Shit::FieldMeta{.displayName = "Width", .tooltip = "宽度（仅 anchorMin==anchorMax 轴生效）", .range = {0, 99999}})
        .Field("m_height",
            &Shit::UITransform::m_height, "float")
        .Meta(Shit::FieldMeta{.displayName = "Height", .tooltip = "高度", .range = {0, 99999}})
        .Field("m_zIndex",
            &Shit::UITransform::m_zIndex, "int")
        .Meta(Shit::FieldMeta{.displayName = "Z-Index", .tooltip = "渲染层级（值越大越靠上）"})
        .Factory<UITransform>()
        .Register<UITransform>();
    return true;
}

} // namespace Shit
