#pragma once

#include <ShitEngine.h>

#include <string>

// ══════════════════════════════════════════════════════
// 可反射的测试类型定义（放在头文件中供 ReflectionScanner 扫描）
// ══════════════════════════════════════════════════════

// Fields 模式：反射所有 public 字段
struct SHIT_REFLECT(BlackList) TestPlayer
{
	SHIT_REFLECT_BODY(TestPlayer)
public:
	SHIT_META(({.displayName = "HP", .tooltip = "Player hit points", .range = {0, 9999}}))
	int         m_hp    = 100;

	SHIT_META(({.displayName = "Move Speed", .range = {0, 20}, .step = 0.5, .unit = "m/s"}))
	float       m_speed = 5.0f;

	std::string m_name;
};

// WhiteList 模式：只反射 SHIT_META(Enable) 字段
struct SHIT_REFLECT(WhiteList) TestEnemy
{
	SHIT_REFLECT_BODY(TestEnemy)

	SHIT_META(Enable)
	float m_health = 50.0f;

	SHIT_META(Enable)
	int m_damage = 10;

	// 这个不会被反射（META 未标记）
	int m_internalId = 0;
};

// 枚举反射
enum class SHIT_ENUM(TestDirection) TestDirection {
    None = 0,
    Left = 1,
    Right = 2,
    Up = 4,
    Down = 8
};
