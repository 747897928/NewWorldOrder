"""应用围合与入口改造；只移除明确被新建筑替代的静态环境实例，保留资产本体。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
removed=[]
retired=['HM_Living_Glass_350','HM_Living_Sliding_Glass_1','HM_Living_Sliding_Glass_2','HM_Living_Sliding_Glass_3','HM_LivingSlidingDoorDetail','HM_Clerestory_South','HM_Clerestory_Side_-550','HM_Terrace_Chair_1','HM_Terrace_Chair_1_Collision']
for n in retired:
    a=actors.get(n)
    if a:
        assert isinstance(a,unreal.StaticMeshActor),n
        assert ea.destroy_actor(a),n
        removed.append(n)
# 原共享网格不重导，仅缩短两处实例的一层竖框；二层玻璃仍有支承。
for n in ['HM_Living_Mullion_140','HM_Living_Mullion_570']:
    a=actors[n];a.set_actor_location(unreal.Vector(a.get_actor_location().x,0,521),False,False)
    a.set_actor_scale3d(unreal.Vector(1,1,338/690))
for n,z in [('HM_Courtyard_Lantern_500_100',30),('HM_Courtyard_Lantern_Glow_500_100',57),('HM_Courtyard_Warm_500_100',70)]:
    actors[n].set_actor_location(unreal.Vector(-760,1420,z),False,False)
for r in json.loads((ROOT/'SourceArt/HomeMap/architectural_closure_manifest.json').read_text(encoding='utf-8')):
    for ins in r['instances']:actors[ins['label']].set_folder_path('HomeMap/Pavilion')
for n in ['HM_DivingTerrace','HM_DivingTerraceGuard','HM_DivingTerraceInlay']:actors[n].set_folder_path('HomeMap/Pool')
for n,x in [('HM_GalleryCurtain_West',-515),('HM_GalleryCurtain_East',535)]:
    a=actors[n];v=a.get_actor_location();v.x=x;a.set_actor_location(v,False,False)
# 日间上限放开两档 EV，白色石材不再因曝光被钳住而大面积溢白；夜间下限不变。
s=actors['HM_Look'].settings;s.set_editor_property('auto_exposure_max_brightness',12.0);actors['HM_Look'].settings=s
# 复用原植物并把盆体埋入花池，避免门前只有孤立模型和空白地坪。
for i,x in enumerate([-380,380]):
    n='HM_EntryPlant_'+str(i);a=actors.get(n)
    if a is None:a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());a.set_actor_label(n)
    c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Game/Environment/HomeMap/Props/SM_Plant_2'));c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('NoCollision')
    a.set_actor_scale3d(unreal.Vector(1.35,1.35,1.35));a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=35+i*100,roll=0),False)
    o,e=a.get_actor_bounds(False);a.set_actor_location(a.get_actor_location()+unreal.Vector(x-o.x,-1230-o.y,10-(o.z-e.z)),False,False);a.set_folder_path('HomeMap/Arrival')
if 'HM_EntryPlanters' in actors:actors['HM_EntryPlanters'].set_folder_path('HomeMap/Arrival')
# 静态默认常开；未来接交互时两扇须改 Movable，并由现有交互系统处理同步。
report={'removed_environment_instances':removed,'entry_clear_width_cm':252,'doors':{'HM_EntryDoor_Left':{'hinge_cm':[-126,-1008,1],'closed_yaw':0,'open_yaw':-100},'HM_EntryDoor_Right':{'hinge_cm':[126,-1008,1],'closed_yaw':180,'open_yaw':280}},'stair_approach_open_x_cm':[140,550],'gallery_west_walk_original_width_cm':200,'gallery_window_sill_projection_cm':18}
report['retired_environment_instances']=retired
(ROOT/'Docs/Tasks/HomeMap/Reports/architectural_closure.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
le.save_current_level()
print('ARCHITECTURAL_CLOSURE_APPLIED',removed)
