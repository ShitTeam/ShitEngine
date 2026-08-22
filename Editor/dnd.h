#pragma once
// ═══════════════════════════════════════════════════════════════
// 编辑器拖拽 MIME 约定（组件引用拖拽 / 场景树层级拖拽共用）
// ═══════════════════════════════════════════════════════════════

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QString>
#include <utility>

// 组件引用拖拽（QDataStream 编码）：
//   [quint32 count, (quint64 uuid, QString typeName) × count]
//   - 检查器组件头拖拽：count = 1（自身）
//   - 场景树对象拖拽：count = 该对象全部组件的数量（目标字段自动挑第一个可赋值的）
//   同时保留对象路径 MIME，场景树内部拖拽改层级不受影响。
inline const char kDndComponentRef[] = "application/x-shitengine-componentref";

// 场景树层级拖拽：[QList<int> rows]（根到节点行号路径）
inline const char kDndObjectPath[] = "application/x-shitengine-objectpath";

/// 编码组件引用列表
inline QByteArray encodeComponentRefs(const QList<std::pair<quint64, QString>>& items)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << static_cast<quint32>(items.size());
    for (const auto& [uuid, typeName] : items)
        stream << uuid << typeName;
    return bytes;
}

/// 解码组件引用列表（格式非法/损坏返回空列表）
inline QList<std::pair<quint64, QString>> decodeComponentRefs(const QByteArray& data)
{
    QList<std::pair<quint64, QString>> items;
    QDataStream stream(data);
    quint32 count = 0;
    stream >> count;
    if (stream.status() != QDataStream::Ok) return items;
    for (quint32 i = 0; i < count && stream.status() == QDataStream::Ok; ++i) {
        quint64 uuid = 0;
        QString typeName;
        stream >> uuid >> typeName;
        items.append({ uuid, typeName });
    }
    return items;
}
