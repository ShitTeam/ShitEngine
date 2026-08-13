#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Reflection/TypeRegistry.h"

#include <vector>

namespace Shit {

TypeRegistry& TypeRegistry::GetInstance() {
	return EngineContext::current().typeRegistry;
}

void TypeRegistry::registerType(TypeInfo info) {
	const std::string name = info.name;
	info.source = m_currentSource;  // 标记注册来源（空=引擎，否则=插件名）

	if (auto nameIt = m_nameMap.find(name); nameIt != m_nameMap.end()) {
		TypeInfo* existing = nameIt->second;

		if (existing->typeIndex != std::type_index(typeid(nullptr))) {
			m_typeIndexMap.erase(existing->typeIndex);
		}

		*existing = std::move(info);

		if (existing->typeIndex != std::type_index(typeid(nullptr))) {
			m_typeIndexMap[existing->typeIndex] = existing;
		}
		return;
	}

	m_typeStorage.push_back(std::move(info));
	TypeInfo* stored = &m_typeStorage.back();

	m_nameMap[name] = stored;

	if (stored->typeIndex != std::type_index(typeid(nullptr))) {
		m_typeIndexMap[stored->typeIndex] = stored;
	}
}

const TypeInfo* TypeRegistry::getType(std::string_view name) const {
	// MinGW 13.1 unordered_map::find(string_view) 需要显式构造 string
	auto it = m_nameMap.find(std::string(name));
	return it != m_nameMap.end() ? it->second : nullptr;
}

const TypeInfo* TypeRegistry::getType(std::type_index ti) const {
	auto it = m_typeIndexMap.find(ti);
	return it != m_typeIndexMap.end() ? it->second : nullptr;
}

void TypeRegistry::forEach(std::function<void(const TypeInfo&)> callback) const {
	for (const auto& info : m_typeStorage) {
		callback(info);
	}
}

void TypeRegistry::initBuiltinTypes() {
	auto reg = [this](const char* name, size_t size, std::type_index ti) {
		TypeInfo info;
		info.name      = name;
		info.size      = size;
		info.typeIndex = ti;
		registerType(std::move(info));
	};

	reg("int",                 sizeof(int),                 typeid(int));
	reg("unsigned int",         sizeof(unsigned int),        typeid(unsigned int));
	reg("long",                sizeof(long),                typeid(long));
	reg("unsigned long",        sizeof(unsigned long),       typeid(unsigned long));
	reg("long long",           sizeof(long long),           typeid(long long));
	reg("unsigned long long",   sizeof(unsigned long long),  typeid(unsigned long long));

	reg("float",               sizeof(float),               typeid(float));
	reg("double",              sizeof(double),              typeid(double));
	reg("bool",                sizeof(bool),                typeid(bool));
	reg("char",                sizeof(char),                typeid(char));
	reg("unsigned char",        sizeof(unsigned char),       typeid(unsigned char));

	reg("int8_t",   sizeof(std::int8_t),   typeid(std::int8_t));
	reg("int16_t",  sizeof(std::int16_t),   typeid(std::int16_t));
	reg("int32_t",  sizeof(std::int32_t),   typeid(std::int32_t));
	reg("int64_t",  sizeof(std::int64_t),   typeid(std::int64_t));
	reg("uint8_t",  sizeof(std::uint8_t),   typeid(std::uint8_t));
	reg("uint16_t", sizeof(std::uint16_t),  typeid(std::uint16_t));
	reg("uint32_t", sizeof(std::uint32_t),  typeid(std::uint32_t));
	reg("uint64_t", sizeof(std::uint64_t),  typeid(std::uint64_t));

	reg("std::string", sizeof(std::string), typeid(std::string));
	reg("size_t",      sizeof(std::size_t), typeid(std::size_t));
}

void TypeRegistry::resolveBases() {
	for (auto& info : m_typeStorage) {
		if (info.baseType || info.baseTypeName.empty()) continue;
		auto* resolved = getType(info.baseTypeName);
		if (resolved) {
			info.baseType = resolved;
		}
	}
}

void TypeRegistry::setRegistrationSource(std::string_view source) {
	m_currentSource = std::string(source);
}

size_t TypeRegistry::unregisterTypesBySource(std::string_view source) {
	// 先收集匹配来源的类型名（unregisterType 会重建索引，逐个卸载）
	std::vector<std::string> names;
	for (const auto& info : m_typeStorage) {
		if (info.source == source) {
			names.push_back(info.name);
		}
	}
	for (const auto& name : names) {
		unregisterType(name);
	}
	return names.size();
}

bool TypeRegistry::unregisterType(std::string_view name) {
	auto it = m_nameMap.find(std::string(name));
	if (it == m_nameMap.end()) return false;

	TypeInfo* info = it->second;

	// 清除所有引用此类型的基类链接
	for (auto& stored : m_typeStorage) {
		if (stored.baseType == info) {
			stored.baseType = nullptr;
		}
	}

	// 从 storage 中移除（deque 中间 erase 会使后续元素地址失效，故擦除后重建索引）
	for (auto si = m_typeStorage.begin(); si != m_typeStorage.end(); ++si) {
		if (&(*si) == info) {
			m_typeStorage.erase(si);
			break;
		}
	}

	// 重建 name/index 索引（被删元素之后所有 TypeInfo 的地址都已变化）
	m_nameMap.clear();
	m_typeIndexMap.clear();
	for (auto& stored : m_typeStorage) {
		m_nameMap[stored.name] = &stored;
		if (stored.typeIndex != std::type_index(typeid(nullptr))) {
			m_typeIndexMap[stored.typeIndex] = &stored;
		}
	}

	// deque 中段 erase 会使其后所有 TypeInfo 的地址改变，但它们的 baseType 指针
	// 仍指向移动前的旧地址（悬垂）。这里按名称重连所有剩余类型的基类链接，
	// 基类已被卸载（不在此 storage）的类型 baseType 置空，避免 use-after-free。
	for (auto& stored : m_typeStorage) {
		if (stored.baseTypeName.empty()) continue;
		stored.baseType = nullptr;
		if (auto* resolved = getType(stored.baseTypeName)) {
			stored.baseType = resolved;
		}
	}

	return true;
}

} // namespace Shit
