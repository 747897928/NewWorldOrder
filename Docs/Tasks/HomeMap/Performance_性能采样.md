# HomeMap 2026-09-06 性能采样

## 2026-09-12 林带增量后的 High 验证

- 当前 High CSV/JSON 已更新为本批 340 个新林带实例后的 11 视角结果；前一轮三档 High 历史版本在 a4edf418。Medium/Legacy 同名档案仍为 9 月 11 日，未包含新林带。
- RTX 5060 Ti / i5-10400F、1920×1080、High/Scalability 2、TSR、100%，仅本项目编辑器与独立游戏进程；采样期间编辑器实时更新关闭，进程正常退出后恢复。日志 HomeMap_Pavilion_Profile_High_20260912_183351.log，14,520 帧，各视角舍弃 180 帧、统计 1,020 帧，实际位置和分辨率断言通过。
- 十一视角 Frame P95 为 10.771～12.845 ms，最差屋顶。屋顶 Frame 平均 10.992 ms、GPU 7.034 ms、RenderThread 约 10.96 ms、DrawCalls 约 724；庭院 Frame 平均 10.669 ms、GPU 8.616 ms、P95 12.147 ms。
- 上一轮屋顶 DrawCalls 约 716、GPU 6.906 ms，本轮约 724、7.034 ms；不能把单次差值当作精确新增成本或净优化收益。最大单帧仍达 49.347 ms，不宣称无卡顿、目标卡通过或全屋最终验收。
- 最后追加三个实景截图，均在性能统计段之后，见 Review/Woodland_Rooftop_High_20260912.png、Woodland_Gallery_High_20260912.png、Woodland_Courtyard_High_20260912.png。屋顶能看到新林丛；Gallery 当前构图仍以室内为主，窗外地表仍较空，不宣称所有窗景完成。
- 本轮没有重跑 Medium、Legacy、完整夜间或连续移动，也没有 Shipping/目标硬件数据。新林带的源几何、材质与剔除设置见 Woodland_外围林带深化.md。


## 2026-09-12 三档十一视角采样归档

- 本历史段 High 归档在 a4edf418，HomeMap_Pavilion_Medium.csv.gz、HomeMap_Pavilion_LegacyLow.csv.gz 及对应 performance_pavilion_*.json 是 9 月 11 日受控采样，包含 bcd52ac3 的康体区最终增量与新增健身、瑜伽视角。下方历史 High 归档可从 bcd52ac3 恢复。
- RTX 5060 Ti / i5-10400F，独立 UnrealEditor 游戏进程、RenderOffscreen、1920×1080、100% 分辨率比例。只保留本项目编辑器，采样时关闭实时视口，结束后恢复；无其他 UE 项目运行。每处剔除 180 帧、统计 1,020 帧，分辨率、相机位置及最终画质组断言通过。
- High/Scalability 2、TSR：十一视角 Frame P95 11.645～16.050 ms，最差健身房。Medium/Scalability 1、TSR：10.069～13.989 ms，最差衣帽间。LegacyLow/Scalability 0、TAA、关闭 Nanite/VSM/Lumen：9.295～12.030 ms，最差庭院。
- 三档稳定段 P95 低于 16.67 ms，但 High/Medium 仍有约 44/42 ms 单帧峰值；不能宣称无卡顿。High 多数视角渲染线程高于 GPU，不应无差别降低水面和纹理质量。
- Legacy 实景在全部稳定段之后抓取，避免截图成本污染统计。`Review/LegacyLow_Courtyard_20260911.png` 为庭院，`Review/LegacyLow_GymMirrorFail_20260911.png` 保留本批修复前的失败证据。随后通过同一 `CameraActor_9` 做局部复查，`Review/LegacyLow_GymMirror_20260912.png` 已确认健身镜回退为中性实体面，`Review/Gym_High_20260912.png` 确认 High 仍保留原 Lumen 反射；具体材质结论见 `Reports/legacy_mirror_fallback_20260912.json`。
- 修复使用 `/Game/Environment/HomeMap/Materials/Hub/M_Mirror` 的 Quality Switch：Default 保留 High/Medium 原值，Low 使用非金属烟熏镜回退；采样脚本显式设置 `r.MaterialQualityLevel`（LegacyLow=0、Medium=2、High=1）。没有新增 PlanarReflection、反射捕获或全局 Lumen，也没有重跑三档完整采样。
- 尚未完成目标显卡、完整移动、完整夜间或 Shipping 验收。Legacy 是当前 D3D12 工作站的降档成本测试，不是 D3D11/SM5 或 GTX 兼容性验证。下方历史未测状态不覆盖本轮新增证据。


## 2026-09-09 外围与屋顶休息区：并行编辑器负载下采样

- 本段 Reports/HomeMap_Pavilion_High.csv.gz 与 performance_pavilion_high.json 历史记录保留在 Git bcd52ac3；9 月 8 日的九视角归档保留在 Git 97b5e5f8。包括 25,600 面外围背景、72 个树实例、屋顶新座椅/花池/提灯及局部光源。
- RTX 5060 Ti / i5-10400F，独立游戏进程、RenderOffscreen、1920×1080、High/Scalability 2、TSR、ScreenPercentage=100。本项目编辑器实时视口关闭，采样进程正常退出后恢复；未改其他项目窗口或配置。
- 本次另有 FPSMultiplayerTemplate40、GameAnimationSample、MPSystemV3e51 三个 UE 编辑器运行。该条件与此前不同，CPU/GPU 外部竞争未经隔离；本结果不能用于判断新增资产的净回归幅度，不能据此盲目削减场景质量，也不能用于目标卡 60 FPS 验收。
- 日志 HomeMap_Pavilion_Profile_High_20260909_221148.log 于 22:19:37 正常关闭。实际 11,401 帧，九处位置、最终画质组和 1920×1080 分辨率断言通过，每处舍弃 180 帧后统计 1,020 帧。启动曾在 SteamSockets 初始化处等待，之后正常进入采样，未将等待误判成结束而重启。
- 各视角依次为 Frame 平均 / Frame P95 / GPU 平均，单位 ms：庭院 20.903 / 29.381 / 13.352；客厅 19.886 / 26.606 / 13.146；Gallery 19.669 / 24.251 / 16.583；厨房 19.865 / 25.432 / 16.745；花园 18.541 / 22.866 / 14.885；卧室 22.484 / 27.545 / 20.446；衣帽间 21.222 / 26.694 / 18.250；出征区 19.339 / 25.399 / 15.855；屋顶 19.549 / 24.679 / 14.938。
- 本条件下九处 P95 全部超过 16.67 ms，明确未通过稳定 60 FPS。渲染线程均值 18.3～22.5 ms；屋顶 DrawCalls 平均 701（上次 661），庭院 1,419（上次 1,396），增加实例并非没有成本，但调用数变化不能解释全部耗时变化。保留原始分类数据，后续须在受控条件下重测，不能把本次耗时全归因于树带。
- Medium、Legacy、完整移动和夜间性能仍未完成；本轮夜景截图仅作视觉检查，不是性能样本。此前章节的通过范围只对其对应提交有效。

## 2026-09-08 屋顶与五米深水后的九视角采样

- 本段 Reports/HomeMap_Pavilion_High.csv.gz 与 performance_pavilion_high.json 归档可从 Git 97b5e5f8 恢复；上一轮八视角归档可从 Git 8c62ec3c 恢复。包含最后 26 株屋顶植物、墙窗与端柱、整条屋顶路线和五米深水，不再沿用改造前采样。
- RTX 5060 Ti / i5-10400F，独立游戏进程、RenderOffscreen、1920×1080、High/Scalability 2、TSR、ScreenPercentage=100；编辑器视口实时更新关闭，独立进程结束后恢复。日志 HomeMap_Pavilion_Profile_High_20260908_222558.log 正常关闭。11,400 帧，九处相机位置、实际分辨率与最后生效画质组通过断言，各视角舍弃 180 帧后统计 1,020 帧。
- 以下依次为 Frame 平均 / Frame P95 / GPU 平均，单位 ms：庭院 12.667 / 15.137 / 10.611；客厅 13.181 / 16.336 / 8.579；Gallery 11.223 / 13.746 / 7.969；厨房 11.455 / 14.069 / 8.582；花园 10.465 / 12.694 / 6.991；卧室 11.042 / 13.279 / 8.972；衣帽间 11.484 / 13.758 / 8.352；出征区 11.095 / 13.173 / 8.804；屋顶 11.378 / 13.706 / 7.493。
- 本机九处稳定段 P95 均低于 16.67 ms，客厅最接近该预算；不能宣称无卡顿，也不能外推 RTX 3060/4060 已达到目标。固定相机采样不包含完整移动、夜间、Shipping 或目标旧显卡验证。没有因为这次通过而更改项目全局画质或降低全部模型精度。

## 2026-09-08 收纳与树冠更新后的八视角采样

- 本段归档保留在 Git 8c62ec3c；前次五视角原始数据保留在 Git 提交 72e46bb5。历史章节不再对应当前同名归档。
- RTX 5060 Ti / i5-10400F，独立 UnrealEditor 游戏进程、RenderOffscreen、1920×1080、High/Scalability 2、TSR、ScreenPercentage=100。日志 HomeMap_Pavilion_Profile_High_20260908_205327.log 正常关闭；10,201 帧，各视角丢弃切换后 180 帧后统计 1,020 帧，实际分辨率、最后生效的画质组和八处相机位置断言均通过。
- 各项依次为 Frame 平均 / Frame P95 / GPU 平均，单位 ms：庭院 10.767 / 13.154 / 8.122；客厅 9.764 / 11.521 / 7.281；Gallery 9.895 / 12.832 / 6.683；厨房 11.278 / 20.040 / 8.033；花园 11.596 / 18.802 / 6.987；卧室 11.406 / 17.473 / 8.933；衣帽间 11.438 / 16.965 / 8.460；出征区 9.255 / 11.517 / 7.112。
- 厨房、花园、卧室和衣帽间 P95 超过 16.67 ms，不能宣布稳定 60 FPS。不能仅凭单次运行差值认定某件新增资产是原因；本次仍需区分 CPU/渲染提交和 GPU 成本。
- 本次包含 32,304 面树冠、三套收纳/顶面木作和初版屋顶。采样之后为纠正用户指出的白板观感，将屋顶从 1,080 面扩展到 5,616 面，增加 26 个植物实例；最后这批内容不在采样内，必须补测，不能用本表验收最终关卡。
- Medium 日志 HomeMap_Pavilion_Profile_Medium_20260908_205734.log 虽正常结束，但末段为响应用户现场检查恢复了编辑器实时更新，测试条件变化；本轮不发布它作为受控比较结果。Legacy 尚未运行。脚本中的 Tier 0 是当前 D3D12 工作站关闭 Nanite/VSM/Lumen 的测试入口，不能视作 GTX 显卡或 D3D11 兼容验证。
- 当前仍缺目标 RTX 3060/4060、Medium、Legacy 硬件及完整移动、夜间和 Shipping 验证。分档支持已加到脚本不代表测试已完成。

## 2026-09-08 当前建筑改造后的有效五视角采样

- 本次包含开放楼梯入口、Gallery 墙窗、门与花池、跳台立柱、餐厨和庭院新增细节。正式地图 HomeMap_Courtyard，默认白天，RTX 5060 Ti / i5-10400F，独立 UnrealEditor 游戏进程，RenderOffscreen，1920×1080，High/Scalability 2，TSR，ScreenPercentage=100。
- 实际日志分辨率通过 1920×1080 断言；CSV 记录 6,600 帧。每个视角丢弃切换后 180 帧，再统计 1,020 帧，五处实际 XYZ 与预期相机位置均通过断言。进程自行结束，日志正常关闭。
- 庭院：Frame 平均 11.171 ms、P95 14.542 ms；GPU 平均 7.874 ms；RHI DrawCalls 平均 1,246。
- 客厅：Frame 平均 9.686 ms、P95 10.874 ms；GPU 平均 7.177 ms；RHI DrawCalls 平均 645。
- Gallery：Frame 平均 9.253 ms、P95 10.554 ms；GPU 平均 6.553 ms；RHI DrawCalls 平均 421。
- 厨房：Frame 平均 10.014 ms、P95 12.459 ms；GPU 平均 6.937 ms；RHI DrawCalls 平均 481。
- 花园：Frame 平均 9.112 ms、P95 10.637 ms；GPU 平均 5.921 ms；RHI DrawCalls 平均 197。
- 五处稳定段 P95 均低于 16.67 ms，但仍有单帧峰值：花园最大 42.237 ms，厨房 23.219 ms，庭院 21.605 ms。不能写成无卡顿或所有目标硬件稳定 60 FPS。
- 渲染线程仍普遍高于 GPU 时间。当前优先减少无价值的材质/实例提交和昂贵阴影；池水与透明通道不是这组固定白天相机的主要成本，不应为此降低整栋住宅建模质量。
- 原始无损 CSV：Reports/HomeMap_Pavilion_High.csv.gz；全部 GPU 分类、纹理流送、相机与帧统计：Reports/performance_pavilion_high.json。日志为 Saved/Logs/HomeMap_Pavilion_Profile_20260908_183021.log。
- 此结果替代下面 9 月 7 日的待测状态。仍不是 Shipping 包、整场景移动或夜间基准，也不是 RTX 3060/4060、Medium、Legacy 验证；纹理流送统计不代表 GPU 总显存使用。

## 2026-09-07 新增内容的采样状态

- 以下历史结果不包含最新 Pavilion 细节、直线跳台与 3.8 m 深水改造，不能用于新版验收。
- 新的三相机采样实际被运行时设置覆盖为 888×500，因此无效；未把其数值当作 1080p 结果提交。原始文件隔离在本地 Saved/HomeMapCheckpoints/InvalidPavilionProfile。
- SourceArt/HomeMap/profile_pavilion.ps1 已改为进入游戏后设置分辨率并开始 CSV；analyse_pavilion_profile.py 增加实际分辨率断言。用户要求先提交，修正后的采样尚未重跑。
- 当前桌面默认 TSR 已写入配置；本次没有 RTX 3060/4060、Medium 或 Legacy 的新验收结果。

## 条件与范围

- 关卡：HomeMap_Courtyard_BackUp；默认白天，独立 UnrealEditor 游戏进程，1920×1080，High 对应 Scalability 2，TSR，ScreenPercentage=100。
- 实测显卡为 RTX 5060 Ti。未在 RTX 3060、4060 或 Legacy 目标卡上运行，不能据此宣布目标硬件通过。
- 采样约 128 秒、7,421 帧；统计排除开始 15 秒，保留 6,907 帧。固定出生点相机为约 `(-50,-1040,150.25)`；不是全场景漫游，也不是打包后的 Shipping 性能。
- 测量时关闭编辑器视口实时更新，保留独立游戏进程运行。临时窗口和启动参数已恢复，没有修改项目全局画质默认值。
- 本采样在新树冠和门框加入后、两张躺椅摆放及最后一轮夜间灯光微调前完成。躺椅各 3,196 三角面；最后只调整现有灯光，没有增加灯光数量，仍需后续整体复测。
- 后续泳池加深到 212 cm，增加 4 个原有台阶模块实例，并局部下沉池下地形顶点；没有新增水体材质或反射通道。此几何更新也不包含在本次采样中。
- 主卧卫浴细化又新增四种源网格（7,332 / 2,732 / 6,144 / 3,840 三角面；窗帘实例两份），复用现有纹理，并新增一盏半径 300 cm、无阴影的局部 RectLight。这批增量尚未做独立性能复测，不把旧采样当成新版验收。

## 稳定段结果

- FrameTime：平均 16.386 ms，中位 15.483 ms，P95 23.722 ms，最大 67.663 ms。
- RenderThreadTime：平均 16.022 ms，P95 22.217 ms；GameThreadTime 平均 13.824 ms，P95 20.617 ms。
- GPUTime：平均 10.025 ms，P95 15.418 ms。
- RHI DrawCalls：平均约 943，P95 999。
- GPU Lights 平均 1.822 ms；ShadowDepths 0.783 ms；LumenReflections 0.464 ms；TSR 1.508 ms。
- GPU Translucency 平均 0.234 ms；SingleLayerWaterDepthPrepass 0.124 ms；SingleLayerWater 0.118 ms；VolumetricCloud 0.246 ms。
- TextureStreaming 记录的 WantedMips 约 34.7 MB、NonStreamingMips 约 146.4 MB，DesiredDataLoadedPercent=100。这些是该相机的流送统计，不等于场景全部纹理、GPU 总显存占用或目标 6 GB 显卡验证。

## 判断与后续优化方向

- 此采样中渲染线程耗时高于 GPU；优先核查重复构件、材质槽和阴影提交，避免只降低分辨率或继续无差别减面。
- 当前池水和透明通道在此视角下不是主要 GPU 成本。保留单层水，不增加 PlanarReflection；后续需补泳池近景、夹层俯瞰与夜间的移动采样。
- 启动日志累计报告至少 100 次 PSO 创建卡顿，另有角色 Mutable 初始化。启动成本与稳定场景成本分开看；本轮不修改已有角色系统。
- High 的稳定段 P95 已超过 16.67 ms；尚未达到“稳定 60 FPS”的验收标准。Medium、Legacy 和目标硬件仍未完成独立验收。

## 可复核记录

- 原始 CSV：`Saved/Profiling/CSV/Profile(20260906_123239).csv`。
- 仓库内保留无损压缩副本：`Reports/HomeMap_Day_1080p_High.csv.gz`。重跑统计脚本时可自动解压恢复。
- 独立进程停止后 CSV 文件尾部可能未完整刷新，统计仅使用成功解析的 7,421 行；这不是完整运行过程的无遗漏追踪。
- 结构化统计：`Reports/performance_standalone_high.json`。
- 日志：`Saved/Logs/HomeMap_Day_1080p_High_20260906_123158.log`。
- 统计脚本：`SourceArt/HomeMap/analyse_profile_csv.py`。
- VibeUE 独立采样服务返回了预期 trace 路径，但文件未生成；因此本次采用实际生成的 CSV 和日志。`performance_standalone_high_raw.json` 保留该失败信息。不能把缺失 trace 当成已完成 Insights 分析。
