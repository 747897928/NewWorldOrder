"""复用原 M_Fabric，隔离家具烘焙法线，不修改母材质。"""
import unreal
path='/Game/Environment/HomeMap/Materials/Hub/MI_Suite_Linen'
ml=unreal.MaterialEditingLibrary
linen=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('MI_Suite_Linen','/Game/Environment/HomeMap/Materials/Hub',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
ml.set_material_instance_parent(linen,unreal.load_asset('/Game/Environment/HomeMap/Materials/M_Fabric'))
# 原图 Dirt Intencity 是 Lerp.A，B=1，Alpha 为 Dirt Map；值 1 才是无污渍，0 反而最脏。
for key,val in [('Texture Scaling',7),('Normal Map Intencity',.18),('Dirt Intencity',1),('Base Power',1),('Base Exponentin',4),('Base Fallof',2)]:ml.set_material_instance_scalar_parameter_value(linen,key,val)
ml.set_material_instance_vector_parameter_value(linen,'Albedo Color',unreal.LinearColor(.52,.48,.40,1))
ml.set_material_instance_texture_parameter_value(linen,'Normal Map',unreal.load_asset('/Game/Environment/HomeMap/Textures/T_Fabric_N'))
ml.update_material_instance(linen);unreal.EditorAssetLibrary.save_loaded_asset(linen)
