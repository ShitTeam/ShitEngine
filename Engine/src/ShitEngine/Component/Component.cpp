#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/Component.h"

#include <chrono>
#include <random>

namespace Shit {

namespace {
/// 进程级 UUID 生成器（random_device + 时间播种，避免每次构造组件都重播种）
std::mt19937_64& uuidRng() {
    static std::mt19937_64 rng = [] {
        std::random_device rd;
        const std::seed_seq seq{
            rd(), rd(), rd(), rd(),
            static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())
        };
        return std::mt19937_64(seq);
    }();
    return rng;
}
} // namespace

uint64_t GenerateComponentUuid() {
    uint64_t uuid = 0;
    while (uuid == 0) uuid = uuidRng()();  // 0 保留为空引用
    return uuid;
}

Component::Component() : m_uuid(GenerateComponentUuid()) {}