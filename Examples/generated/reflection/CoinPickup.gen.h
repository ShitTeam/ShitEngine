#pragma once

#include <CoinDemo.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_CoinPickup() {
    Shit::ReflectType("CoinPickup", sizeof(CoinPickup))
        .Base("Behavior")
        .Field("m_scoreTextObject",
            &CoinPickup::m_scoreTextObject, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Score Text", .tooltip = "计数显示的 UIText 对象名"})
        .Factory<CoinPickup>()
        .Register<CoinPickup>();
    return true;
}

