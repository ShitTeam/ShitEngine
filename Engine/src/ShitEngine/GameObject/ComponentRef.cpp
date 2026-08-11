#include "ShitEngine/Core/pch.h"
#include "ShitEngine/GameObject/ComponentRef.h"

#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Scene/SceneManager.h"

namespace Shit {

Component* ComponentRefLookup(uint64_t uuid) {
    if (uuid == 0) return nullptr;
    Scene* scene = SceneManager::GetCurrentScene();
    if (!scene) return nullptr;
    return scene->componentByUuid(uuid);
}

} // namespace Shit