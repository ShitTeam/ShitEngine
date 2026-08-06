#pragma once

#include <ShitEngine/Component/SpriteRenderer.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_SpriteRenderer() {
    Shit::ReflectType("SpriteRenderer", sizeof(SpriteRenderer))
        .Base("RendererComponent")
        .Field("m_texturePath",
            &Shit::SpriteRenderer::m_texturePath, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Texture Path", .tooltip = "纹理文件路径（持久化；反序列化后自动重载）"})
        .Field("m_sprite",
            &Shit::SpriteRenderer::m_sprite, "Sprite")
        .Meta(Shit::FieldMeta{.displayName = "Sprite Data", .readOnly = true})
        .Factory<SpriteRenderer>()
        .Register<SpriteRenderer>();
    return true;
}

} // namespace Shit
