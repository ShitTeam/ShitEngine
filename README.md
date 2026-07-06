<div align="center">
  <img src="logo.png" alt="ShitEngine Logo" width="256"/>
  <h1>ShitEngine</h1>
  <p><strong>基于 C++20 和 SDL3 的轻量级 2D 游戏引擎</strong></p>
</div>

## 概述

ShitEngine 是一个从零开始设计的轻量级 2D 游戏引擎，采用面向对象的组件化架构（Component + System）。引擎基于 SDL3 渲染，内置完整的生命周期管理、场景栈、资源缓存、多相机渲染管线等基础设施，适合 2D 像素风游戏的快速原型开发和学习研究。

## 特性

- **组件化架构** — `GameObject` 挂载 `Component`，`System` 负责更新逻辑，职责清晰
- **多相机渲染管线** — 支持分屏、多视口、比例视口（类似 SFML 的 Viewport），等比 Letterbox 不变形
- **全局逻辑分辨率** — 固定 1280x720 逻辑坐标，SDL3 自动缩放适配任意窗口
- **像素完美渲染** — 最近邻缩放 + 像素对齐，像素风无模糊无抖动
- **场景栈管理** — 场景推入/弹出/替换，支持延迟操作，安全切换
- **资源自动缓存** — 纹理自动管理与引用缓存
- **完整输入系统** — 键盘和鼠标的 Down/Pressed/Released 三态检测
- **日志系统** — 基于 spdlog 的多级日志，引擎/用户日志分离
- **JSON 配置** — 通过 `settings.json` 配置窗口、帧率等参数

## 架构

```
Game
├── Window          SDL3 窗口管理
├── Renderer        SDL3 渲染器封装（逻辑分辨率、绘制 API）
├── Time            DeltaTime / TotalTime
├── Input           键盘 & 鼠标三态输入
├── Config          JSON 配置读取
├── ResourceManager 纹理等资源缓存
├── SceneManager    场景栈
│   └── Scene
│       ├── BehaviorSystem  执行 Behavior 生命周期
│       ├── RenderSystem    多相机渲染管线
│       ├── GameObject
│       │   ├── TransformComponent
│       │   ├── RendererComponent（渲染组件基类）
│       │   │   ├── SpriteRenderer
│       │   │   └── CameraComponent
│       │   ├── AnimationComponent
│       │   └── Behavior（用户继承）
```

## 快速开始

### 环境要求

- C++20 编译器（GCC 10+、Clang 11+、MSVC 2019+）
- CMake 3.20+

> 第三方依赖（SDL3、spdlog、glm 等）由 CMake 自动拉取或在预编译包中自带。

### 方式一：CMake FetchContent（推荐）

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame)

include(FetchContent)
FetchContent_Declare(
    ShitEngine
    GIT_REPOSITORY https://github.com/ShitTeam/ShitEngine.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ShitEngine)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE ShitEngine)
```

所有依赖由 CMake 自动拉取，无需手动安装。

### 方式二：add_subdirectory（本地源码）

```cmake
add_subdirectory("path/to/ShitEngine" "${CMAKE_BINARY_DIR}/ShitEngine_Build")
target_link_libraries(MyGame PRIVATE ShitEngine)
```

### 方式三：find_package（预编译库）

从 GitHub Release 下载解压后，artifact 结构如下：

```
ShitEngine/
├── bin/          # ShitEngine.dll (Release) + ShitEngine-d.dll (Debug) + 第三方 DLL
├── lib/          # 导入库 + ShitEngineConfig.cmake
└── include/      # 头文件
```

在你的 CMakeLists.txt 中：

```cmake
find_package(ShitEngine REQUIRED
    PATHS /path/to/ShitEngine/lib/cmake
    NO_DEFAULT_PATH)
add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE ShitEngine::ShitEngine)
```

Config.cmake 会根据 `-DCMAKE_BUILD_TYPE=Debug` 或 `Release` 自动选择对应的库文件（Debug 版本文件名带 `-d` 后缀）。

预编译包自带第三方动态库，无需额外安装 SDL3 等依赖。

> **Linux 用户**：运行前需设置动态库路径：
> ```bash
> export LD_LIBRARY_PATH=/path/to/ShitEngine/lib:$LD_LIBRARY_PATH
> ```

### 示例代码

```cpp
#include <ShitEngine.h>

class Player : public Shit::Behavior {
    Shit::TransformComponent* transform;
    float speed = 200.0f;

    void onStart() override {
        transform = getOwner()->getComponent<Shit::TransformComponent>();
    }

    void onUpdate() override {
        Shit::Vector2 pos = transform->getPosition();
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::W)) pos.y -= speed * Shit::Time::GetDeltaTime();
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::S)) pos.y += speed * Shit::Time::GetDeltaTime();
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::A)) pos.x -= speed * Shit::Time::GetDeltaTime();
        if (Shit::Input::IsKeyPressed(Shit::KeyCode::D)) pos.x += speed * Shit::Time::GetDeltaTime();
        transform->setPosition(pos);
    }
};

int main() {
    if (Shit::Game::Init()) {
        auto scene = std::make_unique<Shit::Scene>("example");
        scene->init();

        auto go = std::make_unique<Shit::GameObject>("player");
        go->addComponent<Shit::TransformComponent>();
        go->addComponent<Shit::SpriteRenderer>()->setTexturePath("textures/player.png");
        go->addComponent<Player>();
        scene->addGameObject(std::move(go));

        auto camera = std::make_unique<Shit::GameObject>("camera");
        camera->addComponent<Shit::TransformComponent>();
        camera->addComponent<Shit::CameraComponent>();
        scene->addGameObject(std::move(camera));

        Shit::SceneManager::PushScene(std::move(scene));
        Shit::Game::Run();
    }
    Shit::Game::Destroy();
}
```

### 配置

创建 `settings.json` 与可执行文件同目录：

```json
{
    "project": "MyGame",
    "window": {
        "title": "My Game",
        "width": 1024,
        "height": 720,
        "targetFPS": 144
    }
}
```

## 多相机分屏

```cpp
// 左半屏相机（世界大小 200x200）
camera1->getComponent<Shit::CameraComponent>()->setViewportRatio({0.0f, 0.0f, 0.5f, 1.0f});

// 右半屏相机（世界大小 100x100）
camera2->getComponent<Shit::CameraComponent>()->setViewportRatio({0.5f, 0.0f, 0.5f, 1.0f});
```

## 更多

- [GitHub 仓库](https://github.com/ShitTeam/ShitEngine) — 源代码和 Issues
- [文档网站](https://engine.shitteam.top) — API 参考和教程

## 许可证

Apache License 2.0
