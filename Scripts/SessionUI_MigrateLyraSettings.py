import os

import unreal


ROOT_WIDGET_PACKAGES = [
    "/Game/UI/Settings/W_LyraSettingScreen",
    "/Game/UI/Settings/W_SettingsPanel",
    "/Game/UI/Settings/W_GameSettingsDetailView",
]
DESTINATION_CONTENT = os.environ.get("NWO_DESTINATION_CONTENT", "")


def log(message):
    unreal.log(f"[SessionUISettingsMigration] {message}")
    print(f"[SessionUISettingsMigration] {message}")


missing = [package for package in ROOT_WIDGET_PACKAGES
           if not unreal.EditorAssetLibrary.does_asset_exist(package)]
if missing:
    raise RuntimeError(f"Lyra source packages are missing: {missing}")
if not DESTINATION_CONTENT:
    raise RuntimeError("Set NWO_DESTINATION_CONTENT to this project's Content directory before migration")

options = unreal.MigrationOptions()
options.set_editor_property("prompt", False)
options.set_editor_property("ignore_dependencies", False)
options.set_editor_property("asset_conflict", unreal.AssetMigrationConflict.SKIP)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
log(f"Migrating roots={ROOT_WIDGET_PACKAGES}")
log(f"Destination={DESTINATION_CONTENT}; conflicts=Skip; dependencies=Included")
asset_tools.migrate_packages(ROOT_WIDGET_PACKAGES, DESTINATION_CONTENT, options)
log("Migration request completed")
