#pragma once

#include <ReflectionBehavior.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_ReflectionTestBehavior() {
    Shit::ReflectType("ReflectionTestBehavior", sizeof(ReflectionTestBehavior))
        .Base("Behavior")
        .Field("m_lineNames",
            &ReflectionTestBehavior::m_lineNames, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "行对象名（R0,R1,...）", .tooltip = "解析多条 UIText 对象显示测试结果"})
        .Field("m_results",
            &ReflectionTestBehavior::m_results, "int")
        .Field("m_lines",
            &ReflectionTestBehavior::m_lines, "int")
        .Factory<ReflectionTestBehavior>()
        .Register<ReflectionTestBehavior>();
    return true;
}

