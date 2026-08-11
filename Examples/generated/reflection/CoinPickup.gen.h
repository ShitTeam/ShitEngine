#pragma once

#include <CoinDemo.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_CoinPickup() {
    Shit::ReflectType("CoinPickup", sizeof(CoinPickup))
        .Base("Behavior")
        .Field("m_scoreText",
            &CoinPickup::m_scoreText, "Shit::ComponentRef<Shit::UIText>")
        .Ref("UIText")
        .Meta(Shit::FieldMeta{.displayName = "Score Text", .tooltip = "计数显示的 UIText 引用（检查器拖拽设置）"})
        .Factory<CoinPickup>()
        .Register<CoinPickup>();
    return true;
}

