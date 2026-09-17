# 中央空间与直线跳水台

## 最新屋顶路线

- 2026-09-08 保留本页的低跳台，新增中央屋顶观景和东侧高跳点，深水池底进一步下沉到 -518，水深 5 m。最新构建入口、侧门碰撞替换与实走报告见 RooftopRoute_屋顶观景与高跳台.md；本页以下的 3.8 m 深水数值及早期脚本属于历史制作阶段。

## 2026-09-08 更新

- 跳台护栏已补两侧各四根可见立柱、底座与玻璃夹具，护栏由 456 增至 3,912 三角面，其余主体尺寸不变。
- 先前重复测试失败已定位为低频输入回调造成目标过冲；仅调整测试器靠近目标时的输入，5/5 路径点与入水状态重新通过。Reports/diving_terrace_walk.json 已更新，旧失败可从 Git 历史复核。
- 新增门外至跳台的连续 9 点验证通过。楼梯入口清空和 Gallery 围合见 ArchitecturalClosure_入口与上层围合.md；本页以下保留 9 月 7 日保存时的历史状态。
- 新有效五视角 1080p High 采样已完成，条件与限制见 Performance_性能采样.md。不能据此推定目标显卡、夜间或完整移动场景均已验收。

## 当前保存内容

- 正式地图：/Game/Environment/HomeMap/Maps/HomeMap_Courtyard，默认白天。
- Living Pavilion 新增三环吊灯、Gallery 木石背景与几何浮雕、屋檐木饰面；复用主卧窗帘模型。修正茶几尺寸、杯书落位、地毯范围与座椅间距；只调整原客厅局部灯，没有新增动态灯。
- 跳台从阳台正面 X=-370..-190 的开口向泳池长轴延伸，Y=145..890，顶面 Z=352；宽 180 cm，玻璃护栏止于 Y=830，末端 60 cm 开放。已撤掉初版屋顶绕行几何，恢复阳台西侧护栏。
- 池水与池沿标高不变。浅水池底 -230 cm，Y=640..940 坡降到 -398 cm，之后为 380 cm 深水段。原 11 级台阶保留，池下地形下沉到 -470 cm。
- 原生 PhysicsVolume 的底部由 -270 扩到 -450，水面与 XY 范围保持。没有修改 Character、Camera、Interaction、Experience、动画或网络实现。

## 资源与成本

- 新跳台主体 648 三角面、护栏 456、铺装嵌条 2,052；新连续池底 28。均为 Static，无新增纹理、动态灯或反射通道。
- Pavilion 吊灯 4,200 三角面、Gallery 背景 6,832、屋檐细节 1,080。材质沿用 HomeMap 木材、石材、金属和发光材质。
- 可行走跳台、护栏和池底使用 Complex As Simple；装饰嵌条、吊灯关闭碰撞。小型套件不启用 Nanite。
- 这些是几何预算记录，不是目标硬件性能证明。最新场景独立性能复测待完成，见 Performance_性能采样.md。

## 源文件与导入

- 主源：SourceArt/HomeMap/HomeMap_Master.blend。正式资源在 /Game/Environment/HomeMap/Architecture/Hub。
- build_pavilion_details.py 与 finish_pavilion_source.py 记录之前已完成的 BFEU 增量生产；不要直接重跑一键覆盖既有内容。
- build_diving_terrace.py 只重建自己拥有的三个 HM_DivingTerrace 对象；deepen_diving_pool_source.py 更新连续池底、池壁、池下地形与阳台开口，并同步 architecture.json。
- 当前机器 BFEU 安装副本启用失败，报缺失内部 fbxio/io_scene_fbx_4_4；不修改第三方源码。最新跳台和池底暂用 Blender 内置 FBX，Forward=-Z、Up=Y、米制、Actor 单位变换；包围盒和碰撞射线已在 UE 检查。
- UE 执行 place_diving_terrace.py 调用显式清单同步，仅触及对应静态模型与现有 PhysicsVolume 变换。新增清单为 diving_terrace_manifest.json 与 diving_pool_manifest.json。
- 跳台改型改变了材质槽数量，UE 重导会保留旧槽。仅该清单启用 rebuild_material_slots，并验证分段数量后按新 FBX 顺序重建材质槽。StaticMeshEditorSubsystem.set_lod_material_slot 的参数顺序是 material_slot_index、lod_index、section_index，调用使用关键字防止错绑。
- 当前材质与变换已保存，不要为了恢复 BFEU 而重导已正确内容。

## 已完成验证与交接限制

- 客厅家具至楼梯、Gallery 实走 11 个路径点通过：Reports/pavilion_walk.json。
- 直线跳台首次实测从原楼梯连续经过 5 个路径点，在约 (-282,1387,-35) 进入 MOVE_Swimming，未触及池阶；这次成功结果已在会话工具输出核对。
- 后续重复自动化未完成，在 Gallery 的 (-450,-730) 目标附近停住，最新原始报告为 Reports/diving_terrace_walk.json。必须保留该失败，不宣称重复稳定通过。下一轮先区分输入注入、转向过冲与场景碰撞原因。
- 下潜目标 Z=-280、游回浅水台阶并恢复 Walking 已完成，池底射线命中 -230、坡面 -314、深水底 -398；记录在 Reports/deep_pool_return.json。附加向东探测在原家具附近受阻，报告单独标记，不作为泳池出口。
- 当前用户要求先整理并提交成果；此提交是制作检查点，尚需重复通行和新版性能验收。
- 本地 Saved 检查点与失败性能文件不提交。仓库关卡检查点仍引用共享网格；完整恢复应同时取回 Git 对应的网格与 Blender 源文件。
