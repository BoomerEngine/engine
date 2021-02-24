/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiCanvasArea.h"
#include "uiInputAction.h"
#include "uiStyleValue.h"
#include "base/canvas/include/canvas.h"
#include "base/font/include/fontInputText.h"
#include "base/canvas/include/canvasGeometryBuilder.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasAreaElement);
RTTI_END_TYPE();

ICanvasAreaElement::~ICanvasAreaElement()
{}

void ICanvasAreaElement::prepareGeometry(CanvasArea* owner, float sx, float sy, Size& outCanvasSizeAtCurrentScale)
{
    // no baked geometry
}

InputActionPtr ICanvasAreaElement::handleMouseClick(CanvasArea* owner, Position virtualPosition, const base::input::MouseClickEvent& evt)
{
    return nullptr;
}

//--

RTTI_BEGIN_TYPE_CLASS(CanvasArea);
    RTTI_METADATA(ElementClassNameMetadata).name("CanvasArea");
RTTI_END_TYPE();

CanvasArea::CanvasArea()
    : m_viewOffset(0,0)
    , m_viewScale(1,1)
{
    hitTest(true);
    allowFocusFromClick(true);
    enableAutoExpand(true, true);
}

CanvasArea::~CanvasArea()
{
    m_elementProxies.clearPtr();
    m_elementProxyMap.clear();
}

void CanvasArea::addElement(ICanvasAreaElement* elem, Position initialPlacement, base::StringID kind)
{
    if (elem)
    {
        auto proxy = new ElementProxy;
        proxy->element = AddRef(elem);
        proxy->virtualPlacement = initialPlacement;
        elem->prepareGeometry(this, m_viewScale.x, m_viewScale.y, proxy->sizeAtCurrentScale);
        proxy->virtualSize = proxy->sizeAtCurrentScale / m_viewScale;
        proxy->kind = kind;
        proxy->visible = !m_hiddenElementKinds.contains(kind);

        m_elementProxies.pushBack(proxy);
        m_elementProxyMap[elem] = proxy;
    }
}

void CanvasArea::removeElement(ICanvasAreaElement* elem)
{
    ElementProxy* proxy = nullptr;
    if (m_elementProxyMap.find(elem, proxy))
    {
        m_elementProxies.remove(proxy);
        m_elementProxyMap.remove(elem);
        delete proxy;
    }        
}

void CanvasArea::moveElement(ICanvasAreaElement* elem, Position newVirtualPlacement)
{
    ElementProxy* proxy = nullptr;
    if (m_elementProxyMap.find(elem, proxy))
        proxy->virtualPlacement = newVirtualPlacement;
}

void CanvasArea::filterElement(base::StringID kind, bool visible)
{
    if (kind)
    {
        if (visible)
            m_hiddenElementKinds.remove(kind);
        else
            m_hiddenElementKinds.insert(kind);

        for (auto* proxy : m_elementProxies)
            if (proxy->kind == kind)
                proxy->visible = visible;

        updateCachedGeometryAndSizes();
    }
}

void CanvasArea::resetView()
{
    m_viewOffset.x = 0.0f;
    m_viewOffset.y = 0.0f;
    m_viewScale.x = 1.0f;
    m_viewScale.y = 1.0f;

    updateCachedGeometryAndSizes();

    call(EVENT_CANVAS_VIEW_CHANGED);
}

void CanvasArea::zoomToFit()
{
    m_scheduledZoomChangeMode = 1;
}

void CanvasArea::zoomToFill()
{
    m_scheduledZoomChangeMode = 3;
}

void CanvasArea::resetZoomToOne()
{
    m_scheduledZoomChangeMode = 2;
}

float CanvasArea::virtualAreaLeft() const
{
    return (0.0f - m_viewOffset.x) / m_viewScale.x;
}

float CanvasArea::virtualAreaTop() const
{
    return (0.0f - m_viewOffset.y) / m_viewScale.y;
}

float CanvasArea::virtualAreaRight() const
{
    return (cachedDrawArea().size().x - m_viewOffset.x) / m_viewScale.x;
}

float CanvasArea::virtualAreaBottom() const
{
    return (cachedDrawArea().size().y - m_viewOffset.y) / m_viewScale.y;
}

Position CanvasArea::absoluteToVirtual(const Position& absolutePos) const
{
    auto x = absolutePos.x - cachedDrawArea().absolutePosition().x;
    auto y = absolutePos.y - cachedDrawArea().absolutePosition().y;

    x = (x - m_viewOffset.x) / m_viewScale.x;
    y = (y - m_viewOffset.y) / m_viewScale.y;

    return Position(x, y);
}

Position CanvasArea::absoluteToVirtual(const base::Point& absolutePos) const
{
    return absoluteToVirtual(absolutePos.toVector());
}

void CanvasArea::scrollToOffset(base::Vector2 offset)
{
    if (offset != m_viewOffset)
    {
        m_viewOffset = offset;
        call(EVENT_CANVAS_VIEW_CHANGED);
    }
}

void CanvasArea::updateCachedGeometryAndSizes()
{
    for (auto* proxy : m_elementProxies)
    {
        proxy->element->prepareGeometry(this, m_viewScale.x, m_viewScale.y, proxy->sizeAtCurrentScale);
        proxy->virtualSize = proxy->sizeAtCurrentScale / m_viewScale;
    }
}

void CanvasArea::renderForeground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity);

    processScheduledZoomChange();

    for (const auto* elem : m_elementProxies)
    {
        if (elem->visible)
        {
            float x = (elem->virtualPlacement.x * m_viewScale.x) + m_viewOffset.x;
            float y = (elem->virtualPlacement.y * m_viewScale.y) + m_viewOffset.y;

            x += drawArea.left();
            y += drawArea.top();

            //canvas.placement(x, y, m_viewScale.x, m_viewScale.y);
            elem->element->render(this, x, y, m_viewScale.x, m_viewScale.y, canvas, mergedOpacity);
        }
    }

    renderStatusMessages(drawArea, canvas, mergedOpacity);
}

void CanvasArea::renderBackground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderBackground(stash, drawArea, canvas, mergedOpacity);
}

void CanvasArea::processScheduledZoomChange()
{
    if (0 == m_scheduledZoomChangeMode || m_elementProxies.empty())
        return;

    if (1 == m_scheduledZoomChangeMode || 3 == m_scheduledZoomChangeMode)
    {
        const auto ZOOM_MARGIN = 20.0f;

        auto sx = cachedDrawArea().size().x;
        auto sy = cachedDrawArea().size().y;

        if (sx <= ZOOM_MARGIN * 3.0f || sy <= ZOOM_MARGIN * 3.0f)
            return;

        ui::Position minPos = Position::INF();
        ui::Position maxPos = -Position::INF();

        for (auto* proxy : m_elementProxies)
        {
            if (proxy->visible)
            {
                minPos = base::Min(proxy->virtualPlacement, minPos);
                maxPos = base::Max(proxy->virtualPlacement + proxy->virtualSize, maxPos);
            }
        }

        if (minPos.x > maxPos.x || minPos.y > maxPos.y)
            return;

        auto avaiableSizeX = sx - (ZOOM_MARGIN * 2.0f);
        auto avaiableSizeY = sy - (ZOOM_MARGIN * 2.0f);
        auto neededSizeX = std::max<float>(maxPos.x - minPos.x, 1.0f);
        auto neededSizeY = std::max<float>(maxPos.y - minPos.y, 1.0f);

        auto zoom = 1.0f;

        if (1 == m_scheduledZoomChangeMode)
            zoom = std::min<float>(avaiableSizeX / neededSizeX, avaiableSizeY / neededSizeY);
        else if (3 == m_scheduledZoomChangeMode)
            zoom = std::max<float>(avaiableSizeX / neededSizeX, avaiableSizeY / neededSizeY);

        zoom = std::clamp<float>(zoom, m_minZoom, m_maxZoom);

        m_viewScale.x = zoom;
        m_viewScale.y = zoom;

        m_viewOffset.x = (sx / 2.0f) - (neededSizeX * zoom / 2.0f);
        m_viewOffset.y = (sy / 2.0f) - (neededSizeY * zoom / 2.0f);

        call(EVENT_CANVAS_VIEW_CHANGED);

        updateCachedGeometryAndSizes();
    }
    else if (2 == m_scheduledZoomChangeMode)
    {
        const auto center = cachedDrawArea().size() / 2.0f;
        setZoom(base::Vector2(1, 1), center);
    }

    m_scheduledZoomChangeMode = 0;
}

void CanvasArea::setZoom(base::Vector2 zoom, Position pos)
{
    const auto snapDist = (m_stepZoom - 1.0f) * 0.5f;
    if (std::fabsf(zoom.x - 1.0f) < snapDist)
        zoom.x = 1.0f;
    if (std::fabsf(zoom.y - 1.0f) < snapDist)
        zoom.y = 1.0f;

    if (zoom != m_viewScale)
    {
        auto vx = (pos.x - m_viewOffset.x) / m_viewScale.x;
        auto vy = (pos.y - m_viewOffset.y) / m_viewScale.y;

        m_viewScale = zoom;

        auto nvx = (vx * m_viewScale.x) + m_viewOffset.x;
        auto nvy = (vy * m_viewScale.y) + m_viewOffset.y;

        m_viewOffset.x -= (nvx - pos.x);
        m_viewOffset.y -= (nvy - pos.y);

        updateCachedGeometryAndSizes();

        call(EVENT_CANVAS_VIEW_CHANGED);
    }
}

void CanvasArea::stepZoom(int delta, Position pos)
{
    auto zoom = m_viewScale;

    if (delta > 0)
    {
        zoom.x = std::min<float>(m_maxZoom, m_viewScale.x * m_stepZoom);
        zoom.y = std::min<float>(m_maxZoom, m_viewScale.y * m_stepZoom);
    }
    else if (delta < 0)
    {
        zoom.x = std::max<float>(m_minZoom, m_viewScale.x / m_stepZoom);
        zoom.y = std::max<float>(m_minZoom, m_viewScale.y / m_stepZoom);
    }

    setZoom(zoom, pos);
}

bool CanvasArea::handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta)
{
    auto pos = evt.absolutePosition().toVector() - cachedDrawArea().absolutePosition();

    if (delta > 0.0f)
        stepZoom(1, pos);
    else if (delta < 0.0f)
        stepZoom(-1, pos);

    return TBaseClass::handleMouseWheel(evt, delta);
}

class CanvasAreaBackgroundScroll : public MouseInputAction
{
public:
    CanvasAreaBackgroundScroll(CanvasArea* area)
        : MouseInputAction(area, base::input::KeyCode::KEY_MOUSE1)
        , m_area(area)
    {
    }

    virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
    {
        if (auto area = m_area.lock())
        {
            auto offset = area->viewOffset();

            offset.x += evt.delta().x;// / m_area->viewScale().x;
            offset.y += evt.delta().y;// / m_area->viewScale().y;

            area->scrollToOffset(offset);
        }
        return InputActionResult();
    }

    virtual bool allowsHoverTracking() const { return false; }
    virtual bool fullMouseCapture() const { return true; }

protected:
    base::RefWeakPtr<CanvasArea> m_area;
};


InputActionPtr CanvasArea::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
{
    if (evt.rightClicked())
        return base::RefNew<CanvasAreaBackgroundScroll>(this);

    return TBaseClass::handleMouseClick(area, evt);
}

//--

void CanvasArea::statusMessage(base::StringID id, base::StringView txt, base::Color color /*= base::Color::WHITE*/, float decayTime /*= 3.0f*/)
{
    auto lock = CreateLock(m_statusMessagesLock);

    for (auto& msg : m_statusMessages)
    {
        if (msg.id == id)
        {
            msg.text = base::StringBuf(txt);
            msg.color = color;
            msg.expiresAt = base::NativeTimePoint::Now() + (double)decayTime;
            return;
        }
    }

    auto& msg = m_statusMessages.emplaceBack();
    msg.id = id;
    msg.text = base::StringBuf(txt);
    msg.color = color;
    msg.expiresAt = base::NativeTimePoint::Now() + (double)decayTime;
}

void CanvasArea::renderStatusMessages(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) const
{
    auto lock = CreateLock(m_statusMessagesLock);

    for (int i = m_statusMessages.lastValidIndex(); i >= 0; --i)
    {
        if (m_statusMessages[i].expiresAt.reached())
            m_statusMessages.erase(i); // keep order
    }

    if (!m_statusMessages.empty())
    {
        if (const auto fonts = evalStyleValueIfPresentPtr<style::FontFamily>("font-family"_id))
        {
            if (fonts->normal)
            {
                const auto size = 24;// std::max<uint32_t>(1, std::floorf(evalStyleValue<float>("font-size"_id, 14.0f) * cachedStyleParams().pixelScale));

                uint32_t totalAreaWidth = 0;
                uint32_t totalAreaHeight = 0;

                base::InplaceArray<uint32_t, 10> linePos;
                for (const auto& msg : m_statusMessages)
                {
                    base::font::FontStyleParams styleParams;
                    styleParams.size = size;
                    base::font::FontAssemblyParams assemblyParams;

                    base::font::FontInputText text(msg.text);
                    base::font::FontMetrics metrics;
                    fonts->normal->measureText(styleParams, assemblyParams, text, metrics);

                    totalAreaWidth = std::max<uint32_t>(totalAreaWidth, metrics.textWidth);
                    linePos.pushBack(totalAreaHeight);
                    totalAreaHeight += metrics.lineHeight;
                }

				base::canvas::Geometry geometry;

				{
					base::canvas::GeometryBuilder b(geometry);

					b.fillColor(base::Color(70, 70, 70, 70));
					b.strokeColor(base::Color(255, 255, 255, 255));
					b.beginPath();
					b.roundedRect(50, 50, totalAreaWidth + 40, totalAreaHeight + 40, 15.0f);
					b.fill();
					b.stroke();

					for (uint32_t i = 0; i < m_statusMessages.size(); ++i)
					{
						const auto& msg = m_statusMessages[i];

						b.pushTransform();
						b.translate(50 + 20, 50 + 20 + linePos[i]);
						b.fillColor(msg.color);
						b.print(fonts->normal, size, msg.text, -1, 0);
					}
				}

                canvas.place(drawArea.absolutePosition(), geometry);
            }
        }
    }
}

//--

END_BOOMER_NAMESPACE(ui)