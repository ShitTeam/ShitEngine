#pragma once

#include <ShitEngine/Physics/Joint2D.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Joint2D() {
    Shit::ReflectType("Joint2D", sizeof(Joint2D))
        .Base("Component")
        .Field("m_type",
            &Shit::Joint2D::m_type, "JointType")
        .Meta(Shit::FieldMeta{.displayName = "Type", .tooltip = "Distance=距离, Revolute=铰链, Weld=焊接, Prismatic=滑动"})
        .Field("m_connectedBody",
            &Shit::Joint2D::m_connectedBody, "ComponentRef<RigidBody2D>")
        .Ref("RigidBody2D")
        .Meta(Shit::FieldMeta{.displayName = "Connected Body", .tooltip = "另一端刚体（bodyB）；留空则暂不创建"})
        .Field("m_anchor",
            &Shit::Joint2D::m_anchor, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Anchor", .tooltip = "世界锚点（像素）"})
        .Field("m_length",
            &Shit::Joint2D::m_length, "float")
        .Meta(Shit::FieldMeta{.displayName = "Length", .tooltip = "距离关节自然长度（像素）", .range = {0, 4096}, .step = 1})
        .Field("m_enableSpring",
            &Shit::Joint2D::m_enableSpring, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Spring", .tooltip = "作为弹簧（false=刚性距离）"})
        .Field("m_hertz",
            &Shit::Joint2D::m_hertz, "float")
        .Meta(Shit::FieldMeta{.displayName = "Hertz", .tooltip = "弹簧刚度 (Hz)", .range = {0, 60}, .step = 0.1})
        .Field("m_dampingRatio",
            &Shit::Joint2D::m_dampingRatio, "float")
        .Meta(Shit::FieldMeta{.displayName = "Damping Ratio", .tooltip = "弹簧阻尼比", .range = {0, 2}, .step = 0.01})
        .Field("m_enableMotor",
            &Shit::Joint2D::m_enableMotor, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Motor", .tooltip = "启用电机"})
        .Field("m_motorSpeed",
            &Shit::Joint2D::m_motorSpeed, "float")
        .Meta(Shit::FieldMeta{.displayName = "Motor Speed", .tooltip = "目标角速度（度/秒）", .range = {-720, 720}, .step = 1})
        .Field("m_maxMotorTorque",
            &Shit::Joint2D::m_maxMotorTorque, "float")
        .Meta(Shit::FieldMeta{.displayName = "Max Motor Torque", .tooltip = "最大电机扭矩", .range = {0, 100000}, .step = 1})
        .Field("m_enableLimit",
            &Shit::Joint2D::m_enableLimit, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Limit", .tooltip = "启用角度限制"})
        .Field("m_lowerAngle",
            &Shit::Joint2D::m_lowerAngle, "float")
        .Meta(Shit::FieldMeta{.displayName = "Lower Angle", .tooltip = "角度下限（度）", .range = {-360, 360}, .step = 1})
        .Field("m_upperAngle",
            &Shit::Joint2D::m_upperAngle, "float")
        .Meta(Shit::FieldMeta{.displayName = "Upper Angle", .tooltip = "角度上限（度）", .range = {-360, 360}, .step = 1})
        .Field("m_axisAngle",
            &Shit::Joint2D::m_axisAngle, "float")
        .Meta(Shit::FieldMeta{.displayName = "Axis Angle", .tooltip = "滑动轴方向（度，相对 bodyA 局部）", .range = {-360, 360}, .step = 1})
        .Field("m_lowerTranslation",
            &Shit::Joint2D::m_lowerTranslation, "float")
        .Meta(Shit::FieldMeta{.displayName = "Lower Translation", .tooltip = "滑动下限（像素）", .range = {-4096, 4096}, .step = 1})
        .Field("m_upperTranslation",
            &Shit::Joint2D::m_upperTranslation, "float")
        .Meta(Shit::FieldMeta{.displayName = "Upper Translation", .tooltip = "滑动上限（像素）", .range = {-4096, 4096}, .step = 1})
        .Field("m_maxMotorForce",
            &Shit::Joint2D::m_maxMotorForce, "float")
        .Meta(Shit::FieldMeta{.displayName = "Max Motor Force", .tooltip = "最大电机力", .range = {0, 100000}, .step = 1})
        .Factory<Joint2D>()
        .Register<Joint2D>();
    return true;
}

} // namespace Shit
