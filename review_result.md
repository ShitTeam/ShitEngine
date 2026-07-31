# 审查结果

## 确认的 Bug（23 条）

### [0] HIGH Engine/src/ShitEngine/Component/AnimationComponent.cpp:58
**addAnimation 覆盖正在播放的同名动画导致 m_currentAnimation 悬垂 use-after-free**

详情：m_animations[animationName] = std::move(animation) 会析构 map 中已存在的旧 Animation。若 animationName == m_currentAnimationName（即 m_currentAnimation 正指向它），旧 Animation 对象被销毁后 m_currentAnimation 变成悬垂指针，下一次 onUpdate 调用 m_currentAnimation->isLooping() 即为 use-after-free。play(name, sheet, ...)（第84-85行）覆盖 map 后立刻重新赋值 m_currentAnimation，是安全的；addAnimation 没有这一步，存在缺陷。

建议修复：addAnimation 在 map 赋值后，若 animationName == m_currentAnimationName，则重新指向 m_currentAnimation = m_animations[animationName].get();（与 play(name, sheet,...) 一致）。

### [1] HIGH Engine/src/ShitEngine/Core/Config.cpp:20
**loadFromJson 在 try/catch 之外调用，settings.json 字段类型不匹配时抛出未捕获异常导致启动崩溃**

详情：init() 中 file >> j 的解析在 try/catch(14-19行)内，但 loadFromJson(j)（第20行）在 try 块外。函数内多处无类型校验直接 get<>：.get<std::string>()（28/30/35行，遇数字/布尔/null 抛 type_error::302）、.get<unsigned int>()（38/42/46行，遇字符串/布尔/null 抛 type_error::302）、im["actions"].items() / im["axes"].items()（59/72行，遇非对象抛 type_error::302）、key.get<std::string>()（63/76/82行）。任一类型不匹配（如 "width":"1280"、"title":123、"actions":"Jump"）时异常穿透 Config::init → Game::init（Game.cpp:41）→ main()（Runtime/src/main.cpp:21 无 try/catch）→ std::terminate 崩溃。这正落在"Config 类型校验"关注点，先前修复只加了正数钳制，未处理类型错误。

建议修复：将 loadFromJson(j) 包进 try/catch（失败时 ST_CORE_WARN 并使用默认配置），或在每个 get<> 前用 is_string()/is_number_integer() 校验类型，跳过非法字段。建议在 loadFromJson 内统一用 checkedGet 辅助函数。

### [2] HIGH Engine/src/ShitEngine/Physics/PhysicsSystem2D.cpp:76
**旋转单位不一致：物理层用弧度写回 Transform，渲染层把同一值当角度传给 SDL**

详情：b2Rot_GetAngle(rot) 返回弧度，PhysicsSystem2D::update 直接 transform->setRotation(angle) 把弧度写入 TransformComponent；RigidBody2D::onAttach(第48行) 也用 b2MakeRot(transform->getRotation()) 把 Transform 的旋转当作弧度建刚体。但 SpriteRenderer::onRender(第53行) 把 transform->getRotation() 直接作为 SDL_RenderTextureRotated 的 angle 参数（SDL 规定该参数为度）。后果：任何物理驱动的旋转显示时被当作角度，视觉旋转比真实物理旋转小约 57.3 倍（看起来几乎不动）；用户按编辑器 .range={-360,360} 的度约定设 rotation=90 时，刚体实际被创建为旋转 90 弧度（约 2.04 rad）。默认 0 度不受影响，因此该 bug 只在用到旋转时暴露。

建议修复：统一单位约定（编辑器与 SDL 均为度）：创建刚体时 b2MakeRot(glm::radians(transform->getRotation()))，同步回写时 transform->setRotation(glm::degrees(b2Rot_GetAngle(rot)))；或在 SpriteRenderer 中把弧度转成度。

### [3] HIGH Engine/src/ShitEngine/System/BehaviorSystem.cpp:24
**BehaviorSystem::update 遍历 m_behaviors 期间同步注销会擦除正在遍历的元素，导致迭代器失效/悬垂**

详情：update() 用范围 for 遍历 m_behaviors 并执行用户代码 onStart()/onUpdate()，而 unregisterBehavior()（BehaviorSystem.cpp:60-63）直接对 m_behaviors 做 std::remove+erase，没有任何延迟或墓碑机制。两条真实触发路径：(a) Behavior 的 onStart/onUpdate 里调用 getOwner()->removeComponent<MyBehavior>()（GameObject.h:141-144 立即调 onDetach→Behavior::onDetach→unregisterBehavior），当前元素被 erase 后循环 ++it 解引用已释放内存；(b) 运行期 EventBus::ProcessEvents（Game::run 中先于 SceneManager::Update 执行）里 createGameObject+addComponent 的 GO 仍在 m_pendingAdditions，其 Behavior 当帧被 flush 进 m_behaviors 并执行 onUpdate，若 onUpdate 调用 scene->removeGameObject(getOwner())，Scene.cpp:127-130 会对 pending 对象立即 clean()→onDetach→unregisterBehavior，同样在遍历中擦除 m_behaviors。若 erase 的元素位于当前迭代器之前，vector erase 使当前位置之后的迭代器全部失效，同样 UB。后果：崩溃/堆破坏，或 onStart/onUpdate 被重复调用。

建议修复：仿照 registerBehavior 的延迟模式：unregisterBehavior 只把指针放入待注销列表（或打墓碑标记），在 update() 的遍历结束后统一清理 m_behaviors；遍历前也可先对 m_behaviors 快照/按下标索引并跳过空槽。同时 removeComponent 的生命周期调用应尽量延迟到帧末。

### [4] MEDIUM Engine/src/ShitEngine/Component/AnimationComponent.cpp:67
**play() 无法重放一个自然播完的同一非循环动画（不重置 m_currentTime）**

详情：m_currentTime 只在切换动画名时重置（if (m_currentAnimationName != name)）。非循环动画自然播完后，onUpdate 把 m_currentTime 停在 totalLen、m_isPlaying=false，但 m_currentAnimationName 不变。此时再次调用 play(同name)：m_currentTime 保持 totalLen，下一帧 onUpdate 立即命中 m_currentTime >= totalLen 分支，动画只显示一帧（末帧）即停止，相当于无法重播。stop() 后重播正常（stop 已清零 m_currentTime），所以只有自然播完这条路径会触发。

建议修复：在 play(name) 中当 m_isPlaying == false（或 m_currentTime 已到达总长）时把 m_currentTime 重置为 0，保证每次显式 play 都从头开始。

### [5] MEDIUM Engine/src/ShitEngine/Component/CameraComponent.cpp:73
**worldToScreen 与 screenToWorld 非互逆：分屏视口原点(vpX/vpY)只加在一侧**

详情：worldToScreen 按注释“修复：去掉 vpX/vpY”返回视口局部坐标（localOffsetX/Y 不含 viewportRatio.x/y）；而 screenToWorld 第102-103行仍使用 globalOffsetX/Y = vpX + (vpW-scaledW)/2，期望全局屏幕坐标。两者不是互逆变换：对 viewportRatio.x>0 或 y>0 的相机（引擎明确支持多相机分屏），worldToScreen(screenToWorld(p)) 得到 p - (vpX/ppu, vpY/ppu)。渲染管线（在 SDL viewport 内用局部坐标画）自洽，但任何依赖头文件注释所描述的互逆关系、或把 worldToScreen 输出当全局坐标用（如叠加到全屏 UI/调试绘制）的代码都会出现 vpX/vpY 偏移。

建议修复：统一两个函数使用同一坐标系：要么 worldToScreen 也加回 vpX/vpY（全局坐标），要么 screenToWorld 去掉 vpX/vpY（视口局部坐标），并在头文件明确各自期望的坐标空间。

### [6] MEDIUM Engine/src/ShitEngine/Core/TextInputGate.cpp:87
**聚焦的文本控件不消费键盘事件，Escape 失焦当帧游戏仍会收到 Escape 动作；编辑键也同时进入全局 Input 状态**

详情：Game::run() 中 Input::HandleEvent(event) 先于 TextInputGate::HandleEvent(event) 执行，且门控没有任何“消费”机制：KEY_DOWN 会无条件写入 Input::m_currentKeys。触发条件：1) 文本输入框聚焦时按 Escape，releaseFocus(m_focused) 完成失焦，但 Input::IsKeyPressed(Escape)/IsKeyDown(Escape) 当帧仍为 true，绑定到 Escape 的游戏动作（如暂停菜单）会在关闭输入框的同一帧触发；2) 聚焦时按 Backspace/Delete/方向键/Home/End，onKeyDown 处理编辑的同时这些键也进入 m_currentKeys，绑定到这些键的游戏动作（菜单导航、角色移动）会在打字时被误触发。这是输入路由层的真实行为缺陷，没有任何阻断/消费路径。

建议修复：在 Game::run() 中让 TextInputGate 先处理事件，并在 HasFocus() 时对已处理的按键（Escape 失焦、导航/编辑键）标记为已消费，令 Input::HandleEvent 跳过写入 m_currentKeys；或给 TextInputGate::handleEvent 增加返回 bool（是否消费），Input 据此跳过对应事件。

### [7] MEDIUM Engine/src/ShitEngine/GameObject/GameObject.cpp:98
**clean()/removeComponent 对 m_components 的遍历在生命周期回调内可重入，自我移除组件会造成迭代器失效与回调重复调用**

详情：clean()（GameObject.cpp:98-101）用范围 for 遍历 m_components 依次调 onDetach()/onDestroy()，且调用期间条目仍在 map 中；removeComponent<T>()（GameObject.h:141-144）也不做防重入。若某组件在其 onDetach/onDestroy（用户代码）中调用 getOwner()->removeComponent<SameType>()，会先把该条目 erase 掉（当前迭代器指向已擦除节点，随后 ++it 即 UB），同时该组件的 onDetach/onDestroy 被执行两次；removeComponent 内部先 onDetach 再 onDestroy 也存在同样的二次进入风险。触发场景：组件自毁/切换形态时在 onDestroy 里移除自身组件。

建议修复：在调用生命周期回调前先把组件指针从 m_components 取出（unique_ptr::extract / find+take），回调后再统一析构；clean 遍历时对已移除条目重新校验，或改为循环直到 map 空、每轮取出一个条目再调回调。

### [8] MEDIUM Engine/src/ShitEngine/Physics/PhysicsSystem2D.cpp:62
**Transform→物理体同步缺失：Kinematic 用 Transform 移动会被每帧物理回写覆盖，且无刚体传送 API**

详情：update() 只做物理→Transform 单向同步（Dynamic/Kinematic 每帧覆盖 transform 的位置/旋转）。用户常见做法——在 Behavior::onUpdate 里对 Kinematic 平台调用 transform->setPosition() 移动——会被下一帧物理回写立即还原为刚体的旧位置，平台不动；引擎也没有暴露 b2Body_SetTransform 等价的传送/移动 API（RigidBody2D 只有 applyForce/applyImpulse/setLinearVelocity）。Static 刚体虽被跳过不回写，但其碰撞体也不会跟随被移动的 Transform。所有三种类型在创建后都无法通过 Transform 改变刚体位置。

建议修复：在 RigidBody2D 上暴露 teleport/setTransform 接口，内部调用 b2Body_SetTransform（对 Dynamic/Kinematic 同时调用 b2Body_SetAwake），或在每帧步进前对 Kinematic 刚体做 Transform→body 同步。

### [9] MEDIUM Engine/src/ShitEngine/Reflection/TypeRegistry.cpp:119
**unregisterType 从 std::deque 中间 erase 后，m_nameMap/m_typeIndexMap 中指向后续元素的指针全部悬垂**

详情：TypeRegistry.h 注释（第69行）"deque 保证元素地址稳定"只对 push_back 成立；std::deque::erase 删除中间元素会使被删元素之后的元素引用/指针失效。unregisterType 第117-121行 erase 后，所有注册在被删类型之后的 TypeInfo 地址在 m_nameMap（第114行 erase 之前仍持有这些指针）和 m_typeIndexMap 中变为悬垂。之后任何 getType()/registerType()/forEach() 都会解引用悬垂指针 → UB（读已释放内存/错误数据）。当前仓库内无调用点（grep 仅见声明），但这是为插件卸载设计的公共 API，首次使用即破坏整个注册表。

建议修复：删除后重建索引：erase 前收集剩余 TypeInfo，重新填充 m_nameMap/m_typeIndexMap；或将存储改为 std::list（元素地址在 erase 时稳定）；或采用墓碑标记+延迟移除，避免中途移动元素。

### [10] MEDIUM Engine/src/ShitEngine/Scene/Scene.cpp:35
**系统 update 期间调用 registerSystem 会向 m_systems 尾部 push_back，使正在遍历的 vector 在扩容时失效**

详情：Scene::update() 用范围 for 遍历 m_systems（vector<System*>）执行 system->update()；而 registerSystem<T>()（Scene.h:71-72）立即 m_systems.push_back，且没有像 unregisterSystem 那样的延迟队列。若任意系统/Behavior 的 update 中动态注册新系统（例如行为里按条件开启技能系统），push_back 触发扩容时缓存的范围 for 迭代器与 end 全部悬垂，++it/解引用即 UB；即使未扩容，新系统也会被本帧末尾的 m_pendingActions 处理逻辑之外——实际是排在已捕获的 end 之后被跳过，行为不可预期。

建议修复：给注册也加延迟：新增 m_pendingAddSystems，在 Scene::update 的 systems 遍历结束后统一注册并重新排序；或在遍历时用下标+每次取 size() 的方式，注册的新系统下帧生效。

### [11] MEDIUM Engine/src/ShitEngine/Scene/SceneManager.cpp:43
**processPendingActions 处理动作期间再次 PushScene/PopScene 会修改正被遍历的 m_pendingActions**

详情：processPendingActions 用范围 for 遍历 m_pendingActions。处理 Pop/Clear/Replace 时会调用 Scene::destroy→go->clean()→组件 onDetach/onDestroy（用户代码），若其中再调用 SceneManager::PushScene/PopScene（如'场景结束跳下一场景'），m_pendingActions.push_back 在遍历中发生：扩容则迭代器悬垂→UB；未扩容则新动作排在已捕获的 end 之后，随后 m_pendingActions.clear()（SceneManager.cpp:62）将其静默丢弃，场景切换永远不执行。

建议修复：进入处理前先 swap 出本地队列副本（std::vector<PendingAction> local; local.swap(m_pendingActions);），对副本处理，期间新请求进入原队列下帧执行；或在循环内每次用下标重读 size() 支持追加。

### [12] MEDIUM Engine/src/ShitEngine/UI/UIRenderSystem.cpp:105
**渲染阶段和排序都直接解引用 renderer 裸指针，按钮 onClick 回调中立即 removeComponent 会留下悬空指针导致 use-after-free**

详情：m_uiRenderers 和本帧缓存的 visible 快照都持有 UIRendererComponent 裸指针。按钮处理循环（第 85 行 onPointerUp）会在渲染循环（第 104-107 行）之前执行用户 onClick 回调。若回调调用 GameObject::removeComponent<UIText/UIButton/UIImage>()（该 API 立即释放组件，而 destroy()/removeGameObject 是帧末延迟的，安全），则第 105 行 entry.renderer->getOwner() 和第 106 行 onRender 均解引用已释放内存；守卫 `if (!entry.renderer->getOwner())` 本身即是对悬空指针的访问，无法兜底。更糟的是该悬空指针会一直残留在 m_uiRenderers 里，下一帧第 25-28 行 stable_sort 的 lambda 和收集循环第 41 行 renderer->isVisible() 同样解引用它，必然崩溃。

建议修复：onDetach/unregisterUIRenderer 时把残留引用置空（或用 shared/weak 所有权），并在排序/渲染/收集前统一按身份校验指针仍存活；更彻底的做法是让 removeComponent 也像 destroy() 一样延迟到帧末清理。

### [13] MEDIUM Engine/src/ShitEngine/UI/UITextInput.cpp:103
**多行 UITextArea 无法用 Enter 插入换行：onTextInput 的控字符过滤与 TextInputGate 不转发 Return 两处相互矛盾，两条路径都是死代码**

详情：TextInputGate::handleEvent（TextInputGate.cpp 第 78-85 行）只把 Left/Right/Home/End/Backspace/Delete/Up/Down 转发给 onKeyDown，故意不转发 Return（注释称 'RETURN 由 TEXT_INPUT 处理'）；而 UITextInput::onTextInput 第 103 行把 firstByte<0x20（含 '\n' 0x0A、'\r' 0x0D，仅排除 '\t'）的事件整体拒绝，第 106 行专门处理 '\n' 的多行分支永远走不到。于是 UITextArea::onKeyDown 的 KeyCode::Return 分支（UITextArea.cpp 第 115-118 行 insertNewline）和 onTextInput 的 '\n' 分支都无法触发，聚焦的多行输入框按 Enter 完全没反应。

建议修复：在控字符门之前处理换行（或在 m_isMultiline 时放行 '\n'/'\r'），并/或把 Return 也加入 TextInputGate 的 navKeys 转发给 onKeyDown，二选一即可让换行插入生效。

### [14] LOW Engine/include/ShitEngine/Audio/AudioPlayer.h:47
**Play() 传入不存在的 group 名时静默退化为“无分组”播放，不注册、不套用分组增益，且无日志**

详情：静态 Play(filePath, group) 通过 getTrackGroup(group) 取组，组不存在时返回 nullptr，play() 将 nullptr 视为“无组”（分组增益取 1.0，且不 registerTrack）。用户若拼错组名（如 Play("bgm.ogg", "music") 而组叫 "Music"），声音照常播放但完全不受该组 setVolume/pauseAll/stopAll 影响，也没有任何警告，音量分层静默失效，符合“组不存在时不报错”的静默错误模式。

建议修复：在 Play()/play() 中当调用方显式传入组名但 getTrackGroup 返回 nullptr 时打 ST_CORE_WARN 提示组不存在；或提供带错误返回/可选创建组的语义。

### [15] LOW Engine/include/ShitEngine/GameObject/GameObject.h:91
**addComponent 在把组件插入 m_components 之前就执行 onAttach；若 onAttach 销毁了 owner，后续对已释放对象写 map**

详情：addComponent 流程：setOwner→onCreate（line 88）→onAttach（line 92）→ 才执行 m_components[type_index] = ...（line 96）。若组件自己的 onAttach/onCreate 里触发场景销毁本 GO（如 scene->removeGameObject(owner)，对 pending 对象会立即 clean→释放；或 owner->destroy() 后同帧被清理），第 96 行会向已释放/已清空的 GameObject 的 m_components 写入→堆破坏；且该组件因尚未插入 map，clean() 的 onDetach 循环看不到它，若它是 Behavior，已注册进 m_pendingBehaviors 的指针将成为悬垂指针。

建议修复：先插入 m_components 再执行 onCreate/onAttach（用裸指针持有返回值），或在 onAttach 之后重新校验 owner 的 m_needDestroy/是否仍存活，失效则回滚插入并释放组件。

### [16] LOW Engine/include/ShitEngine/GameObject/GameObject.h:93
**addComponent 无条件把 m_isRegistered 置 true，导致在 UIRenderSystem 注册之前添加的 UI 渲染组件永远不会被系统登记、永不渲染**

详情：GameObject::addComponent 调用 onAttach() 后无条件设置 new_component->m_isRegistered = true。UIRendererComponent::onAttach 只有在场景里能找到 UIRenderSystem 时才 registerUIRenderer 并置位；若组件在系统创建之前加入（例如场景系统稍后注册），m_isRegistered 被强制置 true，后续系统注册时不会再触发 onAttach，该 UIImage/UIText 等既不在 m_uiRenderers 中、也不参与命中检测。与 setScene（第 70-74 行只对未注册组件调 onAttach、不强制置位）行为不一致。

建议修复：删除 addComponent 里的强制置位，由 onAttach 报告实际注册结果；或在 UIRenderSystem 创建/注册时补扫一次场景中未注册的 UI 渲染组件。

### [17] LOW Engine/src/ShitEngine/Audio/AudioPlayer.cpp:155
**MIX_SetTrackAudio 的返回值被忽略，失败时后续 MIX_PlayTrack 报误导性错误**

详情：play() 中 MIX_SetTrackAudio(handle, audio) 的 bool 返回值未检查（SDL_mixer 3.x 该函数返回 bool 表示成功/失败，失败时调用 SDL_GetError）。若失败（例如 audio 指针与 mixer 不匹配等），track 没有任何输入，随后 MIX_PlayTrack 也会失败，但日志只报“播放音频失败”，无法区分是输入绑定失败还是播放失败，排障困难；且失败路径上 handle 已被正确销毁，无泄漏，属健壮性缺陷。

建议修复：检查 MIX_SetTrackAudio 的返回值，失败时立即 ST_CORE_ERROR 并 MIX_DestroyTrack(handle) 后 return nullptr。

### [18] LOW Engine/src/ShitEngine/Component/AnimationComponent.cpp:29
**非循环动画结束判断在时间递增之前，末帧多停留一帧、isPlaying 晚一帧才为 false**

详情：m_currentTime >= totalLen 的检查位于函数开头、m_currentTime += dt 之前。越过总长的那个 update 先执行 applyCurrentFrame（getFrame 把超出的时间 clamp 到最后一帧），要等下一个 update 才进入停止分支。因此非循环动画实际播完 totalLen + 一个 dt 才停，最后一帧被显示两个更新周期，isPlaying() 在动画真正结束后仍多返回一帧 true。

建议修复：把结束判断移到 m_currentTime += dt 之后，或在该帧 clamp 到 totalLen 后立即 m_isPlaying=false，使播放时长严格等于 totalLen。

### [19] LOW Engine/src/ShitEngine/Component/CameraComponent.cpp:68
**worldToScreen/screenToWorld 缺少 zoom<=0 与 worldSize<=0 防护，可产生除零/NaN 坐标**

详情：getPixelPerUnit() 有 worldSize/vp<=0 的防护，但 worldToScreen/screenToWorld 没有，且 setZoom()/setSize() 不加 clamp。m_zoom==0 时 screenToWorld 除以 finalPpu==0 得 ±inf；m_worldSize.x<=0 时 basePpu=vpW/0 为 inf、scaledW=0*inf 为 NaN，NaN 会流入 SpriteRenderer 的 SDL_FRect 造成不可预知渲染。属配置性输入（编辑器 range 只约束元数据），不会崩溃但产生无效坐标。

建议修复：setZoom 时 clamp 到大于 0 的极小值，并在 worldToScreen/screenToWorld 里像 getPixelPerUnit 一样在 worldSize/vp<=0 时返回安全值（如相机中心）或直接返回 0。

### [20] LOW Engine/src/ShitEngine/Core/Game.cpp:38
**Log::Init 非幂等，Game::Init 失败重试或 Destroy 后重新初始化必然失败**

详情：Log.cpp 第14/18行 spdlog::stdout_color_mt("Shit"/"App") 在同名 logger 已存在时抛 spdlog_ex（spdlog 全局 registry 按名查重）。因此若第一次 Game::Init 在 Log::Init 成功之后某步失败（如 SDL_Init 失败、Window::Init 失败），应用修复后重试 Game::Init 时第二次 Log::Init 抛异常被捕获返回 false，Game::Init 永远返回 false，引擎在单进程内无法恢复；Game::Destroy 之后再次 Game::Init 同样失败。m_isInited 标志在 init() 入口从未被检查，形同虚设。这正落在"初始化失败路径"关注点。

建议修复：Log::Init 开头加幂等守卫：if (s_CoreLogger && s_ClientLogger) return true;（或先用 spdlog::get(name) 检查）；Game::init 入口检查 if (m_isInited) return true;

### [21] LOW Engine/src/ShitEngine/GameObject/GameObject.cpp:72
**setScene() 与 System::init() 对未注册组件调用 onAttach 但从不置 m_isRegistered，与 addComponent 行为不一致**

详情：addComponent（GameObject.h:93）在 onAttach 后设置 m_isRegistered=true，而 GameObject::setScene（GameObject.cpp:70-75）和 System::init（System.cpp:11-22）调用 comp->onAttach() 后都不设置该标志。当前所有 GO 由 createGameObject 创建时立即 setScene，addComponent 时 m_scene 已非空，标志恒为 true，故暂时触发不了；但一旦出现 m_scene 为空的组件（如 Scene::addGameObject 运行期路径 Scene.cpp:83-85 直接把外部构造的 GO 放 pending 而不 setScene），setScene/System::init 会反复对该组件重复调用 onAttach，对 Behavior 即重复注册进 m_pendingBehaviors→onStart/onUpdate 每帧被调用两次。属于脆弱的隐式不变量。

建议修复：在 setScene 与 System::init 调用 onAttach 后同样置 m_isRegistered = true，或统一收敛到 Component 的一个 attachOnce() 入口；并让 registerBehavior 对重复注册去重。

### [22] LOW Engine/src/ShitEngine/UI/UIRenderSystem.cpp:85
**按钮按下后若在释放前离开 visible 集合（setVisible(false) 或 owner 被移除），m_isPressed 残留，下次释放会在按钮上误触发 onClick**

详情：按钮被 onPointerDown 置为 Pressed/m_isPressed=true 后，若在 mouseUp 前被移出 visible（例如 setVisible(false)，或按钮随 owner 被延迟销毁），本帧及之后都不会再对它调用 onPointerUp，m_isPressed 与 Pressed 状态永久残留。当按钮重新可见且用户随后在按钮上做一次全新点击并释放时，第 85 行条件中 button->wasPointerDown() 仍为 true，onPointerUp 会以 wasPressed=true 触发 m_onClick——一次从未完成的按下的虚假点击；未点击时按钮也常驻 Pressed 着色。

建议修复：在按钮进入 visible/发生 onPointerEnter 时，若鼠标并未按住（!Input::IsMouseButtonPressed）且状态仍为 Pressed、m_isPressed 为 true，则复位为 Normal/Highlighted；或为按下记录 (owner, pressFrame)，失配即清理。


## 架构建议

审查完成。以下是我对 ShitEngine 核心架构的分析与按优先级排序的建议清单。

---

## P0 — 确定性 Bug（正确性/生命周期，建议立即修复）

**1. `Scene::registerSystem<T>()` 非延迟，运行中注册会迭代器失效**
现状问题：`Scene.cpp:35` 的 `for (auto* system : m_systems)` 在迭代 `m_systems` 向量；而 `Scene.h:71` 的 `registerSystem` 直接 `m_systems.push_back(...)`，可能触发 realloc。若在任一系统 update 内（如 BehaviorSystem 的 onUpdate 中、或更靠前的系统里）调用 `scene->registerSystem<X>()`，range-for 的迭代器立即悬垂 → UB。对比 `unregisterSystem` 已走 `m_pendingRemoveSystems` 延迟路径，register 却即时生效，明显不一致。
建议：让 registerSystem 也走延迟队列（与 unregisterSystem 对称，统一在 `processPendingAdditions` 帧末处理）。
预期收益：消除一类隐蔽的随机崩溃；API 使用心智模型统一为"场景结构变更帧末生效"。

**2. `m_isRegistered` 存在"双重真相"，"组件先于驱动系统注册"的路径永久失效**
现状问题：三条路径对 `m_isRegistered` 的写入不一致——
- `GameObject.h:93` addComponent 在 onAttach 之后**无条件**置 true（即使 onAttach 因找不到驱动系统而失败）；
- `System.cpp:18` System::init() 的补扫对未注册组件调 onAttach 但**不置** true；
- `Behavior.cpp:19` / `RendererComponent.cpp:20` 组件自己在 onAttach 内置 true。

后果：若在 `registerSystem<BehaviorSystem>()` 之前 `addComponent<Behavior>()`，onAttach 找不到系统、不注册，但 GameObject 已把 registered 置 true，之后 System::init() 补扫被跳过 → Behavior 的 onStart/onUpdate 永远不被调用，且无任何告警。当前示例恰好 `scene->init()` 先于建对象，掩盖了此问题。
建议：删掉 addComponent 里对 `m_isRegistered` 的强制赋值，让注册状态只由组件自身的 onAttach/onDetach 维护；并把"onAttach 未找到驱动系统"记录为待办，在系统注册时回调补挂（见 P2 建议 6）。
预期收益：修复一个静默失效类缺陷；消除"先加组件后加系统"这一非法但未被阻止的用法。

**3. `Scene::init()` 依赖使用者手动调用，SceneManager 不兜底**
现状问题：`SceneManager::PushScene` 不调用 `scene->init()`；插件必须自己记得在 createMainScene 里 `scene->init()`。漏调则场景没有任何 System，屏幕不清理、UI 不渲染、Behavior 不驱动，全静默。
建议：SceneManager 首次 update 时若场景系统为空则自动 `init()`（或把默认系统注册移入 Scene 构造）。同时 init 是 virtual 的，可让 `Scene::init` 的默认实现注册行为保留，但调用点收敛到 SceneManager。
预期收益：消除"空场景"静默故障；降低插件编写门槛。

---

## P1 — 单例架构（编辑器集成 / 多实例的最大障碍）

**4. 13 个进程级 Meyer 单例，编辑器无法在进程内跑预览/多实例**
现状问题：`Game/Config/Log/Window/Time/Input/Renderer/ResourceManager/AudioPlayer/EventBus/SceneManager/TextInputGate/TypeRegistry` 全部是 `static GetInstance() + 静态门面`。这决定了引擎生命周期不可嵌套、不可多实例化（编辑器预览、双窗口、headless 测试都做不了），也完全无法做依赖注入与单元测试。
建议：引入一个可拥有的 `EngineContext`（聚合各子系统实例），静态 API 保留为对"当前上下文"的薄转发以兼容现有代码。第一步可低成本做到：把各单例内部实现抽成实例类（如 `InputImpl`），`GetInstance()` 转发到 `EngineContext::current()`，编辑器即可创建第二个 context 做 in-proc 预览。
预期收益：为 Qt 编辑器集成、自动化测试、未来多窗口铺路；改动量可控（静态门面是纯转发，接口不变）。

**5. Prefab 是 lambda builder，无法序列化/数据驱动**
现状问题：`Prefab.h` 的 `Prefab::Build([]{...})` 只能存可调用对象，无法落盘、无法在编辑器里编辑后保存、无法深拷贝（不能作为组件状态快照复用）。
建议：将 Prefab 改为"组件克隆 + 反射字段拷贝"模型：`Prefab` 持有每个组件的 `TypeInfo` + 序列化字段数据（JSON 或二进制 blob），`instantiate` 通过 `TypeRegistry` 的 factory + `SetValue` 重建组件。JSON 序列化可直接复用已完成的反射偏移系统。
预期收益：编辑器场景/预制体管线（保存、加载、撤销）具备数据基础；运行时实例化开销从闭包回调变为 memcpy 级拷贝。

---

## P2 — 组件/系统解耦

**6. 组件 onAttach 硬编码 `scene->getSystem<X>()` 找驱动系统**
现状问题：`RendererComponent.cpp:18`、`Behavior.cpp:17`、`RigidBody2D.cpp:26` 都在 onAttach 里按具体系统类型查询。新增一个组件就要知道"哪个系统驱动我"，且系统不存在时静默跳过（叠加 Bug 2 后永久失效）。组件↔系统靠 type 互相锁定，模块间形成隐式硬耦合。
建议：组件只向 Scene 声明"我需要 X 能力"，由 Scene/系统侧注册 attach 回调。最小实现：`System` 增加 `registerComponentHandler(type_index, fn)`，Scene 维护 `type_index → 回调表`，组件 onAttach 时广播给所有系统（不查询具体系统类型）。
预期收益：组件与驱动系统解耦，新组件只需实现接口；系统注册顺序不再敏感；为 Bug 2 提供统一修复点。

**7. `getComponents()` 暴露内部 map + 系统"map 所有权 / vector 缓存"双簿记**
现状问题：`GameObject.h:64` 返回 `m_components` 的**可变引用**（`System.cpp:15` 借此遍历），外部可任意增删组件绕开生命周期；`Scene` 的 `m_systemsMap`（所有权）+ `m_systems`（缓存裸指针）双份存储，任何一处忘记同步都会悬垂或漏遍历。
建议：暴露 `forEachComponent(callback)` 只读遍历；系统容器统一为 `std::vector<std::unique_ptr<System>>` + 类型索引由注册时一次性生成，或单一 `unordered_map` 持所有权、遍历改为按 priority 排序的 `std::vector<System*>` 但由 `Scene::update` 一处重建而非运行时 push_back。
预期收益：内部存储可自由替换（为 ECS 化铺路）；消除双簿记失同步风险。

**8. 组件存储与查询粒度：`unordered_map` + 每组件独立堆分配，无按类型批量查询**
现状问题：每个 GameObject 一张 `unordered_map<type_index, unique_ptr<Component>>`，每个组件一次堆分配。要"取所有 Transform"或"取所有 SpriteRenderer"只能 O(物体数) 遍历 + O(1) 哈希命中，cache 不友好，且 `typeid` 每次构造 `type_index`。
建议：中等代价方案——Scene 维护懒构建的 `type_index → vector<Component*>` 索引，首次查询时扫描构建，add/remove 时标记脏。终极方案——对热路径组件（Transform/SpriteRenderer）引入 archetype/sparse-set ECS，`addComponent` 仍返回组件指针保持 API 兼容。
预期收益：BehaviorSystem/RenderSystem/PhysicsSystem 的每帧遍历从"指针追逐 + 哈希"变为紧凑数组迭代，为千级物体规模做准备。

---

## P3 — 内存管理与错误处理

**9. 错误处理策略不一致**
现状问题：init 链用 bool（`Game::init`）、部分函数返回 nullptr、大量仅 `ST_CORE_WARN`（如 `PhysicsSystem2D` 无效世界、collider 缺刚体），Release 下静默失败无观测手段；`System.cpp:12` 甚至假设 `m_scene` 非空（若未 setScene 直接 init 则空指针解引用）。
建议：确立契约——致命初始化失败返回 bool + ERROR（现状已基本如此）；非致命降级 WARN + 函数式 fallback（返回 nullptr）；新增 `ST_CORE_ASSERT` 在 Debug 下拦截逻辑不变量（如 System 必须有 scene）。跨 DLL 边界不要抛异常（C++ ABI 不可控）。
预期收益：故障可观测、可定位；把"静默失效"（Bug 2、建议 3）从源头消灭。

**10. 跨 DLL 所有权与反射 factory 悬空（编辑器集成隐患）**
现状问题：`CreateMainScene` 在插件 DLL 内 `new` Scene，由 EXE 的 `unique_ptr` 释放——依赖 MSVC 共享 CRT 才安全；`TypeRegistry` 中插件类型注册的 `std::function factory`（在插件 DLL 分配）在 `PluginManager::UnloadAll()`（先于 `Game::Destroy`）后仍驻留，编辑器若在卸载插件后调用 `Create` 即崩溃。
建议：插件边界一律返回 `unique_ptr`/经引擎导出的释放函数；`Game::Destroy` 前统一 `TypeRegistry::UnregisterType` 清除插件注册（含 factory）；给 TypeRegistry 增加"按来源清理"能力。
预期收益：DLL 卸载/热重载安全，是编辑器"运行中重载插件"特性的前提。

**11. addComponent 返回裸指针，removeComponent 立即销毁 → 悬垂**
现状问题：`addComponent` 返回的指针在 `removeComponent<T>()`（同步调用 onDetach/onDestroy + erase）后立即悬垂；持有者（如 UIButton 回调捕获的 UIText 指针）无感知。
建议：短期——文档 + `getComponent<T>()` 在 map 中二次确认；长期——组件销毁延迟到帧末统一处理（与 GameObject 销毁一致），并考虑 `WeakComponentRef<ComponentType>`（存 owner + type，查询时校验）。
预期收益：消除一类难排查的 use-after-free；与既有的"延迟销毁"哲学对齐。

---

## P4 — 渲染 / 模块耦合 / 代码组织

**12. RenderSystem 绕过 Renderer 直接操作 SDL_Renderer；每帧两次 vector 全量拷贝**
现状问题：`RenderSystem.cpp:60-84` 直接 `SDL_SetRenderViewport/SetRenderClipRect` 在 `Renderer::GetRenderer()` 裸指针上，绕过了 Renderer 的抽象（Renderer 已维护逻辑分辨率/视口语义）；且每个相机、每帧 `auto cameras = m_cameras; auto renderers = m_renderers;` 两次全量拷贝（多相机时呈倍数增长）。
建议：RenderSystem 通过 Renderer 提供的 `Frame/SetViewport/SetClip` 高级 API 绘制；"快照防迭代失效"改为仅在 onRender 可能变更列表时拷贝，或引入 `m_isDirty` + 索引遍历，避免每帧每相机无条件拷贝。
预期收益：渲染层封装收敛（为接入不同后端/编辑器视图铺路）；每帧堆分配量下降。

**13. 物理模块把像素/米比例做成静态全局**
现状问题：`PhysicsSystem2D.h:56` `static float s_pixelsPerMeter` + `b2SetLengthUnitsPerMeter`（Box2D 全局），多场景/未来多世界（如暂停菜单叠物理）会互相污染；`RigidBody2D` 通过 friend 直接读系统私有字段（`m_worldIndex/m_worldGeneration`）跨模块钻取内部。
建议：像素/米比例作为 `PhysicsSystem2D` 实例字段；物理句柄封装提供最小接口（`createBody/step`）供 RigidBody/Collider 调用，而非 friend 拆私有。
预期收益：物理模块可多实例化；组件↔系统边界更干净。

**14. 头文件依赖与 PCH**
现状问题：`GameObject.h` 以 `#include` 引入 `Scene.h/Behavior.h/RendererComponent.h`（9-12 行），触发 GameObject↔Scene 包含环（靠 Scene 前向声明勉强维持）；`ShitEngine.h` 伞形头把所有消费者拖入 SDL/spdlog/nlohmann/全 UI+物理，任何插件改动任一头文件都会波及全量重编。
建议：GameObject.h 只前向声明 Scene/Behavior/RendererComponent，把实现 include 下沉到 .cpp（仅 `getComponent<T>` 模板需要在头里，可依赖 Component.h 单头）；`ShitEngine.h` 保持聚合但各子头遵循 IWYU，`pch.h` 仅限引擎内部 `target_precompile_headers` 不进入插件可见路径。
预期收益：编译时间下降、耦合面收敛；插件仅 include 所需子头。

**15. EventBus 单线程却加 mutex + 每事件一个 shared_ptr**
现状问题：`EventBus.h:121` 有 mutex，但引擎是纯单线程主循环，`Emit` 每事件 `make_shared<EventType>` 一次堆分配（`EventBus.cpp` 派发时再解包）；mutex 既不防真并发，又给"这里线程安全"的错误暗示。
建议：若保持单线程，去掉 mutex，队列改用 segmented ring buffer 或 `std::deque<std::unique_ptr<Event>>` 减少分配；若规划多线程（加载/物理），则显式定义线程模型并保留 mutex。
预期收益：热路径（Emit）零锁零/低分配；线程语义明确。

---

## P5 — 生命周期语义与遍历健壮性（较低优先级）

**16. 场景栈只更新栈顶，ReplaceScene 是破坏性的**
现状问题：`SceneManager::update` 只驱动 `getCurrentScene()`（栈顶），下层场景的 System 完全不 tick（含 UI 输入），语义上是"严格栈"。但 `processReplaceScene` 先 `clearScene()` 再压入——若调用方想"暂停后替换回主菜单"会连带销毁暂停场景；且栈顶外场景的 AudioPlayer（全局）仍会播放，语义割裂。
建议：明确区分两种模式（`PushScene` 覆盖式暂停 / `ReplaceScene` 换场景），在文档与 API 命名上固定；若未来需要"游戏时间 vs UI 时间"分离，给 System 增加 `setActive(bool)` 而不依赖栈位置。
预期收益：场景管理语义可预测，避免"暂停场景被误销毁"类事故。

**17. BehaviorSystem 遍历中修改列表会跳过元素**
现状问题：`BehaviorSystem.cpp:24` 的 `for (auto& b : m_behaviors)` 无快照；若某 Behavior 的 onUpdate 内对另一个 Behavior 调 `removeComponent`（触发 onDetach → `unregisterBehavior` → erase-remove），std::remove 会把后续元素前移，range-for 自增后**跳过**一个元素。
建议：与其他系统一致，迭代期间收集"待删除"，遍历结束后统一处理；或干脆复制快照（Behavior 数量级小，拷贝便宜）。
预期收益：行为列表在运行时变更时遍历完整，消除偶发"某脚本一帧不执行"。

---

## 总结（针对 6 个分析维度的结论）

- **组件/系统扩展性**：最薄弱。注册状态双重真相（Bug 2）、组件硬找驱动系统（建议 6）、按类型批量查询缺失（建议 8）、registerSystem 迭代器失效（Bug 1）——四条共同指向：需要一个"Scene 侧统一注册表 + 延迟结构变更"的中枢。
- **内存管理**：所有权设计总体健康（unique_ptr + 裸指针只表达非拥有关系是合理约定），主要风险在跨 DLL 边界（建议 10）与 removeComponent 悬垂（建议 11）。
- **错误处理**：不一致是最大问题；建议 9 建立 bool/nullptr/ASSERT 三分契约。
- **模块耦合**：UI/Physics/Reflection 单看都较干净，但都通过"组件→具体系统类型"与"单例全局"隐性耦合（建议 6/13/15）；Reflection 设计（offset+size+工厂）质量较好，是后续序列化/编辑器的资产。
- **代码组织**：头文件环与伞形聚合可收敛（建议 14），PCH 使用正确（内部限定）。
- **编辑器集成**：两个硬阻塞——进程级单例（建议 4）与 lambda Prefab（建议 5）；两个隐患——插件 factory 悬空（建议 10）、跨 DLL new/delete。

建议修复顺序：P0 三项（纯 bug，改动小收益大）→ P1 两项（编辑器方向的地基）→ P2 三项（架构核心收敛）→ P3/P4 按需。

关键文件路径：`Engine/include/ShitEngine/GameObject/GameObject.h`、`Engine/src/ShitEngine/Scene/Scene.cpp`、`Engine/src/ShitEngine/System/System.cpp`、`Engine/include/ShitEngine/GameObject/Prefab.h`、`Engine/src/ShitEngine/Physics/PhysicsSystem2D.cpp`、`Engine/src/ShitEngine/Render/RenderSystem.cpp`。