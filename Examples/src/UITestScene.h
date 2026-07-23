#pragma once
#include <ShitEngine.h>
#include <memory>

/// @brief Build a test scene that covers all UI subsystems
/// @details Coverage:
///   - UICanvas + UITransform anchor positioning (center / stretch / parent chain)
///   - UIImage rendering and button color tint
///   - UIText rendering with setText dynamic update
///   - UIButton hover/pressed color transitions and onClick callback
///   - UITextBox single-line input with placeholder, cursor, character limit
std::unique_ptr<Shit::Scene> createUITestScene();
