import unreal

# 新副本 UI 的设计数据写入 Widget Blueprint；运行时不由 C++ 构造控件树。
# 脚本只增量维护本文件声明的资产和控件，每五个新增控件编译一次。
ROOT = '/Game/UI/Menu/Expedition'
WS = unreal.WidgetService
BS = unreal.BlueprintService
WHITE = '(R=0.82,G=0.93,B=1.0,A=1.0)'
MUTED = '(R=0.34,G=0.52,B=0.64,A=1.0)'
CYAN = '(R=0.08,G=0.69,B=0.95,A=1.0)'
PANEL = '(R=0.008,G=0.019,B=0.03,A=0.98)'


class WidgetBuilder:
    def __init__(self, name, parent):
        self.path = ROOT + '/' + name
        self.asset = unreal.load_asset(self.path) if unreal.EditorAssetLibrary.does_asset_exist(self.path) else None
        if not self.asset:
            factory = unreal.WidgetBlueprintFactory()
            factory.set_editor_property('parent_class', parent)
            self.asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.WidgetBlueprint, factory)
            assert self.asset
            print('CREATED', self.path)
        self.add_count = 0

    def add(self, kind, name, parent='', variable=False):
        if not WS.widget_exists(self.path, name):
            result = WS.add_component(self.path, kind, name, parent, variable)
            assert result.success, (name, result.error_message)
            self.add_count += 1
            print('ADDED', self.path, name)
            if self.add_count % 5 == 0:
                unreal.BlueprintEditorLibrary.compile_blueprint(self.asset)
        return name

    def prop(self, name, key, value):
        assert WS.set_property(self.path, name, key, str(value)), (self.path, name, key, value)

    def text(self, name, parent, label, size=18, color=WHITE, bold=False, variable=False):
        self.add('TextBlock', name, parent, variable)
        self.prop(name, 'Text', 'NSLOCTEXT("ExpeditionUI", "' + name + '", "' + label + '")')
        font = unreal.WidgetFontInfo()
        font.font_family = '/Engine/EngineFonts/Roboto.Roboto'
        font.typeface = 'Bold' if bold else 'Regular'
        font.size = size
        font.color = color
        assert WS.set_font(self.path, name, font)
        self.prop(name, 'ColorAndOpacity', '(SpecifiedColor=' + color + ')')
        return name

    def space(self, name, parent, height=16, fill=False):
        self.add('Spacer', name, parent)
        self.prop(name, 'Size', '(X=16,Y=' + str(height) + ')')
        if fill:
            self.prop(name, 'Size Rule', 'Fill')

    def box(self, name, parent, color=PANEL, padding=20):
        self.add('Border', name, parent)
        self.prop(name, 'BrushColor', color)
        self.prop(name, 'Padding', '(Left={0},Top={0},Right={0},Bottom={0})'.format(padding))
        return name

    def finish(self):
        unreal.BlueprintEditorLibrary.compile_blueprint(self.asset)
        assert unreal.EditorAssetLibrary.save_asset(self.path, only_if_is_dirty=False)
        print('SAVED', self.path, 'widgets', len(WS.get_hierarchy(self.path)))


def build_button():
    b = WidgetBuilder('W_ExpeditionButton', unreal.LyraButtonBase)
    # 通用按钮由内部内容决定 Desired Size；页面若需要主操作按钮的大尺寸，应在页面 Slot 上配置。
    # 已创建资产的根 SizeBox 不能被服务直接移除；关闭所有 Override 后，它只透传子内容的 Desired Size。
    b.add('SizeBox', 'ButtonSize')
    size_widget = unreal.load_object(None, b.path + '.' + b.path.rsplit('/', 1)[-1] + ':WidgetTree.ButtonSize')
    size_widget.clear_min_desired_height()
    b.add('Overlay', 'ButtonLayers', 'ButtonSize')
    b.box('ButtonFill', 'ButtonLayers', '(R=0.015,G=0.047,B=0.068,A=1.0)', 0)
    b.add('Image', 'ButtonFrame', 'ButtonLayers', True)
    brush = unreal.WidgetBrushInfo()
    brush.resource_path = '/Game/UI/Hud/Art/MI_UI_QuickBar_Border_Square.MI_UI_QuickBar_Border_Square'
    brush.draw_as = 'Box'
    brush.margin = '(Left=0.5,Top=0.5,Right=0.5,Bottom=0.5)'
    brush.tint_color = '(R=0.16,G=0.4,B=0.53,A=1.0)'
    assert WS.set_brush(b.path, 'ButtonFrame', 'Brush', brush)
    b.add('HorizontalBox', 'ButtonContent', 'ButtonLayers')
    for name in ['ButtonFill', 'ButtonFrame', 'ButtonContent']:
        b.prop(name, 'Slot.HorizontalAlignment', 'Fill')
        b.prop(name, 'Slot.VerticalAlignment', 'Fill')
    b.prop('ButtonContent', 'Padding', '(Left=22,Top=12,Right=22,Bottom=12)')
    b.text('Label', 'ButtonContent', 'Continue', 18, WHITE, True, True)
    b.prop('Label', 'Size Rule', 'Fill')
    b.prop('Label', 'Vertical Alignment', 'Center')
    if WS.widget_exists(b.path, 'Arrow'):
        result = WS.remove_component(b.path, 'Arrow', True)
        assert result.success, result.error_message
    b.add('SizeBox', 'ArrowSize', 'ButtonContent')
    b.prop('ArrowSize', 'WidthOverride', '24')
    b.prop('ArrowSize', 'HeightOverride', '24')
    b.prop('ArrowSize', 'Vertical Alignment', 'Center')
    b.add('Image', 'ArrowIcon', 'ArrowSize')
    arrow_brush = unreal.WidgetBrushInfo()
    arrow_brush.resource_path = '/Game/UI/Textures/T_ArrowRight.T_ArrowRight'
    arrow_brush.draw_as = 'Image'
    arrow_brush.tint_color = CYAN
    assert WS.set_brush(b.path, 'ArrowIcon', 'Brush', arrow_brush)
    b.prop('ButtonFill', 'Visibility', 'HitTestInvisible')
    b.prop('ButtonFrame', 'Visibility', 'HitTestInvisible')
    b.prop('ButtonContent', 'Visibility', 'HitTestInvisible')
    unreal.BlueprintEditorLibrary.compile_blueprint(b.asset)
    cdo = unreal.get_default_object(unreal.load_class(None, b.path + '.W_ExpeditionButton_C'))
    cdo.set_editor_property('style', unreal.load_class(None, '/Game/UI/Foundation/Buttons/ButtonStyle-Clear.ButtonStyle-Clear_C'))
    cdo.set_editor_property('button_text', unreal.Text('Continue'))
    # UpdateButtonText 是 ULyraButtonBase 的表现事件，文字控件仅在本按钮蓝图内部使用。
    if not any(n.node_title.startswith('Event UpdateButtonText') for n in BS.get_nodes_in_graph(b.path, 'EventGraph', 0, '', False)):
        result = BS.build_graph(b.path, 'EventGraph', [
            {'ref': 'TextChanged', 'type': 'event', 'params': {'event': 'UpdateButtonText'}},
            {'ref': 'GetLabel', 'type': 'variable_get', 'params': {'variable': 'Label'}},
            {'ref': 'SetLabel', 'type': 'function_call', 'params': {'class': 'TextBlock', 'function': 'SetText'}},
        ], [
            {'from_': 'TextChanged.then', 'to': 'SetLabel.execute'},
            {'from_': 'TextChanged.InText', 'to': 'SetLabel.InText'},
            {'from_': 'GetLabel.Label', 'to': 'SetLabel.self'},
        ], [], False, True)
        assert result.success, (list(result.errors), list(result.warnings))
    b.finish()
    return b


def graph(b, nodes, connections, defaults=(), band=0):
    result = BS.build_graph(b.path, 'EventGraph', nodes, [
        {'from_': a, 'to': z} for a, z in connections], list(defaults), False, True)
    assert result.success, (list(result.errors), list(result.warnings))
    for i, spec in enumerate(nodes):
        BS.set_node_position(b.path, 'EventGraph', result.ref_to_node_id[spec['ref']],
                             (i % 4) * 380, band + (i // 4) * 260)
    return result


def node(ref, kind, **params):
    return {'ref': ref, 'type': kind, 'params': params}


def add_button_feedback(b):
    if any(n.node_title.startswith('Event On Hovered') for n in BS.get_nodes_in_graph(b.path,'EventGraph',0,'',False)):
        return
    b.add('Image', 'ActiveFrame', 'ButtonLayers', True)
    b.prop('ActiveFrame', 'Slot.HorizontalAlignment', 'Fill')
    b.prop('ActiveFrame', 'Slot.VerticalAlignment', 'Fill')
    b.prop('ActiveFrame', 'Visibility', 'HitTestInvisible')
    b.prop('ActiveFrame', 'RenderOpacity', '0')
    brush = WS.get_brush(b.path,'ButtonFrame','Brush')
    brush.tint_color = CYAN
    assert WS.set_brush(b.path,'ActiveFrame','Brush',brush)
    unreal.BlueprintEditorLibrary.compile_blueprint(b.asset)
    nodes = [node('Frame','variable_get',variable='ActiveFrame'),
             node('Hover','function_call',class_='CommonButtonBase',function='IsHovered')]
    nodes[1]['params']['class']=nodes[1]['params'].pop('class_')
    for ref, cls, fn in [('Selected','CommonButtonBase','GetSelected'),('Focused','Widget','HasAnyUserFocus'),
                          ('Either','KismetMathLibrary','BooleanOR'),('Active','KismetMathLibrary','BooleanOR'),
                          ('Opacity','KismetMathLibrary','SelectFloat'),('Apply','Widget','SetRenderOpacity')]:
        nodes.append(node(ref,'function_call',**{'class':cls,'function':fn}))
    edges=[('Hover.ReturnValue','Either.A'),('Selected.ReturnValue','Either.B'),
           ('Either.ReturnValue','Active.A'),('Focused.ReturnValue','Active.B'),
           ('Active.ReturnValue','Opacity.bPickA'),('Opacity.ReturnValue','Apply.InOpacity'),
           ('Frame.ActiveFrame','Apply.self')]
    for i,event in enumerate(['BP_OnHovered','BP_OnUnhovered','BP_OnSelected','BP_OnDeselected','BP_OnFocusReceived','BP_OnFocusLost']):
        ref='Event'+str(i)
        nodes.append(node(ref,'event',event=event))
        edges.append((ref+'.then','Apply.execute'))
    graph(b,nodes,edges,[{'node_ref':'Opacity','pin_name':'A','value':'1'},
                         {'node_ref':'Opacity','pin_name':'B','value':'0'}],600)
    BS.add_comment_node(b.path,'EventGraph','焦点、悬停与选中共用高亮；样式均在本按钮蓝图维护。',-80,500,1500,120)
    b.finish()


def build_list_item():
    b=WidgetBuilder('W_ExpeditionListItem',unreal.ShootObjectEntryButtonBase)
    desired_parent = unreal.ShootObjectEntryButtonBase
    generated = unreal.load_class(None, b.path + '.W_ExpeditionListItem_C')
    if generated and not unreal.MathLibrary.class_is_child_of(generated, desired_parent):
        unreal.BlueprintEditorLibrary.reparent_blueprint(b.asset, desired_parent)
    b.add('SizeBox','ButtonSize')
    b.prop('ButtonSize','MinDesiredHeight','136')
    b.add('Overlay','ButtonLayers','ButtonSize')
    b.box('ButtonFill','ButtonLayers','(R=0.012,G=0.035,B=0.052,A=1.0)',0)
    b.add('Image','ButtonFrame','ButtonLayers',True)
    brush=WS.get_brush(ROOT+'/W_ExpeditionButton','ButtonFrame','Brush')
    assert WS.set_brush(b.path,'ButtonFrame','Brush',brush)
    b.add('HorizontalBox','ItemContent','ButtonLayers')
    b.prop('ItemContent','Padding','16')
    b.add('SizeBox','ThumbnailSize','ItemContent')
    b.prop('ThumbnailSize','WidthOverride','100')
    b.prop('ThumbnailSize','HeightOverride','96')
    b.add('Image','Thumbnail','ThumbnailSize',True)
    b.prop('Thumbnail','ColorAndOpacity','(R=0.12,G=0.24,B=0.32,A=1)')
    b.space('ThumbGap','ItemContent',1)
    b.add('VerticalBox','Labels','ItemContent')
    b.prop('Labels','Size Rule','Fill')
    b.prop('Labels','Vertical Alignment','Center')
    b.text('MissionTitle','Labels','Expedition',21,WHITE,True,True)
    b.space('LabelGap','Labels',8)
    b.text('MissionSubtitle','Labels','Select a destination',15,MUTED,False,True)
    b.prop('MissionSubtitle','AutoWrapText','True')
    for name in ['ButtonFill','ButtonFrame','ItemContent']:
        b.prop(name,'Slot.HorizontalAlignment','Fill')
        b.prop(name,'Slot.VerticalAlignment','Fill')
        b.prop(name,'Visibility','HitTestInvisible')
    unreal.BlueprintEditorLibrary.compile_blueprint(b.asset)
    cdo=unreal.get_default_object(unreal.load_class(None,b.path+'.W_ExpeditionListItem_C'))
    cdo.set_editor_property('style',unreal.load_class(None,'/Game/UI/Foundation/Buttons/ButtonStyle-Clear.ButtonStyle-Clear_C'))
    cdo.set_editor_property('selectable',True)
    cdo.set_editor_property('interactable_when_selected',True)
    if not any(('On List Item Object Set' in n.node_title or 'On Entry Object Set' in n.node_title)
               for n in BS.get_nodes_in_graph(b.path,'EventGraph',0,'',False)):
        nodes=[node('Item','event',event='OnListItemObjectSet'),
               node('Definition','cast',target_class='LyraUserFacingExperienceDefinition')]
        edges=[('Item.then','Definition.execute'),('Item.ListItemObject','Definition.Object')]
        previous='Definition.then'
        for ref,member,widget in [('Title','TileTitle','MissionTitle'),('Sub','TileSubTitle','MissionSubtitle')]:
            nodes.extend([node(ref+'Data','member_get',member=member,**{'class':'LyraUserFacingExperienceDefinition'}),
                          node(ref+'Widget','variable_get',variable=widget),
                          node(ref+'Set','function_call',**{'class':'TextBlock','function':'SetText'})])
            edges.extend([('Definition.AsLyra User Facing Experience Definition',ref+'Data.self'),
                          (ref+'Data.'+member,ref+'Set.InText'),(ref+'Widget.'+widget,ref+'Set.self'),
                          (previous,ref+'Set.execute')])
            previous=ref+'Set.then'
        graph(b,nodes,edges,band=0)
    add_button_feedback(b)
    b.finish()
    return b


if __name__ == '__main__':
    add_button_feedback(build_button())
    build_list_item()
