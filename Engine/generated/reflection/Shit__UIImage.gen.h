#pragma once

#include <ShitEngine/UI/UIImage.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UIImage() {
    Shit::ReflectType("UIImage", sizeof(UIImage))
        .Base("UIRendererComponent")
        .Field("m_sprite",
            &Shit::UIImage::m_sprite, "Sprite")
        .Meta(Shit::FieldMeta{.displayName = "Sprite", .readOnly = true})
        .Field("m_color",
            &Shit::UIImage::m_color, "Color")
        .Meta(Shit::FieldMeta{.displayName = "Color", .tooltip = "颜色叠加（用于按钮状态切换着色）"})
        .Factory<UIImage>()
        .Register<UIImage>();
    return true;
}

} // namespace Shit
