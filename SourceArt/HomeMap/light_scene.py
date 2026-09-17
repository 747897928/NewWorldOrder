import unreal, math

# 在 dress_scene.py 建立的场景工具上下文中执行；只更新本任务命名空间。
sun=existing['HM_Sun']
sun.set_actor_rotation(unreal.Rotator(pitch=-36,yaw=-62,roll=0),False)
sun.light_component.set_intensity(12000)
sun.light_component.set_editor_property('use_temperature',True)
sun.light_component.set_editor_property('temperature',5200)
sun.light_component.set_editor_property('atmosphere_sun_light',True)
sun.light_component.set_editor_property('light_source_angle',1.1)
sky=existing['HM_Sky'].light_component
sky.set_intensity(1.1)
sky.set_editor_property('real_time_capture',True)

pp=actor(unreal.PostProcessVolume,'Look',(0,0,0),folder='Lighting')
pp.set_editor_property('unbound',True)
s=pp.settings
settings={'dynamic_global_illumination_method':unreal.DynamicGlobalIlluminationMethod.LUMEN,
          'reflection_method':unreal.ReflectionMethod.LUMEN,
          'auto_exposure_min_brightness':7.5,'auto_exposure_max_brightness':9.0,
          'auto_exposure_bias':0.0,'auto_exposure_speed_up':2.0,'auto_exposure_speed_down':1.0,
          'bloom_intensity':.22,'vignette_intensity':.16,
          'motion_blur_amount':0.0,'white_temp':5700.0,
          'local_exposure_highlight_contrast_scale':.85,'local_exposure_shadow_contrast_scale':.9}
for k,v in settings.items():
    s.set_editor_property('override_'+k,True)
    s.set_editor_property(k,v)
pp.settings=s

def rect(name,loc,intensity=2200,width=180,height=180,radius=600,shadow=False,rot=(-90,0,0),temp=3400):
    a=actor(unreal.RectLight,name,loc,rot,'Lighting')
    c=a.light_component
    c.set_intensity(intensity)
    c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
    c.set_editor_property('source_width',width)
    c.set_editor_property('source_height',height)
    c.set_editor_property('attenuation_radius',radius)
    c.set_editor_property('use_temperature',True)
    c.set_editor_property('temperature',temp)
    c.set_cast_shadows(shadow)
    return a

for name,loc,power,radius,shadow in [
 ('Living_Key',(-290,-300,435),4200,800,True),('Living_Window',(300,-280,435),2600,650,False),
 ('Foyer_Light',(0,-770,295),1800,440,False),('Kitchen_Light',(990,-650,315),3800,660,True),
 ('Dining_Light',(1050,-120,310),2000,450,False),('Mission_Light',(-1000,-530,315),2800,620,True),
 ('Bedroom_Light',(-1110,260,315),2700,560,True),('Wardrobe_Light',(-1030,820,315),2000,400,False),
 ('Bathroom_Light',(-1070,1200,315),2400,440,False),('Gym_Light',(1080,300,315),3400,580,True),
 ('Yoga_Light',(1080,1080,315),2400,600,False)]:rect(name,loc,power,radius=radius,shadow=shadow)

for i,(x,y,z) in enumerate([(-1250,70,100),(-1250,410,100),(-475,-550,160)]):
    a=actor(unreal.PointLight,'Practical_'+str(i),(x,y,z),folder='Lighting')
    c=a.light_component;c.set_intensity(140);c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
    c.set_editor_property('attenuation_radius',200);c.set_editor_property('use_temperature',True)
    c.set_editor_property('temperature',2800);c.set_cast_shadows(False)

# 自有玻璃只用一层表面透光，不开启 PlanarReflection 或场景捕获。
def translucent(name,color,opacity,roughness):
    path='/Game/Environment/HomeMap/Materials/Hub/M_'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):return unreal.load_asset(path)
    m=at.create_asset('M_'+name,'/Game/Environment/HomeMap/Materials/Hub',unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('two_sided',True)
    m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE)
    col=ml.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-400,0);col.constant=unreal.LinearColor(*color,1)
    ml.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for k,v,y in [(unreal.MaterialProperty.MP_OPACITY,opacity,150),(unreal.MaterialProperty.MP_ROUGHNESS,roughness,230),(unreal.MaterialProperty.MP_SPECULAR,.5,310)]:
        n=ml.create_material_expression(m,unreal.MaterialExpressionConstant,-400,y);n.r=v;ml.connect_material_property(n,'',k)
    if name=='Pool_Water':
        uv=ml.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate,-900,400)
        time=ml.create_material_expression(m,unreal.MaterialExpressionTime,-900,550)
        n=ml.create_material_expression(m,unreal.MaterialExpressionCustom,-500,450)
        n.set_editor_property('code','float a=sin(UV.x*95+T*.55+sin(UV.y*36))*.045; float b=cos(UV.y*87-T*.43+sin(UV.x*42))*.045; return normalize(float3(a,b,1));')
        n.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        uv_input=unreal.CustomInput();uv_input.set_editor_property('input_name','UV')
        time_input=unreal.CustomInput();time_input.set_editor_property('input_name','T')
        n.set_editor_property('inputs',[uv_input,time_input])
        ml.connect_material_expressions(uv,'',n,'UV');ml.connect_material_expressions(time,'',n,'T')
        ml.connect_material_property(n,'',unreal.MaterialProperty.MP_NORMAL)
    ml.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    print('CREATED',path)
    return m

glass=translucent('Architectural_Glass',(.32,.4,.41),.075,.08)
water=translucent('Pool_Water',(.025,.20,.18),.42,.095)
for r in records:
    if r['mat']=='Glass':existing['HM_'+r['name']].static_mesh_component.set_material(0,glass)
plane=unreal.load_asset('/Engine/BasicShapes/Plane')
a=shape('Pool_Water',(0,790,-18),(850,1000,1),'Pool','Pool',False,mesh=plane)
a.static_mesh_component.set_material(0,water)
a.static_mesh_component.set_cast_shadow(False)

mirror=material('Mirror',(.72,.76,.78),.075,1)
existing['HM_Bath_Mirror'].static_mesh_component.set_material(0,mirror)
existing['HM_Gym_Mirror'].static_mesh_component.set_material(0,mirror)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('LIGHTING_SAVED')
