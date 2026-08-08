#pragma once

#include <QString>
#include <Qt>   // Qt::MouseButton

#include <SDL3/SDL_scancode.h>

/// 编辑器内共享的 Qt ↔ SDL 键码工具（播放态输入转发 / 项目设置页按键捕获共用，
/// 保证「显示名 = 转发名 = 存储名」一致）。

/// Qt 键盘键 → SDL 扫描码；未识别返回 SDL_SCANCODE_UNKNOWN（调用方应丢弃）
SDL_Scancode sdlScancodeForQtKey(int qtKey);

/// SDL 扫描码 → SDL 官方 scancode 名（"Space"/"Left Shift"）；未知返回空串
QString sdlKeyName(SDL_Scancode sc);

/// Qt 键盘键 → SDL 官方 scancode 名；未识别返回空串
QString sdlKeyNameForQtKey(int qtKey);

/// Qt 鼠标钮 → 引擎绑定键名（"MouseButton.Left" 等）；无效返回空串
QString mouseBindingName(Qt::MouseButton button);