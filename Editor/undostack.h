#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QString>

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <vector>

/// 快照型撤销/重做栈（场景级）
///
/// 每次编辑会话 = 一次"手势"：begin() 记录手势前场景快照 → 发生编辑 →
/// commit() 记录手势后的快照并对比；有差异才压栈，无差异丢弃。
/// undo()/redo() 返回要恢复到的场景快照（由外部应用到场景）；
/// 同时把"被离开的那一帧"补进反向栈，保证 undo/redo 往返一致。
class UndoStack
{
public:
    using SnapshotFn = std::function<nlohmann::json()>;

    /// 注入"取当前场景快照"的函数（由持有者实现，如 SceneSerializer::toJson 排除编辑器相机）
    void setSnapshotter(SnapshotFn fn) { m_snapshot = std::move(fn); }

    void clear();                 ///< 清空（新建/打开/保存后调用）
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    int undoCount() const { return static_cast<int>(m_undo.size()); }
    int redoCount() const { return static_cast<int>(m_redo.size()); }

    /// 开始一次编辑事务（before 快照）。若已处于事务中则复用原 before（连续编辑归为一步）
    void begin();
    /// 结束事务并提交；无实际差异返回 false
    bool commit(const QString &label);

    /// 撤销：返回应恢复的快照（无票可撤返回 nullopt）
    std::optional<nlohmann::json> undo();
    /// 重做：返回应恢复的快照
    std::optional<nlohmann::json> redo();

private:
    struct Entry {
        QString label;
        nlohmann::json before;
        nlohmann::json after;
    };

    std::vector<Entry> m_undo;
    std::vector<Entry> m_redo;
    std::optional<nlohmann::json> m_txn;  ///< 未提交的事务（before 快照）
    SnapshotFn m_snapshot;
};

#endif // UNDOSTACK_H