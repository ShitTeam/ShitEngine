#include "undostack.h"

#include <utility>

void UndoStack::clear()
{
    m_undo.clear();
    m_redo.clear();
    m_txn.reset();
}

void UndoStack::begin()
{
    if (m_txn) return;   // 已在事务中：复用该 before，后续变化归入同一步
    m_txn = m_snapshot ? m_snapshot() : nlohmann::json();
}

bool UndoStack::commit(const QString &label)
{
    if (!m_txn) return false;
    nlohmann::json before = std::move(*m_txn);
    m_txn.reset();
    if (!m_snapshot) return false;

    const nlohmann::json after = m_snapshot();
    if (before == after) return false;   // 无实际变化：不入栈

    m_undo.push_back({ label, std::move(before), after });
    m_redo.clear();                       // 新编辑使重做分支失效
    return true;
}

std::optional<nlohmann::json> UndoStack::undo()
{
    if (m_undo.empty() || !m_snapshot) return std::nullopt;
    m_txn.reset();                         // 悬空事务作废

    Entry e = std::move(m_undo.back());
    m_undo.pop_back();

    // 当前帧（= e.after）补进重做栈，先记为 before，重做时再换成最新
    const nlohmann::json current = m_snapshot();
    m_redo.push_back({ e.label, current, std::move(e.after) });
    return e.before;
}

std::optional<nlohmann::json> UndoStack::redo()
{
    if (m_redo.empty() || !m_snapshot) return std::nullopt;
    m_txn.reset();

    Entry e = std::move(m_redo.back());
    m_redo.pop_back();

    const nlohmann::json current = m_snapshot();
    m_undo.push_back({ e.label, std::move(e.before), current });
    return e.after;
}