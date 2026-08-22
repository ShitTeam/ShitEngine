#ifndef ASSETPATHS_H
#define ASSETPATHS_H

class QString;

/// 统一资产路径服务（编辑器内唯一入口，替代此前各 Dock 的四份分歧实现）：
/// - 存储规范：项目内路径存「相对项目根」，逃逸项目或无项目存绝对
/// - 解析：相对路径先试项目根、再试 exe 目录兜底；绝对路径原样
/// 项目根由 MainWindow 打开/关闭项目时注入（setProjectRoot）。
namespace AssetPaths {

/// 设置项目根（空串 = 无项目，路径按绝对存储/解析）
void setProjectRoot(const QString& root);
const QString& projectRoot();

/// 规范化为存储形态：绝对路径在项目内 → 相对项目根；逃逸（../）或无项目 → 绝对
QString toRelative(const QString& path);

/// 解析为可加载的绝对路径：绝对原样；相对 → 项目根优先、exe 目录兜底
///（都解析不到时返回项目根拼接结果，让引擎报出可见的加载失败）
QString toAbsolute(const QString& path);

} // namespace AssetPaths

#endif // ASSETPATHS_H
