#pragma once

#include "ShitEngine/Reflection/TypeInfo.h"
#include "ShitEngine/Core/Core.h"

#include <string_view>
#include <functional>
#include <deque>
#include <unordered_map>
#include <string>

// ── P1-3: 类型名 demangle 依赖 ──────────────────────
#if defined(__GNUC__) || defined(__clang__)
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define SHIT_HAS_CXXABI 1
#endif
#endif

namespace Shit {

class TypeInfoBuilder;

class SHIT_API TypeRegistry {
public:
	void registerType(TypeInfo info);
	void initBuiltinTypes();
	void resolveBases();                ///< 解析所有未解析的基类引用（消除 SIOF）
	bool unregisterType(std::string_view name);  ///< 按名称卸载类型
	/// @brief 设置后续注册类型的来源标记（空 = 引擎内置，插件注册前设为插件名）
	void setRegistrationSource(std::string_view source);
	/// @brief 卸载指定来源注册的全部类型（插件卸载前调用，防止 factory 悬垂）
	size_t unregisterTypesBySource(std::string_view source);

	[[nodiscard]] const TypeInfo* getType(std::string_view name) const;

	/// @brief 按 type_index 查询（Prefab 动态克隆等场景用）
	[[nodiscard]] const TypeInfo* getType(std::type_index ti) const;

	template<typename T>
	[[nodiscard]] const TypeInfo* getType() const {
		auto it = m_typeIndexMap.find(std::type_index(typeid(T)));
		return it != m_typeIndexMap.end() ? it->second : nullptr;
	}

	void forEach(std::function<void(const TypeInfo&)> callback) const;
	[[nodiscard]] size_t count() const { return m_typeStorage.size(); }

	// ── 静态门面 ──

	static TypeRegistry& GetInstance();

	static void Register(TypeInfo info) { GetInstance().registerType(std::move(info)); }
	static void InitBuiltinTypes() { GetInstance().initBuiltinTypes(); }
	static void ResolveBases() { GetInstance().resolveBases(); }
	static bool UnregisterType(std::string_view name) { return GetInstance().unregisterType(name); }
	static void SetRegistrationSource(std::string_view source) { GetInstance().setRegistrationSource(source); }
	static size_t UnregisterTypesBySource(std::string_view source) { return GetInstance().unregisterTypesBySource(source); }
	static const TypeInfo* Get(std::string_view name) { return GetInstance().getType(name); }
	static const TypeInfo* Get(std::type_index ti) { return GetInstance().getType(ti); }

	template<typename T>
	static const TypeInfo* Get() { return GetInstance().getType<T>(); }

	static void ForEach(std::function<void(const TypeInfo&)> callback) { GetInstance().forEach(std::move(callback)); }
	static size_t Count() { return GetInstance().count(); }

	TypeRegistry(const TypeRegistry&) = delete;
	TypeRegistry& operator=(const TypeRegistry&) = delete;
	TypeRegistry(TypeRegistry&&) = delete;
	TypeRegistry& operator=(TypeRegistry&&) = delete;

private:
	friend class EngineContext;
	TypeRegistry() = default;
	~TypeRegistry() = default;

	friend class TypeInfoBuilder;

	// deque 保证元素地址稳定（push_back/emplace_back 不失效指向元素的指针）
	std::deque<TypeInfo>                                  m_typeStorage;
	std::unordered_map<std::string, TypeInfo*>           m_nameMap;
	std::unordered_map<std::type_index, const TypeInfo*> m_typeIndexMap;
	std::string                                           m_currentSource;  ///< 当前注册来源标记
};

// ── P1-3: 类型名 demangle ────────────────────────────
// GCC/MinGW: abi::__cxa_demangle, MSVC: typeid 直接可读
inline std::string DemangleTypeName(const char* mangled) {
#if defined(SHIT_HAS_CXXABI)
	int status = 0;
	char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
	if (status == 0 && demangled) {
		std::string result(demangled);
		::free(demangled);
		return result;
	}
#endif
	// MSVC 或 demangle 失败：直接返回原名
	return std::string(mangled);
}

/// 计算成员指针相对于类基址的偏移量（供反射注册代码使用）
/// 用法: memberOffset(&MyClass::myField)
/// 需要通过 friend 声明获取 private/protected 成员的访问权
template<typename T, typename M>
inline size_t memberOffset(M T::*member) {
	return reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member));
}

class SHIT_API TypeInfoBuilder {
public:
	TypeInfoBuilder& Base(const TypeInfo* baseType) {
		m_info.baseType = baseType;
		return *this;
	}

	/// @brief 按名称指定基类（延迟解析，消除 SIOF）
	/// Register() 时尝试解析，ResolveBases() 时再次尝试
	TypeInfoBuilder& Base(const char* name) {
		m_info.baseTypeName = name ? name : "";
		return *this;
	}

	template<typename T>
	TypeInfoBuilder& Base() {
		m_info.baseType = TypeRegistry::Get<T>();
		return *this;
	}

	// 原始 Field 注册（offset + size）
	TypeInfoBuilder& Field(const char* name, size_t offset, size_t size, const char* typeName) {
		m_info.fields.push_back({name, offset, size, typeName});
		return *this;
	}

	// ── P1-3: 成员指针重载（自动推导 offset + size + typeName）──
	template<typename T, typename M>
	TypeInfoBuilder& Field(const char* name, M T::*member) {
		m_info.fields.push_back({
			name,
			memberOffset(member),
			sizeof(M),
			DemangleTypeName(typeid(M).name())
		});
		return *this;
	}

	template<typename T, typename M>
	TypeInfoBuilder& Field(const char* name, M T::*member, const char* typeName) {
		m_info.fields.push_back({
			name,
			memberOffset(member),
			sizeof(M),
			typeName
		});
		return *this;
	}

	// ── P2: 字段元数据（编辑器属性显示用）───────────
	TypeInfoBuilder& Meta(const FieldMeta& meta) {
		if (!m_info.fields.empty()) {
			m_info.fields.back().meta.push_back(meta);
		}
		return *this;
	}

	// ── P3: 枚举常量 ──────────────────────────────────
	TypeInfoBuilder& Value(const char* name, int64_t val) {
		m_info.enumValues.push_back({name, val});
		return *this;
	}

	// ── P1-2: 工厂注册 ─────────────────────────────────
	template<typename T>
	TypeInfoBuilder& Factory() {
		m_info.factory = makeFactory<T>(std::is_abstract<T>{});
		return *this;
	}

	template<typename T>
	void Register() {
		m_info.typeIndex = std::type_index(typeid(T));

		// 尝试立即解析 baseTypeName（若已注册则直接链接，未注册时留待 ResolveBases 处理）
		if (!m_info.baseType && !m_info.baseTypeName.empty()) {
			auto* resolved = TypeRegistry::Get(m_info.baseTypeName.c_str());
			if (resolved) {
				m_info.baseType = resolved;
			}
		}

		TypeRegistry::Register(std::move(m_info));
	}

private:
	friend TypeInfoBuilder ReflectType(const char* name, size_t size);

	explicit TypeInfoBuilder(const char* name, size_t size)
		: m_info{name, size, nullptr, {}, {}, {}, typeid(nullptr)} {}

	// ── Factory SFINAE 辅助：抽象类型跳过 factory ──
	template<typename T>
	static std::function<void*(void*)> makeFactory(std::false_type /* not abstract */) {
		return [](void* mem) -> void* {
			if (mem) return ::new(mem) T();
			return new T();
		};
	}

	template<typename T>
	static std::function<void*(void*)> makeFactory(std::true_type /* abstract */) {
		return nullptr;
	}

	TypeInfo m_info;
};

inline TypeInfoBuilder ReflectType(const char* name, size_t size) {
	return TypeInfoBuilder(name, size);
}

} // namespace Shit
