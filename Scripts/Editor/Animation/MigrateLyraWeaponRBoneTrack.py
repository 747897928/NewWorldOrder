import unreal


SOURCE_PREFIX = "/Game/Characters/Heroes/Mannequin/Animations/"
TARGET_PREFIX_TEMPLATE = "/Game/Characters/Heroes/CC/{side}/Animations/"


def _build_items(side, group, allow_missing_sources, additive_only):
    target_prefix = TARGET_PREFIX_TEMPLATE.format(side=side)
    items = []
    missing = []

    for object_path in unreal.EditorAssetLibrary.list_assets(
        target_prefix + group, recursive=True, include_folder=False
    ):
        target_path = str(object_path).split(".")[0]
        target = unreal.load_asset(target_path)
        if not target or target.get_class().get_name() != "AnimSequence":
            continue

        source_path = SOURCE_PREFIX + target_path[len(target_prefix) :]
        source = unreal.load_asset(source_path)
        if not source or source.get_class().get_name() != "AnimSequence":
            missing.append(target_path)
            continue

        is_additive = (
            source.get_editor_property("additive_anim_type")
            != unreal.AdditiveAnimationType.AAT_NONE
        )
        if additive_only is not None and is_additive != additive_only:
            continue

        source_model = source.get_editor_property("data_model_interface")
        target_model = target.get_editor_property("data_model_interface")
        source_rate = source_model.get_frame_rate()
        target_rate = target_model.get_frame_rate()
        same_contract = (
            source_model.get_number_of_keys() == target_model.get_number_of_keys()
            and source_rate.numerator == target_rate.numerator
            and source_rate.denominator == target_rate.denominator
        )
        if not same_contract:
            raise RuntimeError("源/目标采样契约不一致：" + target_path)

        items.append((source_path, target_path))

    if missing and not allow_missing_sources:
        raise RuntimeError("缺少同路径 Lyra 源动画：" + " | ".join(missing))

    return items, missing


def _quaternion_dot(left, right):
    return abs(
        left.x * right.x
        + left.y * right.y
        + left.z * right.z
        + left.w * right.w
    )


def migrate(side, group, allow_missing_sources=False, additive_only=None):
    """把 Lyra weapon_r 的逐帧局部轨道迁到一组正式 CC 动画。

    该工具只接受同路径、同关键帧数和同帧率的源/目标 AnimSequence。
    添加的是 hand_r 子骨骼 weapon_r 的 Local Transform，不能改成 ik_hand_gun。
    AnimationDataController 接受的是动画原始绝对关键帧。普通动画直接读取 Local Pose；
    Additive 动画必须先还原 Full Pose，禁止把已经减过 Base Pose 的差值再次写入并二次做差。
    additive_only=True 只重建 Additive 子集，False 只处理普通动画，None 处理全部。
    """

    items, missing = _build_items(
        side, group, allow_missing_sources, additive_only
    )

    changed = 0
    maximum_translation_error = 0.0
    minimum_quaternion_dot = 1.0
    failures = []

    for source_path, target_path in items:
        try:
            source = unreal.load_asset(source_path)
            target = unreal.load_asset(target_path)
            target_model = target.get_editor_property("data_model_interface")
            key_count = int(target_model.get_number_of_keys())

            options = unreal.AnimPoseEvaluationOptions()
            options.should_retarget = False
            options.retrieve_additive_as_full_pose = (
                source.get_editor_property("additive_anim_type")
                != unreal.AdditiveAnimationType.AAT_NONE
            )

            positions = []
            rotations = []
            scales = []
            for frame_index in range(key_count):
                pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(
                    source, frame_index, options
                )
                transform = unreal.AnimPoseExtensions.get_bone_pose(
                    pose, "weapon_r", unreal.AnimPoseSpaces.LOCAL
                )
                positions.append(transform.translation)
                rotations.append(transform.rotation)
                scales.append(transform.scale3d)

            controller = target.get_editor_property("controller")
            controller.open_bracket("迁移 Lyra weapon_r 轨道", should_transact=False)
            try:
                if not target_model.is_valid_bone_track_name("weapon_r"):
                    if not controller.add_bone_curve("weapon_r", should_transact=False):
                        raise RuntimeError("添加 weapon_r BoneCurve 失败")
                if not controller.set_bone_track_keys(
                    "weapon_r",
                    positions,
                    rotations,
                    scales,
                    should_transact=False,
                ):
                    raise RuntimeError("写入 weapon_r 关键帧失败")
            finally:
                controller.close_bracket(should_transact=False)

            if not unreal.EditorAssetLibrary.save_loaded_asset(
                target, only_if_is_dirty=False
            ):
                raise RuntimeError("保存目标动画失败")

            for frame_index in (0, key_count // 2, key_count - 1):
                source_pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(
                    source, frame_index, options
                )
                target_pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(
                    target, frame_index, options
                )
                source_transform = unreal.AnimPoseExtensions.get_bone_pose(
                    source_pose, "weapon_r", unreal.AnimPoseSpaces.LOCAL
                )
                target_transform = unreal.AnimPoseExtensions.get_bone_pose(
                    target_pose, "weapon_r", unreal.AnimPoseSpaces.LOCAL
                )
                translation_error = (
                    source_transform.translation - target_transform.translation
                ).length()
                quaternion_dot = _quaternion_dot(
                    source_transform.rotation, target_transform.rotation
                )
                maximum_translation_error = max(
                    maximum_translation_error, translation_error
                )
                minimum_quaternion_dot = min(
                    minimum_quaternion_dot, quaternion_dot
                )

            changed += 1
        except Exception as error:
            failures.append(target_path + "|" + str(error))

    print(
        "BATCH_RESULT|{}|{}|ADDITIVE_ONLY={}|TOTAL={}|CHANGED={}|MISSING={}|FAIL={}|MAX_DT={}|MIN_QDOT={}".format(
            side,
            group,
            additive_only,
            len(items),
            changed,
            len(missing),
            len(failures),
            maximum_translation_error,
            minimum_quaternion_dot,
        )
    )
    for path in missing:
        print("BATCH_MISSING|" + path)
    for failure in failures:
        print("BATCH_FAIL|" + failure)

    if failures:
        raise RuntimeError("weapon_r 批量迁移存在失败资产")

    return changed
