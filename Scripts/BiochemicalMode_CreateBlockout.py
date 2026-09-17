import math
import unreal


MAP_PATH = "/Game/Maps/TestMap_Biochemical"
MANAGED_FOLDER = "BioBlockout"
CUBE_PATH = "/Engine/BasicShapes/Cube"


def log(message):
    unreal.log(f"[BiochemicalBlockout] {message}")
    print(f"[BiochemicalBlockout] {message}")


def folder_matches(actor):
    folder = str(actor.get_folder_path())
    return folder == MANAGED_FOLDER or folder.startswith(MANAGED_FOLDER + "/")


def spawn_box(actor_subsystem, mesh, label, location, size, rotation=(0.0, 0.0, 0.0),
              folder="Geometry"):
    actor = actor_subsystem.spawn_actor_from_object(
        mesh,
        unreal.Vector(*location),
        unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if actor is None:
        raise RuntimeError(f"Cannot spawn blockout box: {label}")
    actor.set_actor_label(label)
    actor.set_folder_path(f"{MANAGED_FOLDER}/{folder}")
    actor.set_actor_scale3d(unreal.Vector(
        size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return actor


def spawn_ramp(actor_subsystem, mesh, label, start, end, width, thickness, folder):
    dx = end[0] - start[0]
    dy = end[1] - start[1]
    dz = end[2] - start[2]
    horizontal = math.sqrt(dx * dx + dy * dy)
    length = math.sqrt(horizontal * horizontal + dz * dz)
    midpoint = (
        (start[0] + end[0]) * 0.5,
        (start[1] + end[1]) * 0.5,
        (start[2] + end[2]) * 0.5,
    )
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = -math.degrees(math.atan2(dz, horizontal))
    return spawn_box(
        actor_subsystem, mesh, label, midpoint,
        (length, width, thickness), (pitch, yaw, 0.0), folder)


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

if asset_subsystem.does_asset_exist(MAP_PATH):
    if not level_subsystem.load_level(MAP_PATH):
        raise RuntimeError(f"Cannot load existing map: {MAP_PATH}")
    existing_managed = [
        actor for actor in actor_subsystem.get_all_level_actors()
        if folder_matches(actor)
    ]
    if existing_managed:
        log(f"Blockout already exists; actors={len(existing_managed)}")
        raise RuntimeError(
            "Refusing to overwrite an existing BioBlockout. Audit the map before regeneration.")
else:
    if not level_subsystem.new_level(MAP_PATH, False):
        raise RuntimeError(f"Cannot create map: {MAP_PATH}")

cube = unreal.load_asset(CUBE_PATH)
if cube is None:
    raise RuntimeError(f"Cannot load cube mesh: {CUBE_PATH}")

# 总尺度约 15000 × 12000 cm。外环保持闭合，但内部用低墙、设备块和三条横向通道打断长直线。
boxes = [
    ("ArenaFloor", (0, 0, -25), (15000, 12000, 50), (0, 0, 0), "Arena"),
    ("Boundary_North", (0, 5950, 250), (15000, 100, 500), (0, 0, 0), "Arena"),
    ("Boundary_South", (0, -5950, 250), (15000, 100, 500), (0, 0, 0), "Arena"),
    ("Boundary_East", (7450, 0, 250), (100, 12000, 500), (0, 0, 0), "Arena"),
    ("Boundary_West", (-7450, 0, 250), (100, 12000, 500), (0, 0, 0), "Arena"),
    ("Central_Core", (0, 0, 200), (1800, 1800, 400), (0, 0, 0), "Arena"),
    ("Connector_NorthCover", (0, 2900, 150), (2500, 350, 300), (0, 0, 0), "Routes"),
    ("Connector_SouthCover", (0, -3300, 150), (2500, 350, 300), (0, 0, 0), "Routes"),
    ("Connector_WestCover", (-2600, 400, 150), (350, 2200, 300), (0, 0, 0), "Routes"),
    ("Connector_EastCover", (3000, 500, 150), (350, 2200, 300), (0, 0, 0), "Routes"),
    ("RouteBlock_NW", (-5800, 2400, 225), (1200, 800, 450), (0, 20, 0), "Routes"),
    ("RouteBlock_SE", (5600, -2400, 225), (1300, 900, 450), (0, -15, 0), "Routes"),

    # A 仓库天桥：容量最大，正面坡道、后侧慢坡和跳下撤离边缘。
    ("A_WarehouseDeck", (-4300, -2200, 450), (3000, 1800, 100), (0, 0, 0), "Stronghold_A"),
    ("A_WarehouseBackWall", (-5650, -2200, 750), (300, 1800, 700), (0, 0, 0), "Stronghold_A"),
    ("A_WarehouseRailNorth", (-4300, -3050, 600), (3000, 100, 300), (0, 0, 0), "Stronghold_A"),
    ("A_WarehouseRailSouth", (-4300, -1350, 600), (1600, 100, 300), (0, 0, 0), "Stronghold_A"),
    ("A_WarehouseSupport01", (-5200, -2600, 200), (300, 300, 400), (0, 0, 0), "Stronghold_A"),
    ("A_WarehouseSupport02", (-3400, -1800, 200), (300, 300, 400), (0, 0, 0), "Stronghold_A"),

    # B 通风设备区：正面窄口之外，保留东侧维护路和西侧顶部突破面。
    ("B_VentDeck", (3800, -1700, 400), (2400, 1900, 100), (0, 0, 0), "Stronghold_B"),
    ("B_VentUnit01", (3350, -1900, 650), (700, 700, 500), (0, 0, 0), "Stronghold_B"),
    ("B_VentUnit02", (4200, -1400, 600), (650, 650, 400), (0, 0, 0), "Stronghold_B"),
    ("B_FrontChokeNorth", (2550, -2200, 250), (500, 150, 500), (0, 0, 0), "Stronghold_B"),
    ("B_FrontChokeSouth", (2550, -1200, 250), (500, 150, 500), (0, 0, 0), "Stronghold_B"),
    ("B_MaintenanceWall", (5100, -1700, 250), (150, 1300, 500), (0, 0, 0), "Stronghold_B"),

    # C 观察塔：容量最小、视野最高，但有主坡与后侧短坡两条进攻路线。
    ("C_ObservationDeck", (2500, 3400, 825), (1500, 1300, 100), (0, 0, 0), "Stronghold_C"),
    ("C_ObservationCore", (2500, 3400, 400), (600, 600, 800), (0, 0, 0), "Stronghold_C"),
    ("C_ObservationRailNorth", (2500, 4000, 975), (1500, 100, 300), (0, 0, 0), "Stronghold_C"),
    ("C_ObservationRailEast", (3200, 3400, 975), (100, 1300, 300), (0, 0, 0), "Stronghold_C"),
]

for label, location, size, rotation, folder in boxes:
    spawn_box(actor_subsystem, cube, label, location, size, rotation, folder)

ramps = [
    ("A_MainRamp", (-2100, -2200, 50), (-2900, -2200, 450), 700, 100, "Stronghold_A"),
    ("A_BackFlankRamp", (-6100, -900, 50), (-5400, -1400, 450), 500, 100, "Stronghold_A"),
    ("B_MainRamp", (2100, -1700, 50), (2700, -1700, 400), 500, 100, "Stronghold_B"),
    ("B_MaintenanceRamp", (5600, -700, 50), (4950, -1100, 400), 400, 100, "Stronghold_B"),
    ("C_MainRamp", (700, 3000, 50), (1800, 3300, 825), 550, 100, "Stronghold_C"),
    ("C_RearRamp", (4000, 4500, 50), (3150, 3850, 825), 400, 100, "Stronghold_C"),
]
for label, start, end, width, thickness, folder in ramps:
    spawn_ramp(actor_subsystem, cube, label, start, end, width, thickness, folder)

# 出生点只做白盒标记。Human 与 Infected 使用 PlayerStartTag，正式 GameMode 再按阵营筛选。
spawn_points = [
    ("HumanStart_01", (-700, -800, 120), "HumanStart"),
    ("HumanStart_02", (700, -800, 120), "HumanStart"),
    ("HumanStart_03", (-700, 800, 120), "HumanStart"),
    ("HumanStart_04", (700, 800, 120), "HumanStart"),
    ("InfectedPocket_NW", (-6500, 4700, 120), "InfectedStart"),
    ("InfectedPocket_NE", (6500, 4700, 120), "InfectedStart"),
    ("InfectedPocket_SW", (-6500, -4700, 120), "InfectedStart"),
    ("InfectedPocket_SE", (6500, -4700, 120), "InfectedStart"),
]
for label, location, start_tag in spawn_points:
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(*location),
        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    if actor is None:
        raise RuntimeError(f"Cannot spawn PlayerStart: {label}")
    actor.set_actor_label(label)
    actor.set_folder_path(f"{MANAGED_FOLDER}/Markers/{start_tag}")
    actor.set_editor_property("player_start_tag", unreal.Name(start_tag))

# 提供最低限度的编辑器与 PIE 可见光照，不在 C++ 硬编码视觉参数。
sun = actor_subsystem.spawn_actor_from_class(
    unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 2500.0),
    unreal.Rotator(pitch=-45.0, yaw=-35.0, roll=0.0))
sun.set_actor_label("BioBlockout_Sun")
sun.set_folder_path(f"{MANAGED_FOLDER}/Lighting")
sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property(
    "intensity", 5.0)

sky = actor_subsystem.spawn_actor_from_class(
    unreal.SkyLight, unreal.Vector(0.0, 0.0, 500.0),
    unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
sky.set_actor_label("BioBlockout_Sky")
sky.set_folder_path(f"{MANAGED_FOLDER}/Lighting")
sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity", 1.0)

if not level_subsystem.save_current_level():
    raise RuntimeError(f"Cannot save map: {MAP_PATH}")

managed_count = len([
    actor for actor in actor_subsystem.get_all_level_actors()
    if folder_matches(actor)
])
log(f"Created {MAP_PATH}; managed actors={managed_count}")
