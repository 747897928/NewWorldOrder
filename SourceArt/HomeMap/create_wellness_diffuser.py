"""新灯具独立的发光面材质，不修改已用于全屋的旧 M_Warm_Light。"""
import unreal

path='/Game/Environment/HomeMap/Materials/Hub/M_WellnessDiffuser'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    m=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_WellnessDiffuser','/Game/Environment/HomeMap/Materials/Hub',unreal.Material,unreal.MaterialFactoryNew())
    ml=unreal.MaterialEditingLibrary
    color=ml.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-400,0)
    color.set_editor_property('parameter_name','DiffuserColor')
    color.set_editor_property('default_value',unreal.LinearColor(1,.88,.7,1))
    strength=ml.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-400,160)
    strength.set_editor_property('parameter_name','EmissionStrength')
    strength.set_editor_property('default_value',12)
    mult=ml.create_material_expression(m,unreal.MaterialExpressionMultiply,-180,90)
    rough=ml.create_material_expression(m,unreal.MaterialExpressionConstant,-180,250)
    rough.set_editor_property('r',.65)
    assert ml.connect_material_expressions(color,'',mult,'A')
    assert ml.connect_material_expressions(strength,'',mult,'B')
    assert ml.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    assert ml.connect_material_property(mult,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert ml.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    ml.recompile_material(m)
    print('CREATED',path,unreal.EditorAssetLibrary.save_loaded_asset(m))
else:
    print('EXISTS',path)
