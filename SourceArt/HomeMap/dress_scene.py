import unreal, json, math
from pathlib import Path

ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
at=unreal.AssetToolsHelpers.get_asset_tools()
ml=unreal.MaterialEditingLibrary
existing={a.get_actor_label():a for a in ea.get_all_level_actors()}
asset_data=json.loads((ROOT/'Docs/Tasks/HomeMap/Reports/assets.json').read_text(encoding='utf-8'))
catalog={r['path'].split('/')[-1]:r for r in asset_data}

def actor(cls,name,loc,rot=(0,0,0),folder='Dressing'):
    label='HM_'+name
    a=existing.get(label)
    if not a:
        a=ea.spawn_actor_from_class(cls,unreal.Vector(*loc),unreal.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
        a.set_actor_label(label)
        existing[label]=a
        print('CREATED',label)
    a.set_actor_location(unreal.Vector(*loc),False,False)
    a.set_actor_rotation(unreal.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]),False)
    a.set_folder_path('HomeMap/'+folder)
    return a

def material(name,color,roughness=.5,metallic=0,emissive=False):
    path='/Game/Environment/HomeMap/Materials/Hub/M_'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):return unreal.load_asset(path)
    m=at.create_asset('M_'+name,'/Game/Environment/HomeMap/Materials/Hub',unreal.Material,unreal.MaterialFactoryNew())
    c=ml.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-400,0)
    c.constant=unreal.LinearColor(*color,1)
    ml.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if emissive else unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,val,y in [(unreal.MaterialProperty.MP_ROUGHNESS,roughness,180),(unreal.MaterialProperty.MP_METALLIC,metallic,270)]:
        n=ml.create_material_expression(m,unreal.MaterialExpressionConstant,-400,y);n.r=val;ml.connect_material_property(n,'',prop)
    ml.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    print('CREATED',path)
    return m

MAT={key:unreal.load_asset('/Game/Environment/HomeMap/Materials/'+name) for key,name in {
    'Plaster':'M_Wall_White','Oak':'M_Wall_Wood','Stone':'M_Wall_Marble',
    'Metal':'M_Metall_Black','Glass':'MI_Glass_1','Glow':'M_Wall_Glow'}.items()}
MAT['Floor']=unreal.load_asset('/Game/Environment/HomeMap/Materials/M_Floor')
MAT['Pool']=material('Pool_Tile',(.045,.21,.22),.28)
MAT['Garden']=material('Garden',(.065,.095,.045),.95)
MAT['Limestone']=material('Limestone',(.32,.29,.24),.68)
MAT['Rubber']=material('Rubber',(.018,.022,.024),.86)
MAT['Glow']=material('Warm_Light',(5,2.9,1.25),.5,0,True)
MAT['Bronze']=material('Bronze',(.31,.17,.065),.28,.78)

records=json.loads((ROOT/'SourceArt/HomeMap/architecture.json').read_text(encoding='utf-8'))
for r in records:
    a=existing['HM_'+r['name']]
    c=a.static_mesh_component
    mat=MAT[r['mat']]
    if r['name'].startswith('Floor'):mat=MAT['Floor']
    if r['folder'] in ['Courtyard','Landscape'] and r['mat']=='Stone':mat=MAT['Limestone']
    c.set_material(0,mat)
    c.set_visibility(True)
    if r['mat']=='Glass':c.set_cast_shadow(False)
print('MODIFIED_ARCHITECTURE_MATERIALS',len(records))

cube=unreal.load_asset('/Engine/BasicShapes/Cube')
cylinder=unreal.load_asset('/Engine/BasicShapes/Cylinder')
def shape(name,loc,dims,mat='Metal',folder='Dressing',collision=True,mesh=None,rot=(0,0,0)):
    a=actor(unreal.StaticMeshActor,name,loc,rot,folder)
    c=a.static_mesh_component
    c.set_static_mesh(mesh or cube)
    c.set_material(0,MAT[mat])
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in dims)))
    return a

def prop(mesh_name,name,loc,yaw=0,scale=1,collision=False):
    r=catalog[mesh_name]
    # 原家具存在离中心很远的 pivot（尤其厨房），按实际包围盒底部中心摆放。
    center=[(r['min'][i]+r['max'][i])*.5*scale for i in [0,1]]
    theta=math.radians(yaw)
    offset=[center[0]*math.cos(theta)-center[1]*math.sin(theta),center[0]*math.sin(theta)+center[1]*math.cos(theta)]
    pos=(loc[0]-offset[0],loc[1]-offset[1],loc[2]-r['min'][2]*scale)
    a=actor(unreal.StaticMeshActor,name,pos,(0,yaw,0))
    a.static_mesh_component.set_static_mesh(unreal.load_asset(r['path']))
    a.set_actor_scale3d(unreal.Vector(scale,scale,scale))
    a.static_mesh_component.set_collision_profile_name('BlockAll' if collision and r['collision'] else 'NoCollision')
    if collision and not r['collision']:
        # 碰撞代理仅覆盖家具主体，不让枕头、灯罩或叶片阻挡相机和角色。
        dx=(r['max'][0]-r['min'][0])*scale*.88
        dy=(r['max'][1]-r['min'][1])*scale*.88
        dz=min((r['max'][2]-r['min'][2])*scale,95)
        blocker=shape(name+'_Collision',(loc[0],loc[1],loc[2]+dz/2),(dx,dy,dz),'Rubber','Collision',True,rot=(0,yaw,0))
        blocker.set_actor_hidden_in_game(True)
        blocker.set_is_temporarily_hidden_in_editor(True)
    return a

# 客厅围合在主通路两侧，入口到泳池的中轴不放家具。
prop('SM_Carpet','Living_Rug',(-265,-335,1),0,1.6)
prop('SM_Sofa','Living_Sofa',(-300,-510,2),0,1.1,True)
prop('SM_Coffe_Table_1','Living_Table',(-295,-290,2),0,1.0,True)
prop('SM_Coffe_Table_2','Living_Side_Table',(-440,-170,2),0,1)
prop('SM_Armchair','Living_Armchair',(-300,-120,2),180,1.1,True)
prop('SM_Armchair','Window_Chair',(330,-180,2),135,1.05,True)
prop('SM_Ottoman','Window_Ottoman',(240,-300,2),0,1.1,True)
prop('SM_Coffe_Table_3','Window_Table',(420,-280,2),0,1)
prop('SM_Floor_Lamp','Living_Lamp',(-480,-550,2))
prop('SM_Stack_of_Books_1','Living_Books',(-315,-290,43),20)
prop('SM_Cup','Living_Cup',(-265,-285,43))
prop('SM_Vase_3','Window_Vase',(420,-280,57))

# 厨房和餐厅：中岛到厨柜、餐桌到玻璃保留较宽操作带。
prop('SM_Kitchen','Kitchen_Fitted',(970,-940,0))
prop('SM_Table','Dining_Table',(1060,-120,0),0,1,True)
for i,(x,y,yaw) in enumerate([(985,-235,0),(1125,-235,0),(985,-10,180),(1125,-10,180),(875,-120,-90),(1245,-120,90)]):
    prop('SM_Chair','Dining_Chair_'+str(i),(x+60,y,0),yaw,1,True)
prop('SM_Kitchen_Table_Decor','Dining_Centerpiece',(1060,-120,88))
prop('SM_Bowl_with_Oranges','Island_Fruit',(920,-430,101))
prop('SM_Kitchen_Decor_12','Island_Decor',(1090,-430,101))
prop('SM_Ceiling_Lamp_2','Dining_Pendant',(1050,-120,190))

# 主卧、衣帽、现有梳妆交互和卫浴。
prop('SM_Carpet','Bedroom_Rug',(-1060,270,1),0,1.4)
prop('SM_Bed','Master_Bed',(-1120,240,2),90,1.05,True)
for y in [70,410]:
    prop('SM_Bed_Table','Bedside_'+str(y),(-1250,y,2),0,1,True)
    prop('SM_Point_Lamp_1','Bed_Lamp_'+str(y),(-1250,y,56))
prop('SM_Armchair','Bedroom_Chair',(-1320,510,2),-60,1,True)
bp=unreal.load_class(None,'/Game/Environment/Props/Gameplay/Dressing_Table_Set/Blueprint/BP_Dressing_Table_Set.BP_Dressing_Table_Set_C')
actor(bp,'Vanity_Interactive',(-1050,910,0),(0,180,0),'Gameplay')
prop('SM_Ottoman','Wardrobe_Seat',(-1100,720,0),0,1,True)
prop('SM_Bag_1','Wardrobe_Bag',(-1340,730,273))
prop('SM_Vase_5','Bath_Vase',(-1330,1280,91))
shape('Bath_Basin',(-1180,1280,94),(55,42,12),'Plaster','Private')
shape('Bath_Faucet',(-1180,1310,109),(4,4,32),'Bronze','Private')
shape('Bath_Faucet_Nose',(-1180,1300,123),(4,24,4),'Bronze','Private')
shape('Shower_Head',(-835,1370,225),(30,25,3),'Metal','Private',False)
shape('Shower_Riser',(-835,1380,135),(3,3,180),'Metal','Private',False)

# 康体区器械用环境几何组成；不创建任何运动玩法。
for i,y in enumerate([240,490]):
    shape('Treadmill_Deck_'+str(i),(1210,y,10),(190,85,20),'Rubber','Wellness')
    for side in [-1,1]:
        shape('Treadmill_Post_'+str(i)+'_'+str(side),(1290,y+side*37,67),(7,7,120),'Metal','Wellness')
        shape('Treadmill_Rail_'+str(i)+'_'+str(side),(1250,y+side*37,115),(90,6,6),'Metal','Wellness')
    shape('Treadmill_Console_'+str(i),(1300,y,127),(20,78,14),'Metal','Wellness')
    shape('Treadmill_Display_'+str(i),(1288,y,133),(3,50,10),'Pool','Wellness',False)
shape('Weights_Rack',(1180,80,70),(280,55,8),'Metal','Wellness')
for x in [1060,1300]:shape('Rack_Leg_'+str(x),(x,80,35),(10,45,70),'Metal','Wellness')
for i,x in enumerate(range(1060,1320,45)):
    shape('Dumbbell_Bar_'+str(i),(x,80,84),(4,40,4),'Bronze','Wellness',False)
    for y in [65,95]:shape('Dumbbell_Weight_'+str(i)+'_'+str(y),(x,y,84),(20,10,20),'Rubber','Wellness',False,mesh=cylinder,rot=(90,0,0))
shape('Gym_Mirror',(1385,360,170),(3,580,190),'Metal','Wellness',False)
for i,y in enumerate([950,1240]):
    shape('Yoga_Mat_'+str(i),(1100,y,1.5),(190,75,3),'Rubber','Wellness',False)
    shape('Yoga_Block_'+str(i),(1010,y+65,10),(24,16,20),'Oak','Wellness',False)
prop('SM_Armchair','Wellness_Rest',(1240,820,0),-40,1,True)
prop('SM_Coffe_Table_3','Wellness_Table',(1330,970,0))
prop('SM_Vase_7','Wellness_Vase',(1330,970,57))

# 副本交互仍由现有 Actor 处理；实例只换门形视觉网格和材质。
terminal=actor(unreal.load_class(None,'/Game/Gameplay/Interactables/Stations/BP_ExpeditionTerminal.BP_ExpeditionTerminal_C'),
    'Expedition_Gate',(-1230,-500,125),(0,0,0),'Gameplay')
for component in terminal.get_components_by_class(unreal.StaticMeshComponent):
    component.set_static_mesh(unreal.load_asset('/Game/Environment/HomeMap/Architecture/SM_Door_Enter'))
    component.set_relative_location(unreal.Vector(0,0,-125),False,False)
    component.set_relative_scale3d(unreal.Vector(1.8,2.2,1.2))
prop('SM_Chair_2','Mission_Chair',(-940,-760,0),180,1,True)
prop('SM_Opened_Book','Mission_Book',(-950,-890,94))
prop('SM_Bag_3','Mission_Bag',(-840,-870,94))
for z in [90,160,230]:shape('Trophy_Shelf_'+str(z),(-1030,-60,z),(330,40,6),'Oak','Mission')
for i,n in enumerate(['SM_Decor_2','SM_Decor_3','SM_Decor_4','SM_Decor_9','SM_Vase_10','SM_Stack_of_Books_2']):
    prop(n,'Trophy_'+str(i),(-1130+(i%3)*100,-60,94+(i//3)*70),i*13)

# 植物优先复用两套已迁移资产；入口、转角和背景成组布置。
for i,(x,y,s) in enumerate([(-470,-840,1.8),(470,-840,1.7),(-1350,-150,1.6),(1350,-700,1.6),(-780,60,1.5),(-1320,550,1.3),
    (-750,950,1.3),(1350,850,1.7),(1350,1320,1.5),(750,1310,1.5),(-600,210,1.4),(600,210,1.4),(-600,1360,1.4),(600,1360,1.4)]):
    prop('SM_Plant_'+str(i%2+1),'Plant_'+str(i),(x,y,0),i*43,s)
for side in [-1,1]:
    for i in range(5):prop('SM_Plant_'+str(i%2+1),'Garden_Plant_'+str(side)+'_'+str(i),(side*(770+i*135),1570,49),i*65,1.9)
for i,x in enumerate([-230,230]):
    prop('SM_Armchair','Terrace_Chair_'+str(i),(x,145,0),180,1.1,True)
prop('SM_Coffe_Table_2','Terrace_Table',(-370,145,0))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('DRESSING_SAVED',len(existing))
