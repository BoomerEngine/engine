/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiScrollArea.h"

namespace ui
{

    //--

    DECLARE_UI_EVENT(EVENT_CANVAS_VIEW_CHANGED)

    //--

    /// drawable element of the canvas area
    class BASE_UI_API ICanvasAreaElement : public base::IReferencable
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasAreaElement);

    public:
        virtual ~ICanvasAreaElement();

        // prepare/cache draw geometry for given zoom level, called whenever zoom level changes
        virtual void prepareGeometry(ui::CanvasArea* owner, float sx, float sy, ui::Size& outCanvasSizeAtCurrentScale);

        // render this element into the canvas
        virtual void render(ui::CanvasArea* owner, float x, float y, float sx, float sy, base::canvas::Canvas& canvas, float mergedOpacity) = 0;

        // service input action
        virtual ui::InputActionPtr handleMouseClick(ui::CanvasArea* owner, ui::Position virtualPosition, const base::input::MouseClickEvent& evt);
    };

    //--

    /// a simple canvas
    class BASE_UI_API CanvasArea : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasArea, IElement);

    public:
        CanvasArea();
        virtual ~CanvasArea();

        //--

        // current offset
        inline base::Vector2 viewOffset() const { return m_viewOffset; }
        inline base::Vector2 viewScale() const { return m_viewScale; }

        //--

        // add element to canvas
        void addElement(ICanvasAreaElement* elem, Position initialVirtualPlacement, base::StringID kind = "default"_id);

        // remove element from canvas
        void removeElement(ICanvasAreaElement* elem);

        // move canvas element to a different position
        void moveElement(ICanvasAreaElement* elem, Position newVirtualPlacement);

        // toggle visibility of elements on of
        void filterElement(base::StringID kind, bool visible);

        // reset view to 0,0 and uniform scale
        void resetView();

        // zoom to see all content
        void zoomToFit();

        // zoom to fill window with content
        void zoomToFill();

        // reset zoom to 1-1
        void resetZoomToOne();

        // scroll to given offset
        void scrollToOffset(base::Vector2 offset);

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

        // compute virtual position from absolute position
        Position absoluteToVirtual(const base::Point& absolutePos) const;

        // compute virtual position from absolute position
        Position absoluteToVirtual(const Position& absolutePos) const;

        //--

        // add a general status message to be printed
        void statusMessage(base::StringID id, base::StringView<char> txt, base::Color color = base::Color::WHITE, float decayTime = 3.0f);

    protected:
        struct ElementProxy
        {
            CanvasAreaElementPtr element;
            Position virtualPlacement;
            Size virtualSize;
            Size sizeAtCurrentScale;
            base::StringID kind;
            bool visible = true;
        };

        base::Array<ElementProxy*> m_elementProxies;
        base::HashMap<ICanvasAreaElement*, ElementProxy*> m_elementProxyMap;

        base::HashSet<base::StringID> m_hiddenElementKinds;

        base::Vector2 m_viewOffset;
        base::Vector2 m_viewScale;

        float m_stepZoom = 1.1f;
        float m_minZoom = 0.1f;
        float m_maxZoom = 10.0f;

        int m_scheduledZoomChangeMode = 0;

        //--

        struct StatusMessage
        {
            base::StringID id;
            base::StringBuf text;
            base::Color color;
            base::NativeTimePoint expiresAt;
        };

        mutable base::Array<StatusMessage> m_statusMessages;
        base::Mutex m_statusMessagesLock;

        //--

        void stepZoom(int delta, Position pos);
        void setZoom(base::Vector2 zoom, Position pos);
        void processScheduledZoomChange();

        void updateCachedGeometryAndSizes();

        void renderStatusMessages(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) const;

        virtual void renderForeground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual void renderBackground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual bool handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override;
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    };

    //--

} // ui