import unreal, math
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
# 先执行 refine_scene 以恢复材质与家具工具上下文，再应用本轮最终调优。
exec(compile((ROOT/'SourceArt/HomeMap/refine_scene.py').read_text(encoding='utf-8'),'refine_scene.py','exec'))

sun=existing['HM_Sun'];sun.set_actor_rotation(unreal.Rotator(pitch=-6,yaw=-62,roll=0),False)
sun.light_component.set_intensity(600);sun.light_component.set_editor_property('temperature',5300)
existing['HM_Sky'].light_component.set_intensity(1)
existing['HM_Living_Key'].light_component.set_intensity(7500)
settings=existing['HM_Look'].settings
for name,value in [('auto_exposure_min_brightness',4.5),('auto_exposure_max_brightness',10.0),('bloom_intensity',.15),('white_temp',6500.0)]:
    settings.set_editor_property('override_'+name,True);settings.set_editor_property(name,value)
existing['HM_Look'].settings=settings
cloudpath='/Game/Environment/HomeMap/Materials/Hub/MI_HomeMap_Clouds'
cloudmat=unreal.load_asset(cloudpath) if unreal.EditorAssetLibrary.does_asset_exist(cloudpath) else unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst',cloudpath)
for name,value in [('Cloud_GlobalCoverage',.22),('Cloud_GlobalDensity',.6),('StormClouds',0)]:ml.set_material_instance_scalar_parameter_value(cloudmat,name,value)
existing['HM_Clouds'].get_component_by_class(unreal.VolumetricCloudComponent).set_material(cloudmat)
unreal.EditorAssetLibrary.save_loaded_asset(cloudmat)

# 单层水处理实际水深的散射和吸收；不增加 PlanarReflection 或全场景捕获。
waterpath='/Game/Environment/HomeMap/Materials/Hub/M_Pool_Water'
water=unreal.load_asset(waterpath)
water.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
water.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
nodes=unreal.MaterialNodeService.list_expressions(waterpath)
for n in nodes:
    if n.class_name=='MaterialExpressionCustom':
        unreal.MaterialNodeService.set_expression_property(waterpath,n.id,'Code','float a=sin(UV.x*260+T*.35+sin(UV.y*140))*.012; float b=cos(UV.y*230-T*.28+sin(UV.x*160))*.012; return normalize(float3(a,b,1));')
    elif n.class_name=='MaterialExpressionConstant3Vector' and n.pos_y==0:unreal.MaterialNodeService.set_expression_property(waterpath,n.id,'Constant','(R=0.0,G=0.0,B=0.0,A=1.0)')
    elif n.class_name=='MaterialExpressionConstant' and n.pos_y==150:unreal.MaterialNodeService.set_expression_property(waterpath,n.id,'R','0.0')
    elif n.class_name=='MaterialExpressionConstant' and n.pos_y==230:unreal.MaterialNodeService.set_expression_property(waterpath,n.id,'R','0.065')
if not any(n.class_name=='MaterialExpressionSingleLayerWaterMaterialOutput' for n in nodes):
    out=ml.create_material_expression(water,unreal.MaterialExpressionSingleLayerWaterMaterialOutput,50,750)
    for i,(pin,values) in enumerate([('ScatteringCoefficients',(.0004,.0010,.0014)),('AbsorptionCoefficients',(.004,.0015,.0009)),('ColorScaleBehindWater',(1,1,1))]):
        c=ml.create_material_expression(water,unreal.MaterialExpressionConstant3Vector,-400,750+i*170);c.constant=unreal.LinearColor(*values,1)
        assert ml.connect_material_expressions(c,'',out,pin),pin
ml.recompile_material(water);unreal.EditorAssetLibrary.save_loaded_asset(water)
for x in [-420,420]:
    existing['HM_Pool_Light_'+str(x)].light_component.set_editor_property('specular_scale',0)
    existing['HM_Pool_Light_'+str(x)].light_component.set_intensity(900)
    existing['HM_Pool_Light_Strip_'+str(x)].static_mesh_component.set_material(0,material('Pool_Led',(.12,2.2,3),.5,0,True))

def tiled_material(name,color,tile,roughness):
    path='/Game/Environment/HomeMap/Materials/Hub/M_'+name
    m=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else at.create_asset('M_'+name,'/Game/Environment/HomeMap/Materials/Hub',unreal.Material,unreal.MaterialFactoryNew())
    ml.delete_all_material_expressions(m)
    uv=ml.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate,-750,0)
    code=f'float2 p=UV/float2({tile[0]},{tile[1]}); float2 edge=min(frac(p),1-frac(p)); float joint=step(.003,min(edge.x,edge.y)); float h=frac(sin(dot(floor(p),float2(12.9898,78.233)))*43758.5453); return float3({color[0]},{color[1]},{color[2]})*lerp(.64,.96+h*.08,joint);'
    n=ml.create_material_expression(m,unreal.MaterialExpressionCustom,-400,0);n.set_editor_property('code',code);n.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inp=unreal.CustomInput();inp.set_editor_property('input_name','UV');n.set_editor_property('inputs',[inp])
    ml.connect_material_expressions(uv,'',n,'UV');ml.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR)
    r=ml.create_material_expression(m,unreal.MaterialExpressionConstant,-400,300);r.r=roughness;ml.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);return m
paving=tiled_material('Courtyard_Stone',(.31,.30,.27),(1.2,.6),.72)
tile=tiled_material('Pool_Mosaic',(.13,.24,.26),(.125,.125),.34)
for label,a in existing.items():
    if label.startswith(('HM_Deck_Side','HM_Deck_Living','HM_Arrival_','HM_Pool_Coping')):a.static_mesh_component.set_material(0,paving)
    if label.startswith(('HM_Pool_Base','HM_Pool_Side','HM_Pool_End','HM_Pool_Step')):a.static_mesh_component.set_material(0,tile)
for label in ['HM_Pavilion_Crown','HM_Pavilion_Frame_-585','HM_Pavilion_Frame_585']:existing[label].static_mesh_component.set_material(0,MAT['Plaster'])

# 花池收窄并贴近玻璃，池沿到硬质花池保留约 188 cm 的净宽。
for side in [-1,1]:
    oldx=side*650;x=side*680
    shape('Court_Planter_'+str(oldx),(x,780,20),(40,750,40),'Limestone','Landscape')
    shape('Court_Soil_'+str(oldx),(x,780,42),(30,725,4),'Garden','Landscape')
    for i,y in enumerate(range(470,1120,160)):prop('SM_Plant_'+str(i%2+1),'Court_Green_'+str(oldx)+'_'+str(y),(x,y,43),i*49,.6)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('POLISH_SAVED')
