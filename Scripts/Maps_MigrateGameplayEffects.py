import unreal


SOURCE_MAP = "/Game/Maps/HomeMap"
TARGET_MAPS = [
    "/Game/Maps/TestMap_ListenServer",
    "/Game/Maps/TestMap_SplitScreen",
]
SOURCE_FOLDER = "GameplayEffects"


def log(message):
    unreal.log(f"[GameplayEffectsMigration] {message}")
    print(f"[GameplayEffectsMigration] {message}")


def actor_folder(actor):
    return str(actor.get_folder_path())


def is_managed_actor(actor):
    folder = actor_folder(actor)
    return folder == SOURCE_FOLDER or folder.startswith(SOURCE_FOLDER + "/")


def actor_signature(actor):
    location = actor.get_actor_location()
    rotation = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    return (
        actor.get_actor_label(),
        actor.get_class().get_path_name(),
        round(location.x, 3), round(location.y, 3), round(location.z, 3),
        round(rotation.pitch, 3), round(rotation.yaw, 3), round(rotation.roll, 3),
        round(scale.x, 3), round(scale.y, 3), round(scale.z, 3),
    )


def world_actors(world):
    return list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor))


level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not level_subsystem.load_level(SOURCE_MAP):
    raise RuntimeError(f"Cannot load source map: {SOURCE_MAP}")

source_actors = [
    actor for actor in actor_subsystem.get_all_level_actors()
    if is_managed_actor(actor)
]
if not source_actors:
    raise RuntimeError(f"No actors found under {SOURCE_FOLDER} in {SOURCE_MAP}")

source_signatures = sorted(actor_signature(actor) for actor in source_actors)
log(f"Source actors={len(source_actors)}")

# 在切换编辑器当前地图前，同时把源 Actor 复制到两个已加载的目标 UWorld。
# DuplicateActors 会保留蓝图实例属性、组件配置和 Transform，比按类重新 Spawn 更可靠。
targets_to_verify = []
for target_path in TARGET_MAPS:
    target_world = unreal.load_asset(target_path)
    if target_world is None:
        raise RuntimeError(f"Cannot load target map asset: {target_path}")

    existing = [actor for actor in world_actors(target_world) if is_managed_actor(actor)]
    if existing:
        existing_signatures = sorted(actor_signature(actor) for actor in existing)
        if existing_signatures != source_signatures:
            raise RuntimeError(
                f"{target_path} already has a non-matching {SOURCE_FOLDER} folder; "
                "refusing to create duplicates")
        log(f"Already synchronized: {target_path} actors={len(existing)}")
    else:
        duplicated = list(actor_subsystem.duplicate_actors(
            source_actors, target_world, unreal.Vector(0.0, 0.0, 0.0)))
        if len(duplicated) != len(source_actors):
            raise RuntimeError(
                f"Duplicate count mismatch for {target_path}: "
                f"expected={len(source_actors)} actual={len(duplicated)}")
        for source_actor, duplicated_actor in zip(source_actors, duplicated):
            duplicated_actor.set_folder_path(source_actor.get_folder_path())
            duplicated_actor.set_actor_label(source_actor.get_actor_label())
        if not unreal.EditorAssetLibrary.save_loaded_asset(
                target_world, only_if_is_dirty=False):
            raise RuntimeError(f"Cannot save target map: {target_path}")
        log(f"Duplicated: {target_path} actors={len(duplicated)}")
        del duplicated
    del existing
    del target_world
    targets_to_verify.append(target_path)

# 逐图重新加载验证磁盘结果，避免只验证内存中的 DuplicateActors 返回值。
# 切图前必须释放所有源/目标 Actor 与 UWorld 的 Python 引用；否则 UE 会把仍被 Python 持有的旧世界
# 判定为 World Memory Leak，并在 LoadLevel 中触发致命断言。
del source_actors
unreal.SystemLibrary.collect_garbage()
for target_path in targets_to_verify:
    if not level_subsystem.load_level(target_path):
        raise RuntimeError(f"Cannot reload target map: {target_path}")
    target_actors = [
        actor for actor in actor_subsystem.get_all_level_actors()
        if is_managed_actor(actor)
    ]
    target_signatures = sorted(actor_signature(actor) for actor in target_actors)
    if target_signatures != source_signatures:
        raise RuntimeError(f"Persisted actor verification failed: {target_path}")
    log(f"Verified: {target_path} actors={len(target_actors)}")
    del target_actors
    unreal.SystemLibrary.collect_garbage()

if not level_subsystem.load_level(SOURCE_MAP):
    raise RuntimeError(f"Cannot return to source map: {SOURCE_MAP}")
log("Migration completed and returned to HomeMap")
