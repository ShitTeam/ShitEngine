#pragma once

#include <Behaviors.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_PlayerAnimatorController() {
    Shit::ReflectType("PlayerAnimatorController", sizeof(PlayerAnimatorController))
        .Base("Behavior")
        .Factory<PlayerAnimatorController>()
        .Register<PlayerAnimatorController>();
    return true;
}

