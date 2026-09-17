import unreal

# 搜索页与大厅页的固定层级、英文源文案和材质全部写入 Widget Blueprint。
# C++ 只提供真实会话/成员状态和语义动作，页面设计者可以自由移动或替换非业务控件。
ROOT = '/Game/UI/Menu/Expedition'
SOURCE = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_dir() + 'Scripts/SessionUI_BuildExpeditionWidgets.py')
_shared = {'__name__': 'session_ui_widget_helpers'}
with open(SOURCE, 'r', encoding='utf-8') as _handle:
    exec(compile(_handle.read(), SOURCE, 'exec'), _shared)

WidgetBuilder = _shared['WidgetBuilder']
WS = unreal.WidgetService
WHITE = _shared['WHITE']
MUTED = _shared['MUTED']
CYAN = _shared['CYAN']
PANEL = _shared['PANEL']


def fill(builder, name):
    builder.prop(name, 'Slot.HorizontalAlignment', 'Fill')
    builder.prop(name, 'Slot.VerticalAlignment', 'Fill')


def text(builder, name, parent, value, size=18, color=WHITE, bold=False, variable=False):
    return builder.text(name, parent, value, size, color, bold, variable)


def set_button_text(builder, name, value, key):
    builder.prop(name, 'bOverride_ButtonText', 'True')
    builder.prop(name, 'ButtonText', 'NSLOCTEXT("ExpeditionUI", "' + key + '", "' + value + '")')


def icon(builder, name, parent, asset_number, size=28, tint=CYAN, variable=False):
    box_name = name + 'Size'
    builder.add('SizeBox', box_name, parent)
    builder.prop(box_name, 'WidthOverride', str(size))
    builder.prop(box_name, 'HeightOverride', str(size))
    builder.prop(box_name, 'Vertical Alignment', 'Center')
    builder.add('Image', name, box_name, variable)
    brush = unreal.WidgetBrushInfo()
    brush.resource_path = (
        '/Game/UI/Textures/Session/White/T_UI_Icon_{0:02d}.'
        'T_UI_Icon_{0:02d}'.format(asset_number))
    brush.draw_as = 'Image'
    brush.tint_color = tint
    assert WS.set_brush(builder.path, name, 'Brush', brush)
    return name


def frame_brush():
    brush = unreal.WidgetBrushInfo()
    brush.resource_path = (
        '/Game/UI/Hud/Art/MI_UI_QuickBar_Border_Square.'
        'MI_UI_QuickBar_Border_Square')
    brush.draw_as = 'Box'
    brush.margin = '(Left=0.5,Top=0.5,Right=0.5,Bottom=0.5)'
    brush.tint_color = '(R=0.14,G=0.39,B=0.54,A=1)'
    return brush


def add_panel(builder, name, parent, padding=20, color=PANEL):
    builder.add('Border', name, parent)
    builder.prop(name, 'BrushColor', color)
    builder.prop(name, 'Padding', '(Left={0},Top={0},Right={0},Bottom={0})'.format(padding))
    return name


def add_page_shell(builder, title_value, subtitle_value):
    builder.add('Overlay', 'ScreenLayers')
    add_panel(builder, 'ScreenDim', 'ScreenLayers', 0, '(R=0.002,G=0.006,B=0.012,A=0.82)')
    builder.add('SafeZone', 'SafeArea', 'ScreenLayers')
    builder.add('Border', 'OuterPadding', 'SafeArea')
    builder.prop('OuterPadding', 'BrushColor', '(R=0,G=0,B=0,A=0)')
    builder.prop('OuterPadding', 'Padding', '(Left=48,Top=36,Right=48,Bottom=32)')
    builder.add('VerticalBox', 'Page', 'OuterPadding')
    builder.add('HorizontalBox', 'Header', 'Page')
    builder.add('VerticalBox', 'HeaderText', 'Header')
    text(builder, 'Brand', 'HeaderText', 'NEW WORLD ORDER', 13, MUTED, True)
    builder.space('BrandGap', 'HeaderText', 6)
    text(builder, 'PageTitle', 'HeaderText', title_value, 40, WHITE, True)
    text(builder, 'PageSubtitle', 'HeaderText', subtitle_value, 16, MUTED)
    builder.space('HeaderFill', 'Header', 1, True)
    # SafeZone 本身填满 CommonUI 层；OuterPadding 的内容对齐由 Border 自身的 Fill 配置负责。
    for name in ['ScreenDim', 'SafeArea']:
        fill(builder, name)
    builder.prop('ScreenDim', 'Visibility', 'HitTestInvisible')
    builder.space('HeaderBodyGap', 'Page', 22)


def build_squad_list_item():
    b = WidgetBuilder('W_SquadListItem', unreal.ShootSquadListItem)
    b.add('SizeBox', 'RowSize')
    b.prop('RowSize', 'MinDesiredHeight', '92')
    b.add('Overlay', 'ButtonLayers', 'RowSize')
    add_panel(b, 'ButtonFill', 'ButtonLayers', 0, '(R=0.009,G=0.028,B=0.043,A=0.98)')
    b.add('Image', 'ButtonFrame', 'ButtonLayers')
    assert WS.set_brush(b.path, 'ButtonFrame', 'Brush', frame_brush())
    b.add('HorizontalBox', 'RowContent', 'ButtonLayers')
    b.prop('RowContent', 'Padding', '(Left=18,Top=14,Right=18,Bottom=14)')
    icon(b, 'SquadIcon', 'RowContent', 2, 32)
    b.space('IconGap', 'RowContent', 14)
    b.add('VerticalBox', 'MissionColumn', 'RowContent')
    b.prop('MissionColumn', 'Size Rule', 'Fill')
    b.prop('MissionColumn', 'Vertical Alignment', 'Center')
    text(b, 'MissionText', 'MissionColumn', 'Expedition', 19, WHITE, True, True)
    b.space('MissionGap', 'MissionColumn', 4)
    text(b, 'MapText', 'MissionColumn', 'Destination', 13, MUTED, False, True)
    b.add('HorizontalBox', 'PlayerColumn', 'RowContent')
    b.prop('PlayerColumn', 'Vertical Alignment', 'Center')
    icon(b, 'PlayersIcon', 'PlayerColumn', 10, 24)
    b.space('PlayersGap', 'PlayerColumn', 8)
    text(b, 'CurrentPlayersText', 'PlayerColumn', '0', 17, WHITE, True, True)
    text(b, 'PlayersSlash', 'PlayerColumn', ' / ', 17, MUTED, False)
    text(b, 'MaxPlayersText', 'PlayerColumn', '4', 17, WHITE, True, True)
    b.space('PingColumnGap', 'RowContent', 34)
    b.add('VerticalBox', 'PingColumn', 'RowContent')
    b.prop('PingColumn', 'Vertical Alignment', 'Center')
    text(b, 'PingLabel', 'PingColumn', 'PING', 11, MUTED, True)
    text(b, 'PingText', 'PingColumn', '-- ms', 16, CYAN, True, True)
    b.space('ArrowGap', 'RowContent', 20)
    icon(b, 'ArrowIcon', 'RowContent', 20, 22)
    b.add('Image', 'ActiveFrame', 'ButtonLayers', True)
    glow = unreal.WidgetBrushInfo()
    glow.resource_path = '/Game/UI/Menu/Art/MI_UI_TileButton_Base_Glow.MI_UI_TileButton_Base_Glow'
    glow.draw_as = 'Box'
    glow.margin = '(Left=0.5,Top=0.5,Right=0.5,Bottom=0.5)'
    glow.tint_color = CYAN
    assert WS.set_brush(b.path, 'ActiveFrame', 'Brush', glow)
    b.prop('ActiveFrame', 'RenderOpacity', '0')
    for name in ['ButtonFill', 'ButtonFrame', 'RowContent', 'ActiveFrame']:
        fill(b, name)
        b.prop(name, 'Visibility', 'HitTestInvisible')
    b.finish()
    cdo = unreal.get_default_object(unreal.load_class(None, b.path + '.W_SquadListItem_C'))
    cdo.set_editor_property('style', unreal.load_class(
        None, '/Game/UI/Foundation/Buttons/ButtonStyle-Clear.ButtonStyle-Clear_C'))
    cdo.set_editor_property('selectable', True)
    cdo.set_editor_property('interactable_when_selected', True)
    unreal.EditorAssetLibrary.save_asset(b.path, only_if_is_dirty=False)


def build_player_slot():
    b = WidgetBuilder('W_PlayerSlot', unreal.ShootSquadPlayerSlot)
    b.add('SizeBox', 'SlotSize')
    b.prop('SlotSize', 'MinDesiredHeight', '76')
    b.add('Overlay', 'SlotLayers', 'SlotSize')
    add_panel(b, 'SlotFill', 'SlotLayers', 0, '(R=0.009,G=0.029,B=0.044,A=0.98)')
    b.add('Image', 'SlotFrame', 'SlotLayers')
    assert WS.set_brush(b.path, 'SlotFrame', 'Brush', frame_brush())
    b.add('HorizontalBox', 'SlotContent', 'SlotLayers')
    b.prop('SlotContent', 'Padding', '(Left=16,Top=12,Right=16,Bottom=12)')
    icon(b, 'PlayerIcon', 'SlotContent', 8, 34)
    b.space('PlayerIconGap', 'SlotContent', 14)
    b.add('VerticalBox', 'PlayerTextColumn', 'SlotContent')
    b.prop('PlayerTextColumn', 'Size Rule', 'Fill')
    b.prop('PlayerTextColumn', 'Vertical Alignment', 'Center')
    text(b, 'PlayerNameText', 'PlayerTextColumn', 'Open Slot', 18, WHITE, True, True)
    b.add('HorizontalBox', 'RoleLabels', 'PlayerTextColumn')
    text(b, 'HostText', 'RoleLabels', 'HOST', 11, '(R=1,G=0.70,B=0.18,A=1)', True, True)
    b.space('RoleGap', 'RoleLabels', 8)
    text(b, 'LocalText', 'RoleLabels', 'YOU', 11, CYAN, True, True)
    b.add('HorizontalBox', 'ReadyColumn', 'SlotContent')
    b.prop('ReadyColumn', 'Vertical Alignment', 'Center')
    icon(b, 'ReadyIcon', 'ReadyColumn', 14, 24, '(R=0.18,G=0.92,B=0.58,A=1)', True)
    b.space('ReadyGap', 'ReadyColumn', 8)
    text(b, 'ReadyText', 'ReadyColumn', 'WAITING', 14, MUTED, True, True)
    for name in ['SlotFill', 'SlotFrame', 'SlotContent']:
        fill(b, name)
        b.prop(name, 'Visibility', 'HitTestInvisible')
    b.finish()


def build_find_squad():
    b = WidgetBuilder('W_FindSquad', unreal.ShootFindSquadScreen)
    add_page_shell(b, 'FIND SQUADS', 'Choose an available squad and join its expedition.')
    b.add('Border', 'ResultsPanel', 'Page')
    b.prop('ResultsPanel', 'Size Rule', 'Fill')
    b.prop('ResultsPanel', 'BrushColor', '(R=0.005,G=0.019,B=0.030,A=0.98)')
    b.prop('ResultsPanel', 'Padding', '18')
    b.add('VerticalBox', 'ResultsContent', 'ResultsPanel')
    b.add('HorizontalBox', 'ColumnHeaders', 'ResultsContent')
    text(b, 'MissionHeader', 'ColumnHeaders', 'EXPEDITION', 12, MUTED, True)
    b.prop('MissionHeader', 'Size Rule', 'Fill')
    text(b, 'PlayersHeader', 'ColumnHeaders', 'PLAYERS', 12, MUTED, True)
    b.space('HeaderColumnGap', 'ColumnHeaders', 92)
    text(b, 'PingHeader', 'ColumnHeaders', 'PING', 12, MUTED, True)
    b.space('HeadersGap', 'ResultsContent', 12)
    b.add('Overlay', 'ResultsLayers', 'ResultsContent')
    b.prop('ResultsLayers', 'Size Rule', 'Fill')
    b.add('ScrollBox', 'ResultsScroll', 'ResultsLayers')
    b.add('DynamicEntryBox', 'SquadEntries', 'ResultsScroll', True)
    b.prop('SquadEntries', 'EntryBoxType', 'Vertical')
    b.prop('SquadEntries', 'EntrySpacing', '(X=0,Y=10)')
    b.prop('SquadEntries', 'EntryHorizontalAlignment', 'HAlign_Fill')
    b.prop('SquadEntries', 'NumDesignerPreviewEntries', '6')
    b.prop('SquadEntries', 'EntryWidgetClass',
           '/Game/UI/Menu/Expedition/W_SquadListItem.W_SquadListItem_C')
    # 空状态由 EventGraph 根据真实搜索结果切换显隐，因此需要暴露为蓝图变量。
    b.add('VerticalBox', 'EmptyState', 'ResultsLayers', True)
    b.prop('EmptyState', 'Horizontal Alignment', 'Center')
    b.prop('EmptyState', 'Vertical Alignment', 'Center')
    icon(b, 'EmptyIcon', 'EmptyState', 17, 54, MUTED)
    b.space('EmptyGap', 'EmptyState', 14)
    text(b, 'EmptyTitle', 'EmptyState', 'Searching for squads', 20, WHITE, True)
    text(b, 'EmptyHint', 'EmptyState', 'Available squads will appear here.', 14, MUTED)
    b.space('StatusGap', 'ResultsContent', 14)
    text(b, 'SearchStatus', 'ResultsContent', 'Connecting to online services…', 14, MUTED, False, True)
    b.space('ActionsGap', 'Page', 16)
    b.add('HorizontalBox', 'Actions', 'Page')
    b.add('W_ExpeditionButton', 'RefreshButton', 'Actions', True)
    set_button_text(b, 'RefreshButton', 'Refresh', 'RefreshSquads')
    b.add('W_ExpeditionButton', 'BackButton', 'Actions', True)
    set_button_text(b, 'BackButton', 'Back', 'BackFromSquads')
    b.space('ActionFill', 'Actions', 1, True)
    b.add('W_ExpeditionButton', 'JoinButton', 'Actions', True)
    set_button_text(b, 'JoinButton', 'Join Squad', 'JoinSquad')
    b.finish()


def build_squad_lobby():
    b = WidgetBuilder('W_SquadLobby', unreal.ShootExpeditionLobbyScreen)
    add_page_shell(b, 'SQUAD LOBBY', 'Ready up together, then deploy to the selected expedition.')
    b.add('HorizontalBox', 'LobbyBody', 'Page')
    b.prop('LobbyBody', 'Size Rule', 'Fill')
    b.add('Border', 'MissionPanel', 'LobbyBody')
    b.prop('MissionPanel', 'Size Rule', 'Fill')
    b.prop('MissionPanel', 'BrushColor', '(R=0.005,G=0.019,B=0.030,A=0.98)')
    b.prop('MissionPanel', 'Padding', '22')
    b.add('VerticalBox', 'MissionContent', 'MissionPanel')
    text(b, 'MissionKicker', 'MissionContent', 'SELECTED EXPEDITION', 12, CYAN, True)
    b.space('MissionTitleGap', 'MissionContent', 9)
    text(b, 'MissionTitle', 'MissionContent', 'Expedition selected by the host', 27, WHITE, True, True)
    b.space('MissionDescGap', 'MissionContent', 10)
    text(b, 'MissionDescription', 'MissionContent',
         'All squad members share this real lobby world before deployment.', 15, MUTED)
    b.prop('MissionDescription', 'AutoWrapText', 'True')
    b.space('MissionFill', 'MissionContent', 1, True)
    add_panel(b, 'StatusPanel', 'MissionContent', 16, '(R=0.012,G=0.041,B=0.059,A=0.98)')
    b.add('HorizontalBox', 'StatusRow', 'StatusPanel')
    icon(b, 'StatusIcon', 'StatusRow', 3, 28)
    b.space('StatusIconGap', 'StatusRow', 12)
    b.add('VerticalBox', 'StatusTextColumn', 'StatusRow')
    text(b, 'StatusLabel', 'StatusTextColumn', 'SQUAD STATUS', 11, MUTED, True)
    text(b, 'StatusText', 'StatusTextColumn', 'Waiting for squad members', 16, WHITE, True, True)
    b.space('LobbyColumnGap', 'LobbyBody', 20)
    b.add('Border', 'MembersPanel', 'LobbyBody')
    b.prop('MembersPanel', 'Size Rule', 'Fill')
    b.prop('MembersPanel', 'BrushColor', '(R=0.005,G=0.019,B=0.030,A=0.98)')
    b.prop('MembersPanel', 'Padding', '18')
    b.add('VerticalBox', 'MembersContent', 'MembersPanel')
    b.add('HorizontalBox', 'MembersHeader', 'MembersContent')
    text(b, 'MembersTitle', 'MembersHeader', 'SQUAD MEMBERS', 16, WHITE, True)
    b.space('MembersHeaderFill', 'MembersHeader', 1, True)
    text(b, 'PlayerCountText', 'MembersHeader', '0 / 4', 15, CYAN, True, True)
    b.space('MembersGap', 'MembersContent', 12)
    b.add('DynamicEntryBox', 'PlayerEntries', 'MembersContent', True)
    b.prop('PlayerEntries', 'EntryBoxType', 'Vertical')
    b.prop('PlayerEntries', 'EntrySpacing', '(X=0,Y=9)')
    b.prop('PlayerEntries', 'EntryHorizontalAlignment', 'HAlign_Fill')
    b.prop('PlayerEntries', 'NumDesignerPreviewEntries', '4')
    b.prop('PlayerEntries', 'EntryWidgetClass',
           '/Game/UI/Menu/Expedition/W_PlayerSlot.W_PlayerSlot_C')
    b.space('LobbyActionsGap', 'Page', 16)
    b.add('HorizontalBox', 'LobbyActions', 'Page')
    b.add('W_ExpeditionButton', 'InviteButton', 'LobbyActions', True)
    set_button_text(b, 'InviteButton', 'Invite Friends', 'InviteFriends')
    b.add('W_ExpeditionButton', 'LeaveButton', 'LobbyActions', True)
    set_button_text(b, 'LeaveButton', 'Leave Squad', 'LeaveSquad')
    b.space('LobbyActionFill', 'LobbyActions', 1, True)
    b.add('W_ExpeditionButton', 'ReadyButton', 'LobbyActions', True)
    set_button_text(b, 'ReadyButton', 'Ready', 'Ready')
    b.add('W_ExpeditionButton', 'StartButton', 'LobbyActions', True)
    set_button_text(b, 'StartButton', 'Start Expedition', 'StartExpedition')
    b.finish()


def build_transition():
    b = WidgetBuilder('W_ExpeditionTransition', unreal.LyraActivatableWidget)
    b.add('Overlay', 'ScreenLayers')
    add_panel(b, 'ScreenDim', 'ScreenLayers', 0, '(R=0.002,G=0.006,B=0.012,A=0.88)')
    b.add('SafeZone', 'SafeArea', 'ScreenLayers')
    b.add('Border', 'OuterPadding', 'SafeArea')
    b.prop('OuterPadding', 'BrushColor', '(R=0,G=0,B=0,A=0)')
    b.prop('OuterPadding', 'Padding', '(Left=60,Top=60,Right=60,Bottom=60)')
    b.add('VerticalBox', 'Page', 'OuterPadding')
    b.space('TransitionFillTop', 'Page', 1, True)
    b.add('HorizontalBox', 'TransitionRow', 'Page')
    b.space('TransitionRowFill', 'TransitionRow', 1, True)
    add_panel(b, 'TransitionPanel', 'TransitionRow', 28, '(R=0.005,G=0.019,B=0.030,A=0.98)')
    b.add('VerticalBox', 'TransitionContent', 'TransitionPanel')
    text(b, 'TransitionKicker', 'TransitionContent', 'EXPEDITION', 12, CYAN, True)
    b.space('TransitionTitleGap', 'TransitionContent', 8)
    text(b, 'TransitionTitle', 'TransitionContent', 'PREPARING EXPEDITION', 30, WHITE, True)
    b.space('TransitionStatusGap', 'TransitionContent', 12)
    b.add('HorizontalBox', 'TransitionStatusRow', 'TransitionContent')
    icon(b, 'TransitionIcon', 'TransitionStatusRow', 18, 30)
    b.space('TransitionIconGap', 'TransitionStatusRow', 12)
    text(b, 'TransitionStatus', 'TransitionStatusRow',
         'Synchronizing the squad and loading the selected mission…', 15, MUTED, False, True)
    b.prop('TransitionStatus', 'AutoWrapText', 'True')
    for name in ['ScreenDim', 'SafeArea']:
        fill(b, name)
    b.prop('ScreenDim', 'Visibility', 'HitTestInvisible')
    b.finish()


if __name__ == '__main__':
    build_squad_list_item()
    build_player_slot()
    build_find_squad()
    build_squad_lobby()
    build_transition()
