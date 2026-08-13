#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/Tilemap.h"

#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_render.h>
#include <cmath>
#include <sstream>
#include <string>

namespace Shit {

	Tilemap::Tilemap() = default;
	Tilemap::~Tilemap() = default;

	// ═══════════════════════════════════════════════════════════
	// 生命周期与序列化
	// ═══════════════════════════════════════════════════════════

	void Tilemap::onAfterDeserialize() {
		// 反射直写字段（含 m_gridData）后：解析网格、确保缓冲一致
		parseGridData();
		if (!m_texturePath.empty()) {
			// 触发纹理懒加载（ResourceManager 按路径缓存）
			ResourceManager::GetTexture(m_texturePath);
		}
		ensureTileBuffer();
	}

	void Tilemap::onFieldChanged(const std::string& fieldName) {
		// 检查器直写单个字段后：网格尺寸/瓦片尺寸变化需重建缓冲，纹理变化需重载
		if (fieldName == "m_gridWidth" || fieldName == "m_gridHeight" ||
			fieldName == "m_tileWidth" || fieldName == "m_tileHeight" ||
			fieldName == "m_tileWorldSize") {
			ensureTileBuffer();
			syncGridData();
		} else if (fieldName == "m_texturePath") {
			if (!m_texturePath.empty()) {
				ResourceManager::GetTexture(m_texturePath);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════
	// 渲染
	// ═══════════════════════════════════════════════════════════

	void Tilemap::onRender(SDL_Renderer* renderer, const CameraComponent* camera) const {
		SDL_Texture* texture = ResourceManager::GetTexture(m_texturePath);
		if (!texture || m_tiles.empty()) return;

		float texW = 0.0f, texH = 0.0f;
		SDL_GetTextureSize(texture, &texW, &texH);
		if (texW <= 0 || texH <= 0 || m_tileWidth <= 0 || m_tileHeight <= 0) return;

		const int tilesPerRow = static_cast<int>(texW) / m_tileWidth;
		if (tilesPerRow <= 0) return;

		auto* transform = getOwner()->getComponent<TransformComponent>();
		if (!transform) return;

		// 网格左上角世界坐标 = Transform 位置
		const Vector2 origin = transform->getPosition();

		// 每格世界尺寸（未显式设置 → 瓦片像素尺寸）
		const float cellW = (m_tileWorldSize.x > 0.0f) ? m_tileWorldSize.x : static_cast<float>(m_tileWidth);
		const float cellH = (m_tileWorldSize.y > 0.0f) ? m_tileWorldSize.y : static_cast<float>(m_tileHeight);

		const float pixelPerUnit = camera->getPixelPerUnit();
		// 渲染像素尺寸（世界格 × ppu）
		const float drawW = cellW * pixelPerUnit;
		const float drawH = cellH * pixelPerUnit;
		if (drawW <= 0 || drawH <= 0) return;

		// 相机可见世界范围：用于粗粒度剔除（仅绘制相机内的格子）
		const Vector2 camPos = camera->getPosition();
		const Vector2 camSize = camera->getSize();

		// 起点 = 网格左上角的世界坐标 → 屏幕坐标（像素对齐）
		Vector2 baseScreen = camera->worldToScreen(origin);
		baseScreen.x = std::floor(baseScreen.x);
		baseScreen.y = std::floor(baseScreen.y);

		SDL_FRect src{};
		src.w = static_cast<float>(m_tileWidth);
		src.h = static_cast<float>(m_tileHeight);

		SDL_FRect dst{};
		dst.w = std::floor(drawW + 0.5f);
		dst.h = std::floor(drawH + 0.5f);

		for (int row = 0; row < m_gridHeight; ++row) {
			// 粗剔除：该行世界 Y 超出相机视野则跳过
			const float rowWorldY = origin.y + row * cellH + cellH * 0.5f;
			if (rowWorldY < camPos.y - camSize.y * 0.5f - cellH ||
				rowWorldY > camPos.y + camSize.y * 0.5f + cellH) {
				continue;
			}
			const float screenY = baseScreen.y + row * drawH;
			for (int col = 0; col < m_gridWidth; ++col) {
				const int tileId = m_tiles[row * m_gridWidth + col];
				if (tileId < 0) continue;

				// 列粗剔除
				const float colWorldX = origin.x + col * cellW + cellW * 0.5f;
				if (colWorldX < camPos.x - camSize.x * 0.5f - cellW ||
					colWorldX > camPos.x + camSize.x * 0.5f + cellW) {
					continue;
				}

				// 纹理源矩形（瓦片索引 → 纹理坐标）
				src.x = static_cast<float>((tileId % tilesPerRow) * m_tileWidth);
				src.y = static_cast<float>((tileId / tilesPerRow) * m_tileHeight);

				dst.x = baseScreen.x + col * drawW;
				dst.y = screenY;

				SDL_RenderTextureRotated(renderer, texture, &src, &dst,
					0.0, nullptr, SDL_FLIP_NONE);
			}
		}
	}

	SDL_FRect Tilemap::getGlobalBounds() {
		auto* transform = getOwner()->getComponent<TransformComponent>();
		if (!transform) return SDL_FRect{};

		const float cellW = (m_tileWorldSize.x > 0.0f) ? m_tileWorldSize.x : static_cast<float>(m_tileWidth);
		const float cellH = (m_tileWorldSize.y > 0.0f) ? m_tileWorldSize.y : static_cast<float>(m_tileHeight);
		const Vector2 origin = transform->getPosition();
		return SDL_FRect{ origin.x, origin.y, m_gridWidth * cellW, m_gridHeight * cellH };
	}

	// ═══════════════════════════════════════════════════════════
	// 配置
	// ═══════════════════════════════════════════════════════════

	void Tilemap::setTexturePath(const std::string& path) {
		m_texturePath = path;
		if (!path.empty()) {
			ResourceManager::GetTexture(path);
		}
	}

	void Tilemap::setTileSize(int w, int h) {
		if (w <= 0 || h <= 0) return;
		m_tileWidth = w;
		m_tileHeight = h;
	}

	void Tilemap::setGridSize(int cols, int rows) {
		if (cols < 0 || rows < 0 || (cols == 0 && rows == 0)) return;
		m_gridWidth = cols;
		m_gridHeight = rows;
		ensureTileBuffer();
		syncGridData();
	}

	void Tilemap::setTileWorldSize(const Vector2& size) {
		m_tileWorldSize = size;
	}

	// ═══════════════════════════════════════════════════════════
	// 网格数据
	// ═══════════════════════════════════════════════════════════

	void Tilemap::setTile(int col, int row, int tileId) {
		if (col < 0 || row < 0 || col >= m_gridWidth || row >= m_gridHeight) return;
		ensureTileBuffer();
		m_tiles[row * m_gridWidth + col] = tileId;
		syncGridData();
	}

	int Tilemap::getTile(int col, int row) const {
		if (col < 0 || row < 0 || col >= m_gridWidth || row >= m_gridHeight) return -1;
		const size_t idx = static_cast<size_t>(row) * static_cast<size_t>(m_gridWidth) + static_cast<size_t>(col);
		if (idx >= m_tiles.size()) return -1;   // 防 addComponent 后尚未 ensure 的越界
		return m_tiles[idx];
	}

	void Tilemap::clear() {
		std::fill(m_tiles.begin(), m_tiles.end(), -1);
		syncGridData();
	}

	bool Tilemap::isEmpty() const {
		for (int v : m_tiles) {
			if (v >= 0) return false;
		}
		return true;
	}

	int Tilemap::getTileCount() const {
		int count = 0;
		for (int v : m_tiles) {
			if (v >= 0) ++count;
		}
		return count;
	}

	void Tilemap::setGridData(const std::string& data) {
		m_gridData = data;
		parseGridData();
	}

	// ═══════════════════════════════════════════════════════════
	// 内部
	// ═══════════════════════════════════════════════════════════

	void Tilemap::ensureTileBuffer() {
		const size_t need = static_cast<size_t>(m_gridWidth) * static_cast<size_t>(m_gridHeight);
		if (m_tiles.size() < need) {
			m_tiles.resize(need, -1);
		} else if (m_tiles.size() > need) {
			m_tiles.resize(need);
		}
	}

	void Tilemap::parseGridData() {
		// 格式：cols,rows,id0,id1,...
		std::stringstream ss(m_gridData);
		std::string token;
		std::vector<int> values;
		while (std::getline(ss, token, ',')) {
			// 容错：跳过空 token（如末尾逗号）
			if (token.empty()) continue;
			try {
				values.push_back(std::stoi(token));
			} catch (...) {
				values.push_back(-1);
			}
		}

		if (values.size() >= 2) {
			m_gridWidth = values[0];
			m_gridHeight = values[1];
			ensureTileBuffer();
			std::fill(m_tiles.begin(), m_tiles.end(), -1);
			const size_t max = std::min(m_tiles.size(), values.size() - 2);
			for (size_t i = 0; i < max; ++i) {
				m_tiles[i] = values[2 + i];
			}
		} else {
			// 无效/空数据 → 保持当前尺寸，全空格
			ensureTileBuffer();
			std::fill(m_tiles.begin(), m_tiles.end(), -1);
		}
	}

	void Tilemap::syncGridData() {
		std::string out;
		out.reserve(2 + m_tiles.size() * 3);
		out += std::to_string(m_gridWidth);
		out += ',';
		out += std::to_string(m_gridHeight);
		out += ',';
		for (size_t i = 0; i < m_tiles.size(); ++i) {
			if (i != 0) out += ',';
			out += std::to_string(m_tiles[i]);
		}
		m_gridData = std::move(out);
	}

} // namespace Shit
