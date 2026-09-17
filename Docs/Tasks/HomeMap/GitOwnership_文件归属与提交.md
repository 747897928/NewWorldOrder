# HomeMap 文件归属与提交

## 必须提交

- Content/Environment/HomeMap：正式关卡、模型、材质及其他被引用资源。关卡与新增依赖必须一起提交，不能只提交 umap。
- SourceArt/HomeMap/HomeMap_Master.blend：当前正式 Blender 源文件，不能忽略或当垃圾删除。
- SourceArt/HomeMap/BFEU_Production/StaticMesh：正式导出的 FBX。已跟踪的必要导出信息也保留。
- SourceArt/HomeMap 的制作/导入脚本、资产清单 JSON、production_state.json：记录坐标、材质、碰撞、导入与复现方式，属于生产源文件。历史脚本的可重跑范围以对应文档为准，不能因为脚本名字很多就整目录忽略。
- Docs/Tasks/HomeMap：状态、交接、选定的 UE 评审截图与可复核测试/性能记录。

## 已忽略，不需要提交

- Saved：临时截图、MCP 请求与响应、日志、性能原始临时输出及本地检查点；仓库根规则已覆盖。
- SourceArt/HomeMap/ExportedAssets：BFEU 自动生成的辅助导入文件；项目 .gitignore 已覆盖。
- SourceArt/HomeMap/*.blend1 至 *.blend9：Blender 自动滚动备份；本次补充精确规则。正式 .blend 不在此规则内。
- 项目工具缓存、构建产物沿用现有规则，不为了清空工作区新增大范围 Content/、SourceArt/ 或 *.uasset 忽略规则。

## 提交边界

- 每个可保存、可复查的制作批次结束后提交并推送，不等待整栋住宅完成或额度耗尽。额度检查只是提醒，无法防止突然断线，因此主要依靠短批次保存点。
- 提交前同时检查暂存区和未暂存区。本任务只提交 HomeMap 相关文件；若其他任务已有暂存改动，使用明确路径的 git commit --only，保持其暂存内容不变。
- 2026-09-11 检查发现火箭筒、角色动画、角色网格、TestMap_ListenServer 等其他任务改动，以及项目目录外的三张图片。它们不是本轮 HomeMap 产出，未删除、未忽略，也未混入本次提交。
- 推送后核对远端提交号，再汇报 HomeMap 是否还有未提交内容；其他任务仍有改动时，不宣称整个仓库干净。
