#pragma once
#include <ShitEngine.h>
#include <memory>

/// @brief Test scene for InputMapping system
/// @details Coverage:
///   - Action Down/Pressed/Released (Jump, Attack, Sprint, Interact)
///   - Axis values (Horizontal, Vertical movement)
///   - Live display of all input states via UIText
///   - Press keys to see state changes in real-time
std::unique_ptr<Shit::Scene> createInputMappingTestScene();