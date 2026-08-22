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
        .Property("flipped",
            "bool",
            [](void* obj) -> void* {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                static thread_local bool v;
                v = self->isFlipped();
                return &v;
            },
            [](void* obj, const void* val) {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                self->setFlipped(*static_cast<const bool*>(val));
            })
        .Property("sourceRectX",
            "float",
            [](void* obj) -> void* {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                static thread_local float v;
                v = self->getSourceRectX();
                return &v;
            },
            [](void* obj, const void* val) {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                self->setSourceRectX(*static_cast<const float*>(val));
            })
        .Property("sourceRectY",
            "float",
            [](void* obj) -> void* {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                static thread_local float v;
                v = self->getSourceRectY();
                return &v;
            },
            [](void* obj, const void* val) {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                self->setSourceRectY(*static_cast<const float*>(val));
            })
        .Property("sourceRectW",
            "float",
            [](void* obj) -> void* {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                static thread_local float v;
                v = self->getSourceRectW();
                return &v;
            },
            [](void* obj, const void* val) {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                self->setSourceRectW(*static_cast<const float*>(val));
            })
        .Property("sourceRectH",
            "float",
            [](void* obj) -> void* {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                static thread_local float v;
                v = self->getSourceRectH();
                return &v;
            },
            [](void* obj, const void* val) {
                auto* self = static_cast<Shit::SpriteRenderer*>(obj);
                self->setSourceRectH(*static_cast<const float*>(val));
            })
        .Factory<SpriteRenderer>()
        .Register<SpriteRenderer>();
    return true;
}

} // namespace Shit
