import unreal

# 正式副本选择页的视觉结构全部保存在 Widget Blueprint；本脚本只用于可重复地创建和校验资产。
ROOT = '/Game/UI/Menu/Expedition'
SOURCE = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_dir() + 'Scripts/SessionUI_BuildExpeditionWidgets.py')
_shared = {'__name__': 'session_ui_widget_helpers'}
with open(SOURCE, 'r', encoding='utf-8') as _handle:
    exec(compile(_handle.read(), SOURCE, 'exec'), _shared)

WidgetBuilder = _shared['WidgetBuilder']
graph = _shared['graph']
node = _shared['node']
add_button_feedback = _shared['add_button_feedback']
WS = unreal.WidgetService
BS = unreal.BlueprintService
WHITE = _shared['WHITE']
MUTED = _shared['MUTED']
CYAN = _shared['CYAN']
PANEL = _shared['PANEL']
COMMON_USER_WIDGET = unreal.load_class(None, '/Script/CommonUI.CommonUserWidget')


def set_slot_fill(builder, widget_name):
    builder.prop(widget_name, 'Slot.HorizontalAlignment', 'Fill')
    builder.prop(widget_name, 'Slot.VerticalAlignment', 'Fill')


def set_button_text(builder, widget_name, text, key):
    builder.prop(widget_name, 'bOverride_ButtonText', 'True')
    builder.prop(widget_name, 'ButtonText', 'NSLOCTEXT("ExpeditionUI", "' + key + '", "' + text + '")')


def set_clear_style(path):
    unreal.BlueprintEditorLibrary.compile_blueprint(unreal.load_asset(path))
    generated = unreal.load_class(None, path + '.' + path.rsplit('/', 1)[-1] + '_C')
    cdo = unreal.get_default_object(generated)
    cdo.set_editor_property(
        'style',
        unreal.load_class(None, '/Game/UI/Foundation/Buttons/ButtonStyle-Clear.ButtonStyle-Clear_C'))


def bordered_panel(builder, name, parent, padding=24, color=PANEL):
    builder.add('Border', name, parent)
    builder.prop(name, 'BrushColor', color)
    builder.prop(name, 'Padding', '(Left={0},Top={0},Right={0},Bottom={0})'.format(padding))
    return name


def build_mode_tab():
    b = WidgetBuilder('W_ExpeditionModeTab', unreal.LyraButtonBase)
    b.add('Overlay', 'ButtonLayers')
    bordered_panel(b, 'ButtonFill', 'ButtonLayers', 0, '(R=0.012,G=0.035,B=0.052,A=0.98)')
    b.add('Image', 'ButtonFrame', 'ButtonLayers', True)
    frame = WS.get_brush(ROOT + '/W_ExpeditionButton', 'ButtonFrame', 'Brush')
    assert WS.set_brush(b.path, 'ButtonFrame', 'Brush', frame)
    b.add('Border', 'ContentPadding', 'ButtonLayers')
    b.prop('ContentPadding', 'BrushColor', '(R=0,G=0,B=0,A=0)')
    b.prop('ContentPadding', 'Padding', '(Left=26,Top=16,Right=26,Bottom=16)')
    b.text('Label', 'ContentPadding', 'Single Player', 17, WHITE, True, True)
    b.prop('Label', 'Justification', 'Center')
    for name in ['ButtonFill', 'ButtonFrame', 'ContentPadding']:
        set_slot_fill(b, name)
        b.prop(name, 'Visibility', 'HitTestInvisible')
    set_clear_style(b.path)
    generated = unreal.load_class(None, b.path + '.W_ExpeditionModeTab_C')
    cdo = unreal.get_default_object(generated)
    cdo.set_editor_property('selectable', True)
    cdo.set_editor_property('interactable_when_selected', True)
    cdo.set_editor_property('button_text', unreal.Text('Single Player'))
    if not any(n.node_title.startswith('Event UpdateButtonText')
               for n in BS.get_nodes_in_graph(b.path, 'EventGraph', 0, '', False)):
        result = BS.build_graph(b.path, 'EventGraph', [
            {'ref': 'TextChanged', 'type': 'event', 'params': {'event': 'UpdateButtonText'}},
            {'ref': 'GetLabel', 'type': 'variable_get', 'params': {'variable': 'Label'}},
            {'ref': 'SetLabel', 'type': 'function_call',
             'params': {'class': 'TextBlock', 'function': 'SetText'}},
        ], [
            {'from_': 'TextChanged.then', 'to': 'SetLabel.execute'},
            {'from_': 'TextChanged.InText', 'to': 'SetLabel.InText'},
            {'from_': 'GetLabel.Label', 'to': 'SetLabel.self'},
        ], [], False, True)
        assert result.success, (list(result.errors), list(result.warnings))
    add_button_feedback(b)
    b.finish()


def add_role_card(builder, prefix, parent, title, subtitle):
    builder.add('Border', prefix + 'Card', parent)
    builder.prop(prefix + 'Card', 'BrushColor', '(R=0.012,G=0.044,B=0.064,A=0.98)')
    builder.prop(prefix + 'Card', 'Padding', '22')
    builder.prop(prefix + 'Card', 'Size Rule', 'Fill')
    builder.add('VerticalBox', prefix + 'Content', prefix + 'Card')
    builder.text(prefix + 'Kicker', prefix + 'Content', subtitle, 13, MUTED, True)
    builder.space(prefix + 'GapA', prefix + 'Content', 8)
    builder.text(prefix + 'Title', prefix + 'Content', title, 24, WHITE, True)
    builder.space(prefix + 'GapB', prefix + 'Content', 16)
    builder.add('SizeBox', prefix + 'PortraitSize', prefix + 'Content')
    builder.prop(prefix + 'PortraitSize', 'HeightOverride', '150')
    builder.add('Overlay', prefix + 'PortraitLayers', prefix + 'PortraitSize')
    bordered_panel(builder, prefix + 'PortraitFill', prefix + 'PortraitLayers', 0,
                   '(R=0.02,G=0.075,B=0.10,A=1)')
    builder.add('Image', prefix + 'PortraitFrame', prefix + 'PortraitLayers')
    frame = WS.get_brush(ROOT + '/W_ExpeditionButton', 'ButtonFrame', 'Brush')
    assert WS.set_brush(builder.path, prefix + 'PortraitFrame', 'Brush', frame)
    set_slot_fill(builder, prefix + 'PortraitFill')
    set_slot_fill(builder, prefix + 'PortraitFrame')
    builder.space(prefix + 'GapC', prefix + 'Content', 16)
    builder.text(prefix + 'Device', prefix + 'Content', 'Waiting for input', 15, MUTED, False, True)
    builder.text(prefix + 'State', prefix + 'Content', 'Choose a character', 14, CYAN, True, True)


def build_local_setup():
    b = WidgetBuilder('W_LocalCoopSetup', COMMON_USER_WIDGET)
    bordered_panel(b, 'Panel', '', 22, '(R=0.008,G=0.025,B=0.039,A=0.98)')
    b.add('VerticalBox', 'Content', 'Panel')
    b.add('HorizontalBox', 'Header', 'Content')
    b.text('Title', 'Header', 'DEVICE ASSIGNMENT', 18, WHITE, True)
    b.space('HeaderFill', 'Header', 1, True)
    b.text('Hint', 'Header', 'Move left or right, then confirm', 14, MUTED)
    b.space('HeaderGap', 'Content', 16)
    b.add('HorizontalBox', 'RoleCards', 'Content')
    add_role_card(b, 'Male', 'RoleCards', 'Male Protagonist', 'LEFT CHARACTER')
    b.space('RoleGap', 'RoleCards', 18)
    add_role_card(b, 'Female', 'RoleCards', 'Female Protagonist', 'RIGHT CHARACTER')
    b.space('FooterGap', 'Content', 14)
    b.add('HorizontalBox', 'Footer', 'Content')
    b.text('DeviceOne', 'Footer', 'Keyboard & Mouse  •  Not confirmed', 14, WHITE)
    b.space('FooterFill', 'Footer', 1, True)
    b.text('DeviceTwo', 'Footer', 'Waiting for second device', 14, MUTED)
    b.finish()


def add_info_stat(builder, prefix, parent, label, value):
    builder.add('Border', prefix + 'Card', parent)
    builder.prop(prefix + 'Card', 'BrushColor', '(R=0.013,G=0.045,B=0.066,A=0.98)')
    builder.prop(prefix + 'Card', 'Padding', '(Left=20,Top=14,Right=20,Bottom=14)')
    builder.prop(prefix + 'Card', 'Size Rule', 'Fill')
    builder.add('VerticalBox', prefix + 'Content', prefix + 'Card')
    builder.text(prefix + 'Label', prefix + 'Content', label, 13, MUTED, True)
    builder.space(prefix + 'Gap', prefix + 'Content', 5)
    builder.text(prefix + 'Value', prefix + 'Content', value, 17, WHITE, True, True)


def add_page_button(builder, name, parent, text, key, fill=False):
    builder.add('W_ExpeditionButton', name, parent, True)
    set_button_text(builder, name, text, key)
    if fill:
        builder.prop(name, 'Size Rule', 'Fill')
    return name


def build_selection_page():
    b = WidgetBuilder('W_ExpeditionSelection', unreal.ShootHostSessionScreen)
    b.add('Overlay', 'ScreenLayers')
    bordered_panel(b, 'ScreenDim', 'ScreenLayers', 0, '(R=0.002,G=0.006,B=0.012,A=0.84)')
    b.add('SafeZone', 'SafeArea', 'ScreenLayers')
    b.add('Border', 'OuterPadding', 'SafeArea')
    b.prop('OuterPadding', 'BrushColor', '(R=0,G=0,B=0,A=0)')
    b.prop('OuterPadding', 'Padding', '(Left=46,Top=34,Right=46,Bottom=28)')
    b.add('VerticalBox', 'Page', 'OuterPadding')
    b.add('HorizontalBox', 'Header', 'Page')
    b.add('VerticalBox', 'HeaderText', 'Header')
    b.text('Brand', 'HeaderText', 'NEW WORLD ORDER', 14, MUTED, True)
    b.space('BrandGap', 'HeaderText', 7)
    b.text('PageTitle', 'HeaderText', 'EXPEDITION', 42, WHITE, True)
    b.text('PageSubtitle', 'HeaderText', 'Choose a destination and deployment mode.', 16, MUTED)
    b.space('HeaderFill', 'Header', 1, True)
    add_page_button(b, 'BackButton', 'Header', 'Back', 'Back')
    b.prop('BackButton', 'Vertical Alignment', 'Top')
    b.space('HeaderBodyGap', 'Page', 24)
    b.add('HorizontalBox', 'Body', 'Page')
    b.prop('Body', 'Size Rule', 'Fill')

    b.add('SizeBox', 'LeftRailSize', 'Body')
    b.prop('LeftRailSize', 'WidthOverride', '470')
    bordered_panel(b, 'LeftPanel', 'LeftRailSize', 18, '(R=0.006,G=0.022,B=0.034,A=0.98)')
    b.add('VerticalBox', 'LeftContent', 'LeftPanel')
    b.text('ListKicker', 'LeftContent', 'AVAILABLE OPERATIONS', 13, CYAN, True)
    b.space('ListTitleGap', 'LeftContent', 7)
    b.text('ListTitle', 'LeftContent', 'Select Expedition', 25, WHITE, True)
    b.text('ListSubtitle', 'LeftContent', 'Each destination uses its configured experience.', 14, MUTED)
    b.space('ListGap', 'LeftContent', 16)
    b.add('ScrollBox', 'ExpeditionScroll', 'LeftContent')
    b.prop('ExpeditionScroll', 'Size Rule', 'Fill')
    b.add('DynamicEntryBox', 'ExpeditionEntries', 'ExpeditionScroll', True)
    b.prop('ExpeditionEntries', 'EntryBoxType', 'Vertical')
    b.prop('ExpeditionEntries', 'EntrySpacing', '(X=0,Y=12)')
    b.prop('ExpeditionEntries', 'EntryHorizontalAlignment', 'HAlign_Fill')
    b.prop('ExpeditionEntries', 'NumDesignerPreviewEntries', '3')
    b.prop('ExpeditionEntries', 'EntryWidgetClass',
           '/Game/UI/Menu/Expedition/W_ExpeditionListItem.W_ExpeditionListItem_C')

    b.space('ColumnGap', 'Body', 24)
    b.add('VerticalBox', 'RightContent', 'Body')
    b.prop('RightContent', 'Size Rule', 'Fill')
    b.add('HorizontalBox', 'ModeTabs', 'RightContent')
    for name, label, key in [
            ('SingleTab', 'Single Player', 'SinglePlayer'),
            ('LocalTab', 'Local Co-op', 'LocalCoop'),
            ('OnlineTab', 'Online Co-op', 'OnlineCoop')]:
        b.add('W_ExpeditionModeTab', name, 'ModeTabs', True)
        set_button_text(b, name, label, key)
        b.prop(name, 'Size Rule', 'Fill')
    b.space('ModeDetailGap', 'RightContent', 16)

    bordered_panel(b, 'MissionPanel', 'RightContent', 20, '(R=0.006,G=0.022,B=0.034,A=0.985)')
    b.prop('MissionPanel', 'Size Rule', 'Fill')
    b.add('VerticalBox', 'MissionContent', 'MissionPanel')
    b.add('SizeBox', 'HeroSize', 'MissionContent')
    b.prop('HeroSize', 'HeightOverride', '278')
    b.add('Overlay', 'HeroLayers', 'HeroSize')
    b.add('Image', 'HeroImage', 'HeroLayers', True)
    b.prop('HeroImage', 'ColorAndOpacity', '(R=0.08,G=0.20,B=0.27,A=1)')
    b.add('Image', 'HeroFrame', 'HeroLayers')
    frame = WS.get_brush(ROOT + '/W_ExpeditionButton', 'ButtonFrame', 'Brush')
    assert WS.set_brush(b.path, 'HeroFrame', 'Brush', frame)
    set_slot_fill(b, 'HeroImage')
    set_slot_fill(b, 'HeroFrame')
    b.space('HeroTextGap', 'MissionContent', 18)
    b.text('MissionTitle', 'MissionContent', 'Select an expedition', 34, WHITE, True, True)
    b.space('MissionDescGap', 'MissionContent', 7)
    b.text('MissionDescription', 'MissionContent', 'Choose an operation from the list to review its deployment details.', 17, MUTED, False, True)
    b.prop('MissionDescription', 'AutoWrapText', 'True')
    b.space('StatsGap', 'MissionContent', 18)
    b.add('HorizontalBox', 'Stats', 'MissionContent')
    add_info_stat(b, 'Players', 'Stats', 'SQUAD SIZE', 'Up to 1')
    b.space('StatsSpacer', 'Stats', 12)
    add_info_stat(b, 'Access', 'Stats', 'AVAILABLE MODES', 'Local')
    b.space('ActionsGap', 'MissionContent', 18)
    b.add('WidgetSwitcher', 'ModeSwitcher', 'MissionContent', True)

    b.add('HorizontalBox', 'SingleActions', 'ModeSwitcher')
    b.text('SingleHint', 'SingleActions', 'Deploy with your current protagonist.', 14, MUTED)
    b.space('SingleFill', 'SingleActions', 1, True)
    add_page_button(b, 'StartButton', 'SingleActions', 'Start Expedition', 'StartExpedition')

    b.add('W_LocalCoopSetup', 'LocalSetup', 'ModeSwitcher', True)

    b.add('HorizontalBox', 'OnlineActions', 'ModeSwitcher')
    add_page_button(b, 'CreateSquadButton', 'OnlineActions', 'Create Squad', 'CreateSquad', True)
    b.space('OnlineGap', 'OnlineActions', 14)
    add_page_button(b, 'FindSquadsButton', 'OnlineActions', 'Find Squads', 'FindSquads', True)

    for name in ['ScreenDim', 'SafeArea']:
        set_slot_fill(b, name)
    b.prop('ScreenDim', 'Visibility', 'HitTestInvisible')
    unreal.BlueprintEditorLibrary.compile_blueprint(b.asset)
    b.finish()


if __name__ == '__main__':
    build_mode_tab()
    build_local_setup()
    build_selection_page()
