"""树冠构建设置、柜体替换与地面材质；限定 HomeMap 环境，不改交互配置。"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ea=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);le=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert le.get_current_level().get_path_name().startswith('/Game/Environment/HomeMap/Maps/HomeMap_Courtyard.')
actors={a.get_actor_label():a for a in ea.get_all_level_actors()}
retired=['HM_Wardrobe_Cabinet_730','HM_Wardrobe_Front_730','HM_Wardrobe_Cabinet_890','HM_Wardrobe_Front_890','HM_Mission_Cabinet_-1060','HM_Mission_Cabinet_-840']
for name in retired:
    a=actors.get(name)
    if a:assert isinstance(a,unreal.StaticMeshActor);ea.destroy_actor(a)
for n in ['HM_DressingJoinery','HM_ExpeditionJoinery','HM_PrivateCeilingFinish']:actors[n].set_folder_path('HomeMap/Joinery')
a=actors['HM_Wardrobe_Bag'];o,e=a.get_actor_bounds(False);a.set_actor_location(a.get_actor_location()+unreal.Vector(-1330-o.x,735-o.y,222-(o.z-e.z)),False,False)
# 只调整既有交互 Actor 的环境摆放，贴近北侧实墙，释放入口；不改蓝图或交互半径。
actors['HM_Vanity_Interactive'].set_actor_location(unreal.Vector(-1050,980,0),False,False)
for label in ['HM_Wardrobe_Seat','HM_Wardrobe_Seat_Collision']:
    a=actors[label];o,e=a.get_actor_bounds(False);a.set_actor_location(a.get_actor_location()+unreal.Vector(-1070-o.x,770-o.y,0),False,False)
# 新源模型有 32,304 三角面。只写属性不会强制重建，旧回退网格可能仍只有 5,784 面。
mesh=unreal.load_asset('/Game/Environment/HomeMap/Landscape/Meshes/SM_HM_CourtyardCanopyTree')
sm=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);n=sm.get_nanite_settings(mesh)
n.enabled=False;n.fallback_target=unreal.NaniteFallbackTarget.PERCENT_TRIANGLES;n.fallback_percent_triangles=1;n.fallback_relative_error=0
sm.set_nanite_settings(mesh,n,apply_changes=True)
assert mesh.get_num_triangles(0)==32304
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
# 原室内灯覆盖收纳面，沿用数量，不给每个抽屉添加动态灯。
for label,intensity in [('HM_Wardrobe_Light',1800),('HM_Mission_Light',1800)]:
    light=actors[label].get_component_by_class(unreal.LightComponent);light.set_intensity(intensity)
actors['HM_Mission_Light'].set_actor_rotation(unreal.Rotator(pitch=-35,yaw=-90,roll=0),False)
ml=unreal.MaterialEditingLibrary;at=unreal.AssetToolsHelpers.get_asset_tools();base='/Game/Environment/HomeMap/Materials/Hub'

def node(m,cls,x,y):return ml.create_material_expression(m,cls,x,y)
def mat(name):
    path=base+'/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        m=unreal.load_asset(path)
        if unreal.EditorAssetLibrary.get_metadata_tag(m,'HomeMap.SurfaceBuilt')=='1':return m,False
        ml.delete_all_material_expressions(m)
        return m,True
    return at.create_asset(name,base,unreal.Material,unreal.MaterialFactoryNew()),True
def scalar(m,value,x,y):
    e=node(m,unreal.MaterialExpressionConstant,x,y);e.r=value;return e
def texture(m,path,uv,x,y,normal=False):
    t=unreal.load_asset(path);assert t,path
    e=node(m,unreal.MaterialExpressionTextureSample,x,y);e.texture=t
    if normal:e.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    ml.connect_material_expressions(uv,'',e,'UVs');return e
def worlduv(m,scale):
    w=node(m,unreal.MaterialExpressionWorldPosition,-1100,0)
    mask=node(m,unreal.MaterialExpressionComponentMask,-900,0)
    for k,v in [('r',True),('g',True),('b',False),('a',False)]:mask.set_editor_property(k,v)
    assert ml.connect_material_expressions(w,'',mask,'')
    uv=node(m,unreal.MaterialExpressionMultiply,-700,0);ml.connect_material_expressions(mask,'',uv,'A');ml.connect_material_expressions(scalar(m,scale,-900,120),'',uv,'B');return w,uv

garden,new=mat('M_HomeGardenSurface')
if new:
    world,uv=worlduv(garden,.004)
    detail=texture(garden,'/Game/Environment/HomeMap/Textures/T_Soil_A',uv,-480,240)
    custom=node(garden,unreal.MaterialExpressionCustom,-150,0)
    custom_inputs=[]
    for name in ['World','Detail']:
        inp=unreal.CustomInput();inp.set_editor_property('input_name',name);custom_inputs.append(inp)
    custom.set_editor_property('inputs',custom_inputs)
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property('code','''struct H { float hash(float2 p) { float3 q=frac(float3(p.xyx)*0.1031); q+=dot(q,q.yzx+33.33); return frac((q.x+q.y)*q.z); } }; H h;
float2 p=World.xy/750.0, i=floor(p), f=frac(p); f=f*f*(3-2*f);
float n=lerp(lerp(h.hash(i),h.hash(i+float2(1,0)),f.x),lerp(h.hash(i+float2(0,1)),h.hash(i+1),f.x),f.y);
return lerp(float3(.026,.045,.017),float3(.085,.115,.036),n)*(.72+.65*Detail.r);''')
    ml.connect_material_expressions(world,'',custom,'World');ml.connect_material_expressions(detail,'RGB',custom,'Detail');ml.connect_material_property(custom,'',unreal.MaterialProperty.MP_BASE_COLOR)
    ml.connect_material_property(scalar(garden,.94,0,260),'',unreal.MaterialProperty.MP_ROUGHNESS)
    ml.recompile_material(garden);unreal.EditorAssetLibrary.set_metadata_tag(garden,'HomeMap.SurfaceBuilt','1');unreal.EditorAssetLibrary.save_loaded_asset(garden)
for a in actors.values():
    if isinstance(a,unreal.StaticMeshActor):
        c=a.static_mesh_component
        for i,m in enumerate(c.get_materials()):
            if m and m.get_path_name().startswith(base+'/M_Garden.'):c.set_material(i,garden)
roof,new=mat('M_RoofAggregate')
if new:
    world,uv=worlduv(roof,.004)
    texturebase='/Game/NiagaraExamples/Gallery/Megascans/Surfaces/Gravel_Ground_xbnefjm/Medium/xbnefjm_tier_2/Textures/'
    color=texture(roof,texturebase+'T_xbnefjm_2K_B',uv,-440,0)
    normal=texture(roof,texturebase+'T_xbnefjm_2K_N',uv,-440,230,True)
    tint=node(roof,unreal.MaterialExpressionMultiply,-80,0)
    ml.connect_material_expressions(color,'RGB',tint,'A');ml.connect_material_expressions(scalar(roof,.1,-260,-130),'',tint,'B')
    ml.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
    ml.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
    ml.connect_material_property(scalar(roof,.88,-100,450),'',unreal.MaterialProperty.MP_ROUGHNESS)
    ml.recompile_material(roof);unreal.EditorAssetLibrary.set_metadata_tag(roof,'HomeMap.SurfaceBuilt','1');unreal.EditorAssetLibrary.save_loaded_asset(roof)
# 屋顶独立植被类型，不清理庭院里已有植被；盆体埋入低花池，只露出叶冠。
ftpath='/Game/Environment/HomeMap/Landscape/FT_HM_RoofPlanting'
if not unreal.EditorAssetLibrary.does_asset_exist(ftpath):
    unreal.FoliageService.create_foliage_type('/Game/Environment/HomeMap/Props/SM_Plant_1','/Game/Environment/HomeMap/Landscape','FT_HM_RoofPlanting',.75,.85,False,0,0,8000)
ft=unreal.load_asset(ftpath);ft.set_editor_property('cast_dynamic_shadow',False);ft.set_editor_property('enable_density_scaling',True);unreal.EditorAssetLibrary.save_loaded_asset(ft)
if unreal.FoliageService.get_instance_count(ftpath)<=0:
    locs=[unreal.Vector(x,y,368) for x in [-785,785] for y in [-800,-685,-570,-455,-340,0,120,240,360,480,950,1080,1210]]
    unreal.FoliageService.add_foliage_instances(ftpath,locs,.75,.85,False,True,False)
le.save_current_level()
print('CANOPY_JOINERY_APPLIED',mesh.get_num_triangles(0))
