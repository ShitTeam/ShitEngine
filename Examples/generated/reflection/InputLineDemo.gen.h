#pragma once

#include <Behaviors.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_InputLineDemo() {
    Shit::ReflectType("InputLineDemo", sizeof(InputLineDemo))
        .Base("Behavior")
        .Field("m_lineNames",
            &InputLineDemo::m_lineNames, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "行对象名（L0,L1,...）", .tooltip = "解析多条 UIText 对象用于显示状态"})
        .Field("m_lines",
            &InputLineDemo::m_lines, "int")
        .Factory<InputLineDemo>()
        .Register<InputLineDemo>();
    return true;
}

