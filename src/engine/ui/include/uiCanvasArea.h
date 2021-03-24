/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiScrollArea.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

DECLARE_UI_EVENT(EVENT_CANVAS_VIEW_CHANGED)

//--

/// drawable element of the canvas area
class ENGINE_UI_API ICanvasAreaElement : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasAreaElement);

public:
    virtual ~ICanvasAreaElement();

    // prepare/cache draw geometry for given zoom level, called whenever zoom level changes
    virtual void prepareGeometry(CanvasArea* owner, float sx, float sy, Size& outCanvasSizeAtCurrentScale);

    // render this element into the canvas
    virtual void render(CanvasArea* owner, float x, float y, float sx, float sy, Canvas& canvas, float mergedOpacity) = 0;

    // service input action
    virtual InputActionPtr handleMouseClick(CanvasArea* owner, Position virtualPosition, const InputMouseClickEvent& evt);
};

//--

/// a simple canvas
class ENGINE_UI_API CanvasArea : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasArea, IElement);

public:
    CanvasArea();
    virtual ~CanvasArea();

    //--

    // current offset
    inline Vector2 viewOffset() const { return m_viewOffset; }
    inline Vector2 viewScale() const { return m_viewScale; }

    //--

    // add element to canvas
    void addElement(ICanvasAreaElement* elem, Position initialVirtualPlacement, StringID kind = "default"_id);

    // remove element from canvas
    void removeElement(ICanvasAreaElement* elem);

    // move canvas element to a different position
    void moveElement(ICanvasAreaElement* elem, Position newVirtualPlacement);

    // toggle visibility of elements on of
    void filterElement(StringID kind, bool visible);

    // reset view to 0,0 and uniform scale
    void resetView();

    // zoom to see all content
    void zoomToFit();

    // zoom to fill window with content
    void zoomToFill();

    // reset zoom to 1-1
    void resetZoomToOne();

    // scroll to given offset
    void scrollToOffset(Vector2 offset);

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
    Position absoluteToVirtual(const Point& absolutePos) const;

    // compute virtual position from absolute position
    Position absoluteToVirtual(const Position& absolutePos) const;

    //--

    // add a general status message to be printed
    void statusMessage(StringID id, StringView txt, Color color = Color::WHITE, float decayTime = 3.0f);

protected:
    struct ElementProxy : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_UI_CANVAS)

    public:
        CanvasAreaElementPtr element;
        Position virtualPlacement;
        Size virtualSize;
        Size sizeAtCurrentScale;
        StringID kind;
        bool visible = true;
    };

    Array<ElementProxy*> m_elementProxies;
    HashMap<ICanvasAreaElement*, ElementProxy*> m_elementProxyMap;

    HashSet<StringID> m_hiddenElementKinds;

    Vector2 m_viewOffset;
    Vector2 m_viewScale;

    float m_stepZoom = 1.1f;
    float m_minZoom = 0.1f;
    float m_maxZoom = 10.0f;

    int m_scheduledZoomChangeMode = 0;

    //--

    struct StatusMessage
    {
        StringID id;
        StringBuf text;
        Color color;
        NativeTimePoint expiresAt;
    };

    mutable Array<StatusMessage> m_statusMessages;
    Mutex m_statusMessagesLock;

    //--

    void stepZoom(int delta, Position pos);
    void setZoom(Vector2 zoom, Position pos);
    void processScheduledZoomChange();

    void updateCachedGeometryAndSizes();

    void renderStatusMessages(const ElementArea& drawArea, Canvas& canvas, float mergedOpacity) const;

    virtual void renderForeground(DataStash& stash, const ElementArea& drawArea, Canvas& canvas, float mergedOpacity) override;
    virtual void renderBackground(DataStash& stash, const ElementArea& drawArea, Canvas& canvas, float mergedOpacity) override;
    virtual bool handleMouseWheel(const InputMouseMovementEvent& evt, float delta) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
