"""应用直线跳台、阳台开口和深水池壳；PhysicsVolume 只延长底部，不改移动逻辑。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
p=ROOT/'SourceArt/HomeMap/sync_static_kit.py';exec(compile(p.read_text(encoding='utf-8'),str(p),'exec'))
sync_kit('diving_terrace_manifest.json');sync_kit('diving_pool_manifest.json')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
records=json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8'))
for r in records:
    n=r['name']
    if not n.startswith(('Balcony_Front_Guard','Balcony_West_Guard','Pool_Side_','Pool_End_')):continue
    a=actors.get('HM_'+n)
    if a is None:
        a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*r['loc']));a.set_actor_label('HM_'+n);actors['HM_'+n]=a
        c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset('/Game/Environment/HomeMap/Architecture/Hub/'+r['mesh']))
        mat='M_Metall_Black' if n.endswith('_Cap') else 'Hub/M_Architectural_Glass'
        c.set_material(0,unreal.load_asset('/Game/Environment/HomeMap/Materials/'+mat));c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('BlockAll')
    a.set_actor_location(unreal.Vector(*r['loc']),False,False);a.set_actor_scale3d(unreal.Vector(*r.get('scale',[1,1,1])))
    a.set_folder_path('HomeMap/Pool' if n.startswith('Pool') else 'HomeMap/Gallery')
for n in ['HM_DivingTerrace','HM_DivingTerraceGuard','HM_DivingTerraceInlay']:actors[n].set_folder_path('HomeMap/DivingTerrace')
actors['HM_Pool_Base'].set_folder_path('HomeMap/Pool');actors['HM_Site_Ground'].set_folder_path('HomeMap/Landscape')
volume=actors['HM_Pool_SwimmingVolume_Deep']
assert isinstance(volume,unreal.PhysicsVolume) and volume.get_editor_property('water_volume')
volume.set_actor_location(unreal.Vector(0,1265,-234),False,False);volume.set_actor_scale3d(unreal.Vector(4.25,6.25,2.16))
assert actors['HM_Pool_Water'].get_actor_location().z==-18
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('STRAIGHT_DIVING_AND_380CM_POOL_SAVED')
