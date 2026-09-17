"""导入屋顶路线与五米深水；仅关卡环境配置，不修改角色和游泳实现。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
p=ROOT/'SourceArt/HomeMap/sync_static_kit.py';exec(compile(p.read_text(encoding='utf-8'),str(p),'exec'))
sync_kit('rooftop_route_manifest.json');sync_kit('diving_pool_manifest.json')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
# 早期整片高窗已由带侧门的新墙窗替代；保留它会堵住可见门洞。
old=actors.get('HM_Clerestory_Side_550')
if old:
 assert isinstance(old,unreal.StaticMeshActor);ea.destroy_actor(old)
for row in json.loads((ROOT/'SourceArt/HomeMap/rooftop_route_manifest.json').read_text(encoding='utf-8')):
 for ins in row['instances']:actors[ins['label']].set_folder_path('HomeMap/Rooftop')
for r in json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8')):
 if not r['name'].startswith(('Pool_Side_','Pool_End_')):continue
 a=actors['HM_'+r['name']];a.set_actor_location(unreal.Vector(*r['loc']),False,False);a.set_actor_scale3d(unreal.Vector(*r['scale']))
volume=actors['HM_Pool_SwimmingVolume_Deep'];assert isinstance(volume,unreal.PhysicsVolume) and volume.get_editor_property('water_volume')
# 原体积 XY 与水面不变；下边界 -570，覆盖五米池底下方，保留原浅水台阶 Walking 区。
volume.set_actor_location(unreal.Vector(0,1265,-294),False,False);volume.set_actor_scale3d(unreal.Vector(4.25,6.25,2.76))
actors['HM_Pool_Base'].set_folder_path('HomeMap/Pool');actors['HM_Site_Ground'].set_folder_path('HomeMap/Landscape')
actors['HM_Site_Ground'].static_mesh_component.set_material(0,unreal.load_asset('/Game/Environment/HomeMap/Materials/Hub/M_HomeGardenSurface'))
assert actors['HM_Pool_Water'].get_actor_location().z==-18
print('saved',unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True))
print('ROOFTOP_AND_500CM_POOL_APPLIED')
