#ifndef PATHFIELDWIDGET_H
#define PATHFIELDWIDGET_H

#include <QString>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class QToolButton;

/// 路径字段的文件语义（由字段名关键字推导）
struct PathFieldSpec {
    bool isPath = false;       // 是否路径类字段（决定渲染为路径控件）
    QString fileFilter;        // QFileDialog 过滤器（浏览按钮用）
    QString dialogTitle;       // 浏览对话框标题
    QStringList suffixes;      // 接受拖入的后缀（小写；空 = 不限）
};

/// 按反射字段名推导路径语义：texture/sprite/sheet/tileset/icon → 图片，
/// audio/sound/music → 音频，font → 字体，anim/clip/state → 动画剪辑，
/// scene → 场景，prefab → 预置体；未命中返回 isPath=false
PathFieldSpec pathSpecForFieldName(const std::string& fieldName);

/// 「路径行 + 浏览按钮」复合控件：资源路径字段的统一编辑体验
/// - 拖拽：接受匹配后缀的本地文件（拖到哪个字段填哪个）
/// - 浏览：QFileDialog 按字段语义过滤（起始目录 = 项目根）
/// - 手输：相对（项目根基准）/绝对路径均可，统一解析
/// - 存储：提交值统一 AssetPaths::toRelative 规范化（项目内相对，逃逸绝对）
/// 变更经 pathCommitted 信号发出（一次一提交），由调用方接撤销栈；setPath 仅回显不发信号
class PathFieldWidget : public QWidget {
    Q_OBJECT
public:
    PathFieldWidget(const PathFieldSpec& spec, QWidget* parent = nullptr);

    /// 回显存储值（每帧回读用；不触发提交）
    void setPath(const QString& storedPath);
    QString path() const { return m_stored; }

signals:
    /// 用户通过拖拽/浏览/手输提交了新路径（值为规范化存储形态；清空 = 空串）
    void pathCommitted(const QString& storedPath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void commitAbsolute(const QString& absolutePath);   // 规范化后发信号
    void browse();
    bool suffixAccepted(const QString& suffix) const;

    PathFieldSpec m_spec;
    QString m_stored;
    QLineEdit* m_edit = nullptr;
    QToolButton* m_browseBtn = nullptr;
    QToolButton* m_clearBtn = nullptr;
};

#endif // PATHFIELDWIDGET_H
