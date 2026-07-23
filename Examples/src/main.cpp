#include <ShitEngine.h>
#include "Player.h"
#include "UITestScene.h"
#include "InputMappingTestScene.h"

#ifdef _WIN32
#include <Windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    if (Shit::Game::Init()) {
        // Choose which scene to run:
        //   - createUITestScene()         : UI system (Canvas/Image/Text/Button/TextBox)
        //   - createInputMappingTestScene() : InputMapping (action/axis)

        auto scene = createInputMappingTestScene();
        Shit::SceneManager::PushScene(std::move(scene));

        // If you want a camera (for 2D game rendering), uncomment:
        // auto* cameraGO = scene->createGameObject("MainCamera");
        // cameraGO->addComponent<Shit::TransformComponent>();
        // cameraGO->addComponent<Shit::CameraComponent>();

        Shit::Game::Run();
    }

    Shit::Game::Destroy();
    return 0;
}