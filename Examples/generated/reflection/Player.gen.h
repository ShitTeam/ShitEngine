#pragma once

#include <Player.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_Player() {
    Shit::ReflectType("Player", sizeof(Player))
        .Base("Behavior")
        .Field("speed",
            &Player::speed, "float")
        .Meta(Shit::FieldMeta{.displayName = "Speed", .tooltip = "移动速度（像素/秒）", .range = {0, 1000}, .step = 10})
        .Factory<Player>()
        .Register<Player>();
    return true;
}

