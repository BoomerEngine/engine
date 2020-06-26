/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiScrollArea.h"
#include "base/memory/include/structurePool.h"

namespace ui
{
    //--

    typedef base::Vector2 VirtualPosition;

    class VirtualAreaElement;
    class VirtualAreaMoveSelectedBlocks;

    struct VirtualAreaElementPositionState
    {
        VirtualAreaElement* elem = nullptr;
        VirtualPosition position;
    };

    //-

    /// element in the virtual area
    class BASE_UI_API VirtualAreaElement : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(VirtualAreaElement, IElement);

    public:
        VirtualAreaElement();

        ///--

        /// get current virtual position of the element
        /// NOTE: this is the position on the abstract virtual canvas at 1:1 scale, actual draw position and size will be much much different
        /// NOTE: this is usually the position of the CENTER of the element
        INLINE const VirtualPosition& virtualPosition() const { return m_virtualPosition; }

        /// get last actual (after scaling and styling) position of the block
        /// NOTE: this position still has to be shifter by view offset to get a real position
        INLINE const Position& actualPosition() const { return m_actualPosition; }

        // update virtual position
        virtual void virtualPosition(const VirtualPosition& pos, bool updateInContainer = true);

        ///--

        // should we draw this element at given scale ?
        virtual bool shouldDrawAtScale(float scale) const;

        // change the selection visualization
        virtual void updateSelectionVis(bool selected);

        // block position was updated
        virtual void updateActualPosition(const base::Vector2& pos);

    private:
        VirtualPosition m_virtualPosition;
        Position m_actualPosition;
    };

    //--

    /// An abstract area, similar to canvas area that render arbitrary placed virtual elements
    /// This is a basis for graph editor like controls
    class BASE_UI_API VirtualArea : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(VirtualArea, IElement);

    public:
        VirtualArea();
        virtual ~VirtualArea();

        //--

        ElementEventProxy OnSelectionChanged;

        //--

        // current offset 
        INLINE base::Vector2 viewOffset() const { return m_viewOffset; }

        // current scale (uniform)
        INLINE float viewScale() const { return m_viewScale; }

        //--

        // reset view to 0,0 and uniform scale
        void resetView();

        // zoom to see all content
        void zoomToFit();

        // scroll to given offset
        void scrollToOffset(base::Vector2 offset);

        // auto scroll to make given absolute position more visible, used in input actions to scroll the view automatically when mouse is close to the edge of the area
        // NOTE: returns true if area was scrolled
        bool autoScrollNearEdge(const base::Point& absolutePosition, float dt, base::Vector2* outViewScrollAmount=nullptr);

        //--

        // get the left bound of the virtual area
        float virtualAreaLeft() const;

        // get the top bound of the virtual area
        float virtualAreaTop() const;

        // get the right bound of the virtual area
        float virtualAreaRight() const;

        // get the bottom bound of the virtual area
        float virtualAreaBottom() const;

        //--

        // calculate absolute position for given virtual position at current scale and view offset
        Position virtualToAbsolute(const VirtualPosition& pos) const;

        // compute virtual position from absolute position on the screen
        VirtualPosition absoluteToVirtual(const base::Point& absolutePos) const;

        // compute virtual position from absolute position
        VirtualPosition absoluteToVirtual(const Position& absolutePos) const;

        // find the area element at given virtual position
        // NOTE: finds the top most element
        VirtualAreaElement* findElementAtVirtualPos(const VirtualPosition& virtualPos) const;

        // find the area element at given absolute pixel position (as clicked by the mouse for example)
        // NOTE: finds the top most element
        VirtualAreaElement* findElementAtAbsolutePos(const base::Point& absolutePos, float margin=0.0f) const;

        //--

        /// attach child to this element
        virtual void attachChild(IElement* childElement);

        /// detach child from this element
        virtual void detachChild(IElement* childElement);

        //--

        /// reset selection on all blocks, posts OnSelectionChanged
        void resetSelection(bool postEvent=true);

        // is given item selected ?
        bool isSelected(const VirtualAreaElement* elem) const;

        // iterate selection
        bool enumSelectedElements(const std::function<bool(VirtualAreaElement * elem)>& enumFunc) const;

        /// set new selection for a set of items, posts OnSelectionChanged
        void select(const base::Array<const VirtualAreaElement*>& items, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

        /// set new selection of one item, posts OnSelectionChanged
        void select(const VirtualAreaElement* item, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

        /// select objects in virtual area, elements must be fully contained in it
        void selectArea(VirtualPosition tl, VirtualPosition br, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

        /// apply selection state directly, without any undo/redo step creation
        void applySelection(const base::Array<const VirtualAreaElement*>& selection, bool postEvent = true);

        /// apply position state directly, without any undo/redo step creation
        void applyMove(const base::Array<VirtualAreaElementPositionState>& positionState);

        //--

        // perform selection change action, can be overridden to create undo/redo step for the selection process
        virtual bool actionChangeSelection(const base::Array<const VirtualAreaElement*>& oldSelection, const base::Array<const VirtualAreaElement*>& newSelection);

        // perform movement of elements, not all editors require undo for this
        virtual bool actionMoveElements(const base::Array<VirtualAreaElementPositionState>& oldPositions, const base::Array<VirtualAreaElementPositionState>& newPositions);

    protected:
        //--

        float m_stepZoom = 1.1f;
        float m_minZoom = 0.05f;
        float m_maxZoom = 2.0f;

        int m_scheduledZoomChangeMode = 0;
        int m_scheduledOffsetChangeMode = 0;
        base::Vector2 m_scheduledOffset;
        int m_gridSize = 16;

        //--

        base::Vector2 m_viewOffset;
        float m_viewScale = 1.0f;
        float m_viewInvScale = 1.0f;

        //--

        struct VirtualElementProxy
        {
            VirtualAreaElement* element = nullptr;

            Position actualPosition; // actual position - without the offset
            Size actualSize; // as calculated by styling

            VirtualPosition virtualPosition; // cached when element moves
            Size virtualSize; // derived from styled size / scale
            bool visible = true; // should we even draw the element
            bool selected = false; // should we consider the element selected
            bool sizeCached = false; // when proxy was just created it's size is invalid

            int drawOrder = 0;  // assigned draw order
            VirtualElementProxy* drawNext = nullptr;
            VirtualElementProxy* drawPrev = nullptr;

            VirtualPosition savedVirtualPosition;

            void changeSelectionStateProtected(bool state);
        };

        base::mem::StructurePool<VirtualElementProxy> m_elementProxiesPool;
        base::Array<VirtualElementProxy*> m_elementProxies;
        base::HashSet<VirtualElementProxy*> m_elementFreshSet;
        base::HashMap<VirtualAreaElement*, VirtualElementProxy*> m_elementProxyMap;

        VirtualElementProxy* findProxyAtAbsolutePos(const base::Point& absolutePos, float margin = 0.0f) const;
        bool enumerateProxyAtAbsolutePos(const base::Point& absolutePos, const std::function<bool(VirtualElementProxy * proxy)>& enumFunc) const;

        //--

        base::HashSet<VirtualElementProxy*> m_selection;
        base::HashSet<VirtualElementProxy*> m_tempSelection;

        void select(const base::Array<VirtualElementProxy*>& items, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);
        void select(VirtualElementProxy* proxy, ItemSelectionMode mode = ItemSelectionModeBit::Default, bool postEvent = true);

        //--

        VirtualElementProxy* m_drawOrderHead = nullptr;
        VirtualElementProxy* m_drawOrderTail = nullptr;

        void removeFromDrawList(VirtualElementProxy* proxy);
        void addToDrawListOnTop(VirtualElementProxy* proxy);

        //--

        void stepZoom(int delta, Position pos);
        void setZoom(float zoom, Position pos);
        void processScheduledZoomChange();
        void processScheduledOffsetChange();

        void invalidateAllStoredSizes();
        void cacheSizes();

        //--

        bool beginBlockMove();
        void updateBlockMove(base::Vector2 virtualDelta);
        void finishBlockMove(bool apply=true);

        void updateProxyPositionFor(VirtualAreaElement* elem);

        void renderGridBackground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity);

        //--

        virtual void renderForeground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual void renderBackground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual bool handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;

        virtual bool iterateDrawChildren(ElementDrawListToken& token) const override;
        virtual void renderCustomOverlayElements(HitCache& hitCache, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual void adjustCustomOverlayElementsPixelScale(float& scale) const override;

        //--

        friend class VirtualAreaMoveSelectedBlocks;
        friend class VirtualAreaElement;
    };

    //--

} // ui