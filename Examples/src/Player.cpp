#include "Player.h"

void Player::onStart() {
    transform = getOwner()->getComponent<Shit::TransformComponent>();
}

void Player::onUpdate() {
    Shit::Vector2 pos = transform->getPosition();

    // 优先用 settings.json 的轴映射；没有配置时仍可退回原始 WASD
    float h = Shit::Input::GetAxis("Horizontal");
    float v = Shit::Input::GetAxis("Vertical");

    if (h == 0.0f && v == 0.0f) {
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::A)) h = -1.0f;
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::D)) h = 1.0f;
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::W)) v = 1.0f;
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::S)) v = -1.0f;
    }

    // 轴约定：Vertical 正方向 = 上（W），屏幕 Y 向下，所以减 y
    pos.x += h * speed * Shit::Time::GetDeltaTime();
    pos.y -= v * speed * Shit::Time::GetDeltaTime();
    transform->setPosition(pos);
}
