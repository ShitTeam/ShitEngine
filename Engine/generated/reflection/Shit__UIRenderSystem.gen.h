#pragma once

#include <ShitEngine/UI/UIRenderSystem.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UIRenderSystem() {
    Shit::ReflectType("UIRenderSystem", sizeof(UIRenderSystem))
        .Base("System")
        .Factory<UIRenderSystem>()
        .Register<UIRenderSystem>();
    return true;
}

} // namespace Shit
