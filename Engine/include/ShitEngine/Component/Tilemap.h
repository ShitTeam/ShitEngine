#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "Component.h"
#include "RendererComponent.h"
#include <SDL3/SDL_rect.h>
#include <string>
#include <vector>

namespace Shit {
	class CameraComponent;

	/**
	 * @brief 2D 瓦片地图组件
	 *
	 * 把一张瓦片集纹理（sprite sheet）按 m_gridWidth×m_gridHeight 网格铺排成地图。
	 * 网格左上角对齐 GameObject 的 Transform 位置，每个格子从纹理裁取对应瓦片绘制。
	 *
	 * 瓦片 id 语义：
	 *   -1  = 空格（不绘制）
	 *   ≥0  = 瓦片索引，映射到纹理的 (id % 每行数, id / 每行数) 源矩形
	 *
	 * 网格数据以反射字符串字段 m_gridData 持久化（逗号分隔的瓦片 id，序列化器原生
	 * 支持 std::string），运行时用 m_tiles（std::vector<int>）高效存储；onAfterDeserialize
	 * 把 m_gridData 解析到 m_tiles。编辑器/脚本通过 setTile / resize 修改并自动同步 m_gridData。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) Tilemap : public RendererComponent {
		SHIT_REFLECT_BODY(Tilemap)
	public:
		Tilemap();
		~Tilemap() override;

		void onAfterDeserialize() override;
		void onFieldChanged(const std::string& fieldName) override;

		/// 按相机绘制网格（RendererComponent 纯虚实现）
		void onRender(SDL_Renderer* renderer, const CameraComponent* camera) const override;
		/// 世界坐标轴对齐包围盒（覆盖整个网格）
		SDL_FRect getGlobalBounds() override;

		// --- 配置 ---
		void setTexturePath(const std::string& path);
		const std::string& getTexturePath() const { return m_texturePath; }

		void setTileSize(int w, int h);               ///< 设置单个瓦片纹理像素尺寸
		int getTileWidth() const { return m_tileWidth; }
		int getTileHeight() const { return m_tileHeight; }

		void setGridSize(int cols, int rows);         ///< 调整网格行列数（保留重叠部分数据）
		int getGridWidth() const { return m_gridWidth; }
		int getGridHeight() const { return m_gridHeight; }

		void setTileWorldSize(const Vector2& size);   ///< 每格世界尺寸（默认=瓦片像素）
		const Vector2& getTileWorldSize() const { return m_tileWorldSize; }

		// --- 网格数据 ---
		void setTile(int col, int row, int tileId);   ///< 放置瓦片（-1 擦除）
		int getTile(int col, int row) const;          ///< 读取瓦片（越界返回 -1）
		void clear();                                 ///< 全部擦除
		bool isEmpty() const;                         ///< 是否全空
		int getTileCount() const;                     ///< 当前瓦片数量（非空格）

		// --- 序列化载体（m_gridData 供反射持久化；内部使用） ---
		const std::string& getGridData() const { return m_gridData; }
		void setGridData(const std::string& data);    ///< 反序列化用：解析外部字符串（含尺寸头）

	private:
		friend class TilemapDrawTool;   // 编辑器刷图工具直接访问网格

		// 反射可序列化字段
		SHIT_META(({.displayName = "Texture", .tooltip = "瓦片集纹理路径（sprite sheet）"}))
		std::string m_texturePath;
		SHIT_META(({.displayName = "Tile Width", .tooltip = "单个瓦片在纹理中的像素宽度"}))
		int m_tileWidth = 32;
		SHIT_META(({.displayName = "Tile Height", .tooltip = "单个瓦片在纹理中的像素高度"}))
		int m_tileHeight = 32;
		SHIT_META(({.displayName = "Grid Width", .tooltip = "网格列数"}))
		int m_gridWidth = 10;
		SHIT_META(({.displayName = "Grid Height", .tooltip = "网格行数"}))
		int m_gridHeight = 10;
		SHIT_META(({.displayName = "Tile World Size", .tooltip = "每格世界尺寸（默认=瓦片像素；留 0 自动）"}))
		Vector2 m_tileWorldSize{ 0.0f, 0.0f };
		// 序列化载体：逗号分隔瓦片 id（含尺寸头 [cols,rows,...]），随 .scene 持久化
		SHIT_META(({.displayName = "Grid Data", .tooltip = "序列化载体，由刷图工具自动维护", .readOnly = true}))
		std::string m_gridData;

		// 运行时网格（非反射，从 m_gridData 解析而来）
		SHIT_META(Disable)
		std::vector<int> m_tiles;

		/// 把 m_gridData 解析到 m_tiles（含尺寸头 [cols,rows,...]）
		void parseGridData();
		/// 把 m_tiles 序列化到 m_gridData（含尺寸头，供检查器/保存）
		void syncGridData();
		/// 确保 m_tiles 尺寸与 m_gridWidth×m_gridHeight 一致（扩容补 -1，缩容截断）
		void ensureTileBuffer();
	};
}
