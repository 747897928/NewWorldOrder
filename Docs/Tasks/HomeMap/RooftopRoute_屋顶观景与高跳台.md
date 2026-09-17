# 屋顶观景与双高度跳水路线

## 后续更新

- 2026-09-09 已将本页记录的临时长凳改成软垫休息区，并新增外围背景、树带和一盏桌灯；最新源文件入口与实景见 LandscapeLounge_外围与屋顶休息区.md。本页下文保留屋顶路线初版的制作和测试条件，不将当时的“无新增灯光”描述套用到最新关卡。

## 设计与边界

- 保留 U 型主体、中央挑空、现有 3.52 m 低跳台；新增中央屋顶观景平台，不制作完整三层房间。高频衣柜、昼夜控制和副本入口仍在一层。
- 路线为原楼梯 → Gallery 东侧常开侧门 → 22 级外楼梯 → 中央屋顶 → 东侧高跳台。外楼梯净宽约 180 cm，平台跨步 22 cm，踏步高约 16.73 cm、深 30 cm。原角色 MaxStepHeight=45 cm，未调整该属性。
- 屋顶行走面 Z=742，高跳板面 Z=744，水面 Z=-18；高跳点离水约 7.62 m，X=180..340，前端 Y=1000。低台 X=-370..-190，两条跳水线分开。高跳台端头开放，其余长边有可见金属立柱和横杆。
- 上层外楼梯与高跳台支撑落在东翼屋顶，避免把新增立柱放到一层楼梯入口。中央屋顶设木石长凳；没有新增动态灯、Gameplay 输入或交互系统。
- 补楼梯东侧实墙、窗洞、窗框和玻璃；侧门在 Y=-910..-730。旧 HM_Clerestory_Side_550 整片高窗已被替代，其透明碰撞会封住新门，必须同时从 UE、Master 和 architecture.json 移除，不能只藏掉画面。
- Gallery 与阳台玻璃栏的十处端点补立柱和底座。仍需以实际镜头检查围合与通行，不能仅凭构件数量判断完成。

## 泳池环境配置

- 水面、池沿、11 级入口台阶保持原位，浅水池底仍 -230。Y=640..940 坡降至 -518，Y=940..1900 为平底，深水 500 cm；池下地形局部下沉到 -590。
- HM_Pool_SwimmingVolume_Deep 仍是原生 APhysicsVolume，沿用该类的 bWaterVolume、FluidFriction 和 Priority。这里只改 Actor 位置/缩放，下界 -570、上界 -18，XY 范围保持 X=-425..425、Y=640..1890，不改游泳代码。
- 低台与高台都复用已有 Falling 入水预测和 Swimming 动画；验证以真实角色运动与状态记录为准。

## 资产与保存

- 主源 SourceArt/HomeMap/HomeMap_Master.blend 已保存；改造前本地检查点 Saved/HomeMapCheckpoints/HomeMap_BeforeRooftopRoute.blend，完整前态还可从 Git 8c62ec3c 恢复。
- 新增八套网格共 26,808 三角面，普通 StaticMesh、ComplexAsSimple，复用现有木/石/黑金属/玻璃材质，没有新贴图。池底仍 28 面，池下地形仍 5,000 面。
- 构建入口 build_rooftop_route.py；深水增量 deepen_rooftop_pool.py；导入 place_rooftop_route.py 读取 rooftop_route_manifest.json 和 diving_pool_manifest.json；finish_rooftop_source.py 同步移除已替代的整片侧窗。
- 历史 deepen_diving_pool_source.py 与 place_diving_terrace.py 会恢复旧水深/体积底部，不得再当成最新制作入口直接执行。
- 导入曾超过 MCP 的 30 秒返回时限；随后逐项复读确认全部八套 Actor 已存在、体积下界已更新、dirty packages 为 0。超时本身不作为成功依据，也没有重复盲导。

## 验证状态

- Reports/rooftop_route_walk.json：使用真实 BP_ShootCharacter，仅初始归位，之后通过 CharacterMovement 连续从入口走到屋顶、高跳台；14/14 路径点通过，经历 Falling 后在约 (260,1492,-40) 进入 Swimming，没有落到原低跳台。首次侧门处的旧高窗碰撞已修正后重测。
- Reports/rooftop_pool_return.json：接上述角色位置连续下潜到 Z=-400，再游回原浅水台阶，3/3 目标通过，最终恢复 Walking；池底射线实际命中浅水 -230、坡面 -374、深水 -518。
- Review/StairSide_20260908.png、Rooftop_20260908.png、RooftopOverview_20260908.png 为本次 UE 实景，不是效果图。屋顶已可到达，但远景仍明显是早期起伏地形和简单天际线；屋顶生活感、远景质量和夜间舒适度仍须继续制作，不能称为正式最终视觉完成。
- PIE 自动化必须显式给 StartPIE.options.startTransform 的合法出生高度。裸调用的默认变换曾在 (0,0,0) 地板内生成失败；这是本次测试入口问题，不修改正式 PlayerStart 或角色 Spawn 逻辑。退出后用 LevelEditorSubsystem.is_in_play_in_editor 实际确认；异步 StopPIE 返回句柄不等于已经退出。
- 本版重新完成 High 九视角采样，包含屋顶路线、五米深水与最后种植增量；本机 Frame P95 为 12.694～16.336 ms，条件及原始 CSV 见 Performance_性能采样.md。目标硬件、夜间及移动性能仍待验证。
