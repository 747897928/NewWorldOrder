import bpy, json, math, os
from pathlib import Path

# 所有尺寸以 Unreal 厘米记录；Blender 以米建模，FBX 导出保持厘米单位换算。
ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'SourceArt/HomeMap'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'Exports').mkdir(exist_ok=True)
bpy.context.preferences.filepaths.save_version = 0
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0
collection = bpy.data.collections.get('HomeMap_Architecture')
if collection is not None:
    raise RuntimeError('已有建筑集合；请打开已保存主文件继续编辑，禁止覆盖重建。')
collection = bpy.data.collections.new('HomeMap_Architecture')
scene.collection.children.link(collection)
records, modules = [], {}
colors = {'Plaster':(.64,.60,.51,1), 'Oak':(.29,.15,.067,1), 'Stone':(.10,.115,.12,1),
          'Metal':(.018,.022,.025,1), 'Glass':(.28,.46,.48,1), 'Pool':(.06,.28,.28,1),
          'Glow':(.95,.58,.23,1), 'Garden':(.12,.17,.075,1)}
materials = {}
for name, color in colors.items():
    mat = bpy.data.materials.new('Preview_'+name)
    mat.diffuse_color = color
    materials[name] = mat

def box(name, loc, dims, mat='Plaster', folder='Architecture', collision=True):
    key = tuple(dims)
    mesh_name = 'SM_HM_' + '_'.join(str(v).replace('.','p') for v in key)
    if key not in modules:
        bpy.ops.mesh.primitive_cube_add(size=1)
        ob = bpy.context.object
        ob.name = name
        ob.dimensions = [v/100 for v in dims]
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        bevel = ob.modifiers.new('Architectural edge highlights', 'BEVEL')
        bevel.width = min(.008, min(dims)/100/6)
        bevel.segments = 3
        bpy.context.view_layer.objects.active = ob
        bpy.ops.object.modifier_apply(modifier=bevel.name)
        # 每面按实际米数投影，避免长墙和大楼板拉伸原始方块 UV。
        uv = ob.data.uv_layers.active or ob.data.uv_layers.new(name='UVMap')
        for poly in ob.data.polygons:
            axis = max(range(3), key=lambda i:abs(poly.normal[i]))
            axes = [i for i in range(3) if i != axis]
            for li in poly.loop_indices:
                v=ob.data.vertices[ob.data.loops[li].vertex_index].co
                uv.data[li].uv=(v[axes[0]],v[axes[1]])
        ob.data.name = mesh_name
        modules[key] = ob.data
    else:
        ob = bpy.data.objects.new(name, modules[key])
        scene.collection.objects.link(ob)
    for col in list(ob.users_collection): col.objects.unlink(ob)
    collection.objects.link(ob)
    ob.location = [loc[0]/100, -loc[1]/100, loc[2]/100]
    if not ob.data.materials: ob.data.materials.append(materials[mat])
    ob['unreal_material'] = mat
    ob['unreal_folder'] = folder
    records.append(dict(name=name,mesh=mesh_name,loc=loc,dims=dims,mat=mat,folder=folder,collision=collision))
    return ob

# 三翼楼板；庭院保留真正池坑，不用整块地板封住水下空间。
box('Floor_Public',(0,-500,-15),(2800,1000,30),'Oak')
for x,label in [(-1050,'Private'),(1050,'Wellness')]:
    box('Floor_'+label,(x,700,-15),(700,1400,30),'Oak')
box('Deck_Living',(0,125,-15),(1400,250,30),'Stone','Courtyard')
for x in [-570,570]:box('Deck_Side_'+str(x),(x,825,-15),(260,1150,30),'Stone','Courtyard')
box('Deck_North',(0,1390,-15),(1400,180,30),'Stone','Courtyard')
box('Arrival_Platform',(0,-1280,-20),(1200,560,40),'Stone','Courtyard')
box('Site_Ground',(0,0,-190),(10000,10000,80),'Garden','Landscape')

# 南立面开口 240 cm；东西翼保持 330 cm 净高。
for x in [-770,770]:box('South_Wall_'+str(x),(x,-1000,165),(1260,20,330))
box('Entry_Header',(0,-1000,295),(280,20,70),'Stone')
for x in [-1400,1400]:
    box('Outer_Wall_'+str(x),(x,200,165),(20,2400,330))
for x in [-1050,1050]:box('North_Wing_Wall_'+str(x),(x,1400,165),(700,20,330))
for x in [-1050,1050]:box('Roof_Wing_'+str(x),(x,200,350),(740,2440,40),'Plaster','Roof')
box('Roof_Living',(0,-500,480),(1140,1040,40),'Plaster','Roof')
box('Clerestory_South',(0,-1000,400),(1100,6,130),'Glass','Glazing')
for x in [-550,550]:box('Clerestory_Side_'+str(x),(x,-500,400),(6,1000,130),'Glass','Glazing')

# 门厅用低一些的天花和木格栅收敛，客厅挑高形成展开。
box('Foyer_Canopy',(0,-790,315),(480,400,30),'Oak','Roof')
for x in [-255,255]:
    for y in range(-920,-560,30):box('Foyer_Slat_'+str(x)+'_'+str(y),(x,y,150),(9,9,300),'Oak','Screens')
for x in [-560,560]:
    box('Public_Partition_'+str(x),(x,-790,165),(20,420,330),'Oak')
    box('Public_Pier_'+str(x),(x,-100,165),(20,200,330),'Oak')
    box('Public_Header_'+str(x),(x,-400,310),(20,400,40),'Oak')

# 客厅通向庭院的中间开口 260 cm，左右玻璃 420 cm。
for x in [-350,350]:box('Living_Glass_'+str(x),(x,0,225),(420,4,450),'Glass','Glazing')
for x in [-570,-140,140,570]:box('Living_Mullion_'+str(x),(x,0,225),(7,10,450),'Metal','Glazing')
box('Living_Facade_Beam',(0,0,455),(1160,18,20),'Metal','Glazing')
box('Living_Canopy',(0,90,470),(1160,180,20),'Oak','Roof')

# 两翼对庭院的玻璃和敞开入口；窗框统一黑金属。
for x in [-700,700]:
    for y,length in [(75,150),(525,390),(1000,320),(1350,100)]:
        box('Wing_Glass_'+str(x)+'_'+str(y),(x,y,165),(4,length,330),'Glass','Glazing')
    for y in [0,150,330,720,840,1160,1300,1400]:
        box('Wing_Mullion_'+str(x)+'_'+str(y),(x,y,165),(10,7,330),'Metal','Glazing')
    box('Wing_Beam_'+str(x),(x,700,325),(18,1400,20),'Metal','Glazing')

# 卧室、衣帽和卫浴串联，内部门洞 180 cm；健身与瑜伽同样保留通路。
for y in [600,1000]:
    box('Private_Partition_'+str(y),(-1160,y,165),(480,18,330),'Oak')
    box('Private_Header_'+str(y),(-810,y,310),(220,18,40),'Oak')
box('Wellness_Partition',(1180,700,165),(440,18,330),'Oak')
box('Wellness_Header',(830,700,310),(260,18,40),'Oak')

# 泳池底、壁、池沿和八级 15 cm 踏步，所有体块均导出独立简单碰撞。
box('Pool_Base',(0,790,-135),(880,1020,30),'Pool','Pool')
for x in [-440,440]:box('Pool_Side_'+str(x),(x,790,-60),(20,1020,120),'Pool','Pool')
for y in [280,1300]:box('Pool_End_'+str(y),(0,y,-60),(900,20,120),'Pool','Pool')
for x in [-451,451]:box('Pool_Coping_'+str(x),(x,790,0),(42,1040,12),'Stone','Pool')
for y in [270,1310]:box('Pool_Coping_'+str(y),(0,y,0),(940,42,12),'Stone','Pool')
for i in range(8):
    top=-15*(i+1)
    box('Pool_Step_'+str(i),(0,300+i*30,(top-120)/2),(400,30,120+top),'Pool','Pool') if top>-120 else None
box('Pool_Water',(0,790,-18),(850,1000,1),'Water','Pool',False) if 'Water' in materials else None

# 副本入口木饰面背景、嵌入式门框和任务准备工作台。
box('Mission_Feature_Wall',(-1280,-500,165),(35,650,330),'Oak','Mission')
for y in [-630,-370]:box('Gate_Jamb_'+str(y),(-1250,y,130),(35,16,260),'Metal','Mission')
box('Gate_Header',(-1250,-500,265),(35,280,16),'Metal','Mission')
for y in [-620,-380]:box('Gate_Light_'+str(y),(-1229,y,130),(2,3,230),'Glow','Mission',False)
box('Mission_Worktop',(-950,-890,90),(300,70,8),'Stone','Mission')
for x in [-1060,-840]:box('Mission_Cabinet_'+str(x),(x,-890,43),(70,65,86),'Oak','Mission')

# 厨房中岛、衣帽柜、卫浴石台；原有高质量家具由 Unreal 引用。
box('Kitchen_Island',(990,-430,47),(300,105,94),'Oak','Kitchen')
box('Kitchen_Island_Top',(990,-430,97),(310,115,7),'Stone','Kitchen')
for y in [730,890]:
    box('Wardrobe_Cabinet_'+str(y),(-1345,y,135),(90,145,270),'Oak','Private')
    box('Wardrobe_Front_'+str(y),(-1295,y,135),(5,135,260),'Stone','Private')
box('Bath_Vanity',(-1240,1280,42),(270,75,84),'Oak','Private')
box('Bath_Counter',(-1240,1280,87),(280,80,6),'Stone','Private')
box('Bath_Mirror',(-1240,1375,180),(260,3,125),'Metal','Private',False)
box('Shower_Base',(-860,1200,3),(230,280,6),'Stone','Private')
box('Shower_Glass',(-980,1260,120),(3,180,240),'Glass','Glazing')

# 间接光槽仅用轻量发光几何；实际灯光分区配置于关卡。
for x in [-520,520]:box('Living_Cove_'+str(x),(x,-490,447),(3,940,3),'Glow','Lighting',False)
for x in [-1360,1360]:box('Wing_Cove_'+str(x),(x,180,320),(3,2280,3),'Glow','Lighting',False)
for x in [-1050,1050]:box('Public_Cove_'+str(x),(x,-950,320),(630,3,3),'Glow','Lighting',False)

# 庭院边界与花池，减少直接看到无尽空地的角度。
for x in [-1730,1730]:box('Garden_Wall_'+str(x),(x,300,90),(25,3000,180),'Stone','Landscape')
box('Garden_North_Wall',(0,1750,100),(3480,25,200),'Stone','Landscape')
for x in [-1050,1050]:
    box('Planter_North_'+str(x),(x,1570,22),(700,150,44),'Stone','Landscape')
    box('Planter_Soil_'+str(x),(x,1570,46),(670,120,4),'Garden','Landscape')

with open(OUT/'architecture.json','w',encoding='utf-8') as f:json.dump(records,f,ensure_ascii=False,indent=2)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('SAVED_ARCHITECTURE',len(records),'MODULES',len(modules))

# 每种尺寸导出一个本地原点模块，主源文件保留完整装配，关卡按清单恢复位置。
bpy.ops.object.select_all(action='DESELECT')
for mesh in modules.values():
    temp=bpy.data.objects.new(mesh.name,mesh)
    scene.collection.objects.link(temp)
    temp.select_set(True)
    bpy.context.view_layer.objects.active=temp
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports'/f'{mesh.name}.fbx'),use_selection=True,
        object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,
        bake_anim=False,add_leaf_bones=False,use_mesh_modifiers=True,path_mode='AUTO')
    temp.select_set(False)
    bpy.data.objects.remove(temp,do_unlink=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'HomeMap_Master.blend'))
print('EXPORTED',len(modules))
