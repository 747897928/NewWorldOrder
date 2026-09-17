import unreal
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ml=unreal.MaterialEditingLibrary
# 只复用材质构建函数，不执行旧场景摆放和树构建段。
s=(root/'SourceArt/HomeMap/finish_canopy_joinery.py').read_text(encoding='utf-8')
start=s.index('ml=unreal.MaterialEditingLibrary');end=s.index("garden,new=mat('M_HomeGardenSurface')")
exec(s[start:end])
m,new=mat('M_EstateTerrain')
if new:
 world,uv=worlduv(m,.001)
 detail=texture(m,'/Game/Environment/HomeMap/Textures/T_Soil_A',uv,-480,240)
 norm=node(m,unreal.MaterialExpressionVertexNormalWS,-470,-220)
 custom=node(m,unreal.MaterialExpressionCustom,-130,0)
 ins=[]
 for name in ['World','Detail','SurfaceNormal']:
  i=unreal.CustomInput();i.set_editor_property('input_name',name);ins.append(i)
 custom.set_editor_property('inputs',ins);custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
 custom.set_editor_property('code','''struct H { float hash(float2 p) { float3 q=frac(float3(p.xyx)*0.1031); q+=dot(q,q.yzx+33.33); return frac((q.x+q.y)*q.z); } }; H h;
float2 p=World.xy/2500.,i=floor(p),f=frac(p); f=f*f*(3-2*f);
float n=lerp(lerp(h.hash(i),h.hash(i+float2(1,0)),f.x),lerp(h.hash(i+float2(0,1)),h.hash(i+1),f.x),f.y);
float slope=smoothstep(.035,.19,1-abs(SurfaceNormal.z));
float3 grass=lerp(float3(.035,.05,.019),float3(.1,.12,.052),n);
float3 rock=lerp(float3(.12,.115,.085),float3(.24,.21,.15),n);
return lerp(grass,rock,slope)*(.72+.55*Detail.r);''')
 for e,pin,out in [(world,'World',''),(detail,'Detail','RGB'),(norm,'SurfaceNormal','')]:assert ml.connect_material_expressions(e,out,custom,pin)
 assert ml.connect_material_property(custom,'',unreal.MaterialProperty.MP_BASE_COLOR)
 ml.connect_material_property(scalar(m,.96,0,260),'',unreal.MaterialProperty.MP_ROUGHNESS)
 ml.recompile_material(m);unreal.EditorAssetLibrary.set_metadata_tag(m,'HomeMap.SurfaceBuilt','1');unreal.EditorAssetLibrary.save_loaded_asset(m)
print('ESTATE_MATERIAL_READY',m.get_path_name())
