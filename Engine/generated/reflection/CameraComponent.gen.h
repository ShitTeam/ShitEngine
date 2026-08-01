#pragma once

#include <cstddef>
#include <ShitEngine/Component/CameraComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_CameraComponent() {
    Shit::ReflectType("CameraComponent", sizeof(CameraComponent))
        .Base("Component")
        .Field("m_worldSize",
            &Shit::CameraComponent::m_worldSize, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "World Size", .tooltip = "可见世界范围（世界单位）"})
        .Field("m_zoom",
            &Shit::CameraComponent::m_zoom, "float")
        .Meta(Shit::FieldMeta{.displayName = "Zoom", .range = {0.1, 10}, .step = 0.1})
        .Field("m_priority",
            &Shit::CameraComponent::m_priority, "int")
        .Meta(Shit::FieldMeta{.displayName = "Priority", .tooltip = "渲染优先级（小值先画）"})
        .Field("m_viewportRatio",
            &Shit::CameraComponent::m_viewportRatio, "SDL_FRect")
        .Meta(Shit::FieldMeta{.displayName = "Viewport Ratio", .tooltip = "相对于逻辑分辨率的视口区域 (0~1)"})
        .Factory<CameraComponent>()
        .Register<CameraComponent>();
    return true;
}

} // namespace Shit
