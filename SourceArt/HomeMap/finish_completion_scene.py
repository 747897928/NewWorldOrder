"""以已有家具和材质完成厨房、餐厅和花园；范围限于环境 Actor 与光照序列。"""
import unreal,json,math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()};changed=[]
def note(a):
    o,e=a.get_actor_bounds(False);r=a.get_actor_rotation()
    changed.append({'label':a.get_actor_label(),'mesh':a.static_mesh_component.static_mesh.get_path_name(),'location_cm':list(a.get_actor_location().to_tuple()),'rotation':[r.pitch,r.yaw,r.roll],'scale':list(a.get_actor_scale3d().to_tuple()),'bounds':[list(o.to_tuple()),list(e.to_tuple())]})
def prop(name,path,xy,bottom,yaw=0,scale=1,collision=False):
    a=actors.get(name)
    if not a:a=ea.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());a.set_actor_label(name);actors[name]=a
    assert isinstance(a,unreal.StaticMeshActor)
    c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset(path));c.set_mobility(unreal.ComponentMobility.STATIC);c.set_collision_profile_name('NoCollision')
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False);a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    o,e=a.get_actor_bounds(False);a.set_actor_location(a.get_actor_location()+unreal.Vector(xy[0]-o.x,xy[1]-o.y,bottom-o.z+e.z),False,False)
    a.set_folder_path('HomeMap/Completion');note(a)
    if collision:
        o,e=a.get_actor_bounds(False);pn=name+'_Collision';proxy=actors.get(pn)
        if not proxy:proxy=ea.spawn_actor_from_class(unreal.StaticMeshActor,o);proxy.set_actor_label(pn);actors[pn]=proxy
        proxy.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));proxy.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC);proxy.static_mesh_component.set_collision_profile_name('BlockAll')
        proxy.set_actor_location(o,False,False);proxy.set_actor_scale3d(unreal.Vector(e.x*.88/50,e.y*.88/50,min(e.z,45)/50));proxy.set_actor_hidden_in_game(True);proxy.set_is_temporarily_hidden_in_editor(True);proxy.set_folder_path('HomeMap/Collision')
    return a
base='/Game/Environment/HomeMap/'
for name in ['HM_KitchenJoineryFinish','HM_DiningFinish','HM_GardenLoungePavilion','HM_WingFacadeFinish','HM_GardenPavingFinish']:actors[name].set_folder_path('HomeMap/Completion')
for i,x in enumerate([900,990,1080]):
    prop('HM_IslandPendant_'+str(i),base+'Architecture/SM_Ceiling_Lamp_3',(x,-430),204,scale=.9)
for i,(y,mesh,bottom) in enumerate([(-785,'SM_Kitchen_Decor_19',135),(-675,'SM_Kitchen_Decor_9',135),(-570,'SM_Kitchen_Decor_18',135),(-775,'SM_Kitchen_Decor_11_White',193),(-720,'SM_Kitchen_Decor_11_Black',193),(-590,'SM_Kitchen_Decor_12',193)]):
    prop('HM_PantryDecor_'+str(i),base+'Props/'+mesh,(1355,y),bottom,yaw=90)
# 新灶面占据原水果位置，水果移到中岛另一端，避免陈设穿插。
prop('HM_Island_Fruit',base+'Props/SM_Bowl_with_Oranges',(1080,-445),101)
prop('HM_Island_Decor',base+'Props/SM_Kitchen_Decor_12',(1095,-395),101,yaw=25)
plant=actors['HM_Plant_3']
prop('HM_Plant_3',plant.static_mesh_component.static_mesh.get_path_name(),(1270,-905),0,yaw=25,scale=plant.get_actor_scale3d().x)
for i,x in enumerate([960,1160]):prop('HM_DiningPendant_'+str(i),base+'Architecture/SM_Ceiling_Lamp_2',(x,-120),205,scale=.9)
# 新凉亭复用项目沙发、扶手椅与茶几；家具占东侧与两端，西侧留连续入口。
prop('HM_GardenLounge_Sofa',base+'Furniture/SM_Sofa',(1410,1740),5,yaw=90,scale=1,collision=True)
prop('HM_GardenLounge_ChairA',base+'Furniture/SM_Armchair',(1080,1550),5,yaw=0,scale=1,collision=True)
prop('HM_GardenLounge_ChairB',base+'Furniture/SM_Armchair',(1080,1930),5,yaw=180,scale=1,collision=True)
table=prop('HM_GardenLounge_Table',base+'Furniture/SM_Coffe_Table_1',(1145,1740),5,scale=1.7,collision=True)
o,e=table.get_actor_bounds(False);prop('HM_GardenLounge_Bowl',base+'Props/SM_Kitchen_Decor_9',(1150,1740),o.z+e.z+.2)
# 已有花池中的盆体埋入土面，保留叶冠，避免整排花盆直接摆在种植床上。
for n,a in list(actors.items()):
    if n.startswith('HM_Garden_Plant_'):
        o,e=a.get_actor_bounds(False);prop(n,a.static_mesh_component.static_mesh.get_path_name(),(o.x,o.y),8,yaw=a.get_actor_rotation().yaw,scale=a.get_actor_scale3d().x)
# 更高的日间太阳缩短外围大树投进泳池的叶片影子；黄昏和夜晚仍由原交互序列控制。
seq=unreal.load_asset(base+'Cinematics/LS_HomeMap_DayNight')
for b in seq.get_bindings():
    if b.get_name()!='SunLight':continue
    for t in b.get_tracks():
        if not isinstance(t,unreal.MovieScene3DTransformTrack):continue
        for sec in t.get_sections():
            for c in sec.get_all_channels():
                if str(c.channel_name)=='Rotation.Y':
                    for k in c.get_keys():
                        if k.get_time().frame_number.value==0:k.set_value(-48)
unreal.EditorAssetLibrary.save_loaded_asset(seq)
actors['HM_Sun'].set_actor_rotation(unreal.Rotator(pitch=-48,yaw=-62,roll=0),False)
# 深水池灯移入水下并向池底照射；保持两盏 RectLight，不叠加新光源。
for n in ['HM_Pool_Light_-420','HM_Pool_Light_420']:
    a=actors[n];c=a.light_component;c.set_intensity(6000);c.set_editor_property('attenuation_radius',1100);c.set_editor_property('source_width',500);c.set_editor_property('source_height',18)
    p=a.get_actor_location();p.z=-165;a.set_actor_location(p,False,False);a.set_actor_rotation(unreal.Rotator(pitch=-20,yaw=0 if '-420' in n else 180,roll=0),False)
for n in ['HM_Pool_Light_Strip_-420','HM_Pool_Light_Strip_420']:
    p=actors[n].get_actor_location();p.z=-155;actors[n].set_actor_location(p,False,False)
c=actors['HM_Balcony_Warm'].light_component;c.set_editor_property('attenuation_radius',720);c.set_intensity(2800)
for b in seq.get_bindings():
    parent=b.get_parent().get_name()
    for t in b.get_tracks():
        if not hasattr(t,'get_property_name') or str(t.get_property_name())!='Intensity':continue
        for sec in t.get_sections():
            for ch in sec.get_all_channels():
                for key in ch.get_keys():
                    if key.get_time().frame_number.value==120 and parent in ['MoonLight','SkyLight']:key.set_value(32 if parent=='MoonLight' else 1.2)
unreal.EditorAssetLibrary.save_loaded_asset(seq)
# 调整已有灯，避免灯光数量随陈设数量增长。
for name,intensity,radius in [('HM_Kitchen_Light',1700,630),('HM_Dining_Light',1300,460),('HM_Gallery_Warm',2300,650)]:
    c=actors[name].light_component;c.set_intensity(intensity);c.set_editor_property('attenuation_radius',radius)
look=actors['HM_Look'].settings;look.set_editor_property('white_temp',6500);actors['HM_Look'].settings=look
# 新植被集中实例化，不把每一株变成独立动态 Actor。
ftpath=base+'Landscape/FT_HM_GardenFill'
if not unreal.EditorAssetLibrary.does_asset_exist(ftpath):
    unreal.FoliageService.create_foliage_type(base+'Props/SM_Plant_1',base+'Landscape','FT_HM_GardenFill',.6,.8,False,0,0,9000)
ft=unreal.load_asset(ftpath);assert ft
ft.set_editor_property('cast_dynamic_shadow',False)
ft.set_editor_property('enable_density_scaling',True)
unreal.EditorAssetLibrary.save_loaded_asset(ft)
count=unreal.FoliageService.get_instance_count(ftpath)
if count<=0:
    locs=[unreal.Vector(x,y,-22) for x in [-1550,-1370] for y in [1510,1740,1970]]
    locs += [unreal.Vector(x,2290,-22) for x in [-1450,-1120,-790,790,1120,1450]]
    print('FOLIAGE',unreal.FoliageService.add_foliage_instances(ftpath,locs,.6,.8,False,True,False))
le.save_current_level()
(ROOT/'Docs/Tasks/HomeMap/Reports/completion_placement.json').write_text(json.dumps(changed,indent=2),encoding='utf-8')
print('COMPLETION_DRESSING_SAVED',len(changed),'placements')
