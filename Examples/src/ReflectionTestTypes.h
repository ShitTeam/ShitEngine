#pragma once

#include <ShitEngine.h>

#include <string>

// ══════════════════════════════════════════════════════
// 可反射的测试类型定义（放在头文件中供 ReflectionScanner 扫描）
// ══════════════════════════════════════════════════════

// Fields 模式：反射所有 public 字段
SHIT_STRUCT(TestPlayer, Fields)
{
	SHIT_REFLECT(TestPlayer)
public:
	int         m_hp    = 100;
	float       m_speed = 5.0f;
	std::string m_name;
};

// WhiteListFields 模式：只反射 SHIT_META(Enable) 字段
SHIT_STRUCT(TestEnemy, WhiteListFields)
{
	SHIT_REFLECT(TestEnemy)

	SHIT_META(Enable)
	float m_health = 50.0f;

	SHIT_META(Enable)
	int m_damage = 10;

	// 这个不会被反射（META 未标记）
	int m_internalId = 0;
};
