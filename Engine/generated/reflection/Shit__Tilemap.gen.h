#pragma once

#include <ShitEngine/Component/Tilemap.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_Tilemap() {
    Shit::ReflectType("Tilemap", sizeof(Tilemap))
        .Base("RendererComponent")
        .Field("m_texturePath",
            &Shit::Tilemap::m_texturePath, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Texture", .tooltip = "瓦片集纹理路径（sprite sheet）"})
        .Field("m_tileWidth",
            &Shit::Tilemap::m_tileWidth, "int")
        .Meta(Shit::FieldMeta{.displayName = "Tile Width", .tooltip = "单个瓦片在纹理中的像素宽度"})
        .Field("m_tileHeight",
            &Shit::Tilemap::m_tileHeight, "int")
        .Meta(Shit::FieldMeta{.displayName = "Tile Height", .tooltip = "单个瓦片在纹理中的像素高度"})
        .Field("m_gridWidth",
            &Shit::Tilemap::m_gridWidth, "int")
        .Meta(Shit::FieldMeta{.displayName = "Grid Width", .tooltip = "网格列数"})
        .Field("m_gridHeight",
            &Shit::Tilemap::m_gridHeight, "int")
        .Meta(Shit::FieldMeta{.displayName = "Grid Height", .tooltip = "网格行数"})
        .Field("m_tileWorldSize",
            &Shit::Tilemap::m_tileWorldSize, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Tile World Size", .tooltip = "每格世界尺寸（默认=瓦片像素；留 0 自动）"})
        .Field("m_gridData",
            &Shit::Tilemap::m_gridData, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Grid Data", .tooltip = "序列化载体，由刷图工具自动维护", .readOnly = true})
        .Factory<Tilemap>()
        .Register<Tilemap>();
    return true;
}

} // namespace Shit
