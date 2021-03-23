/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiVirtualArea.h"
#include "uiInputAction.h"
#include "uiStyleValue.h"
#include "uiTextLabel.h"

#include "engine/canvas/include/canvas.h"
#include "engine/font/include/fontInputText.h"
#include "engine/canvas/include/geometryBuilder.h"

#pragma optimize ("", off)

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

ConfigProperty<int> cvVirtualAreaAutoScrollBorder("Editor.VirtualArea", "AutoScrollBorder", 100);
ConfigProperty<float> cvVirtualAreaAutoScrollSpeed("Editor.VirtualArea", "AutoScrollSpeed", 800.0f);
ConfigProperty<float> cvVirtualAreaAutoScrollPowerFactor("Editor.VirtualArea", "AutoScrollPowerFactor", 3.0f);

//--

RTTI_BEGIN_TYPE_CLASS(VirtualAreaElement);
    RTTI_METADATA(ElementClassNameMetadata).name("VirtualAreaElement");
RTTI_END_TYPE();

VirtualAreaElement::VirtualAreaElement()
    : m_virtualPosition(0,0)
    , m_actualPosition(0,0)
{
    overlay(true);
    hitTest(true);
    allowFocusFromClick(true);
    layoutVertical();
}

bool VirtualAreaElement::shouldDrawAtScale(float scale) const
{
    return true;
}

void VirtualAreaElement::updateSelectionVis(bool selected)
{
    if (selected)
        addStyleClass("selected"_id);
    else
        removeStyleClass("selected"_id);
}

void VirtualAreaElement::updateActualPosition(const Vector2& pos)
{
    m_actualPosition = pos;
}

void VirtualAreaElement::virtualPosition(const VirtualPosition& pos, bool updateInContainer /*= true*/)
{
    if (pos != m_virtualPosition)
    {
        m_virtualPosition = pos;

        if (updateInContainer)
        {
            if (auto container = rtti_cast<VirtualArea>(parentElement()))
                container->updateProxyPositionFor(this);
        }
    }
}

//--

RTTI_BEGIN_TYPE_CLASS(VirtualArea);
    RTTI_METADATA(ElementClassNameMetadata).name("VirtualArea");
RTTI_END_TYPE();

VirtualArea::VirtualArea()
    : m_viewOffset(0,0)
{
    hitTest(true);
    allowFocusFromClick(true);
    renderOverlayElements(false);
    enableAutoExpand(true, true);
}

VirtualArea::~VirtualArea()
{
    for (auto* proxy : m_elementProxies)
        m_elementProxiesPool.free(proxy);
    m_elementProxies.clear();
    m_elementProxyMap.clear();
}

void VirtualArea::attachChild(IElement* childElement)
{
    TBaseClass::attachChild(childElement);

    if (auto ve = rtti_cast<VirtualAreaElement>(childElement))
    {
        DEBUG_CHECK_EX(!m_elementProxyMap.contains(ve), "Visual element already registered");

        auto* proxy = m_elementProxiesPool.create();
        proxy->element = ve;
        proxy->sizeCached = false;
            
        m_elementProxyMap[ve] = proxy;
        m_elementProxies.pushBack(proxy);
        m_elementFreshSet.insert(proxy);

        addToDrawListOnTop(proxy);
    }
}

void VirtualArea::detachChild(IElement* childElement)
{
    if (auto ve = rtti_cast<VirtualAreaElement>(childElement))
    {
        DEBUG_CHECK_EX(m_elementProxyMap.contains(ve), "Visual element already registered");

        if (auto proxy = m_elementProxyMap.findSafe(ve, nullptr))
        {
            removeFromDrawList(proxy);

            m_selection.remove(proxy);
            m_elementFreshSet.remove(proxy);
            m_elementProxyMap.remove(ve);
            m_elementProxies.remove(proxy);
            m_elementProxiesPool.free(proxy);
        }
    }

    TBaseClass::detachChild(childElement);
}

/*    void VirtualArea::moveElement(IVirtualAreaElement* elem, Position newVirtualPlacement)
{
    ElementProxy* proxy = nullptr;
    if (m_elementProxyMap.find(elem, proxy))
        proxy->virtualPlacement = newVirtualPlacement;
}*/

void VirtualArea::resetView()
{
    m_viewOffset.x = 0.0f;
    m_viewOffset.y = 0.0f;
    m_viewScale = 1.0f;
    m_viewInvScale = 1.0f;

    call(EVENT_VIRTUAL_AREA_OFFSET_CHANGED, m_viewOffset);
    call(EVENT_VIRTUAL_AREA_SCALE_CHANGED, m_viewScale);

    invalidateAllStoredSizes();
}

void VirtualArea::zoomToFit()
{
    m_scheduledOffsetChangeMode = 0;
    m_scheduledZoomChangeMode = 1;
}

float VirtualArea::virtualAreaLeft() const
{
    return (0.0f - m_viewOffset.x) * m_viewInvScale;
}

float VirtualArea::virtualAreaTop() const
{
    return (0.0f - m_viewOffset.y) * m_viewInvScale;
}

float VirtualArea::virtualAreaRight() const
{
    return (cachedDrawArea().size().x - m_viewOffset.x) * m_viewInvScale;
}

float VirtualArea::virtualAreaBottom() const
{
    return (cachedDrawArea().size().y - m_viewOffset.y) * m_viewInvScale;
}

Position VirtualArea::absoluteToVirtual(const Position& absolutePos) const
{
    auto x = absolutePos.x - cachedDrawArea().absolutePosition().x;
    auto y = absolutePos.y - cachedDrawArea().absolutePosition().y;

    x = (x - m_viewOffset.x) * m_viewInvScale;
    y = (y - m_viewOffset.y) * m_viewInvScale;

    return Position(x, y);
}

Position VirtualArea::virtualToAbsolute(const VirtualPosition& pos) const
{
    return (pos * m_viewScale) + m_viewOffset + cachedDrawArea().absolutePosition();
}

Position VirtualArea::absoluteToVirtual(const Point& absolutePos) const
{
    return absoluteToVirtual(absolutePos.toVector());
}

VirtualAreaElement* VirtualArea::findElementAtVirtualPos(const VirtualPosition& virtualPos) const
{
    for (auto* area = m_drawOrderTail; area; area = area->drawPrev)
    {
        if (virtualPos.x >= area->virtualPosition.x && virtualPos.y >= area->virtualPosition.y &&
            virtualPos.x - area->virtualPosition.x <= area->virtualSize.x && virtualPos.y - area->virtualPosition.y <= area->virtualSize.y)
        {
            return area->element;
        }
    }

    return nullptr;
}

VirtualAreaElement* VirtualArea::findElementAtAbsolutePos(const Point& absolutePos, float margin) const
{
    if (auto* proxy = findProxyAtAbsolutePos(absolutePos, margin))
        return proxy->element;
    return nullptr;
}

void VirtualArea::removeFromDrawList(VirtualElementProxy* proxy)
{
    if (proxy->drawNext)
    {
        DEBUG_CHECK(proxy != m_drawOrderTail);
        proxy->drawNext->drawPrev = proxy->drawPrev;
    }
    else
    {
        DEBUG_CHECK(proxy == m_drawOrderTail);
        m_drawOrderTail = proxy->drawPrev;
    }

    if (proxy->drawPrev)
    {
        DEBUG_CHECK(proxy != m_drawOrderHead);
        proxy->drawPrev->drawNext = proxy->drawNext;
    }
    else
    {
        DEBUG_CHECK(proxy == m_drawOrderHead);
        m_drawOrderHead = proxy->drawNext;
    }

    proxy->drawNext = nullptr;
    proxy->drawPrev = nullptr;
}

void VirtualArea::addToDrawListOnTop(VirtualElementProxy* proxy)
{
    DEBUG_CHECK(proxy->drawNext == nullptr);
    DEBUG_CHECK(proxy->drawPrev == nullptr);

    if (m_drawOrderTail != nullptr)
    {
        m_drawOrderTail->drawNext = proxy;
        proxy->drawPrev = m_drawOrderTail;
        m_drawOrderTail = proxy;
    }
    else
    {
        m_drawOrderHead = proxy;
        m_drawOrderTail = proxy;
    }
}

VirtualArea::VirtualElementProxy* VirtualArea::findProxyAtAbsolutePos(const Point& absolutePos, float margin) const
{
    auto actualPos = (absolutePos - cachedDrawArea().absolutePosition()) - m_viewOffset;
    for (auto* cur = m_drawOrderTail; cur; cur = cur->drawPrev)
    {
        if (actualPos.x + margin >= cur->actualPosition.x && actualPos.y + margin >= cur->actualPosition.y
            && actualPos.x <= (cur->actualPosition.x + cur->actualSize.x + margin) && actualPos.y <= (cur->actualPosition.y + cur->actualSize.y + margin))
        {
            return cur;
        }
    }

    return nullptr;
}

bool VirtualArea::enumerateProxyAtAbsolutePos(const Point& absolutePos, const std::function<bool(VirtualElementProxy * proxy)>& enumFunc) const
{
    auto actualPos = (absolutePos - cachedDrawArea().absolutePosition()) - m_viewOffset;
    for (auto* cur = m_drawOrderTail; cur; cur = cur->drawPrev)
    {
        if (actualPos.x >= cur->actualPosition.x && actualPos.y >= cur->actualPosition.y
            && actualPos.x <= (cur->actualPosition.x + cur->actualSize.x) && actualPos.y <= (cur->actualPosition.y + cur->actualSize.y))
        {
            if (enumFunc(cur))
                return true;
        }
    }

    return false;
}

//--

void VirtualArea::resetSelection(bool postEvent /*= true*/)
{
    if (!m_selection.empty())
    {
        for (auto* proxy : m_selection)
        {
            DEBUG_CHECK(proxy->selected);
            if (proxy->selected)
            {
                proxy->selected = false;
                proxy->element->updateSelectionVis(false);
            }
        }

        m_selection.clear();

        if (postEvent)
            call(EVENT_VIRTUAL_AREA_SELECTION_CHANGED);
    }
}

bool VirtualArea::isSelected(const VirtualAreaElement* elem) const
{
    if (auto proxy = m_elementProxyMap.findSafe(elem, nullptr))
    {
        DEBUG_CHECK(m_selection.contains(proxy) == proxy->selected);
        return proxy->selected;
    }

    return false;
}

bool VirtualArea::enumSelectedElements(const std::function<bool(VirtualAreaElement * elem)>& enumFunc) const
{
    for (const auto* proxy : m_selection.keys())
    {
        DEBUG_CHECK(proxy->selected == true);
        if (enumFunc(proxy->element))
            return true;
    }

    return false;
}

void VirtualArea::select(const Array<const VirtualAreaElement*>& items, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    InplaceArray<VirtualElementProxy*, 128> proxies;
    proxies.reserve(items.size());

    for (const auto* item : items)
        if (auto proxy = m_elementProxyMap.findSafe(item, nullptr))
            proxies.pushBack(proxy);


    select(proxies, mode, postEvent);
}

void VirtualArea::select(const VirtualAreaElement* item, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    // TODO: optimize for one item case

    InplaceArray<VirtualElementProxy*, 1> proxies;

    if (auto proxy = m_elementProxyMap.findSafe(item, nullptr))
        proxies.pushBack(proxy);

    select(proxies, mode, postEvent);
}

void VirtualArea::selectArea(VirtualPosition tl, VirtualPosition br, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    const auto xmin = std::min<float>(tl.x, br.x);
    const auto xmax = std::max<float>(tl.x, br.x);
    const auto ymin = std::min<float>(tl.y, br.y);
    const auto ymax = std::max<float>(tl.y, br.y);

    InplaceArray<VirtualElementProxy*, 256> proxies;
    for (auto* proxy : m_elementProxies)
    {
        const auto vmin = proxy->virtualPosition;
        const auto vmax = proxy->virtualPosition + proxy->virtualSize;
        if (vmin.x >= xmin && vmax.x <= xmax && vmin.y >= ymin && vmax.y <= ymax)
            proxies.pushBack(proxy);
    }

    select(proxies, mode, postEvent);
}

void VirtualArea::VirtualElementProxy::changeSelectionStateProtected(bool state)
{
    DEBUG_CHECK(state != selected);
    if (selected != state)
    {
        selected = state;
        element->updateSelectionVis(state);
    }
}

void VirtualArea::select(VirtualElementProxy* proxy, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    InplaceArray<VirtualElementProxy*, 1> proxies;

    if (proxy)
        proxies.pushBack(proxy);

    select(proxies, mode, postEvent);

}

void VirtualArea::applySelection(const Array<const VirtualAreaElement*>& newSelection, bool postEvent /*= true*/)
{
    bool somethingChanged = false;

    // start with an empty set
    m_tempSelection.reset();

    // add objects that we want to directly select
    // O(N) in the number of items
    for (const auto& item : newSelection)
        if (auto proxy = m_elementProxyMap.findSafe(item, nullptr))
            m_tempSelection.insert(proxy);

    // update visualization based on set difference
    // O(N log N) in the number of items
    for (const auto& it : m_tempSelection.keys())
    {
        if (!m_selection.contains(it))
        {
            it->changeSelectionStateProtected(true);
            somethingChanged = true;
        }
    }

    // remove visualization from deselected items - O(N log N) in the number of items
    for (const auto& it : m_selection.keys())
    {
        if (!m_tempSelection.contains(it))
        {
            it->changeSelectionStateProtected(false);
            somethingChanged = true;
        }
    }

    // set new selection
    std::swap(m_selection, m_tempSelection);
    m_tempSelection.reset();

    // notify anybody interested, usually this refreshes some kind of property grid
    if (somethingChanged && postEvent)
        call(EVENT_VIRTUAL_AREA_SELECTION_CHANGED);
}

bool VirtualArea::actionChangeSelection(const Array<const VirtualAreaElement*>& oldSelection, const Array<const VirtualAreaElement*>& newSelection)
{
    // this a non-undo implementation - just apply the new selection
    applySelection(newSelection);
    return true;
}

void VirtualArea::select(const Array<VirtualElementProxy*>& items, ItemSelectionMode mode /*= ItemSelectionModeBit::Default*/, bool postEvent /*= true*/)
{
    // capture current list
    HashSet<const VirtualAreaElement*> currentSelectionSet;
    for (const auto* proxy : m_selection.keys())
        if (proxy && proxy->element)
            currentSelectionSet.insert(proxy->element);

    // build new selection list
    HashSet<const VirtualAreaElement*> selectionSet;
    if (mode.test(ItemSelectionModeBit::Clear))
    {
        // extract elements
        if (mode.test(ItemSelectionModeBit::Select))
        {
            for (const auto& item : items)
                if (item && item->element)
                    selectionSet.insert(item->element);
        }
    }
    // select mode
    else if (mode.test(ItemSelectionModeBit::Select) && !mode.test(ItemSelectionModeBit::Deselect))
    {
        // use original items as base
        selectionSet = currentSelectionSet;

        // items in the list, if not already selected will be selected
        for (const auto& item : items)
            if (item && item->element)
                selectionSet.insert(item->element);
    }
    // deselect mode
    else if (mode.test(ItemSelectionModeBit::Deselect) && !mode.test(ItemSelectionModeBit::Select))
    {
        // use original items as base
        selectionSet = currentSelectionSet;

        // deselect requested items
        for (const auto& item : items)
            if (item && item->element)
                selectionSet.remove(item->element);
    }
    // toggle mode
    else if (mode.test(ItemSelectionModeBit::Toggle))
    {
        // use original items as base
        selectionSet = currentSelectionSet;

        // items in the list, if not already selected will be selected
        for (const auto& item : items)
            if (item && item->element)
                if (!selectionSet.remove(item->element))
                    selectionSet.insert(item->element);
    }

    // apply new selection state
    actionChangeSelection(currentSelectionSet.keys(), selectionSet.keys());
}

//--

void VirtualArea::scrollToOffset(Vector2 offset)
{
    if (offset != m_viewOffset)
    {
        m_viewOffset = offset;
        call(EVENT_VIRTUAL_AREA_OFFSET_CHANGED, m_viewOffset);
    }
}

static float PowerAbs(float x, float factor)
{
    if (x < 0.0f)
        return -powf(-x, factor);
    else
        return powf(x, factor);
}

bool VirtualArea::autoScrollNearEdge(const Point& absolutePosition, float dt, Vector2* outViewScrollAmount /*= nullptr*/)
{
    if (cvVirtualAreaAutoScrollBorder.get() > 1)
    {
        const auto area = cachedDrawArea().rect();
        const auto innerArea = area.inner(cvVirtualAreaAutoScrollBorder.get());
        if (!innerArea.empty())
        {
            float xDelta = 0.0f;
            float yDelta = 0.0f;

            // vertical
            if (/*absolutePosition.y >= area.top() && */absolutePosition.y <= innerArea.top())
                yDelta = (absolutePosition.y - innerArea.top()) / (float)cvVirtualAreaAutoScrollBorder.get();
            else if (/*absolutePosition.y <= area.bottom() && */absolutePosition.y >= innerArea.bottom())
                yDelta = (absolutePosition.y - innerArea.bottom()) / (float)cvVirtualAreaAutoScrollBorder.get();

            // horizontal
            if (/*absolutePosition.x >= area.left() && */absolutePosition.x <= innerArea.left())
                xDelta = (absolutePosition.x - innerArea.left()) / (float)cvVirtualAreaAutoScrollBorder.get();
            else if (/*absolutePosition.x <= area.right() && */absolutePosition.x >= innerArea.right())
                xDelta = (absolutePosition.x - innerArea.right()) / (float)cvVirtualAreaAutoScrollBorder.get();

            //TRACE_INFO("{} {}", xDelta, yDelta);

            // calculate speeds
            float xSpeed = PowerAbs(std::clamp<float>(xDelta, -1.0f, 1.0f), cvVirtualAreaAutoScrollPowerFactor.get()) * cvVirtualAreaAutoScrollSpeed.get();
            float ySpeed = PowerAbs(std::clamp<float>(yDelta, -1.0f, 1.0f), cvVirtualAreaAutoScrollPowerFactor.get()) * cvVirtualAreaAutoScrollSpeed.get();

            // advance positions
            m_viewOffset.x -= xSpeed * dt;
            m_viewOffset.y -= ySpeed * dt;

            // update event
            call(EVENT_VIRTUAL_AREA_OFFSET_CHANGED, m_viewOffset);

            // output the offset
            if (outViewScrollAmount)
                *outViewScrollAmount = -Vector2(xSpeed * dt, ySpeed * dt);
            return true;
        }
    }

    return false;
}

bool VirtualArea::iterateDrawChildren(ElementDrawListToken& token) const
{
    return TBaseClass::iterateDrawChildren(token);
}

void VirtualArea::invalidateAllStoredSizes()
{
    for (auto* elem : m_elementProxies)
    {
        elem->element->invalidateStyle();
        elem->element->invalidateLayout();
        elem->element->invalidateGeometry();

        if (elem->sizeCached)
        {
            elem->sizeCached = false;
            m_elementFreshSet.insert(elem);
        }
    }
}

void VirtualArea::cacheSizes()
{
    if (!m_elementFreshSet.empty())
    {
        for (auto* proxy : m_elementFreshSet.keys())
        {
            if (!proxy->sizeCached)
            {
                const auto totalSize = proxy->element->cachedLayoutParams().calcTotalSize();
                proxy->actualSize = totalSize;
                proxy->virtualPosition = proxy->element->virtualPosition();
                proxy->virtualSize = totalSize * m_viewInvScale;
                proxy->actualPosition = proxy->element->virtualPosition() * m_viewScale;
                proxy->element->updateActualPosition(proxy->actualPosition);
                proxy->sizeCached = true;
            }
        }

        m_elementFreshSet.reset();
    }
}

void VirtualArea::renderForeground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity);
}

void VirtualArea::renderGridBackground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    //canvas.placement(drawArea.absolutePosition().x, drawArea.absolutePosition().y);

    if (auto shadowPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("background"_id))
    {
        auto renderStyle = shadowPtr->evaluate(stash, cachedStyleParams().pixelScale, drawArea);
        if (renderStyle.image)
        {
            renderStyle.customUV = true;

            float alphaScale = 1.0f;
            if (m_viewScale < 1.0f)
                alphaScale = 1.0f - std::clamp<float>((m_viewInvScale - 1.0f) / 2.0f, 0.0f, 1.0f);

            if (alphaScale < 1.0f)
            {
				canvas::Canvas::QuadSetup quad;
                quad.color = renderStyle.innerColor;
				quad.x1 = drawArea.size().x;
				quad.y1 = drawArea.size().y;
				canvas.quad(drawArea.absolutePosition(), quad);
            }

            if (alphaScale > 0.0f)
            {
                renderStyle.innerColor.a *= alphaScale;
                renderStyle.outerColor.a *= alphaScale;

                auto imageSize = (float)renderStyle.image.width;

				canvas::Canvas::QuadSetup quad;
				quad.x1 = drawArea.size().x;
				quad.y1 = drawArea.size().y;
                quad.u0 = (0.0f - m_viewOffset.x * m_viewInvScale) / imageSize;
                quad.v0 = (0.0f - m_viewOffset.y * m_viewInvScale) / imageSize;
                quad.u1 = quad.u0 + (quad.x1 * m_viewInvScale) / imageSize;
                quad.v1 = quad.v0 + (quad.y1 * m_viewInvScale) / imageSize;
				quad.op = canvas::BlendOp::AlphaPremultiplied; // TODO: copy
				quad.image = renderStyle.image;
                quad.color = renderStyle.innerColor;
                quad.wrap = true;

				canvas.quad(drawArea.absolutePosition(), quad);
            }
        }
    }
}

void VirtualArea::renderBackground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    // TBaseClass::renderBackground(drawArea, canvas, mergedOpacity);

    renderGridBackground(stash, drawArea, canvas, mergedOpacity);
    cacheSizes();
}

void VirtualArea::adjustCustomOverlayElementsPixelScale(float& scale) const
{
    scale = m_viewScale;// InvScale;
}

void VirtualArea::renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderCustomOverlayElements(hitCache, stash, outerArea, outerClipArea, canvas, mergedOpacity);

    processScheduledOffsetChange();

    for (auto* elem = m_drawOrderHead; elem; elem = elem->drawNext)
    {
        auto viewPosition = (elem->actualPosition + m_viewOffset) + outerArea.absolutePosition();
        viewPosition.x = (int)(viewPosition.x);
        viewPosition.y = (int)(viewPosition.y);

        auto viewSize = elem->actualSize;
        viewSize.x = (int)(viewSize.x);
        viewSize.y = (int)(viewSize.y);

        if (viewPosition.x + viewSize.x >= outerClipArea.left() && viewPosition.x <= outerClipArea.right()
            && viewPosition.y + viewSize.y >= outerClipArea.top() && viewPosition.y <= outerClipArea.bottom())
        {
            const ElementArea elemDrawArea(viewPosition, viewSize);
            elem->element->render(hitCache, stash, elemDrawArea, outerClipArea, canvas, mergedOpacity, nullptr);
        }
    }

    processScheduledZoomChange();


    // debug elements bounds
    /*{
        canvas::GeometryBuilder b;

        for (const auto* elem : m_elementProxies)
        {
            if (elem->visible && !elem->fresh)
            {
                auto viewPosition = elem->actualPosition + m_viewOffset;
                ElementArea elemDrawArea(viewPosition, elem->actualSize);

                b.strokeColor(Color::CYAN);
                b.beginPath();
                b.rect(elemDrawArea.tl(), elemDrawArea.br());
                b.stroke();

                b.strokeColor(Color::BLUE);
                b.beginPath();
                b.moveTo(viewPosition.x - 10, viewPosition.y);
                b.lineTo(viewPosition.x + 10, viewPosition.y);
                b.stroke();
                b.beginPath();
                b.moveTo(viewPosition.x, viewPosition.y-10);
                b.lineTo(viewPosition.x, viewPosition.y+10);
                b.stroke();
            }
        }

        canvas.placement(outerArea.absolutePosition().x, outerArea.absolutePosition().y);
        canvas.place(b);
    }*/
}

void VirtualArea::processScheduledOffsetChange()
{
    if (m_scheduledOffsetChangeMode)
    {
        m_viewOffset = m_scheduledOffset;
        m_scheduledOffsetChangeMode = 0;

        call(EVENT_VIRTUAL_AREA_OFFSET_CHANGED, m_viewOffset);
    }
}

void VirtualArea::processScheduledZoomChange()
{
    if (1 == m_scheduledZoomChangeMode)
    {
        const auto ZOOM_MARGIN = 50.0f;

        auto sx = cachedDrawArea().size().x;
        auto sy = cachedDrawArea().size().y;

        if (sx <= ZOOM_MARGIN * 3.0f || sy <= ZOOM_MARGIN * 3.0f)
            return;

        Position virtualMinPos = Position::INF();
        Position virtualMaxPos = -Position::INF();

        bool canPerformChange = true;
        for (auto* proxy : m_elementProxies)
        {
            if (proxy->visible)
            {
                if (proxy->sizeCached)
                {
                    virtualMinPos = virtualMinPos.min(proxy->virtualPosition);
                    virtualMaxPos = virtualMaxPos.max(proxy->virtualPosition + proxy->virtualSize);
                }
                else
                {
                    // some elements don't have valid sizes yet
                    canPerformChange = false;
                    break;
                }
            }
        }

        if (canPerformChange && virtualMinPos.x < virtualMaxPos.x && virtualMinPos.y < virtualMaxPos.y)
        {
            auto avaiableSizeX = sx - (ZOOM_MARGIN * 2.0f);
            auto avaiableSizeY = sy - (ZOOM_MARGIN * 2.0f);
            auto neededSizeX = std::max<float>(virtualMaxPos.x - virtualMinPos.x, 1.0f);
            auto neededSizeY = std::max<float>(virtualMaxPos.y - virtualMinPos.y, 1.0f);
            auto centerX = (virtualMaxPos.x + virtualMinPos.x) * 0.5f;
            auto centerY = (virtualMaxPos.y + virtualMinPos.y) * 0.5f;

            auto zoom = 1.0f;

            if (1 == m_scheduledZoomChangeMode)
                zoom = std::min<float>(avaiableSizeX / neededSizeX, avaiableSizeY / neededSizeY);
            else if (3 == m_scheduledZoomChangeMode)
                zoom = std::max<float>(avaiableSizeX / neededSizeX, avaiableSizeY / neededSizeY);

            zoom = std::clamp<float>(zoom, m_minZoom, m_maxZoom);

            m_viewScale = zoom;
            m_viewInvScale = 1.0f / zoom;

            m_scheduledOffset.x = (sx / 2.0f) - (centerX * zoom);
            m_scheduledOffset.y = (sy / 2.0f) - (centerY * zoom);
            m_scheduledOffsetChangeMode = 1;

            invalidateAllStoredSizes();
            m_scheduledZoomChangeMode = 0;

            call(EVENT_VIRTUAL_AREA_SCALE_CHANGED, m_viewScale);
        }
    }   
}

void VirtualArea::setZoom(float zoom, Position pos)
{
    const auto snapDist = (m_stepZoom - 1.0f) * 0.5f;
    if (std::fabsf(zoom - 1.0f) < snapDist)
        zoom = 1.0f;

    if (zoom != m_viewScale)
    {
        auto vx = (pos.x - m_viewOffset.x) / m_viewScale;
        auto vy = (pos.y - m_viewOffset.y) / m_viewScale;

        m_viewInvScale = 1.0f / zoom;
        m_viewScale = zoom;

        auto nvx = (vx * m_viewScale) + m_viewOffset.x;
        auto nvy = (vy * m_viewScale) + m_viewOffset.y;

        m_viewOffset.x -= (nvx - pos.x);
        m_viewOffset.y -= (nvy - pos.y);

        invalidateAllStoredSizes();

        call(EVENT_VIRTUAL_AREA_OFFSET_CHANGED, m_viewOffset);
        call(EVENT_VIRTUAL_AREA_SCALE_CHANGED, m_viewScale);
    }
}

void VirtualArea::stepZoom(int delta, Position pos)
{
    auto zoom = m_viewScale;

    if (delta > 0)
        zoom = std::min<float>(m_maxZoom, m_viewScale * m_stepZoom);
    else if (delta < 0)
        zoom = std::max<float>(m_minZoom, m_viewScale / m_stepZoom);

    setZoom(zoom, pos);
}

bool VirtualArea::handleKeyEvent(const InputKeyEvent& evt)
{
    if (evt.pressedOrRepeated() && evt.keyCode() == InputKey::KEY_HOME)
    {
        zoomToFit();
        return true;
    }

    return TBaseClass::handleKeyEvent(evt);
}

bool VirtualArea::handleMouseWheel(const InputMouseMovementEvent& evt, float delta)
{
    auto pos = evt.absolutePosition().toVector() - cachedDrawArea().absolutePosition();

    if (delta > 0.0f)
        stepZoom(1, pos);
    else if (delta < 0.0f)
        stepZoom(-1, pos);

    return true;
}

class VirtualAreaBackgroundScroll : public MouseInputAction
{
public:
    VirtualAreaBackgroundScroll(VirtualArea* area)
        : MouseInputAction(area, InputKey::KEY_MOUSE1)
        , m_area(area)
    {
    }

    virtual InputActionResult onMouseMovement(const InputMouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
    {
        auto offset = m_area->viewOffset();

        offset.x += evt.delta().x;// / m_area->viewScale().x;
        offset.y += evt.delta().y;// / m_area->viewScale().y;

        m_area->scrollToOffset(offset);
        return InputActionResult();
    }

    virtual bool allowsHoverTracking() const { return false; }
    virtual bool fullMouseCapture() const { return false; } // TODO: wrapping

protected:
    VirtualArea* m_area;
};


InputActionPtr VirtualArea::handleOverlayMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    if (evt.leftClicked())
    {
        auto proxyUnderCursor = findProxyAtAbsolutePos(evt.absolutePosition());

        ItemSelectionMode selectionMode = ItemSelectionModeBit::Default;
        if (evt.keyMask().isCtrlDown())
            selectionMode = ItemSelectionModeBit::Toggle;
        else if (evt.keyMask().isShiftDown())
            selectionMode = ItemSelectionModeBit::Select;
            
        select(proxyUnderCursor, selectionMode);

        if (proxyUnderCursor && proxyUnderCursor != m_drawOrderTail)
        {
            removeFromDrawList(proxyUnderCursor);
            addToDrawListOnTop(proxyUnderCursor);
        }
    }

    return TBaseClass::handleOverlayMouseClick(area, evt);
}

//--

// input action for drawing the area selection
class VirtualAreaRectSelector : public MouseInputAction
{
public:
    VirtualAreaRectSelector(VirtualArea* area, const Point& initialPoint, bool shiftKey, bool ctrlKey, Color selectionColor)
        : MouseInputAction(area, InputKey::KEY_MOUSE0)
        , m_lastAbsolutePoint(initialPoint)
        , m_selectionColor(selectionColor)
        , m_area(area)
        , m_shiftKey(shiftKey)
        , m_ctrlKey(ctrlKey)
    {
        m_initialVirtualPos = m_area->absoluteToVirtual(initialPoint);
        m_currentVirtualPos = m_area->absoluteToVirtual(initialPoint);
    }

    virtual void onFinished() override
    {
        if (m_initialVirtualPos != m_currentVirtualPos)
        {
            ItemSelectionMode mode = ItemSelectionModeBit::Default;
            if (m_ctrlKey)
                mode = ItemSelectionModeBit::Toggle;
            else if (m_shiftKey)
                mode = ItemSelectionModeBit::Select;
            m_area->selectArea(m_initialVirtualPos, m_currentVirtualPos, mode);
        }
    }

    virtual InputActionResult onUpdate(float dt) override
    {
        if (m_area->autoScrollNearEdge(m_lastAbsolutePoint, dt))
            m_currentVirtualPos = m_area->absoluteToVirtual(m_lastAbsolutePoint);

        return InputActionResult();
    }

    virtual void onRender(canvas::Canvas& canvas) override
    {
        const auto minAbs = m_area->virtualToAbsolute(m_initialVirtualPos);
        const auto maxAbs = m_area->virtualToAbsolute(m_currentVirtualPos);

        const auto minX = std::min<float>(minAbs.x, maxAbs.x);
        const auto minY = std::min<float>(minAbs.y, maxAbs.y);
        const auto maxX = std::max<float>(minAbs.x, maxAbs.x);
        const auto maxY = std::max<float>(minAbs.y, maxAbs.y);

		if (maxX > minX && maxY > minY)
		{
			canvas::Geometry geometry;

			{
				canvas::GeometryBuilder builder(geometry);

				builder.strokeColor(m_selectionColor);

				auto fillColor = m_selectionColor;
				fillColor.a = fillColor.a / 3;
				builder.fillColor(fillColor);

				builder.beginPath();
				builder.rect(minX, minY, maxX-minX, maxY-minY);
				builder.fill();
				builder.stroke();
			}

            canvas.pushScissorRect();
            canvas.scissorRect(m_area->cachedDrawArea().absolutePosition(), m_area->cachedDrawArea().size());
			canvas.place(Vector2::ZERO(), geometry);
            canvas.popScissorRect();
        }
    }

    virtual InputActionResult onMouseMovement(const InputMouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
    {
        m_shiftKey = evt.keyMask().isShiftDown();
        m_ctrlKey = evt.keyMask().isCtrlDown();
        m_lastAbsolutePoint = evt.absolutePosition();
        m_currentVirtualPos = m_area->absoluteToVirtual(evt.absolutePosition());
        return InputActionResult();
    }

    virtual bool allowsHoverTracking() const { return false; }
    virtual bool fullMouseCapture() const { return false; }

protected:
    VirtualArea* m_area;
    VirtualPosition m_initialVirtualPos;
    VirtualPosition m_currentVirtualPos;
    Color m_selectionColor;
    Point m_lastAbsolutePoint;
    bool m_shiftKey = false;
    bool m_ctrlKey = false;
};

//--

// input action for moving selected blocks
class VirtualAreaMoveSelectedBlocks : public MouseInputAction
{
public:
    VirtualAreaMoveSelectedBlocks(VirtualArea* area, const Point& initialPoint)
        : MouseInputAction(area, InputKey::KEY_MOUSE0)
        , m_lastAbsolutePoint(initialPoint)
        , m_area(area)
    {
        m_initialVirtualPos = m_area->absoluteToVirtual(initialPoint);
        m_currentVirtualPos = m_area->absoluteToVirtual(initialPoint);
    }

    virtual void onFinished() override
    {
        m_area->finishBlockMove(true);
    }

    virtual void onCanceled() override
    {
        m_area->finishBlockMove(false);
    }

    virtual InputActionResult onUpdate(float dt) override
    {
        if (m_area->autoScrollNearEdge(m_lastAbsolutePoint, dt))
        {
            m_currentVirtualPos = m_area->absoluteToVirtual(m_lastAbsolutePoint);
            updateMovement();
        }

        return InputActionResult();
    }

    virtual InputActionResult onMouseMovement(const InputMouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
    {
        m_lastAbsolutePoint = evt.absolutePosition();
        m_currentVirtualPos = m_area->absoluteToVirtual(evt.absolutePosition());
        updateMovement();
        return InputActionResult();
    }

    void updateMovement()
    {
        auto virtualDelta = m_currentVirtualPos - m_initialVirtualPos;
        m_area->updateBlockMove(virtualDelta);
    }

    virtual bool allowsHoverTracking() const { return false; }
    virtual bool fullMouseCapture() const { return false; }

protected:
    VirtualArea* m_area;
    VirtualPosition m_initialVirtualPos;
    VirtualPosition m_currentVirtualPos;
    Point m_lastAbsolutePoint;
};
//--

InputActionPtr VirtualArea::handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    if (evt.rightClicked())
        return RefNew<VirtualAreaBackgroundScroll>(this);

    if (evt.leftClicked())
    {
        auto proxyUnderCursor = findProxyAtAbsolutePos(evt.absolutePosition());
        if (proxyUnderCursor)
        {
            if (beginBlockMove())
                return RefNew<VirtualAreaMoveSelectedBlocks>(this, evt.absolutePosition());
        }
        else
        {
            const auto selectionColor = evalStyleValue<Color>("selection"_id, Color(0, 0, 255));
            return RefNew<VirtualAreaRectSelector>(this, evt.absolutePosition(), evt.keyMask().isShiftDown(), evt.keyMask().isCtrlDown(), selectionColor);
        }
    }

    return TBaseClass::handleMouseClick(area, evt);
}

//--

bool VirtualArea::actionMoveElements(const Array<VirtualAreaElementPositionState>& oldPositions, const Array<VirtualAreaElementPositionState>& newPositions)
{
    applyMove(newPositions);
    return true;
}

void VirtualArea::applyMove(const Array<VirtualAreaElementPositionState>& positionState)
{
    for (const auto& entry : positionState)
    {
        if (auto proxy = m_elementProxyMap.findSafe(entry.elem, nullptr))
        {
            proxy->virtualPosition = entry.position;
            proxy->actualPosition = proxy->virtualPosition * m_viewScale;
            proxy->element->updateActualPosition(proxy->actualPosition);
            proxy->element->virtualPosition(proxy->virtualPosition, true);
        }
    }
}

void VirtualArea::updateProxyPositionFor(VirtualAreaElement* elem)
{
    if (auto proxy = m_elementProxyMap.findSafe(elem, nullptr))
    {
        proxy->virtualPosition = elem->virtualPosition();
        proxy->actualPosition = proxy->virtualPosition * m_viewScale;
        proxy->element->updateActualPosition(proxy->actualPosition);
    }
}

bool VirtualArea::beginBlockMove()
{
    if (!m_selection.empty())
    {
        for (auto* proxy : m_selection)
            proxy->savedVirtualPosition = proxy->virtualPosition;
        return true;
    }

    return false;
}

void VirtualArea::updateBlockMove(Vector2 virtualDelta)
{
    for (auto* proxy : m_selection)
    {
        auto newPos = proxy->savedVirtualPosition + virtualDelta;
        newPos.x = std::roundf(newPos.x / m_gridSize) * m_gridSize;
        newPos.y = std::roundf(newPos.y / m_gridSize) * m_gridSize;

        if (newPos != proxy->virtualPosition)
        {
            proxy->virtualPosition = newPos;
            proxy->actualPosition = proxy->virtualPosition * m_viewScale;
            proxy->element->updateActualPosition(proxy->actualPosition);
        }
    }
}

void VirtualArea::finishBlockMove(bool apply /*= true*/)
{
    if (apply)
    {
        InplaceArray<VirtualAreaElementPositionState, 256> oldPlacements, newPlacements;
        oldPlacements.reserve(m_selection.size());
        newPlacements.reserve(m_selection.size());

        for (auto* proxy : m_selection)
        {
            auto& oldPlacement = oldPlacements.emplaceBack();
            oldPlacement.elem = proxy->element;
            oldPlacement.position = proxy->savedVirtualPosition;

            auto& newPlacement = newPlacements.emplaceBack();
            newPlacement.elem = proxy->element;
            newPlacement.position = proxy->virtualPosition;
        }

        actionMoveElements(oldPlacements, newPlacements);
    }
    else
    {
        for (auto* proxy : m_selection)
        {
            proxy->virtualPosition = proxy->savedVirtualPosition;
            proxy->actualPosition = proxy->virtualPosition * m_viewScale;
            proxy->element->updateActualPosition(proxy->actualPosition);
        } 
    }
}

//--

END_BOOMER_NAMESPACE_EX(ui)
