import bpy, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
assert Path(bpy.data.filepath).resolve()==(ROOT/'SourceArt/HomeMap/HomeMap_Master.blend').resolve()
scene=bpy.context.scene
for name in ['Cube','Camera','Light']:
    obj=bpy.data.objects.get(name)
    if obj and not any(c.name in ['HomeMap_Architecture','Landscape_Source'] for c in obj.users_collection):obj.hide_render=True
review=bpy.data.collections.get('Source_Review')
if not review:review=bpy.data.collections.new('Source_Review');scene.collection.children.link(review)
cam=bpy.data.objects.get('HomeMap_ReviewCamera')
if not cam:cam=bpy.data.objects.new('HomeMap_ReviewCamera',bpy.data.cameras.new('HomeMap_ReviewCamera'));review.objects.link(cam)
cam.location=(23,-29,19)
cam.rotation_euler=(Vector((0,-.5,1.7))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=40;scene.camera=cam
sun=bpy.data.objects.get('HomeMap_ReviewSun')
if not sun:sun=bpy.data.objects.new('HomeMap_ReviewSun',bpy.data.lights.new('HomeMap_ReviewSun','SUN'));review.objects.link(sun)
sun.rotation_euler=(math.radians(20),math.radians(-25),math.radians(-35));sun.data.energy=2;sun.data.angle=.12
world=scene.world or bpy.data.worlds.new('HomeMap_ReviewWorld');scene.world=world;world.use_nodes=True
world.node_tree.nodes['Background'].inputs['Color'].default_value=(.35,.43,.55,1)
world.node_tree.nodes['Background'].inputs['Strength'].default_value=.45
glass=bpy.data.materials['Preview_Glass'];glass.use_nodes=True
bsdf=glass.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Base Color'].default_value=(.2,.32,.38,1);bsdf.inputs['Alpha'].default_value=.12;bsdf.inputs['Roughness'].default_value=.15
glass.surface_render_method='DITHERED'
scene.render.engine='CYCLES';scene.cycles.samples=16;scene.cycles.use_denoising=True
scene.cycles.device='GPU'
prefs=bpy.context.preferences.addons['cycles'].preferences
try:
    prefs.compute_device_type='OPTIX';prefs.get_devices()
    for d in prefs.devices:d.use=d.type!='CPU'
except Exception:scene.cycles.device='CPU'
scene.render.resolution_x=1400;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.render.filepath=str(ROOT/'Docs/Tasks/HomeMap/Reports/Blender_Architecture_Review.png')
scene.render.image_settings.file_format='PNG'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'SourceArt/HomeMap/HomeMap_Master.blend'))
bpy.ops.render.render(write_still=True)
print('SOURCE_REVIEW',scene.render.filepath)
