# HomeMap 正式关卡与工作区交接

## 正式入口

- 当前地图：/Game/Environment/HomeMap/Maps/HomeMap_Courtyard。用户于 2026-09-06 将最新 BackUp 改为正式名称；旧关卡保留为 HomeMap_Courtyard_demise。
- 已重新打开正式地图，核对新主卧、双台盆、毛巾架、池底和昼夜控制台。Config/DefaultEngine.ini 的 EditorStartupMap 同步指向正式关卡；GameDefaultMap 继续为原 FrontEndMap。
- 正式 Blender 源是 SourceArt/HomeMap/HomeMap_Master.blend。项目根目录同名文件是旧副本，审查时只有已被替换的 Bath_Counter / Bath_Vanity 两个旧占位对象为独有；它不是后续制作入口。该根目录副本已由用户移除。

## 本轮环境成果

- 水面长度由 10 m 改为 16 m，水深 2.12 m，中央台阶之后有约 12.75 m 深水段；原住宅、入口和台阶保持。新增段真实池底约 Z=-229.9 cm，步道约 Z=0.1 cm，草地约 Z=-1.9 cm，均包含碰撞容差。
- 既有角色实际从近端庭院沿台阶走到 Y=1782.53，再沿台阶返回 Y=214.52，终态为步行；Reports/pool_extension_walk.json 保存过程。此结果只证明环境通行，游泳 Gameplay 尚未实现。
- SM_HM_ExpeditionSign：1,528 三角面；SM_HM_FieldNotesBoard：6,152；SM_HM_ArchiveCabinet：3,760，复用为楼上收藏柜；SM_HM_GalleryWritingDesk：1,692。全部放在 Architecture/Hub，使用原木材、金属、石材与织物，不新增纹理。
- 实体参考板不是某份已确认任务系统文档的落实，而是先前对出征准备区环境身份的设计解释。用户随后明确要求保留已经做好的板。本轮只有 StaticMesh 陈设，没有任务业务、任务奖励、独立服务器或 Listen Server 新逻辑。
- 文字面向的水平轴须与观察方向匹配；直接改 Blender 顶点后必须调用 mesh.update() 和 view_layer.update() 再 BFEU 导出，避免导出求值缓存仍是旧几何。文字已在 UE 实际复查。

## Git 整理

- 保留正式 Master、生产 FBX、已验收 ActorTransform 楼梯导出和对应检查点。
- 18 项未跟踪的临时检查文件、旧试验导出目录和早期 Blend 副本移到 Saved/HomeMap/LocalArchive_20260906，未删除。该目录的 manifest.json 记录原路径和归档路径。它们是本机归档，不是正式构建依赖。
- BFEU 自动生成的 SourceArt/HomeMap/ExportedAssets 只含导入脚本和日志，加入 .gitignore；正式 FBX 不忽略。
- 两份 Niagara Gallery 角色/控制器蓝图的检查性改动通过 UE SourceControl.revert_files 恢复，并重载包；不将它们混入 HomeMap 成果。
- 用户已有设计文档、参考图替换和 Luna Git 基线任务包独立保留提交。本轮仅保存任务包文件，没有执行其中的历史重写、仓库删除或 LFS 清理指令。

## 后续分工

- 游泳任务直接读取 Swimming_Handoff_泳池环境接口.md，里面是当前关卡、水域范围、台阶、验证和已有系统边界。
- 建议先让 Luna 完成 Localization_Lyra 语言切换，再单独安排游泳。两个任务都应从本次干净提交开始；游泳必须检查现有 CharacterMovement、动画与联网实现。
- 环境主线仍在房间和景观深化阶段。健身、瑜伽、衣帽细节、外围叶片尺度与整体材质层次仍需继续；本轮没有重新宣布性能达标，完整场景后统一复测目标硬件。
