#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Resource/Resource.h"

#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Render/Renderer.h"

#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <cmath>
#include <random>

namespace Shit {
	namespace {
		// 会话级资源 uuid：random_device 种子的 mt19937_64（与组件 UUID 同源方案，0 保留）
		struct ResourceUuidRng {
			std::mt19937_64 operator()() {
				std::random_device rd;
				std::seed_seq seq{ rd(), rd(), rd(), rd() };
				return std::mt19937_64(seq);
			}
		};
		thread_local auto s_uuidRng = ResourceUuidRng{}();
	}

	std::uint64_t GenerateResourceUuid() {
		std::uint64_t uuid = 0;
		while (uuid == 0) uuid = s_uuidRng();
		return uuid;
	}

	// ── Texture ──

	bool Texture::load() {
		release();
		SDL_Renderer* renderer = Renderer::GetRenderer();
		if (!renderer) {
			m_error = "渲染器未初始化";
			ST_CORE_ERROR("无法加载纹理 {}：{}", m_path, m_error);
			return false;
		}
		m_texture = IMG_LoadTexture(renderer, m_path.c_str());
		if (!m_texture) {
			m_error = SDL_GetError();
			m_state = ResourceState::Failed;
			ST_CORE_ERROR("无法加载纹理 {}: {}", m_path, m_error);
			return false;
		}
		m_state = ResourceState::Loaded;
		m_error.clear();
		return true;
	}

	void Texture::release() {
		if (m_texture) {
			SDL_DestroyTexture(m_texture);
			m_texture = nullptr;
		}
		m_state = ResourceState::Unloaded;
	}

	// ── Font ──

	bool Font::load() {
		release();
		m_font = TTF_OpenFont(m_path.c_str(), static_cast<int>(std::round(m_size)));
		if (!m_font) {
			m_error = SDL_GetError();
			m_state = ResourceState::Failed;
			ST_CORE_ERROR("无法加载字体 {} ({}): {}", m_path, m_size, m_error);
			return false;
		}
		m_state = ResourceState::Loaded;
		m_error.clear();
		return true;
	}

	void Font::release() {
		if (m_font) {
			TTF_CloseFont(m_font);
			m_font = nullptr;
		}
		m_state = ResourceState::Unloaded;
	}

	// ── Audio ──

	bool Audio::load() {
		release();
		if (!m_mixer) {
			m_error = "未设置混音器";
			m_state = ResourceState::Failed;
			ST_CORE_ERROR("AudioManager 未设置混音器，无法加载音频: {}", m_path);
			return false;
		}
		m_audio = MIX_LoadAudio(m_mixer, m_path.c_str(), true);
		if (!m_audio) {
			m_error = SDL_GetError();
			m_state = ResourceState::Failed;
			ST_CORE_ERROR("无法加载音频 {}: {}", m_path, m_error);
			return false;
		}
		m_state = ResourceState::Loaded;
		m_error.clear();
		return true;
	}

	void Audio::release() {
		if (m_audio) {
			MIX_DestroyAudio(m_audio);
			m_audio = nullptr;
		}
		m_state = ResourceState::Unloaded;
	}
}
