/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_ui_glue.inl"
#include "core/reflection/include/variantTable.h"

#define UI_EVENT StringID eventName, const ElementPtr& callingElement, const void* data, bool& continueSearch
#define UI_CALLBACK const ElementPtr& callingElement, const void* data
#define UI_MODEL_CALLBACK const ElementPtr& callingElement, const ModelIndex& index, const void* data
#define UI_TIMER const ElementPtr& callingElement, StringID name, float elapsedTime

#define DECLARE_UI_EVENT(x, ...) \
    inline static const StringID x = #x##_id;

// events always have the valid IElement as source

namespace boomer::rendering
{
    struct FrameParams;
}

BEGIN_BOOMER_NAMESPACE_EX(ui)

typedef Vector2 Position;
typedef Vector2 Size;
typedef Vector2 VirtualPosition;

class DataStash;
class Renderer;
    
class RenderingPanel;
class ModelIndex;

class ConfigBlock;
class IConfigDataInterface;
class ConfigFileStorageDataInterface;

class EventTable;
class ElementArea;

class IElement;
typedef RefWeakPtr<IElement> ElementWeakPtr;
typedef RefPtr<IElement> ElementPtr;

class Window;
typedef RefPtr<Window> WindowPtr;
typedef RefWeakPtr<Window> WindowWeakPtr;

class PopupWindow;
typedef RefPtr<PopupWindow> PopupPtr;
typedef RefWeakPtr<PopupWindow> PopupWeakPtr;

class ScrollArea;
typedef RefPtr<ScrollArea> ScrollAreaPtr;
typedef RefWeakPtr<ScrollArea> ScrollAreaWeakPtr;

class IInputAction;
typedef RefPtr<IInputAction> InputActionPtr;
typedef RefWeakPtr<IInputAction> InputActionWeakPtr;

struct InputActionResult;

enum class ElementHorizontalLayout : uint8_t;
enum class ElementVerticalLayout : uint8_t;

namespace style
{
    class ContentLoader;
    class SelectorMatch;
    class Library;

    struct ParamTable : public IReferencable
    {
        VariantTable values;
    };
};

typedef RefPtr<style::ParamTable> ParamTablePtr;
typedef RefPtr<style::Library> StyleLibraryPtr;
typedef ResourceRef<style::Library> StyleLibraryRef;
typedef RefWeakPtr<style::Library> StyleLibraryWeakPtr;

class CachedGeometryBuilder;
class CachedGeometry;

class HitCache;

class IDockTarget;

class Button;
typedef RefPtr<Button> ButtonPtr;

class TextLabel;
typedef RefPtr<TextLabel> TextLabelPtr;

class Scrollbar;
typedef RefPtr<Scrollbar> ScrollbarPtr;

class CustomImage;
typedef RefPtr<CustomImage> CustomImagePtr;

// message type
enum class MessageType
{
    Info,
    Warning,
    Error,
    Question,
};

// sizer direction
enum class Direction : uint8_t
{
    Horizontal = 0,
    Vertical = 1,
};

// controls window sizing
enum class WindowSizing
{
    FreeSize, // window's size can be adjusted by the user
    AutoSize, // window size is automatic and cannot be changed
};

// function to create popup window - used whenever a popup is expected
typedef std::function<PopupPtr()> TPopupFunc;

//--

/// validation function for text input
typedef std::function<bool(StringView)> TInputValidationFunction;

//--

// drag and drop
class IDragDropData;
typedef RefPtr<IDragDropData> DragDropDataPtr;

class IDragDropHandler;
typedef RefPtr<IDragDropHandler> DragDropHandlerPtr;

/// drag&drop reaction
enum class DragDropStatus : uint8_t
{
    Ignore, // we do not handle D&D
    Invalid, // we handle D&D but this data is invalid
    Valid, // we handle D&D and this data is valid
};

///--

/// window redraw policy
enum class WindowRedrawPolicy
{
    // redraw all windows and all panels
    // this is the slowest option that is also unbounded (it my take a many many ms to redraw the whole UI)
    Everything,

    // redraw the active window and one other window every frame
    // this is the best for general editor operatior
    RoundRobin,

    // redraw only the active window
    // this is best for flying around, this is the butter smooth option
    ActiveOnly,
};

///--

/// rendering panel redraw policy
enum class PanelRedrawPolicy
{
    // only draw this panel when dirty (or if size changes, obviously)
    // this is best for very static panel that nothing changes on
    OnlyDirty,

    // draw this panel every time the window is rendered if active
    // if the owning window is not active the panel is updated only from time to time
    LowPriority,

    // draw this panel every time window is rendered, regardless if it's active or not
    WithOwner,

    // render this panel every frame, even if the window is not active (this will force the window to render as well)
    // NOTE: this is very expensive mode, should be used only for strictly realtime content
    Realtime,
};

//---

/// event notification token, created when even is registered, should be held as long as the event is needed (destroying it will unregister the event)
class ENGINE_UI_API EventToken : public NoCopy
{
public:
    typedef std::function<void()> TUnregisterFunction;

    EventToken(const TUnregisterFunction& func);
    ~EventToken();

    // unbind without unregistering
    void unbind();

    // unregister the event now, does not require the token to be discarded fully
    void unregisterNow();

private:
    TUnregisterFunction m_unregisterFunc;
};

typedef RefPtr<EventToken> EventTokenPtr;

//---

/// Get default expand state for node
enum class TreeNodeDefaultExpandState : uint8_t
{
    DontCare,
    Expanded,
    Collapsed,
};

//---

class Canvas;
class ScrolableCanvas;
class ICanvasContent;

class DynamicStyleElement;
class IDynamicElementRenderer;

class TreeViewDirect;
class ITreeViewModel;

//---


/// tree columns widths
struct ENGINE_UI_API TreeColumnSizes
{
public:
    static const uint32_t MAX_COLUMNS = 8;

    InplaceArray<float, MAX_COLUMNS> m_widths;
};

//--


class ModelIndex;
class IAbstractItemModel;
class IAbstractItemVisualizer;

/// way items are updates
enum class ItemUpdateModeBit : uint8_t
{
    Item = FLAG(0),
    Parents = FLAG(1),
    Children = FLAG(2),
    Force = FLAG(7), // force update even if not visible

    ItemAndParents = Item | Parents,
    ItemAndChildren = Item | Children,
    Hierarchy = Item | Children | Parents,
};

typedef DirectFlags<ItemUpdateModeBit> ItemUpdateMode;

/// item selection mode
enum class ItemSelectionModeBit : uint8_t
{
    Clear = FLAG(0),
    Select = FLAG(1),
    Deselect = FLAG(2),
    Toggle = FLAG(3),
    UpdateCurrent = FLAG(4),
    FocusCurrent = FLAG(5),

    Default = Clear | Select | UpdateCurrent | FocusCurrent,
    DefaultNoFocus = Clear | Select | UpdateCurrent,
    DefaultToggle = Toggle | UpdateCurrent,
};

typedef DirectFlags<ItemSelectionModeBit> ItemSelectionMode;

//--

struct ENGINE_UI_API SearchPattern
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(SearchPattern);

public:
    StringBuf pattern;
    bool regex = false;
    bool wholeWordsOnly = false;
    bool caseSenitive = false;
    bool hasWildcards = false; // should be set based on content

    INLINE bool empty() const { return pattern.empty(); }

    bool operator==(const SearchPattern& other) const;
    bool operator!=(const SearchPattern& other) const;

    void print(IFormatStream& f) const;

    bool testString(StringView txt) const;
};

//--

class EditBox;
typedef RefPtr<EditBox> EditBoxPtr;

class ToolBar;
typedef RefPtr<ToolBar> ToolBarPtr;

class BreadcrumbBar;
typedef RefPtr<BreadcrumbBar> BreadcrumbBarPtr;

class ManagedFrame;
typedef RefPtr<ManagedFrame> ManagedFramePtr;

class ComboBox;
typedef RefPtr<ComboBox> ComboBoxPtr;

class DynamicChoiceBox;
typedef RefPtr<DynamicChoiceBox> DynamicChoiceBoxPtr;

class DynamicChoiceList;
typedef RefPtr<DynamicChoiceList> DynamicChoiceListPtr;

class CheckBox;
typedef RefPtr<CheckBox> CheckBoxPtr;

class ColumnHeaderBar;
typedef RefPtr<ColumnHeaderBar> ColumnHeaderBarPtr;

class Column;
typedef RefPtr<Column> ColumnPtr;

class Splitter;
typedef RefPtr<Column> SplitterPtr;

class TrackBar;
typedef RefPtr<TrackBar> TrackBarPtr;

class ProgressBar;
typedef RefPtr<ProgressBar> ProgressBarPtr;

class MenuBar;
typedef RefPtr<MenuBar> MenuBarPtr;

class MenuButtonContainer;
typedef RefPtr<MenuButtonContainer> MenuButtonContainerPtr;

class SearchBar;
typedef RefPtr<SearchBar> SearchBarPtr;

class Dragger;
typedef RefPtr<Dragger> DraggerPtr;

class Group;
typedef RefPtr<Group> GroupPtr;

class Notebook;
typedef RefPtr<Notebook> NotebookPtr;

//--

class AbstractItemView;
typedef RefPtr<AbstractItemView> AbstractItemViewPtr;

class ItemView;
typedef RefPtr<ItemView> ItemViewPtr;

class ListView;
typedef RefPtr<ListView> ListViewPtr;

class TreeView;
typedef RefPtr<TreeView> TreeViewPtr;

class SimpleListView;
typedef RefPtr<SimpleListView> SimpleListViewPtr;

class ICollectionItem;
typedef RefPtr<ICollectionItem> CollectionItemPtr;

DECLARE_UI_EVENT(EVENT_ITEM_SELECTION_CHANGED)
DECLARE_UI_EVENT(EVENT_ITEM_ACTIVATED, CollectionItemPtr)
DECLARE_UI_EVENT(EVENT_ITEM_CONTEXT_MENU)

class ICollectionView;
typedef RefPtr<ICollectionView> CollectionViewPtr;
typedef RefWeakPtr<ICollectionView> CollectionViewWeakPtr;

class CollectionItems;

class IListItem;
typedef RefPtr<IListItem> ListItemPtr;

class ListViewEx;
typedef RefPtr<ListViewEx> ListViewExPtr;

class ITreeItem;
typedef RefPtr<ITreeItem> TreeItemPtr;

class TreeViewEx;
typedef RefPtr<TreeViewEx> TreeViewExPtr;

//--

/// how to iterate elements
enum class DockPanelIterationMode
{
    All, // iterate over all panels, even if they are in the hidden state
    VisibleOnly, // iterate over panels that are not hidden even if they are not directly visible (ie. tab is not selected)
    ActiveOnly, // iterate only over panels that are directly visible (tab is visible, splitter is not collapsed to zero, window is not minimized, etc)
};

class DockPanel;
typedef RefPtr<DockPanel> DockPanelPtr;

class DockLayoutNode;
typedef RefPtr<DockLayoutNode> DockLayoutNodePtr;

class DockNotebook;
typedef RefPtr<DockNotebook> DockNotebookPtr;

class DockContainer;
typedef RefPtr<DockContainer> DockContainerPtr;

//--

class ScintillaTextEditor;
typedef RefPtr<ScintillaTextEditor> ScintillaTextEditorPtr;

//--

class RenderingScenePanel;

//--

class DataInspector;
typedef RefPtr<DataInspector> DataInspectorPtr;

class IDataBox;
typedef RefPtr<IDataBox> DataBoxPtr;

class ClassPickerBox;
typedef RefPtr<ClassPickerBox> ClassPickerBoxPtr;

//--

class CanvasArea;
typedef RefPtr<CanvasArea> CanvasAreaPtr;

class ICanvasAreaElement;
typedef RefPtr< ICanvasAreaElement> CanvasAreaElementPtr;

class HorizontalRuler;
typedef RefPtr<HorizontalRuler> HorizontalRulerPtr;

class VerticalRuler;
typedef RefPtr<VerticalRuler> VerticalRulerPtr;

//--

class GraphEditor;
typedef RefPtr<GraphEditor> GraphEditorPtr;
typedef RefWeakPtr<GraphEditor> GraphEditorWeakPtr;

class GraphBlockPalette;
typedef RefPtr<GraphBlockPalette> GraphBlockPalettePtr;

class IGraphEditorNode;
typedef RefPtr<IGraphEditorNode> GraphEditorNodePtr;

class GraphEditorBlockNode;
typedef RefPtr<GraphEditorBlockNode> GraphEditorBlockNodePtr;

class IGraphNodeInnerWidget;
typedef RefPtr<IGraphNodeInnerWidget> GraphNodeInnerWidgetPtr;

//--

class NativeWindowRenderer;

//--

extern ENGINE_UI_API void PostWindowMessage(IElement* owner, MessageType type, StringID group, StringView txt);

extern ENGINE_UI_API void PostNotificationMessage(IElement* owner, MessageType type, StringID group, StringView txt);

//--

END_BOOMER_NAMESPACE_EX(ui)
