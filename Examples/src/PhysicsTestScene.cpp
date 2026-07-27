#include "PhysicsTestScene.h"
#include <ShitEngine/Core/Log.h>

#include <ShitEngine/Physics/PhysicsSystem2D.h>
#include <ShitEngine/Physics/RigidBody2D.h>
#include <ShitEngine/Physics/BoxCollider2D.h>
#include <ShitEngine/Physics/CircleCollider2D.h>
#include <ShitEngine/Component/TransformComponent.h>
#include <ShitEngine/Component/CameraComponent.h>
#include <ShitEngine/Component/SpriteRenderer.h>
#include <ShitEngine/Render/Renderer.h>

namespace {
	constexpr const char* kBoxTex = "resource/grass_side.png";
}

std::unique_ptr<Shit::Scene> createPhysicsTestScene() {
	auto scene = std::make_unique<Shit::Scene>("PhysicsTest");
	scene->init();
	scene->registerSystem<Shit::PhysicsSystem2D>();

	auto* physics = scene->getSystem<Shit::PhysicsSystem2D>();
	physics->setGravity({ 0.0f, 500.0f });

	// ── 相机（居中观察物理区域） ──
	{
		auto* cam = scene->createGameObject("Camera");
		cam->addComponent<Shit::TransformComponent>()->setPosition({ 400, 350 });
		cam->addComponent<Shit::CameraComponent>();
	}

	// ── 地面 ──
	{
		auto* go = scene->createGameObject("Ground");
		go->addComponent<Shit::TransformComponent>()->setPosition({ 400, 680 });
		go->addComponent<Shit::RigidBody2D>();	// static 默认
		go->addComponent<Shit::BoxCollider2D>(Shit::Vector2{750, 30});
	}

	// ── 左墙 ──
	{
		auto* go = scene->createGameObject("LeftWall");
		go->addComponent<Shit::TransformComponent>()->setPosition({ 25, 300 });
		go->addComponent<Shit::RigidBody2D>();
		go->addComponent<Shit::BoxCollider2D>(Shit::Vector2{30, 600});
	}

	// ── 右墙 ──
	{
		auto* go = scene->createGameObject("RightWall");
		go->addComponent<Shit::TransformComponent>()->setPosition({ 775, 300 });
		go->addComponent<Shit::RigidBody2D>();
		go->addComponent<Shit::BoxCollider2D>(Shit::Vector2{30, 600});
	}

	// ── 动态盒子 1 ──
	{
		auto* go = scene->createGameObject("Box1");
		go->addComponent<Shit::TransformComponent>()->setPosition({ 350, 100 });
		auto* body = go->addComponent<Shit::RigidBody2D>();
		body->setBodyType(Shit::RigidBody2D::Type::Dynamic);
		go->addComponent<Shit::BoxCollider2D>(Shit::Vector2{60, 60});

		auto* sr = go->addComponent<Shit::SpriteRenderer>();
		sr->setTexturePath(kBoxTex);
	}

	// ── 动态盒子 2 ──
	{
		auto* go = scene->createGameObject("Box2");
		go->addComponent<Shit::TransformComponent>()->setPosition({ 450, 50 });
		auto* body = go->addComponent<Shit::RigidBody2D>();
		body->setBodyType(Shit::RigidBody2D::Type::Dynamic);
		go->addComponent<Shit::BoxCollider2D>(Shit::Vector2{50, 50});

		auto* sr = go->addComponent<Shit::SpriteRenderer>();
		sr->setTexturePath(kBoxTex);
	}

	// ── 圆形（弹跳球） ──
	{
		auto* go = scene->createGameObject("Ball");
		go->addComponent<Shit::TransformComponent>()->setPosition({ 550, 80 });
		auto* body = go->addComponent<Shit::RigidBody2D>();
		body->setBodyType(Shit::RigidBody2D::Type::Dynamic);
		go->addComponent<Shit::CircleCollider2D>(24.0f);

		auto* sr = go->addComponent<Shit::SpriteRenderer>();
		sr->setTexturePath(kBoxTex);
	}

	ST_CORE_INFO("[PhysicsTestScene] 场景已创建：地面 + 墙壁 + 2 个盒子 + 1 个圆形");
	return scene;
}
