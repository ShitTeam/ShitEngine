#pragma once

#include <ShitEngine/Component/SpriteRenderer.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_SpriteRenderer() {
    Shit::ReflectType("SpriteRenderer", sizeof(SpriteRenderer))
        .Base("RendererComponent")
        .Field("m_sprite",
            &Shit::SpriteRenderer::m_sprite, "Sprite")
        .Meta(Shit::FieldMeta{.displayName = "Sprite Data", .readOnly = true})
        .Factory<SpriteRenderer>()
        .Register<SpriteRenderer>();
    return true;
}

} // namespace Shit
