/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_ui_glue.inl"
#include "base/reflection/include/variantTable.h"

#define UI_EVENT base::StringID eventName, const ui::ElementPtr& callingElement, const void* data, bool& continueSearch
#define UI_CALLBACK const ui::ElementPtr& callingElement, const void* data
#define UI_MODEL_CALLBACK const ui::ElementPtr& callingElement, const ui::ModelIndex& index, const void* data
#define UI_TIMER const ui::ElementPtr& callingElement, base::StringID name, float elapsedTime

#define DECLARE_UI_EVENT(x, ...) \
    inline static const base::StringID x = #x##_id;

// events always have the valid ui::IElement as source

namespace ui
{
    typedef base::Vector2 Position;
    typedef base::Vector2 Size;
    typedef base::Vector2 VirtualPosition;

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
    typedef base::RefWeakPtr<IElement> ElementWeakPtr;
    typedef base::RefPtr<IElement> ElementPtr;

    class Window;
    typedef base::RefPtr<Window> WindowPtr;
    typedef base::RefWeakPtr<Window> WindowWeakPtr;

    class PopupWindow;
    typedef base::RefPtr<PopupWindow> PopupPtr;
    typedef base::RefWeakPtr<PopupWindow> PopupWeakPtr;

    class ScrollArea;
    typedef base::RefPtr<ScrollArea> ScrollAreaPtr;
    typedef base::RefWeakPtr<ScrollArea> ScrollAreaWeakPtr;

    class IInputAction;
    typedef base::RefPtr<IInputAction> InputActionPtr;
    struct InputActionResult;

    enum class ElementHorizontalLayout : uint8_t;
    enum class ElementVerticalLayout : uint8_t;

    namespace style
    {
        class SelectorMatch;
        class Library;
        class IStyleLibraryContentLoader;

        struct ParamTable : public base::IReferencable
        {
            base::VariantTable values;
        };
    };

    typedef base::RefPtr<style::ParamTable> ParamTablePtr;
    typedef base::RefPtr<style::Library> StyleLibraryPtr;
    typedef base::res::Ref<style::Library> StyleLibraryRef;
    typedef base::RefWeakPtr<style::Library> StyleLibraryWeakPtr;

    class CachedGeometryBuilder;
    class CachedGeometry;

    class HitCache;

    class IDockTarget;

    class Button;
    typedef base::RefPtr<Button> ButtonPtr;

    class TextLabel;
    typedef base::RefPtr<TextLabel> TextLabelPtr;

    class Scrollbar;
    typedef base::RefPtr<Scrollbar> ScrollbarPtr;

    class Image;
    typedef base::RefPtr<Image> ImagePtr;

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

    // basic ui event
    typedef std::function<void(UI_EVENT)> TEvent;

    // basic ui callback (direct event)
    typedef std::function<void(UI_CALLBACK)> TCallback;

    // basic UI timer function
    typedef std::function<void(UI_TIMER)> TTimer;

    // basic UI callback for stuff with a model
    typedef std::function<void(UI_MODEL_CALLBACK)> TModelCallback;

    // basic UI callback for stuff with a model
    typedef std::function<bool(UI_MODEL_CALLBACK)> TModelCallbackRet;

    // function to create popup window - used whenever a popup is expected
    typedef std::function<PopupPtr()> TPopupFunc;

    //--

    /// validation function for text input
    typedef std::function<bool(base::StringView<char>)> TInputValidationFunction;

    //--

    // drag and drop
    class IDragDropData;
    typedef base::RefPtr<IDragDropData> DragDropDataPtr;

    class IDragDropHandler;
    typedef base::RefPtr<IDragDropHandler> DragDropHandlerPtr;

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
    class BASE_UI_API EventToken : public base::NoCopy
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

    typedef base::RefPtr<EventToken> EventTokenPtr;

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
    struct BASE_UI_API TreeColumnSizes
    {
    public:
        static const uint32_t MAX_COLUMNS = 8;

        base::InplaceArray<float, MAX_COLUMNS> m_widths;
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

    typedef base::DirectFlags<ItemUpdateModeBit> ItemUpdateMode;

    /// item selection mode
    enum class ItemSelectionModeBit : uint8_t
    {
        Clear = FLAG(0),
        Select = FLAG(1),
        Deselect = FLAG(2),
        Toggle = FLAG(3),
        UpdateCurrent = FLAG(4),

        Default = Clear | Select | UpdateCurrent,
        DefaultToggle = Toggle | UpdateCurrent,
    };

    typedef base::DirectFlags<ItemSelectionModeBit> ItemSelectionMode;

    //--

    struct BASE_UI_API SearchPattern
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(SearchPattern);

    public:
        base::StringBuf pattern;
        bool regex = false;
        bool wholeWordsOnly = false;
        bool caseSenitive = false;
        bool hasWildcards = false; // should be set based on content

        INLINE bool empty() const { return pattern.empty(); }

        bool operator==(const SearchPattern& other) const;
        bool operator!=(const SearchPattern& other) const;

        void print(base::IFormatStream& f) const;

        bool testString(base::StringView<char> txt) const;
    };

    //--

    class EditBox;
    typedef base::RefPtr<EditBox> EditBoxPtr;

    class ToolBar;
    typedef base::RefPtr<ToolBar> ToolBarPtr;

    class BreadcrumbBar;
    typedef base::RefPtr<BreadcrumbBar> BreadcrumbBarPtr;

    class ManagedFrame;
    typedef base::RefPtr<ManagedFrame> ManagedFramePtr;

    class ComboBox;
    typedef base::RefPtr<ComboBox> ComboBoxPtr;

    class CheckBox;
    typedef base::RefPtr<CheckBox> CheckBoxPtr;

    class ColumnHeaderBar;
    typedef base::RefPtr<ColumnHeaderBar> ColumnHeaderBarPtr;

    class Column;
    typedef base::RefPtr<Column> ColumnPtr;

    class Splitter;
    typedef base::RefPtr<Column> SplitterPtr;

    class TrackBar;
    typedef base::RefPtr<TrackBar> TrackBarPtr;

    class ProgressBar;
    typedef base::RefPtr<ProgressBar> ProgressBarPtr;

    class MenuBar;
    typedef base::RefPtr<MenuBar> MenuBarPtr;

    class MenuButtonContainer;
    typedef base::RefPtr<MenuButtonContainer> MenuButtonContainerPtr;

    class SearchBar;
    typedef base::RefPtr<SearchBar> SearchBarPtr;

    class Dragger;
    typedef base::RefPtr<Dragger> DraggerPtr;

    class Group;
    typedef base::RefPtr<Group> GroupPtr;

    class Notebook;
    typedef base::RefPtr<Notebook> NotebookPtr;

    //--

    class AbstractItemView;
    typedef base::RefPtr<AbstractItemView> AbstractItemViewPtr;

    class ItemView;
    typedef base::RefPtr<ItemView> ItemViewPtr;

    class ListView;
    typedef base::RefPtr<ListView> ListViewPtr;

    class TreeView;
    typedef base::RefPtr<TreeView> TreeViewPtr;

    //--

    /// how to iterate elements
    enum class DockPanelIterationMode
    {
        All, // iterate over all panels, even if they are in the hidden state
        VisibleOnly, // iterate over panels that are not hidden even if they are not directly visible (ie. tab is not selected)
        ActiveOnly, // iterate only over panels that are directly visible (tab is visible, splitter is not collapsed to zero, window is not minimized, etc)
    };

    class DockPanel;
    typedef base::RefPtr<DockPanel> DockPanelPtr;

    class DockLayoutNode;
    typedef base::RefPtr<DockLayoutNode> DockLayoutNodePtr;

    class DockNotebook;
    typedef base::RefPtr<DockNotebook> DockNotebookPtr;

    class DockContainer;
    typedef base::RefPtr<DockContainer> DockContainerPtr;

    //--

    class ScintillaTextEditor;
    typedef base::RefPtr<ScintillaTextEditor> ScintillaTextEditorPtr;

    //--

    class DataInspector;
    typedef base::RefPtr<DataInspector> DataInspectorPtr;

    class IDataBox;
    typedef base::RefPtr<IDataBox> DataBoxPtr;

    //--

    class CanvasArea;
    typedef base::RefPtr<CanvasArea> CanvasAreaPtr;

    class ICanvasAreaElement;
    typedef base::RefPtr< ICanvasAreaElement> CanvasAreaElementPtr;

    class HorizontalRuler;
    typedef base::RefPtr<HorizontalRuler> HorizontalRulerPtr;

    class VerticalRuler;
    typedef base::RefPtr<VerticalRuler> VerticalRulerPtr;

    //--

    class GraphEditor;
    typedef base::RefPtr<GraphEditor> GraphEditorPtr;
    typedef base::RefWeakPtr<GraphEditor> GraphEditorWeakPtr;

    class GraphBlockPalette;
    typedef base::RefPtr<GraphBlockPalette> GraphBlockPalettePtr;

    class IGraphEditorNode;
    typedef base::RefPtr<IGraphEditorNode> GraphEditorNodePtr;

    class GraphEditorBlockNode;
    typedef base::RefPtr<GraphEditorBlockNode> GraphEditorBlockNodePtr;

    class IGraphNodeInnerWidget;
    typedef base::RefPtr<IGraphNodeInnerWidget> GraphNodeInnerWidgetPtr;

    //--

    extern BASE_UI_API void PostWindowMessage(IElement* owner, MessageType type, base::StringID group, base::StringView<char> txt);

    extern BASE_UI_API void PostNotificationMessage(IElement* owner, MessageType type, base::StringID group, base::StringView<char> txt);

    //--

} // ui
