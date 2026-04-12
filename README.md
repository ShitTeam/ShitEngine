# ShitEngine

ShitEngine 是一个基于 C++ 的轻量级游戏引擎，采用现代组件化架构，支持组件化开发和资源管理，适合 2D 游戏快速原型和学习研究。

## ✨ 特性

- 现代组件化架构，灵活的组件与系统扩展
- 场景与游戏对象管理
- 资源自动缓存与管理（纹理、音频等）
- 支持 SFML 渲染
- 日志、配置、输入等基础系统
- 易于扩展的系统注册与事件分发

## 📁 目录结构

```
include/           # 头文件（API、核心、组件、系统等）
src/               # 源码实现
vendor/            # 第三方依赖（GLM、nlohmann_json、SFML、spdlog等）
cmake-build-*/     # CMake 构建输出
CMakeLists.txt     # CMake 构建脚本
README.md          # 项目说明
```

## 🚀 快速开始

### 依赖

- C++17 或更高
- [SFML](https://www.sfml-dev.org/)
- [spdlog](https://github.com/gabime/spdlog)
- [glm](https://github.com/g-truc/glm)
- [nlohmann/json](https://github.com/nlohmann/json)

### 构建

1. 克隆仓库并初始化子模块（如有）：
	```
	git clone https://your.repo.url/ShitEngine.git
	cd ShitEngine
	```

2. 使用 CMake 构建：
	```
	mkdir build
	cd build
	cmake ..
	cmake --build .
	```

3. 可执行文件在 `build/bin/` 目录下。

### 示例代码

```cpp
#include <ShitEngine.h>

#ifdef _WIN32
	#include <Windows.h>
#endif // _WIN32

int main() {
#ifdef _WIN32
	// 强制控制台输出使用 UTF-8 编码
	SetConsoleOutputCP(65001);
#endif

	if (Shit::Game::Init()) {
		Shit::Game::Run();
	}

	Shit::Game::Destroy();
	return 0;
}
```

## 🛠️ 贡献

欢迎提交 issue 和 PR！请遵循以下流程：

1. Fork 本仓库
2. 新建分支进行开发
3. 提交 PR 并描述你的更改

## 📄 许可证

本项目基于 MIT License 许可。

---

如需更详细的 API 文档或使用示例，请查阅 `include/` 目录和源码注释。
