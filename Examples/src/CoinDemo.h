#pragma once
// ═══════════════════════════════════════════════════════════════
// 碰撞回调 + AudioSource + UI 计数 演示行为（P19 玩法回路）
//   BallDemo   — 小球：onStart 给水平初速滚过地面，记录 onCollisionEnter
//   CoinPickup — 金币：被碰 → 播拾取音效（AudioSource）→ 计数 +1 → 销毁自己
// ═══════════════════════════════════════════════════════════════

#include <ShitEngine.h>

#include <string>

namespace coin_demo_helpers {

inline Shit::GameObject* findByName(Shit::Scene* scene, const std::string& name) {
    if (!scene || name.empty()) return nullptr;
    for (auto& go : scene->getGameObjects()) {
        if (go->getName() == name) return go.get();
    }
    return nullptr;
}

} // namespace coin_demo_helpers

/// 碰撞回调演示：小球 atStart 后水平滚动，路上每碰到对象都打印
class SHIT_REFLECT(BlackList) BallDemo : public Shit::Behavior {
    SHIT_REFLECT_BODY(BallDemo)
public:
    void onStart() override {
        if (auto* body = getOwner()->getComponent<Shit::RigidBody2D>()) {
            body->setLinearVelocity({ m_speed, 0.0f });
        }
    }

    void onCollisionEnter(Shit::GameObject* other) override {
        ST_INFO("[CoinDemo] Ball 碰到 {}（Enter）", other ? other->getName() : "?");
    }

    SHIT_META(({.displayName = "Speed", .tooltip = "水平初速度（像素/秒）"}))
    float m_speed = 180.0f;
};

/// 金币：被任何物体碰撞 → 音效 + 计数 + 销毁自己（演示 onCollisionEnter 回调）
class SHIT_REFLECT(BlackList) CoinPickup : public Shit::Behavior {
    SHIT_REFLECT_BODY(CoinPickup)
public:
    void onStart() override {
        auto* scene = getOwner()->getScene();
        if (auto* textGo = coin_demo_helpers::findByName(scene, m_scoreTextObject)) {
            m_text = textGo->getComponent<Shit::UIText>();
        }
        m_audio = getOwner()->getComponent<Shit::AudioSource>();
    }

    void onCollisionEnter(Shit::GameObject* other) override {
        if (m_collected) return;
        m_collected = true;
        if (m_audio) m_audio->play();                       // AudioSource：非 PlayOnStart，手动触发
        ++m_count;
        if (m_text) m_text->setText("金币: " + std::to_string(m_count));
        ST_INFO("[CoinDemo] 拾取金币 {}（共 {} 枚）", getOwner()->getName(), m_count);
        // 销毁自己：标记销毁（播放态走延时路径，回调内安全）
        if (auto* scene = getOwner()->getScene()) {
            scene->removeGameObject(getOwner());
        }
    }

    SHIT_META(({.displayName = "Score Text", .tooltip = "计数显示的 UIText 对象名"}))
    std::string m_scoreTextObject = "CoinText";

private:
    SHIT_META(Disable)
    Shit::UIText* m_text = nullptr;      ///< 运行时按名解析（自动跳过存档）
    SHIT_META(Disable)
    Shit::AudioSource* m_audio = nullptr;
    SHIT_META(Disable)
    int m_count = 0;
    SHIT_META(Disable)
    bool m_collected = false;
};