#pragma once
#include <ShitEngine.h>

/// 玩家控制器（WASD / 轴映射移动）——数据驱动示例：
/// SHIT_REFLECT 使组件可在检查器编辑、随 .scene 序列化
/// （此前为 P6 迁移遗留的未反射类，扫描器不登记 → 编辑器不可见）
class SHIT_REFLECT(BlackList) Player : public Shit::Behavior {
    SHIT_REFLECT_BODY(Player)
private:
    SHIT_META(Disable)
    Shit::TransformComponent* transform {nullptr};   ///< 运行时按需解析（自动跳过存档）

    SHIT_META(({.displayName = "Speed", .tooltip = "移动速度（像素/秒）", .range = {0, 1000}, .step = 10}))
    float speed {200.0f};

public:
    void onStart() override;
    void onUpdate() override;
};