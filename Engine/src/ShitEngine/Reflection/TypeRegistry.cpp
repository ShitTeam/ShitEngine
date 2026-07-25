#include "ShitEngine/Reflection/TypeRegistry.h"

namespace Shit {

TypeRegistry& TypeRegistry::GetInstance() {
	static TypeRegistry instance;
	return instance;
}

void TypeRegistry::registerType(TypeInfo info) {
	const std::string name = info.name;

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

} // namespace Shit
