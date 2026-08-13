#pragma once

#include <ShitEngine/Render/RenderSystem.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_RenderSystem() {
    Shit::ReflectType("RenderSystem", sizeof(RenderSystem))
        .Base("System")
        .Factory<RenderSystem>()
        .Register<RenderSystem>();
    return true;
}

} // namespace Shit
