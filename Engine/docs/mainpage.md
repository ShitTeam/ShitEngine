# ShitEngine API 参考

> 基于 C++20 与 SDL3 的轻量级 2D 游戏引擎

本站是 ShitEngine 的 **API 参考文档**（由 Doxygen 从源码自动生成）。
引擎的使用手册与教程在主文档站：[**ShitEngine 文档 →**](https://engine.shitteam.top)

---

## 五分钟上手

一个可运行的最小工程只需三步——初始化、运行、销毁：

```cpp
#include <ShitEngine.h>

int main() {
    Shit::Game::Init();    // 日志/窗口/渲染/输入/音频等子系统一次性就绪
    Shit::Game::Run();     // 主循环（事件 → 更新 → 渲染）
    Shit::Game::Destroy(); // 按初始化的逆序清理
}
```

游戏逻辑写在 **Behavior 脚本**里（类似 Unity 的 MonoBehaviour），挂到场景对象上即可被自动驱动：

```cpp
class PlayerMove : public Shit::Behavior {
    void onStart() override  { /* 首帧前调用一次 */ }
    void onUpdate() override {
        // 每帧调用：动作映射查询输入，移动自身 Transform
        float axis = Shit::Input::GetAxis("Horizontal");
        auto* t = getOwner()->getComponent<Shit::TransformComponent>();
        t->setPosition(t->getPosition() + Shit::Vector2{ axis * 5.0f, 0 });
    }
};
```

组件通过 `GameObject::addComponent<T>()` 挂载、`getComponent<T>()` 取回；
场景以 `.scene` 文件数据驱动加载（`SceneManager::LoadSceneFromFile`），
自定义脚本编译为插件 DLL 后即可被 Runtime 与编辑器实例化。

## 新人阅读路径

1. [Game](\ref Shit::Game) —— 引擎生命周期与主循环
2. [GameObject](\ref Shit::GameObject) / [Component](\ref Shit::Component) —— 组件容器模型
3. [Behavior](\ref Shit::Behavior) —— 脚本入口（onStart / onUpdate / 碰撞回调）
4. [SceneManager](\ref Shit::SceneManager) 与 [Scene](\ref Shit::Scene) —— 场景切换与数据驱动加载
5. 之后按需查阅下方模块速查表

## 模块速查表

| 模块 | 职责 | 关键类 |
|------|------|--------|
| 核心框架 | 引擎主循环、窗口、时间、日志、配置 | [Game](\ref Shit::Game)、[EngineContext](\ref Shit::EngineContext)、[Window](\ref Shit::Window)、[Time](\ref Shit::Time) |
| 对象与组件 | 组件容器与内置组件 | [GameObject](\ref Shit::GameObject)、[SpriteRenderer](\ref Shit::SpriteRenderer)、[CameraComponent](\ref Shit::CameraComponent)、[Tilemap](\ref Shit::Tilemap) |
| 场景 | 场景持有 / 切换 / 序列化 | [Scene](\ref Shit::Scene)、[SceneManager](\ref Shit::SceneManager)、[SceneSerializer](\ref Shit::SceneSerializer) |
| 行为脚本 | 用户脚本驱动 | [Behavior](\ref Shit::Behavior)、[BehaviorSystem](\ref Shit::BehaviorSystem) |
| 渲染 | 多相机管线与精灵绘制 | [RenderSystem](\ref Shit::RenderSystem)、[Renderer](\ref Shit::Renderer)、[Sprite](\ref Shit::Sprite) |
| 动画 | 状态机与帧动画剪辑 | [Animator](\ref Shit::Animator)、[AnimationClip](\ref Shit::AnimationClip)、[AnimationComponent](\ref Shit::AnimationComponent) |
| 音频 | 分层增益播放 | [AudioPlayer](\ref Shit::AudioPlayer)、[AudioSource](\ref Shit::AudioSource)、[AudioTrack](\ref Shit::AudioTrack) |
| 输入 | 键鼠三态 + 动作/轴映射 | [Input](\ref Shit::Input)、[KeyCode](\ref Shit::KeyCode) |
| 事件 | 缓冲队列事件总线 | [EventBus](\ref Shit::EventBus)、[Event](\ref Shit::Event) |
| 资源 | 纹理/字体/音频缓存 | [ResourceManager](\ref Shit::ResourceManager)、[Texture](\ref Shit::Texture)、[Font](\ref Shit::Font)、[Audio](\ref Shit::Audio) |
| 物理 | Box2D 2D 物理封装 | [PhysicsSystem2D](\ref Shit::PhysicsSystem2D)、[RigidBody2D](\ref Shit::RigidBody2D)、[Joint2D](\ref Shit::Joint2D) |
| UI | Canvas / 控件体系 | [UICanvas](\ref Shit::UICanvas)、[UIText](\ref Shit::UIText)、[UIButton](\ref Shit::UIButton) |
| 反射 | 运行时类型信息与编辑器绑定 | [TypeRegistry](\ref Shit::TypeRegistry)、[TypeInfo](\ref Shit::TypeInfo)、反射宏 |
| 插件 | 脚本 DLL 动态加载 | [PluginManager](\ref Shit::PluginManager) |
| 数学 | 向量 / 矩阵（glm 别名）与矩形 | `Vector2`（glm::vec2 别名）、[Rect](\ref Shit::Rect) |

> 各模块的完整类列表见左侧导航树；顶部搜索框可直接定位任意符号。

---

*此 API 文档由 Doxygen 自动生成，随每次提交更新。*
