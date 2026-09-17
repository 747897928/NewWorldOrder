"""只同步康体区新增围合套件与环境灯光，不执行旧器械重建。"""
import unreal
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
p=root/'SourceArt/HomeMap/sync_static_kit.py';exec(compile(p.read_text(encoding='utf-8'),str(p),'exec'))
sync_kit('wellness_envelope_manifest.json')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);a={x.get_actor_label():x for x in ea.get_all_level_actors()}
for name in ['HM_YogaFeatureWall','HM_WellnessCeilingDetail']:a[name].set_folder_path('HomeMap/Wellness')
# 主照明放在灯具下方，不能嵌入新吊顶内部；保持原来两个主灯的职责。
for name,z,intensity,temp,width,height in [('HM_Gym_Light',301,1750,4200,300,360),('HM_Yoga_Light',301,850,4200,280,300)]:
 light=a[name];pos=light.get_actor_location();light.set_actor_location(unreal.Vector(pos.x,pos.y,z),False,False)
 c=light.get_component_by_class(unreal.RectLightComponent)
 for k,v in {'intensity':intensity,'temperature':temp,'source_width':width,'source_height':height,'use_temperature':True}.items():c.set_editor_property(k,v)
 print('MODIFIED',name)
# 镜框洗墙半径原值已扩大到 1000 cm，本轮收回到康体空间附近。
a['HM_Gym_Mirror_Wash'].get_component_by_class(unreal.RectLightComponent).set_editor_property('attenuation_radius',450)
light=a.get('HM_Yoga_FeatureWash') or ea.spawn_actor_from_class(unreal.RectLight,unreal.Vector(1060,1320,278))
light.set_actor_label('HM_Yoga_FeatureWash');light.set_folder_path('HomeMap/Wellness');light.set_actor_location(unreal.Vector(1060,1320,278),False,False);light.set_actor_rotation(unreal.Rotator(pitch=-20,yaw=90,roll=0),False)
c=light.get_component_by_class(unreal.RectLightComponent);c.set_mobility(unreal.ComponentMobility.STATIONARY)
for k,v in {'intensity_units':unreal.LightUnits.LUMENS,'intensity':200,'temperature':3800,'use_temperature':True,'source_width':320,'source_height':35,'attenuation_radius':225,'cast_shadows':False,'volumetric_scattering_intensity':0}.items():c.set_editor_property(k,v)
print('MODIFIED',light.get_actor_label())
# 大面积主灯仅负责照度，镜内由真实线形灯具网格承担可见光源形状。
for n in ['HM_Gym_Light','HM_Gym_Mirror_Wash']:
 a[n].get_component_by_class(unreal.LightComponent).set_editor_property('specular_scale',0)
print('WELLNESS_ENVELOPE_SAVED',unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True))
