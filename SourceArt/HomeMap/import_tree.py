import unreal, json
from pathlib import Path

ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
folder='/Game/Environment/HomeMap/Landscape'
at=unreal.AssetToolsHelpers.get_asset_tools();ml=unreal.MaterialEditingLibrary
tasks=[]
files=[ROOT/'SourceArt/HomeMap/Exports/SM_HM_IslandTree.fbx']
files+=list((ROOT/'SourceArt/HomeMap/PolyHaven/island_tree_01/textures').glob('*.jpg'))
for file in files:
    dest=folder+('/Meshes' if file.suffix=='.fbx' else '/Textures')
    if unreal.EditorAssetLibrary.does_asset_exist(dest+'/'+file.stem):continue
    t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=dest;t.automated=True;t.save=True
    if file.suffix=='.fbx':
        opt=unreal.FbxImportUI();opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False
        opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.static_mesh_import_data.combine_meshes=True
        opt.static_mesh_import_data.auto_generate_collision=False;t.options=opt
    tasks.append(t)
at.import_asset_tasks(tasks)
for stem in ['island_tree_01','island_tree_01_leaves','island_tree_01_branches']:
    path=folder+'/Materials/M_'+stem
    if unreal.EditorAssetLibrary.does_asset_exist(path):continue
    m=at.create_asset('M_'+stem,folder+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    if 'leaves' in stem:
        m.set_editor_property('two_sided',True)
        m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    nodes={}
    for i,(suffix,stype) in enumerate([('diff',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),('nor_gl',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),('arm',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)]):
        texture=unreal.load_asset(folder+'/Textures/'+stem+'_'+suffix+'_2k')
        if suffix=='nor_gl':
            texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
            texture.set_editor_property('srgb',False);texture.set_editor_property('flip_green_channel',True)
        if suffix=='arm':texture.set_editor_property('srgb',False)
        texture.set_editor_property('max_texture_size',2048)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        n=ml.create_material_expression(m,unreal.MaterialExpressionTextureSample,-500,i*240)
        n.texture=texture;n.sampler_type=stype;nodes[suffix]=n
    ml.connect_material_property(nodes['diff'],'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    ml.connect_material_property(nodes['nor_gl'],'RGB',unreal.MaterialProperty.MP_NORMAL)
    ml.connect_material_property(nodes['arm'],'G',unreal.MaterialProperty.MP_ROUGHNESS)
    ml.connect_material_property(nodes['arm'],'R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    if 'leaves' in stem:ml.connect_material_property(nodes['diff'],'RGB',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    ml.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
    print('CREATED',path)
mesh=unreal.load_asset(folder+'/Meshes/SM_HM_IslandTree')
for i,slot in enumerate(mesh.static_materials):
    stem=str(slot.material_slot_name)
    mat=unreal.load_asset(folder+'/Materials/M_'+stem)
    assert mat,stem
    mesh.set_material(i,mat)
nanite=mesh.get_editor_property('nanite_settings');nanite.enabled=True;mesh.set_editor_property('nanite_settings',nanite)
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
print('TREE_READY',mesh.get_bounding_box(),[(str(s.material_slot_name),str(s.material_interface)) for s in mesh.static_materials])
