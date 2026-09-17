import unreal, math, json
from pathlib import Path

ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
# 复用制作工具函数，不重复执行旧家具布局。
source=(ROOT/'SourceArt/HomeMap/dress_scene.py').read_text(encoding='utf-8')
exec(compile(source.split('# 客厅围合')[0],'scene_helpers','exec'))
exec(compile((ROOT/'SourceArt/HomeMap/light_scene.py').read_text(encoding='utf-8'),'light_scene.py','exec'))

# 黄昏主视觉以光源与材质平衡为主，曝光限制在温和范围。
sun=existing['HM_Sun'];sun.set_actor_rotation(unreal.Rotator(pitch=-9,yaw=-62,roll=0),False)
sun.light_component.set_intensity(950)
sun.light_component.set_editor_property('temperature',4100)
existing['HM_Sky'].light_component.set_intensity(.85)
pp=existing['HM_Look'];settings=pp.settings
for name,value in [('auto_exposure_min_brightness',5.8),('auto_exposure_max_brightness',6.3),('bloom_intensity',.18),('white_temp',5800.0)]:
    settings.set_editor_property('override_'+name,True);settings.set_editor_property(name,value)
pp.settings=settings

fog=actor(unreal.ExponentialHeightFog,'Landscape_Fog',(0,0,-150),folder='Landscape')
fc=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fc.set_editor_property('fog_density',.018)
fc.set_editor_property('fog_height_falloff',.22)
fc.set_editor_property('start_distance',2400)
fc.set_editor_property('fog_max_opacity',.45)
fc.set_editor_property('enable_volumetric_fog',False)
cloud=actor(unreal.VolumetricCloud,'Clouds',(0,0,0),folder='Landscape')
cc=cloud.get_component_by_class(unreal.VolumetricCloudComponent)
cc.set_editor_property('material',unreal.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst'))
cc.set_editor_property('layer_bottom_altitude',2.0)
cc.set_editor_property('layer_height',3.0)

# 楼梯与护栏已经 BFEU 验证。位置、网格与旋转以 architecture.json 为准，灯光脚本不得覆盖。

# 阴影只保留在有明确空间遮挡收益的灯上。
for name in ['Living_Key','Kitchen_Light','Bedroom_Light','Mission_Light','Gym_Light']:
    existing['HM_'+name].light_component.set_intensity(1500)
for name in ['Living_Window','Dining_Light','Wardrobe_Light','Bathroom_Light','Yoga_Light']:
    existing['HM_'+name].light_component.set_intensity(1000)
existing['HM_Living_Key'].set_actor_location(unreal.Vector(-200,-300,655),False,False)
existing['HM_Living_Key'].light_component.set_intensity(3800)
existing['HM_Living_Window'].set_actor_location(unreal.Vector(-200,-790,310),False,False)
for x in [-520,520]:existing['HM_Living_Cove_'+str(x)].set_actor_location(unreal.Vector(x,-490,680),False,False)
rect('Gallery_Warm',(-100,-810,670),2000,350,150,650,False)
rect('Balcony_Warm',(0,100,665),1100,700,100,450,False)
rect('Stair_Warm',(420,-270,660),1400,110,350,700,False)
for i,x in enumerate([-400,0,400]):
    prop('SM_Ceiling_Lamp_3','Gallery_Pendant_'+str(i),(x,-900,550))
for x in [-500,500]:
    for y in [100,1370]:
        a=actor(unreal.PointLight,'Courtyard_Warm_'+str(x)+'_'+str(y),(x,y,70),folder='Lighting')
        c=a.light_component;c.set_intensity(160);c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
        c.set_editor_property('attenuation_radius',300);c.set_editor_property('use_temperature',True);c.set_editor_property('temperature',2800);c.set_cast_shadows(False)
        shape('Courtyard_Lantern_'+str(x)+'_'+str(y),(x,y,30),(16,16,60),'Metal','Courtyard')
        shape('Courtyard_Lantern_Glow_'+str(x)+'_'+str(y),(x,y,57),(12,12,5),'Glow','Courtyard',False)
for x in [-420,420]:
    rect('Pool_Light_'+str(x),(x,800,-55),650,30,8,640,False,(0,0 if x<0 else 180,0),6500)
    shape('Pool_Light_Strip_'+str(x),(x,800,-45),(3,450,4),'Glow','Pool',False)

# Gallery 的陈设只占后带，西侧廊和观景阳台保持通畅。
prop('SM_Carpet','Gallery_Rug',(-120,-825,353),0,1.15)
prop('SM_Sofa','Gallery_Sofa',(-140,-933,353),0,.9,True)
prop('SM_Coffe_Table_1','Gallery_Table',(-140,-805,353),0,.8,True)
prop('SM_Stack_of_Books_2','Gallery_Books',(-155,-810,386))
prop('SM_Chair_2','Gallery_Study_Chair',(260,-840,353),180,1,True)
shape('Gallery_Desk',(260,-960,431),(190,62,8),'Stone','Gallery')
for x in [185,335]:shape('Gallery_Desk_Leg_'+str(x),(x,-960,392),(8,52,78),'Metal','Gallery')
prop('SM_Opened_Book','Gallery_Study_Book',(240,-960,436))
prop('SM_Vase_4','Gallery_Study_Vase',(320,-960,436))
prop('SM_Plant_1','Gallery_Plant',(-480,-950,353),20,1.4)
for z in [445,520,590]:
    shape('Gallery_Collection_Shelf_'+str(z),(-535,-805,z),(25,290,5),'Oak','Gallery')
for i,name in enumerate(['SM_Decor_2','SM_Decor_4','SM_Vase_9','SM_Decor_9']):
    prop(name,'Gallery_Collection_'+str(i),(-530,-890+i*55,449+(i%2)*75),i*25)
# 原阅读椅让出新楼梯的一层入口，仍留在客厅的休息组团内。
prop('SM_Armchair','Window_Chair',(50,-410,2),135,1.05,True)
prop('SM_Ottoman','Window_Ottoman',(100,-470,2),0,1.1,True)
prop('SM_Coffe_Table_3','Window_Table',(95,-530,2),0,1)
prop('SM_Vase_3','Window_Vase',(95,-530,57))

# 石板、木平台和窄种植带分区；不缩小已通过初验的池边环形动线。
wood=unreal.load_asset('/Game/Environment/HomeMap/Materials/M_Floor')
existing['HM_Deck_North'].static_mesh_component.set_material(0,wood)
existing['HM_Garden_Bench'].static_mesh_component.set_material(0,wood)
for x in [-650,650]:
    shape('Court_Planter_'+str(x),(x,780,20),(80,750,40),'Stone','Landscape')
    shape('Court_Soil_'+str(x),(x,780,42),(65,725,4),'Garden','Landscape')
    for i,y in enumerate(range(470,1120,160)):
        prop('SM_Plant_'+str(i%2+1),'Court_Green_'+str(x)+'_'+str(y),(x,y,43),i*49,.8)

# 单一树种形成外围高低变化，不将高密度扫描件直接铺满院子。
tree=unreal.load_asset('/Game/Environment/HomeMap/Landscape/Meshes/SM_HM_IslandTree')
tree_points=[(-2050,-900,1.5),(-2100,100,1.8),(-2050,1100,1.6),(-1350,2000,1.6),(-400,2200,1.9),
             (650,2180,1.5),(1700,2000,1.7),(2150,1000,1.9),(2200,-200,1.5),(2050,-1200,1.7),(-1400,-1850,1.4),(1350,-1850,1.6)]
for i,(x,y,scale) in enumerate(tree_points):
    a=actor(unreal.StaticMeshActor,'Boundary_Tree_'+str(i),(x,y,-148),(0,i*71,0),'Landscape')
    a.static_mesh_component.set_static_mesh(tree)
    a.static_mesh_component.set_collision_profile_name('NoCollision')
    a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    a.static_mesh_component.set_editor_property('ld_max_draw_distance',12000)

# 场景审核使用抗锯齿，保留项目全局设置；自动化结束时明确记录该会话状态。
unreal.SystemLibrary.execute_console_command(world,'r.AntiAliasingMethod 4')
unreal.SystemLibrary.execute_console_command(world,'r.ScreenPercentage 100')
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('REFINEMENT_SAVED',len(existing))
