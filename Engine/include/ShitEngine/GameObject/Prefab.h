#pragma once
#include "GameObject.h"
#include <functional>

namespace Shit {

class Scene;

/**
 * @brief 预制体，可重复生成相同配置的 GameObject
 *
 * 用法：
 *   auto enemyPrefab = Shit::Prefab::Build([](Shit::GameObject* go) {
 *       go->addComponent<Shit::TransformComponent>();
 *       go->addComponent<Shit::SpriteRenderer>()->setTexturePath("enemy.png");
 *   });
 *
 *   // 在场景中实例化
 *   auto* enemy = scene->instantiate(enemyPrefab);
 */
class Prefab {
public:
    using Builder = std::function<void(GameObject*)>;

    static Prefab Build(Builder builder) {
        Prefab p;
        p.m_builder = std::move(builder);
        return p;
    }

    void apply(GameObject* go) const {
        if (m_builder) m_builder(go);
    }

private:
    Prefab() = default;
    Builder m_builder;
};

} // namespace Shit
