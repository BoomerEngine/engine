/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"
#include "base/storage/include/xmlDataDocument.h"
#include "base/system/include/timing.h"

#include "uiElementLayout.h"
#include "uiActionTable.h"
#include "uiEventFunction.h"
#include "base/containers/include/hashSet.h"

namespace ui
{
    //---

    DECLARE_UI_EVENT(EVENT_CONTEXT_MENU, Position)

    //---

    class StyleStack;
    
    // Short name of the element class, needed for styling (so we dont use the "ui::Button" but "button" in the CSS)
    class BASE_UI_API ElementClassNameMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ElementClassNameMetadata, base::rtti::IMetadata);

    public:
        ElementClassNameMetadata();

        INLINE ElementClassNameMetadata& name(const char* name)
        {
            m_name = name;
            return *this;
        }

        INLINE const char* name() const { return m_name; }

    private:
        const char* m_name;
    };

    //---

    // Binding name for element reference
    // Used when instancing from UI template to bind references to child elements in parent elements
    class BASE_UI_API ElementBindingNameMetadata : public base::rtti::IMetadata
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ElementBindingNameMetadata, base::rtti::IMetadata);

    public:
        ElementBindingNameMetadata();
        ElementBindingNameMetadata(const char* name);

        INLINE const char* nameString() const { return m_name; }

    private:
        const char* m_name;
    };

    //------

    class GeometryBuilder;

    //------

    // rendering layer for the element geometry
    enum class RenderingLayer : uint8_t
    {
        Shadow, // all children shadows are rendered before children
        Background, // rendered before element children
        Overlay, // rendered after element children
    };

    //------

    // visibility state for element
    enum class VisibilityState : uint8_t
    {
        Visible, // element and it's children are visible
        Hidden, // element and it's children are hidden and take no space in the layout
    };

    //------

    // hit test state for element
    enum class HitTestState : uint8_t
    {
        Enabled, // element and it's children can be interacted with
        DisabledLocal, // element cannot be interacted with but the children may be
        DisabledTree, // element and it's children cannot be interacted with
    };

    //------

    // some elements are sized by parents, ie. columns in a grid
    class BASE_UI_API ElementDynamicSizing
    {
    public:
        ElementDynamicSizing();

        struct ColumnInfo
        {
            float m_left = 0.0f;
            float m_right = 0.0f;
            bool m_center = false;
        };

        uint32_t m_numColumns = 0;
        ColumnInfo* m_columns = nullptr;
    };

    //------

    /// element layout calcuation mode for children of the element
    enum class LayoutMode : uint8_t
    {
        // layout elements vertically
        Vertical,

        // layout elements horizontally
        Horizontal,

        // layout elements in a grid
        Grid,

        // layout in columns, sizes are passed from column size supplier
        Columns,

        // icon as in list view
        Icons,
    };

    //------

    /// interface for dock target objects
    class IDockTarget;

    //------

    /// iterator for the children of ui element
    class BASE_UI_API ElementChildIterator
    {
    public:
        INLINE ElementChildIterator(IElement* cur);

        INLINE operator bool() const
        {
            return m_cur != nullptr;
        }

        INLINE IElement* operator->() const
        {
            return m_cur;
        }

        INLINE IElement* operator*() const
        {
            return m_cur;
        }

        INLINE void operator++();

    private:
        IElement* m_cur;
        IElement* m_next;
    };

    //------

    enum class VisitResult : uint8_t
    {
        Recurse,
        NoRecurse,
        Return,
    };

    //------

    /// cached layout information for children, used for more complex shit like list view
    struct BASE_UI_API ElementCachedChildrenLayout : public base::NoCopy
    {
        struct Placement
        {
            Position m_localPos;
            Size m_cachedSize;
            IElement* m_child;
        };

        float m_pixelScale; // pixel scale for which it was computed
        Size m_containerSize; // size of the container when the layout was computed
        base::Array<Placement> m_placements; // placements of the items
    };

    //------

    /// scrollbar visibility
    enum class ScrollMode : uint8_t
    {
        None, // do not scroll the content, keep it at 0
        Hidden, // scrollbar is never displayed but the virtual area can be scrolled with wheel or programatically
        Auto, // scrollbar is displayed if the virtual size is bigger than the real size
        Always, // scrollbar is always displayed but sometimes may be disabled
    };

    //------

    /// cached element style
    struct ElementCachedStyle : public base::NoCopy
    {
        float pixelScale = 1.0f; // combined pixel scaling factor (DPI scaling mostly)
        float opacity = 1.0f; // style opacity
        uint64_t selectorKey = 0; // key of all combined selectors from the stack, can be used to check if styles has to be updated
        uint64_t paramsKey = 0; // key (CRC64) of all extracted styling data, can be used to check if layout and geometry has to be updated
        ParamTablePtr params; // merged style parameters

        ElementCachedStyle();
    };

    //------

    /// generic cached geometry for the widget
    struct ElementCachedGeometry : public base::NoCopy
    {
        RTTI_DECLARE_POOL(POOL_UI_CANVAS)

    public:
        base::canvas::Geometry* shadow = nullptr; // rendered with parent clip rect
        base::canvas::Geometry* background = nullptr; // rendered with widget client clip rect
        base::canvas::Geometry* foreground = nullptr; // rendered with widget client clip rect
        base::canvas::Geometry* overlay = nullptr; // rendered with parent clip rect

        ~ElementCachedGeometry();
    };

    //------

    /// token for iterating over draw children
    struct ElementDrawListToken : public base::NoCopy
    {
        int64_t index = INDEX_NONE;
        const IElement *elem = nullptr;
        void* curToken = nullptr;
        void* nextToken = nullptr;
        ElementArea* m_unadjustedArea = nullptr;

        INLINE const IElement* operator->() const { return elem; }
        INLINE const IElement* operator*() const { return elem; }
    };

    //------

    /// widget timer
    class BASE_UI_API Timer : public base::NoCopy
    {
    public:
        Timer(IElement* owner, base::StringID name = base::StringID::EMPTY()); // starts in disabled state
        Timer(IElement* owner, float timeInterval, bool oneShot = false, base::StringID name = base::StringID::EMPTY()); // starts in initialized state
        ~Timer();

        void stop();
        void startOneShot(float delay);
        void startRepeated(float interval);
        bool scheduled() const; // there's a time set for this time to go on but we might not be running (not attached)
        bool running() const; // we have scheduled time and we are attached

        base::NativeTimePoint nextTick() const;

        template< typename F >
        INLINE void operator=(F func)
        {
            m_func = EventFunctionBinderHelper<decltype(&F::operator())>::bind(func);
        }

    private:
        base::StringID m_name; // name of the timer, does not have to be set but if it's set the handleTimer is called on element as well
        IElement* m_owner = nullptr;
        Renderer* m_renderer = nullptr;
        base::NativeTimePoint m_nextTick; // when should be tick it
        base::NativeTimePoint m_scheduleTime; // when was the timer scheduled
        base::NativeTimeInterval m_tickInterval; // if not set it was a one shot timer
        TEventFunction m_func; // timer function
        Timer* m_next = nullptr;

        void link();
        void unlink();

        bool call();
        void detach();
        void attach();

        friend class IElement;
        friend class Renderer;
    };

    //------

    /// basic ui element
    /// containers form a hierarchy and do all the heavy lifting of a "control"
    /// the tiny bits are left to be implemented by the "elements"
    class BASE_UI_API IElement : public base::IObject
    {
        RTTI_DECLARE_POOL(POOL_UI_OBJECTS)
        RTTI_DECLARE_VIRTUAL_CLASS(IElement, base::IObject);

    public:
        IElement();
        virtual ~IElement();

        //---

        /// get parent
        INLINE IElement* parentElement() const { return static_cast<IElement*>(parent()); }

        /// get list of children
        INLINE ElementChildIterator childrenList() const { return m_firstChild; }

        /// get the window renderer (NULL if element is not visualized via any window)
        INLINE Renderer* renderer() const { return m_renderer; }

        /// get cached style parameter table for this element
        INLINE const ElementCachedStyle& cachedStyleParams() const { return m_cachedStyle; }

        /// get cached layout settings for the element
        INLINE const ElementLayout& cachedLayoutParams() const { return m_cachedLayout; }

        /// get cached last are the element was drawing into (lagged one frame or more)
        INLINE const ElementArea& cachedDrawArea() const { return m_cachedDrawArea; }

        /// is this element created from template ?
        INLINE bool isFromTemplate() const { return m_isFromTemplate; }

        /// do we have focus ?
        INLINE bool isFocused() const { return m_cachedFocusFlag; }

        /// is this a persistent child element (does not get removed even when removeAllChildren is called) ?
        INLINE bool isPersistentChild() const { return m_presistentElement; }

        /// does this element allow focus from click
        INLINE bool isAllowingFocusFromClick() const { return m_allowFocusFromClick; }

        /// does this element allow auto focus from keyboard ?
        INLINE bool isAllowingFocusFromKeyboard() const { return m_allowFocusFromKeyboard; }

        /// does this element allow hovering ?
        INLINE bool isAllowingMouseHover() const { return m_allowHover; }

        /// is this item ignored during automatic layout computation ?
        INLINE bool isIgnoredInAutomaticLayout() const { return m_ignoreInAutomaticLayout; }

        /// is this item providing dynamic column sizes ?
        INLINE bool isProvidingDynamicSizingData() const { return m_providesDynamicSizingData; }

        /// is this an overlay element?
        INLINE bool isOverlay() const { return m_overlay; }

        //----

        /// get visibility state
        ALWAYS_INLINE VisibilityState visibility() const { return m_visibility; }

        /// set element visibility state
        void visibility(VisibilityState visibility);

        /// set simple visibility
        ALWAYS_INLINE void visibility(bool isVisible) { visibility(isVisible ? VisibilityState::Visible : VisibilityState::Hidden); }

        /// get hit test state of the element
        ALWAYS_INLINE HitTestState hitTest() const { return m_hitTest; }

        /// set element visibility state
        ALWAYS_INLINE void hitTest(HitTestState hitTest) { m_hitTest = hitTest; }

        /// set element visibility state
        ALWAYS_INLINE void hitTest(bool hitTest) { m_hitTest = hitTest ? HitTestState::Enabled : HitTestState::DisabledLocal; }

        /// toggle the persistent child flag (prevents element from being removed in the removeAllChildren)
        ALWAYS_INLINE void persistentElementFlag(bool isPersistent) { m_presistentElement = isPersistent; }

        /// toggle the click based focus on this element (some elements are not focused by clicks)
        ALWAYS_INLINE void allowFocusFromClick(bool flag) { m_allowFocusFromClick = flag; }

        /// toggle the keyboard based focus on this element (some elements are not interested in having keyboard focus)
        ALWAYS_INLINE void allowFocusFromKeyboard(bool flag) { m_allowFocusFromKeyboard = flag; }

        /// track hover state on this element (some elements are not interested in having hover effect, also it's heavy on the styling to change it all the time so it's best disabled for large containers - windows, views, groups, etc)
        ALWAYS_INLINE void allowHover(bool flag) { m_allowHover = flag; }

        /// get children layout mode of this element
        INLINE LayoutMode layoutMode() const { return m_layout; }

        /// set children layout mode
        void layoutMode(LayoutMode mode);

        /// set horizontal layout mode
        INLINE void layoutHorizontal() { layoutMode(LayoutMode::Horizontal); }

        /// set vertical layout mode
        INLINE void layoutVertical() { layoutMode(LayoutMode::Vertical); }

        /// set icon layout mode (mostly used in list views)
        INLINE void layoutIcons() { layoutMode(LayoutMode::Icons); }

        /// set column layout mode (mostly used in list and tree views)
        INLINE void layoutColumns() { layoutMode(LayoutMode::Columns); }

        //--

        /// get the name used to identify this element, can be empty if element name does not matter
        INLINE base::StringID name() const { return m_name; }

        // set element name, NOTE: invalidates styling
        void name(base::StringID name);

        /// is the control enabled ?
        INLINE bool isEnabled() const { return m_enabled; }

        /// enable/disable the element
        void enable(bool isEnabled);

        /// ignore in automatic layout computation
        void ignoredInAutomaticLayout(bool flag);

        /// set custom, simple tooltip text
        void tooltip(base::StringView tooltip);

        /// enable dynamic column sizing
        void dynamicSizingData(bool flag);

        /// toggle overlay flag on this element
        void overlay(bool flag);

        /// toggle auto rendering of overlay child elements on this, can be turned of when custom drawing and ordering is performed
        void renderOverlayElements(bool flag);

        /// get current opacity
        INLINE float opacity() const { return m_opacity; }

        /// change opacity
        void opacity(float val) { m_opacity = std::clamp<float>(val, 0.0f, 1.0f); }

        /// expand item in all directions
        void expand();

		//--

        /// bind event, NOTE: event won't be called if owner dies 
        EventFunctionBinder bind(base::StringID name, IElement* owner = nullptr);

        /// unbind event
        void unbind(base::StringID name, IElement* owner);

        //--

        /// get the action table for this element, created on demand
        ActionTable& actions();

        /// get the action table for this element, returns default empty table if element has no table on it's own
        const ActionTable& actions() const;

        /// call action on this element, if action is not found here we propagate it to parent
        virtual bool runAction(base::StringID name, IElement* source);

        /// check action status
        virtual ActionStatusFlags checkAction(base::StringID name) const;

        //--

        // set element custom minimal size
        // this is applied on top of the default style
        void customMinSize(const Size& minSize);
        void customMinSize(float x, float y);

        // set element custom maximum size
        // this is applied on top of the default style
        void customMaxSize(const Size& maxSize);
        void customMaxSize(float x, float y);

        // set element custom margins
        void customMargins(const Offsets& margins);
        void customMargins(float left, float top, float right, float bottom);
        void customMargins(float all);

        // set element custom padding
        void customPadding(const Offsets& padding);
        void customPadding(float left, float top, float right, float bottom);
        void customPadding(float all);

        // set element initial size (size of first measurment when inital window size is calculated)
        void customInitialSize(const Size& minSize);
        void customInitialSize(float x, float y);

        // set custom background style
        void customBackgroundColor(base::Color color);

        // set custom foreground style (text)
        void customForegroundColor(base::Color color);

        // set custom vertical alignment for element
        void customVerticalAligment(ElementVerticalLayout verticalLayout);

        // set custom horizontal alignment for element
        void customHorizontalAligment(ElementHorizontalLayout horizontalLayout);

        // set custom layout proportion scaling
        void customProportion(float proportion);

        // set a custom style override
        void customStyleVariant(base::StringID name, base::Variant value);

        // set a custom style override
        template< typename T >
        INLINE void customStyle(base::StringID name, T value)
        {
            DEBUG_CHECK_EX(name, "Empty name");
            static_assert(!std::is_same < std::remove_cv<T>::type, base::Variant >::value, "Trying to add a variant inside a variant");
            customStyleVariant(name, base::CreateVariant(value));
        }

        // remove custom style override
        void removeCustomStyle(base::StringID name);

        /// check if the window has a given style class specified
        bool hasStyleClass(base::StringID styleClassName) const;

        /// add a style class to the element
        /// causes full invalidation
        /// NOTE: for transient (short-lived) changes consider using the pseudo classes
        void addStyleClass(base::StringID name);

        /// remove style class from the element
        /// causes full invalidation
        void removeStyleClass(base::StringID name);

        /// set additional element "type" for styling, ie. "PushButton for "Button"
        /// allows to massively reskin element without declaring RTTI class for it
        void styleType(base::StringID type);

        /// get the additional style type for element
        base::StringID styleType() const;

        //---

        /// evaluate style value, local styles take precedence over the common styles
        template< typename T >
        INLINE const T& evalStyleRef(base::StringID name, const T& defaultValue = T()) const
        {
            if (m_customLocalStyles)
            {
                if (const auto* val = m_customLocalStyles->findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    return *(const T*)val->data();
                }
            }

            if (m_cachedStyle.params)
            {
                if (const auto* val = m_cachedStyle.params->values.findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    return *(const T*)val->data();
                }
            }

            return defaultValue;
        }

        //---

        /// evaluate style value, local styles take precedence over the common styles
        template< typename T >
        INLINE T evalStyleValue(base::StringID name, T defaultValue = T()) const
        {
            if (m_customLocalStyles)
            {
                if (const auto* val = m_customLocalStyles->findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    return *(const T*)val->data();
                }
            }

            if (m_cachedStyle.params)
            {
                if (const auto* val = m_cachedStyle.params->values.findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    return *(const T*)val->data();
                }
            }

            return defaultValue;
        }

        //---

        /// evaluate style value, local styles take precedence over the common styles
        template< typename T >
        INLINE bool evalStyleValueIfPresent(base::StringID name, T& outVal) const
        {
            if (m_customLocalStyles)
            {
                if (const auto* val = m_customLocalStyles->findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    outVal = *(const T*)val->data();
                    return true;
                }
            }

            if (m_cachedStyle.params)
            {
                if (const auto* val = m_cachedStyle.params->values.findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    outVal = *(const T*)val->data();
                    return true;
                }
            }

            return false;
        }

        /// evaluate style value, local styles take precedence over the common styles
        template< typename T >
        INLINE const T* evalStyleValueIfPresentPtr(base::StringID name) const
        {
            if (m_customLocalStyles)
            {
                if (const auto* val = m_customLocalStyles->findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    return (const T*)val->data();
                }
            }

            if (m_cachedStyle.params)
            {
                if (const auto* val = m_cachedStyle.params->values.findVariant(name))
                {
                    DEBUG_CHECK(val->type() == base::reflection::GetTypeObject<T>());
                    return (const T*)val->data();
                }
            }

            return nullptr;
        }

        ///---

        /// handle primary mouse input (first click)
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt);

        /// handle primary overlay mouse input (first click)
        /// the overlay events are passed in the reverse order to allow the parents to handle the overlay events
        virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt);

        /// handle mouse wheel
        virtual bool handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta);

        /// handle general mouse movement (called only if element is in the hover stack)
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt);

        /// preview key event, this travels down to the focused element
        virtual bool previewKeyEvent(const base::input::KeyEvent& evt);

        /// handle key event, this is called at the focused element, if false is returned the parent is given a chance to handle this event
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt);

        /// handle key event coming from external UI element (input forwarding)
        virtual bool handleExternalKeyEvent(const base::input::KeyEvent& evt);

        /// handle char event
        virtual bool handleCharEvent(const base::input::CharEvent& evt);

        /// handle char event coming from external UI element (input forwarding)
        virtual bool handleExternalCharEvent(const base::input::CharEvent& evt);

        /// handle window system cursor request
        virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const;

        /// handle window system window area query
        virtual bool handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const;

        // handle context menu at given position
        virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition);

        // handle entering hover state
        virtual void handleHoverEnter(const Position& absolutePosition);

        // handle leaving hover state
        virtual void handleHoverLeave(const Position& absolutePosition);

        // handle long hover, if not handled a tooltip as attempted
        virtual bool handleHoverDuration(const Position& absolutePosition);

        // handle gaining focus state
        virtual void handleFocusGained();

        // handle loosing focus state
        virtual void handleFocusLost();

        // handle change of children count, called when child is attached/detached
        virtual void handleChildrenChange();

        /// handle drag&drop action with given data that enters this item at given position
        /// if a drag&drop handler is returned it will be used to coordinate all dragging, returning NULL continues the search
        virtual DragDropHandlerPtr handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition);

        /// generic drag&drop behavior - accept drag&drop data, called by the generic handler only
        /// NOTE: there's no cancel event in the generic behavior because we assume that the simple handling will not create a complicated preview
        virtual void handleDragDropGenericCompletion(const DragDropDataPtr& data, const Position& entryPosition);

        /// notification to sub classes than the enable state of the element has changed
        virtual void handleEnableStateChange(bool isEnabled);

        /// handle setting a property from a template
        virtual bool handleTemplateProperty(base::StringView name, base::StringView value);

        /// handle a child property from a template
        virtual bool handleTemplateChild(base::StringView name, const base::xml::IDocument& doc, const base::xml::NodeID& id);

        /// called after all template's children were inserted
        virtual bool handleTemplateFinalize();

        /// called to insert created child element into this element, by default forward shit to attachBuildChild
        virtual bool handleTemplateNewChild(const base::xml::IDocument& doc, const base::xml::NodeID& id, const base::xml::NodeID& childId, const ElementPtr& childElement);

        /// called when a function-less (no callback) timer expires, by default calls the "OnTimer" event with the name of the timer
        virtual bool handleTimer(base::StringID name, float elapsedTime);

        //--

        // first first focusable element in this item tree
        virtual IElement* focusFindFirst();

        // starting from current element what is the next element to focus
        virtual IElement* focusFindNext();

        // starting from this current child what is the next child to focus in this element
        virtual IElement* focusFindNextSibling(IElement* childSelected);

        // starting from current element what is the previous element to focus
        virtual IElement* focusFindPrev();

        // starting from this current child what is the previous child to focus in this element
        virtual IElement* focusFindPrevSibling(IElement* childSelected);

        //--

		/// get canvas storage of the renderer we are attached to
		base::canvas::IStorage* canvasStorage() const;

        /// query next draw children
        virtual bool iterateDrawChildren(ElementDrawListToken& token) const;

        /// arrange children of this element
        virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const;

        /// compute basic size
        virtual void computeSize(Size& outSize) const;

        /// compute required size for this element
        virtual void computeLayout(ElementLayout& outLayout);

        /// generate the outline of this element
        virtual void prepareBoundaryGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder, float inset) const;

        /// prepare shadow geometry for this element
        virtual void prepareShadowGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const;

        /// prepare background geometry for this element
        virtual void prepareBackgroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const;

        /// prepare foreground geometry for this element
        virtual void prepareForegroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const;

        /// prepare overlay geometry for this element
        virtual void prepareOverlayGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const;

        /// prepare dynamic sizing data
        virtual void prepareDynamicSizing(const ElementArea& drawArea, const ElementDynamicSizing*& dataPtr) const;

        /// render shadow geometry
        virtual void renderShadow(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        /// render background geometry, by default rendered the cached client geometry
        virtual void renderBackground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        /// render foreground geometry
        virtual void renderForeground(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        /// render overlay elements
        virtual void renderOverlay(const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        //---

        /// prepare for rendering, resolves missing styles, caches missing data
        void prepareStyle(StyleStack& styleStack, float pixelScale, bool force);

        /// layout the elements, invisible elements are not updated
        void prepareLayout(bool& outLayoutRecomputed, bool force, bool initial);

        /// render element
        void render(HitCache& hitCache, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity, const ElementDynamicSizing* parentDynamicSizing);

        //---

        /// query the tooltip element
        virtual ElementPtr queryTooltipElement(const Position& absolutePosition) const;

        /// query the drag&drop data this element can produce
        /// NOTE: if this is not null than we will have a D&D action generated once we start dragging this time
        /// NOTE: the type of drag&drop data may depend on the location within the UI element as well as the control keys
        virtual DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const Position& position) const;

        //---

        // invalidate various cached states
        virtual void invalidateStyle();

        /// bind window renderer
        virtual void bindNativeWindowRenderer(Renderer* windowRenderer);

        /// request layout of this element to be invalidated
        void invalidateLayout();

        //// reset all cached geometry
        virtual void invalidateGeometry();

        /// invalidate cached placement of the children
        void invalidateCachedChildrenPlacement();

        /// focus on this element (NOTE: if the element is not part of the hit test shit it's not focused)
        virtual void focus();
        
        //---

        /// remove all children
        void removeAllChildren();

        /// attach child to this element
        virtual void attachChild(IElement* childElement);

        /// detach child from this element
        virtual void detachChild(IElement* childElement);

        //--

        /// call event on this element, notifies any registered callbacks, generic data version
        bool callGeneric(IElement* originator, base::StringID name, const base::Variant& data);

        // call event with specific data
        template< typename T >
        INLINE bool call(IElement* originator, base::StringID name, const T& data)
        {
            static_assert(!std::is_same<T, base::Variant>::value, "To call with a variant use callGeneric");
            return callGeneric(originator, name, base::CreateVariant(data));
        }

        /// call event on this element, notifies any registered callbacks, no data version
        INLINE bool call(base::StringID name)
        {
            return callGeneric(this, name, base::Variant::EMPTY());
        }

        /// call event on this element, notifies any registered callbacks, generic data version
        INLINE bool callGeneric(base::StringID name, const base::Variant& data)
        {
            return callGeneric(this, name, data);
        }

        // call event with specific data
        template< typename T >
        INLINE bool call(base::StringID name, const T& data)
        {
            static_assert(!std::is_same<T, base::Variant>::value, "To call with a variant use callGeneric");
            return callGeneric(name, base::CreateVariant(data));
        }

        /// call event on this element, notifies any registered callbacks, no data version
        INLINE bool call(IElement* originator, base::StringID name)
        {
            return callGeneric(originator, name, base::Variant::EMPTY());
        }

        //---

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createChild(Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            attachChild(child);
            return child;
        }

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createNamedChild(base::StringID name, Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            child->name(name);
            attachChild(child);
            return child;
        }

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createChildWithType(base::StringID typeName, Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            child->styleType(typeName);
            attachChild(child);
            return child;
        }

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createNamedChildWithType(base::StringID name, base::StringID typeName, Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            child->name(name);
            child->styleType(typeName);
            attachChild(child);
            return child;
        }

        //---

        /// called after element was initialize after template instancing
        virtual void initializeTemplateInstanced();

        /// call template initialization
        void callPostTemplateInitialization();

        /// bind named properties by finding child object
        bool bindNamedProperties(const base::HashMap<base::StringView, IElement*>& namedElements);

        //---

        /// query interface for dock target
        virtual IDockTarget* queryDockTarget() const;

        /// get the native window this element belongs to, only true if we are inside a window :)
        virtual Window* findParentWindow() const;

        //---

        /// find parent element implementing given class
        template< typename T >
        INLINE T* findParent(base::StringID name = base::StringID::EMPTY()) const
        {
            return base::rtti_cast<T>(findParent(T::GetStaticClass(), name));
        }

        /// find parent element implementing given class
        IElement* findParent(base::SpecificClassType<IElement> baseClass, base::StringID name = base::StringID::EMPTY()) const;

        /// find child element by name, if name is not specified any matching element will be returned
        IElement* findChildByName(const char* name, base::SpecificClassType<IElement> baseClass, bool recrusive = true) const;

        /// find child element by name, if name is not specified any matching element will be returned
        IElement* findChildByName(base::StringID name, base::SpecificClassType<IElement> baseClass, bool recrusive = true) const;

        /// find child element by name, if name is not specified any matching element will be returned
        template< typename T >
        INLINE T* findChildByName(const char* name, bool recrusive = true) const
        {
            return base::rtti_cast<T>(findChildByName(name, T::GetStaticClass(), recrusive));
        }

        /// find child element by name, if name is not specified any matching element will be returned
        template< typename T >
        INLINE T* findChildByName(base::StringID name, bool recrusive=true) const
        {
            return base::rtti_cast<T>(findChildByName(name, T::GetStaticClass(), recrusive));
        }

        /// visit the whole hierarchy
        IElement* visitChildren(const std::function<const VisitResult(IElement* ptr)>& visitFunc) const;

        //---

        /// invalidate all cached data, used only during style library reload
        static void InvalidateAllDataEverywhere();

        //---

        /// load configuration
        virtual void configLoad(const ConfigBlock& block);

        /// save configuration
        virtual void configSave(const ConfigBlock& block) const;

        //--

    protected:
        friend class ElementChildIterator;

        // get the style selector
        style::SelectorMatchContext* styleSelector();

        /// check if the widget has a given style pseudo class 
        bool hasStylePseudoClass(base::StringID name) const;

        /// add a style pseudo class to the widget
        void addStylePseudoClass(base::StringID name);

        /// remove style pseudo class from the widget
        void removeStylePseudoClass(base::StringID name);

        // refresh cached geometry
        virtual void rebuildCachedGeometry(const ElementArea& drawArea);

        /// render explicit overlay elements
        virtual void renderCustomOverlayElements(HitCache& hitCache, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity);

        /// adjust pixel scale for children
        virtual void adjustCustomOverlayElementsPixelScale(float& scale) const;

        /// adjust dynamically background style before rendering
        virtual bool adjustBackgroundStyle(base::canvas::RenderStyle& outStyle, float& outBorderWidth) const;

        /// adjust dynamically border style before rendering
        virtual bool adjustBorderStyle(base::canvas::RenderStyle& outStyle, float& outBorderWidth) const;

        /// request parent to visit this child for layout update
        void requestChildLayoutUpdate();

        /// request parent to visit this child for style update
        void requestChildStyleUpdate();

        /// enable/disable the "auto expand" on this item - it will change the default (unspecified) expand modes for it
        INLINE void enableAutoExpand(bool enabledX = true, bool enabledY = true) { m_autoExpandX = enabledX; m_autoExpandY = enabledY; }

        // add element to internal child list
        void attachElementToChildList(IElement* element);

        // remove element from internal child list
        void detachElementFromChildList(IElement* element);

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createInternalChild(Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            attachElementToChildList(child);
            return child;
        }

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createInternalNamedChild(base::StringID name, Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            child->name(name);
            attachElementToChildList(child);
            return child;
        }

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createInternalNamedChildWithType(base::StringID name, base::StringID typeName, Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            child->name(name);
            child->styleType(typeName);
            attachElementToChildList(child);
            return child;
        }

        /// create and attach a child element
        template< typename T = IElement, typename... Args >
        INLINE base::RefPtr<T> createInternalChildWithType(base::StringID typeName, Args&& ... args)
        {
            auto child = base::RefNew<T>(std::forward< Args >(args)...);
            child->styleType(typeName);
            attachElementToChildList(child);
            return child;
        }

    private:
        // flags
        bool m_autoExpandX : 1;
        bool m_autoExpandY : 1;
        bool m_cachedLayoutValid : 1;
        bool m_cachedHoveredFlag : 1;
        bool m_cachedFocusFlag : 1;
        bool m_isFromTemplate : 1;
        bool m_isTemplateInitCalled : 1;
        bool m_dynamicLayoutScaling : 1;
        bool m_presistentElement : 1;
        bool m_allowFocusFromClick : 1;
        bool m_allowFocusFromKeyboard : 1;
        bool m_allowHover : 1;
        bool m_ignoreInAutomaticLayout : 1;
        bool m_providesDynamicSizingData : 1;
        bool m_enabled : 1;
        bool m_overlay : 1;
        bool m_renderOverlayElements : 1;

        VisibilityState m_visibility = VisibilityState::Visible;
        HitTestState m_hitTest = HitTestState::DisabledLocal;
        LayoutMode m_layout = LayoutMode::Vertical;

        //--

        // assigned element name/id
        base::StringID m_name;

        // cached style selector
        style::SelectorMatchContext* m_styleSelector = nullptr;

        // local custom styles
        base::VariantTable* m_customLocalStyles = nullptr;

        // area used for this element during last draw
        mutable ElementArea m_cachedDrawArea;

        // size that this elements requires, cached
        ElementLayout m_cachedLayout;

        // cached styles for this element
        ElementCachedStyle m_cachedStyle;

        // cached rendering geometry
        uint64_t m_cachedGeometryStyleHash = 0; // style for which this geometry was created
        Size m_cachedGeometrySize; // size for which the geometry was cached
        ElementCachedGeometry* m_cachedGeometry = nullptr;

        // children of this element
        IElement* m_firstChild = nullptr;
        IElement* m_lastChild = nullptr;
        IElement* m_listNext = nullptr;
        IElement* m_listPrev = nullptr;

        // opacity
        float m_opacity = 1.0f;

        // children that require style update
        typedef base::HashSet<IElement*> TChildrenUpdateList;
        TChildrenUpdateList m_childrenWithDirtyStyle;
        TChildrenUpdateList m_childrenWithDirtyLayout;

        // cached data for children layouts
        base::UniquePtr<ElementCachedChildrenLayout> m_cachedChildrenPlacement;

        // overlay elements, valid only if we have overlay element ;)
        base::Array<IElement*>* m_overlayElements = nullptr;

        //--

        // parent reference (NOTE: use with care)
        //IElement* m_weakParent = nullptr;

        // window renderer, can be used to create windows
        Renderer* m_renderer = nullptr;

        // actions (for menus and toolbars, as well as key shortcuts)
        ActionTable* m_actionTable = nullptr;

        // event list, created only if events are registered
        EventTable* m_eventTable = nullptr;

        // all internal timers
        Timer* m_timers = nullptr;

        //---

        void unlinkFromChildrenList(IElement*& listHead, IElement*& listTail);
        void linkToChildrenList(IElement*& listHead, IElement*& listTail);

        void updateTimers();

        void computeSizeOverlay(Size& outSize) const;
        void computeSizeVertical(Size& outSize) const;
        void computeSizeHorizontal(Size& outSize) const;
        void computeSizeIcons(Size& outSize) const;

        void arrangeChildrenInner(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed, uint8_t& outNeededScrollbar) const;

        void arrangeChildrenVertical(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const;
        void arrangeChildrenHorizontal(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const;
        void arrangeChildrenIcons(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const;
        void arrangeChildrenColumns(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing, Size& outActualSizeUsed) const;

        void arrangeChildrenOverlay(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, Size& outActualSizeUsed) const;

        void prepareCachedGeometry(const ElementArea& drawArea);

        //--

        friend class BaseBuilder;
        friend class ScrollArea;
        friend class Timer;
    };

    //--

    INLINE ElementChildIterator::ElementChildIterator(IElement* cur)
        : m_cur(cur)
    {
            m_next = m_cur ? m_cur->m_listNext : nullptr;
    }

    INLINE void ElementChildIterator::operator++()
    {
        m_cur = m_next;
        m_next = m_cur ? m_cur->m_listNext : nullptr;
    }

    //--

    // make an element from all child elements and arrange them
    extern BASE_UI_API ElementPtr Layout(LayoutMode mode, const std::initializer_list<ElementPtr>& elements);

    // make an element from all child elements arranged vertically
    extern BASE_UI_API ElementPtr LayoutVertically(const std::initializer_list<ElementPtr>& elements);

    // make an element from all child elements arranged vertically
    extern BASE_UI_API ElementPtr LayoutHorizontally(const std::initializer_list<ElementPtr>& elements);

    //--

} // ui