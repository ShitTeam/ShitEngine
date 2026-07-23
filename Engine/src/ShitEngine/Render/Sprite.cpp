#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Render/Sprite.h"
#include "ShitEngine/Render/SpriteSheet.h"

namespace Shit {
    Sprite::Sprite() = default;

    void Sprite::setFrame(const SpriteSheet& sheet, int frameIndex) {
        m_sourceRect = sheet.getFrameRect(frameIndex);
    }
}
