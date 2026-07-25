#pragma once
#include <ShitEngine.h>
#include <memory>

/// @brief Test scene for the reflection system (TypeRegistry + ReflectType)
/// @details Coverage:
///   - Reflection-annotated class definition (SHIT_REFLECT / SHIT_PROPERTY)
///   - Manual type registration via ReflectType() builder
///   - Query by name (TypeRegistry::Get("TestPlayer"))
///   - Query by type_index (TypeRegistry::Get<TestPlayer>())
///   - Iterate all registered types (ForEach + Count)
///   - Register a derived type with base class relationship
///   - Verify builtin types (int / float / std::string) are registered
std::unique_ptr<Shit::Scene> createReflectionTestScene();
