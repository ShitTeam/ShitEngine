#pragma once
#include <ShitEngine.h>
#include <memory>

/// @brief 构建 2D 物理演示场景
/// @details
///   - 静态地面（BoxCollider2D）
///   - 一个降落并堆叠的动态盒子
///   - 一个滚落的圆形
std::unique_ptr<Shit::Scene> createPhysicsTestScene();
