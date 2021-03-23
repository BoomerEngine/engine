/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElement.h"
#include "uiStyleSelector.h"
#include "uiStyleLibrary.h"
#include "uiElementStyle.h"
#include "uiStyleValue.h"
#include "uiElementHitCache.h"
#include "uiRenderer.h"
#include "uiEventTable.h"
#include "uiTextLabel.h"
#include "uiInputAction.h"
#include "uiDragDrop.h"

#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/geometry.h"
#include "core/system/include/scopeLock.h"
#include "core/object/include/rttiHandleType.h"
#include "core/object/include/rttiProperty.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_CLASS(ElementClassNameMetadata);
RTTI_END_TYPE();

ElementClassNameMetadata::ElementClassNameMetadata()
    : m_name(nullptr)
{}

//---

RTTI_BEGIN_TYPE_CLASS(ElementBindingNameMetadata);
RTTI_END_TYPE();

ElementBindingNameMetadata::ElementBindingNameMetadata()
    : m_name(nullptr)
{}

ElementBindingNameMetadata::ElementBindingNameMetadata(const char* name)
    : m_name(name)
{}

//---

ElementDynamicSizing::ElementDynamicSizing()
    : m_numColumns(0)
    , m_columns(nullptr)
{}

//---

ElementCachedStyle::ElementCachedStyle()
{}

//---

ElementCachedGeometry::~ElementCachedGeometry()
{
	delete shadow;
	shadow = nullptr;

	delete background;
	background = nullptr;

	delete foreground;
	foreground = nullptr;

	delete overlay;
	overlay = nullptr;
}

//---

IClipboard::~IClipboard()
{}

void IClipboard::storeObject(const IObject* data)
{
    if (data)
        if (auto buffer = data->toBuffer())
            storeData(data->cls()->name().view(), buffer.data(), buffer.size());
}

ObjectPtr IClipboard::loadObject(ClassType expectedClass) const
{
    if (expectedClass)
    {
        const auto formatName = expectedClass->name().view();

        Buffer data;
        if (loadData(formatName, data))
            if (auto object = IObject::FromBuffer(data.data(), data.size()))
                return object;
    }

    return nullptr;
}

//--

namespace prv
{
    class LocalClipboard : public ISingleton, public IClipboard
    {
        DECLARE_SINGLETON(LocalClipboard);

        virtual void storeText(StringView text) override final
        {
            m_format = "TEXT";
            m_data.reset();
            m_text = StringBuf(text);
        }

        virtual bool loadText(StringBuf& outText) const override final
        {
            if (m_format == "TEXT")
            {
                outText = m_text;
                return true;
            }

            return false;
        }

        virtual void storeData(StringView format, const void* data, uint32_t size) override final
        {
            m_format = StringBuf(format);
            m_data = Buffer::Create(POOL_TEMP, size, 16, data);
            m_text = "";
        }

        virtual bool loadData(StringView format, Buffer& outData) const override final
        {
            if (m_format == format)
            {
                outData = m_data;
                return true;
            }

            return false;
        }

    private:
        Buffer m_data;
        StringBuf m_text;
        StringBuf m_format;

        virtual void deinit() override
        {
            m_data.reset();
            m_text.clear();
            m_format.clear();
        }
    };
}

IClipboard& IClipboard::LocalClipboard()
{
    return prv::LocalClipboard::GetInstance();
}

//---

RTTI_BEGIN_TYPE_CLASS(IElement);
    RTTI_PROPERTY(m_name);
    RTTI_METADATA(ElementClassNameMetadata).name("Element");
RTTI_END_TYPE();

IElement::IElement()
    : m_cachedLayoutValid(false)
    , m_cachedHoveredFlag(false)
    , m_cachedFocusFlag(false)
    , m_isTemplateInitCalled(false)
    , m_isFromTemplate(false)
    , m_dynamicLayoutScaling(false)
    , m_presistentElement(false)
    , m_allowFocusFromClick(true)
    , m_allowFocusFromKeyboard(false)
    , m_allowHover(true)
    , m_ignoreInAutomaticLayout(false)
    , m_providesDynamicSizingData(false)
    , m_enabled(true)
    , m_autoExpandX(false)
    , m_autoExpandY(false)
    , m_overlay(false)
    , m_renderOverlayElements(true)
{
}

IElement::~IElement()
{
    ASSERT_EX(m_renderer == nullptr, "Destroying UI element that is still attached to a window renderer");
    ASSERT_EX(m_listNext == nullptr, "Element destroyed while still being part of hierachy");
    ASSERT_EX(m_listPrev == nullptr, "Element destroyed while still being part of hierachy");

    m_childrenWithDirtyLayout.clear();
    m_childrenWithDirtyStyle.clear();

    for (auto it = childrenList(); it; ++it)
    {
        ASSERT_EX(!it->parentElement() || it->parentElement() == this, "Child was already removed from this parent");
        detachElementFromChildList(*it);
    }

    ASSERT_EX(m_firstChild == nullptr, "Element still contains children");
    ASSERT_EX(m_lastChild == nullptr, "Element still contains children");

    delete m_keyShortctuts;
    m_keyShortctuts = nullptr;

    delete m_customLocalStyles;
    m_customLocalStyles = nullptr;

    delete m_styleSelector;
    m_styleSelector = nullptr;

    delete m_eventTable;
    m_eventTable = nullptr;

    delete m_overlayElements;
    m_overlayElements = nullptr;

    delete m_cachedGeometry;
    m_cachedGeometry = nullptr;
}
    
void IElement::renderOverlayElements(bool flag)
{
    m_renderOverlayElements = flag;
}

void IElement::overlay(bool flag)
{
    if (m_overlay != flag)
    {
        m_overlay = flag;

        auto parent = parentElement();
        if (parent)
        {
            if (flag)
            {
                if (!parent->m_overlayElements)
                    parent->m_overlayElements = new Array<IElement*>;

                DEBUG_CHECK_EX(!parent->m_overlayElements->contains(this), "Parent already has this overlay element in it's overlay list");
                parent->m_overlayElements->pushBack(this);
            }
            else
            {
                DEBUG_CHECK_EX(parent->m_overlayElements, "Parent has no overlay list");
                if (parent->m_overlayElements)
                {
                    DEBUG_CHECK_EX(parent->m_overlayElements->contains(this), "Parent does not have this overlay element in it's overlay list");
                    parent->m_overlayElements->remove(this);

                    if (parent->m_overlayElements->empty())
                    {
                        delete parent->m_overlayElements;
                        parent->m_overlayElements = nullptr;
                    }
                }

            }

            parent->invalidateLayout();
            invalidateLayout();
        }
    }
}

void IElement::visibility(VisibilityState visibility)
{
    if (m_visibility != visibility)
    {
        m_visibility = visibility;
        invalidateLayout();
        invalidateGeometry();
    }
}

void IElement::name(StringID name)
{
    if (m_name != name)
    {
        m_name = name; 

        if (m_styleSelector)
            m_styleSelector->id(name);

        invalidateStyle();
    }
}

void IElement::expand()
{
    customVerticalAligment(ElementVerticalLayout::Expand);
    customHorizontalAligment(ElementHorizontalLayout::Expand);
}

void IElement::layoutMode(LayoutMode mode)
{
    if (m_layout != mode)
    {
        m_layout = mode;
        invalidateLayout();
    }
}

void IElement::customStyleVariant(StringID name, Variant value)
{
    DEBUG_CHECK_EX(name, "Invalid style name");
    DEBUG_CHECK_EX(value, "Invalid style value, use removeCustomStyle to remove a custom style");

    if (name && value)
    {
        if (nullptr == m_customLocalStyles)
            m_customLocalStyles = new VariantTable;

        if (const auto* existingValue = m_customLocalStyles->findVariant(name))
        {
            if (*existingValue == value)
                return;
        }

        m_customLocalStyles->setVariant(name, value);
        invalidateStyle();
        invalidateLayout();
        invalidateGeometry();
    }
}

void IElement::removeCustomStyle(StringID name)
{
    if (m_customLocalStyles)
    {
        if (m_customLocalStyles->remove(name))
        {
            if (m_customLocalStyles->empty())
            {
                delete m_customLocalStyles;
                m_customLocalStyles = nullptr;
            }

            invalidateLayout();
        }
    }
}


void IElement::customMinSize(float x, float y)
{
    if (x >= 0.0f)
        customStyle("min-width"_id, x);
    else
        removeCustomStyle("min-width"_id);

    if (y >= 0.0f)
        customStyle("min-height"_id, y);
    else
        removeCustomStyle("min-height"_id);
}

void IElement::customMinSize(const Size& minSize)
{
    customMinSize(minSize.x, minSize.y);
}

void IElement::customInitialSize(const Size& minSize)
{
    customInitialSize(minSize.x, minSize.y);
}

void IElement::customInitialSize(float x, float y)
{
    if (x > 0.0f)
        customStyle<float>("initial-width"_id, x);
    else
        removeCustomStyle("initial-width"_id);

    if (y > 0.0f)
        customStyle<float>("initial-height"_id, y);
    else
        removeCustomStyle("initial-height"_id);
}

void IElement::customMaxSize(float x, float y)
{
    if (x >= 0.0f)
        customStyle("max-width"_id, x);
    else
        removeCustomStyle("max-width"_id);

    if (y >= 0.0f)
        customStyle("max-height"_id, y);
    else
        removeCustomStyle("max-height"_id);
}

void IElement::customMaxSize(const Size& maxSize)
{
    customMaxSize(maxSize.x, maxSize.y);
}

void IElement::customMargins(float left, float top, float right, float bottom)
{
    if (left > 0.0f)
        customStyle("margin-left"_id, left);
    else
        removeCustomStyle("margin-left"_id);

    if (top > 0.0f)
        customStyle("margin-top"_id, top);
    else
        removeCustomStyle("margin-top"_id);

    if (right > 0.0f)
        customStyle("margin-right"_id, right);
    else
        removeCustomStyle("margin-right"_id);

    if (bottom > 0.0f)
        customStyle("margin-bottom"_id, bottom);
    else
        removeCustomStyle("margin-bottom"_id);
}

void IElement::customMargins(const Offsets& margins)
{
    customMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}

void IElement::customMargins(float all)
{
    customMargins(all, all, all, all);
}

void IElement::customPadding(float left, float top, float right, float bottom)
{
    if (left > 0.0f)
        customStyle("padding-left"_id, left);
    else
        removeCustomStyle("padding-left"_id);

    if (top > 0.0f)
        customStyle("padding-top"_id, top);
    else
        removeCustomStyle("padding-top"_id);

    if (right > 0.0f)
        customStyle("padding-right"_id, right);
    else
        removeCustomStyle("padding-right"_id);

    if (bottom > 0.0f)
        customStyle("padding-bottom"_id, bottom);
    else
        removeCustomStyle("padding-bottom"_id);
}

void IElement::customPadding(const Offsets& padding)
{
    customPadding(padding.left(), padding.top(), padding.right(), padding.bottom());
}

void IElement::customPadding(float all)
{
    customPadding(all, all, all, all);
}

void IElement::customVerticalAligment(ElementVerticalLayout verticalLayout)
{
    customStyle("vertical-align"_id, verticalLayout);
}

void IElement::customHorizontalAligment(ElementHorizontalLayout horizontalLayout)
{
    customStyle("horizontal-align"_id, horizontalLayout);
}

void IElement::customProportion(float proportion)
{
    if (proportion >= 0.0f)
        customStyle("proportion"_id, proportion);
    else
        removeCustomStyle("proportion"_id);
}

void IElement::customBackgroundColor(Color color)
{
    style::RenderStyle style;
    style.innerColor = style.outerColor = color;
    customStyle("background"_id, style);
}

void IElement::customForegroundColor(Color color)
{
    customStyle("color"_id, color);
}

bool IElement::hasStyleClass(StringID name) const
{
    return m_styleSelector ? m_styleSelector->hasClass(name) : false;
}

void IElement::addStyleClass(StringID name)
{
    if (auto* selector = styleSelector())
    {
        if (selector->addClass(name))
            invalidateStyle();
    }
}

void IElement::removeStyleClass(StringID name)
{
    if (m_styleSelector && m_styleSelector->removeClass(name))
        invalidateStyle();
}

bool IElement::hasStylePseudoClass(StringID name) const
{
    return m_styleSelector ? m_styleSelector->hasPseudoClass(name) : false;
}

void IElement::addStylePseudoClass(StringID name)
{
    if (auto* selector = styleSelector())
    {
        if (selector->addPseudoClass(name))
            invalidateStyle();
    }
}

void IElement::removeStylePseudoClass(StringID name)
{
    if (m_styleSelector && m_styleSelector->removePseudoClass(name))
        invalidateStyle();
}

StringID IElement::styleType() const
{
    return m_styleSelector ? m_styleSelector->customType() : StringID();
}

void IElement::styleType(StringID type)
{
    if (auto* selector = styleSelector())
    {
        if (selector->customType(type))
            invalidateStyle();
    }
}

//---

IElement* IElement::findParent(SpecificClassType<IElement> baseClass, StringID name /*= StringID::EMPTY()*/) const
{
    auto* ptr = parentElement();
    while (ptr)
    {
        if (!baseClass || ptr->cls()->is(baseClass))
        {
            if (!name || ptr->name() == name)
                return ptr;
        }

        ptr = ptr->parentElement();
    }

    return nullptr;
}

IElement* IElement::findChildByName(const char* name, SpecificClassType<IElement> baseClass, bool recrusive) const
{
    auto nameId = StringID::Find(name);
    if (!nameId)
        return nullptr;

    return findChildByName(nameId, baseClass, recrusive);
}

IElement* IElement::findChildByName(StringID name, SpecificClassType<IElement> baseClass, bool recrusive) const
{
    //TRACE_INFO("Visiting '{}' '{}'", cls()->name(), name());

    // look at local children
    for (auto* child = m_firstChild; child != nullptr; child = child->m_listNext)
    {
        if (name.empty() || child->m_name == name)
        {
            if (!baseClass || child->cls()->is(baseClass))
            {
                return child;
            }
        }
    }

    // look into the local children
    if (recrusive)
    {
        for (const auto* child = m_firstChild; child != nullptr; child = child->m_listNext)
        {
            auto ret = child->findChildByName(name, baseClass);
            if (ret)
                return ret;
        }
    }

    // not found
    return nullptr;
}

IElement* IElement::visitChildren(const std::function<const VisitResult(IElement* ptr)>& visitFunc) const
{
    for (auto* child = m_firstChild; child != nullptr; child = child->m_listNext)
    {
        auto ret = visitFunc(child);

        // return this item
        if (ret == VisitResult::Return)
            return child;

        // recurse to child items
        if (ret == VisitResult::Recurse)
        {
            if (auto childRet = child->visitChildren(visitFunc))
                return childRet;
        }
    }

    // nothing found
    return nullptr;
}

void IElement::unlinkFromChildrenList(IElement*& listHead, IElement*& listTail)
{
    if (m_listNext)
    {
        ASSERT(this != listTail);
        m_listNext->m_listPrev = m_listPrev;
    }
    else
    {
        ASSERT(this == listTail);
        listTail = m_listPrev;
    }

    if (m_listPrev)
    {
        ASSERT(this != listHead);
        m_listPrev->m_listNext = m_listNext;
    }
    else
    {
        ASSERT(this == listHead);
        listHead = m_listNext;
    }

    m_listNext = nullptr;
    m_listPrev = nullptr;
}

void IElement::linkToChildrenList(IElement*& listHead, IElement*& listTail)
{
    ASSERT_EX(m_listNext == nullptr && m_listPrev == nullptr, "Element already linked");

    if (listHead == nullptr)
    {
        m_listNext = nullptr;
        m_listPrev = nullptr;
        listHead = this;
        listTail = this;
    }
    else
    {
        ASSERT_EX(listTail->m_listNext == nullptr, "Tail element not last");
        m_listPrev = listTail;
        m_listNext = nullptr;
        listTail->m_listNext = this;
        listTail = this;
    }
}

//---

DragDropHandlerPtr IElement::handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition)
{
    return nullptr;
}

void IElement::handleDragDropGenericCompletion(const DragDropDataPtr& data, const Position& entryPosition)
{
    // nothing in the default item
}

InputActionPtr IElement::handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    return InputActionPtr();
}

InputActionPtr IElement::handleOverlayMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    return InputActionPtr();
}
    
bool IElement::handleMouseWheel(const InputMouseMovementEvent &evt, float delta)
{
    return false;
}

bool IElement::handleMouseMovement(const InputMouseMovementEvent& evt)
{
    return false;
}

bool IElement::handleAxisEvent(const InputAxisEvent& evt)
{
    return false;
}

bool IElement::previewKeyEvent(const InputKeyEvent& evt)
{
    return false;
}

bool IElement::handleKeyEvent(const InputKeyEvent& evt)
{
    // handle bounded key events
    if (evt.pressed())
    {
        if (m_keyShortctuts && m_keyShortctuts->processKeyEvent(evt))
            return true;

        if (evt.keyCode() == InputKey::KEY_TAB)
        {
            const auto back = evt.keyMask().isShiftDown();
            if (back)
            {
                if (auto* nextFocus = focusFindPrev())
                    nextFocus->focus();
            }
            else
            {
                if (auto* nextFocus = focusFindNext())
                    nextFocus->focus();
            }
            return true;
        }

        return false;
    }
    else if (evt.released())
    {
        return false;
    }

    return false;
}

bool IElement::handleExternalKeyEvent(const InputKeyEvent& evt)
{
    return false;
}

bool IElement::handleCharEvent(const InputCharEvent& evt)
{
    return false;
}

bool IElement::handleExternalCharEvent(const InputCharEvent& evt)
{
    return false;
}

bool IElement::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, CursorType& outCursorType) const
{
    return false;
}

bool IElement::handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, AreaType& outAreaType) const
{
    if (auto areaType = evalStyleValueIfPresentPtr<AreaType>("windowAreaType"_id))
    {
        outAreaType = *areaType;
        return true;
    }

    outAreaType = AreaType::Client;
    return true;
}

bool IElement::handleContextMenu(const ElementArea& area, const Position& absolutePosition, InputKeyMask controlKeys)
{
    // try local event
    if (call(EVENT_CONTEXT_MENU, absolutePosition))
        return true;

    // propagate to parent
    if (auto parent = parentElement())
        return parent->handleContextMenu(area, absolutePosition, controlKeys);

    // nothing
    return false;
}

bool IElement::handleHoverDuration(const Position& absolutePosition)
{
    return false;
}

void IElement::handleHoverEnter(const Position& absolutePosition)
{
    if (!m_cachedHoveredFlag && m_allowHover)
    {
        m_cachedHoveredFlag = true;
        addStylePseudoClass("hover"_id);
    }
}

void IElement::handleHoverLeave(const Position& absolutePosition)
{
    if (m_cachedHoveredFlag)
    {
        m_cachedHoveredFlag = false;
        removeStylePseudoClass("hover"_id);
    }
}

void IElement::handleFocusGained()
{
    if (!m_cachedFocusFlag && m_allowFocusFromKeyboard || m_allowFocusFromClick)
    {
        addStylePseudoClass("focused"_id);
        m_cachedFocusFlag = true;
    }
}

void IElement::handleFocusLost()
{
    if (m_cachedFocusFlag)
    {
        removeStylePseudoClass("focused"_id);
        m_cachedFocusFlag = false;
    }
}

IElement* IElement::focusFindFirst()
{
    if (isAllowingFocusFromKeyboard())
        return this;

    for (ElementChildIterator it(childrenList()); it; ++it)
    {
        if (auto* first = it->focusFindFirst())
            return first;
    }

    return nullptr;
}

IElement* IElement::focusFindNextSibling(IElement* childSelected)
{
    // find next focus item in local child list
    bool focusOnNext = false;
    for (ElementChildIterator it(childrenList()); it; ++it)
    {
        if (it == childrenList())
        {
            focusOnNext = true;
            continue;
        }
        else if (focusOnNext)
        {
            return it->focusFindFirst();
        }
    }

    // use our next item
    return focusFindNext();
}

IElement* IElement::focusFindNext()
{
    // there's no "next" sibling to focus, try in parent
    if (auto* parent = parentElement())
        return parent->focusFindNextSibling(this);

    // wow, we got to the end of the list, recurse back starting from first
    return focusFindFirst();
}

IElement* IElement::focusFindPrev()
{
    return nullptr;
}

IElement* IElement::focusFindPrevSibling(IElement* childSelected)
{
    return nullptr;
}
    
void IElement::handleChildrenChange()
{
    invalidateCachedChildrenPlacement();
    invalidateLayout();
}

void IElement::tooltip(StringView txt)
{
    customStyle("tooltip"_id, StringBuf(txt));
}

void IElement::ignoredInAutomaticLayout(bool flag)
{
    if (m_ignoreInAutomaticLayout != flag)
    {
        m_ignoreInAutomaticLayout = flag;
        invalidateLayout();
    }
}

void IElement::enable(bool isEnabled)
{
    if (m_enabled != isEnabled)
    {
        m_enabled = isEnabled;

        if (m_enabled)
            removeStylePseudoClass("disabled"_id);
        else
            addStylePseudoClass("disabled"_id);

        handleEnableStateChange(m_enabled);
    }
}

void IElement::dynamicSizingData(bool flag)
{
    if (m_providesDynamicSizingData != flag)
    {
        m_providesDynamicSizingData = flag;

        if (auto* parent = parentElement())
            parent->invalidateLayout();
    }
}

void IElement::handleEnableStateChange(bool isEnabled)
{
    // nothing here
}

bool IElement::handleTimer(StringID name, float elapsedTime)
{
    return false;
}

//--

template< typename T >
static bool ParseNumberArray(StringView str, Array<T>& outList)
{
    InplaceArray<StringView, 20> parts;
    str.slice(";", false, parts);

    for (const auto& part : parts)
    {
        int value = 0;
        if (MatchResult::OK != part.match(value))
            return false;

        outList.pushBack(value);
    }

    return true;
}

static bool ParseOffsets(StringView str, Offsets& outOffsets)
{
    InplaceArray<float, 8> values;
    if (!ParseNumberArray(str, values))
        return false;

    if (values.size() == 1)
    {
        outOffsets = Offsets(values[0]);
        return true;
    }
    else if (values.size() == 4)
    {
        outOffsets = Offsets(values[0], values[1], values[2], values[3]);
        return true;
    }

    return false;
}

static bool ParseSize(StringView str, Size& outSize)
{
    InplaceArray<float, 8> values;
    if (!ParseNumberArray(str, values))
        return false;

    if (values.size() == 1)
    {
        outSize = Size(values[0], values[0]);
        return true;
    }
    else if (values.size() == 2)
    {
        outSize = Size(values[0], values[1]);
        return true;
    }

    return false;
}

//---

void IElement::requestChildLayoutUpdate()
{
    if (auto parent = parentElement())
    {
        if (parent->m_childrenWithDirtyLayout.insert(this))
            parent->requestChildLayoutUpdate();
    }
}

void IElement::requestChildStyleUpdate()
{
    if (auto parent = parentElement())
    {
        if (parent->m_childrenWithDirtyStyle.insert(this))
            parent->requestChildStyleUpdate();
    }
}

void IElement::invalidateCachedChildrenPlacement()
{
    m_cachedChildrenPlacement.reset();
}

void IElement::invalidateLayout()
{
    m_cachedLayoutValid = false;
    invalidateCachedChildrenPlacement();
    requestChildLayoutUpdate();
}

void IElement::invalidateGeometry()
{
    m_cachedGeometryStyleHash = 0;
    m_cachedGeometrySize = Size(0, 0);

    // do not delete the cached geometry data - we may reuse the buffers
    /*delete m_cachedGeometry;
    m_cachedGeometry = nullptr;*/
}

style::SelectorMatchContext* IElement::styleSelector()
{
    if (m_styleSelector)
        return m_styleSelector;

    m_styleSelector = new style::SelectorMatchContext(name(), cls().cast<IElement>());
    return m_styleSelector;
}

void IElement::invalidateStyle()
{
    requestChildStyleUpdate();
}

//------

void IElement::removeAllChildren()
{
    for (auto it = childrenList(); it; ++it)
    {
        ASSERT_EX(it->parentElement() == this, "Child was already removed from this parent");
        if (!it->isPersistentChild())
            detachElementFromChildList(*it);
    }

    invalidateLayout();
}

void IElement::attachChild(IElement* childElement)
{
    attachElementToChildList(childElement);
}

void IElement::detachChild(IElement* childElement)
{
    detachElementFromChildList(childElement);
}

IDockTarget* IElement::queryDockTarget() const
{
    return nullptr;
}

Window* IElement::findParentWindow() const
{
    if (auto parent = parentElement())
        return parent->findParentWindow();
    return nullptr;
}

void IElement::initializeTemplateInstanced()
{
    // nothing
}

void IElement::callPostTemplateInitialization()
{
    if (!m_isTemplateInitCalled)
    {
        m_isTemplateInitCalled = true;
        initializeTemplateInstanced();

        for (auto child = childrenList(); child; ++child)
        {
            ASSERT_EX(child->parentElement() == this, "Child was already removed from this parent");
            child->callPostTemplateInitialization();
        }
    }
}

bool IElement::bindNamedProperties(const HashMap<StringView, IElement*>& namedElements)
{
    // resolve bindings
    bool valid = true;
    for (const auto* prop : cls()->allProperties())
    {
        // only properties
        const auto* propBindingName = prop->findMetadata<ElementBindingNameMetadata>();
        if (!propBindingName || !propBindingName->nameString())
            continue;

        // we can only bind to shared pointers
        if (prop->type()->metaType() != MetaType::StrongHandle)
        {
            valid = false;
            TRACE_WARNING("Auto binding property '{}' in '{}' request a SharedPtr type", prop->name(), cls()->name());
            continue;
        }

        // we can only bind to element classes
        const auto* handleType = static_cast<const IHandleType*>(prop->type().ptr());
        if (!handleType->pointedClass()->is(IElement::GetStaticClass()))
        {
            valid = false;
            TRACE_WARNING("Auto binding property '{}' in '{}' uses invalid handle type '{}'", prop->name(), cls()->name(), prop->type()->name());
            continue;
        }

        // is this a property that wants bindings ?
        //auto child = findChildByName(propBindingName->nameString(), handleClass);
        IElement* child = nullptr;;
        if (namedElements.find(propBindingName->nameString(), child))
        {
            if (!child)
            {
                valid = false;
                TRACE_WARNING("Auto binding property '{}' in '{}' references object '{}' that was not found", prop->name(), cls()->name(), propBindingName->nameString());
                continue;
            }

            // write
            auto* propData = prop->offsetPtr(this);
            handleType->writePointedObject(propData, RefPtr<IElement>(AddRef(child)));
            TRACE_INFO("Auto binding property '{}' in '{}' resolved as object '{}'", prop->name(), cls()->name(), propBindingName->nameString());
        }
    }

    return valid;
}

void IElement::attachElementToChildList(IElement* childElement)
{
    ASSERT(childElement);
    ASSERT(childElement->parentElement() == nullptr);

    // change parent
    ASSERT(childElement->m_listNext == nullptr);
    ASSERT(childElement->m_listPrev == nullptr);

    // link to children list
    childElement->parent(this);
    childElement->linkToChildrenList(m_firstChild, m_lastChild);

    // add reference to the child element
    childElement->addRef();

    // attach the child element to the window and command system
    childElement->bindNativeWindowRenderer(m_renderer);

    // refresh the layout and styles of the element
    m_childrenWithDirtyLayout.insert(childElement);
    m_childrenWithDirtyStyle.insert(childElement);

    // overlay child
    if (childElement->m_overlay)
    {
        if (!m_overlayElements)
            m_overlayElements = new Array<IElement*>;
        m_overlayElements->pushBack(childElement);
    }

    // remove all cached informations about child layout
    invalidateCachedChildrenPlacement();

    // request our update as well
    requestChildLayoutUpdate();
    requestChildStyleUpdate();

    // finally, notify whoever that we have changed the children list
    handleChildrenChange();      
}

void IElement::detachElementFromChildList(IElement *childElement)
{
    ASSERT(childElement);
    ASSERT(!childElement->parentElement() || childElement->parentElement() == this);

    childElement->bindNativeWindowRenderer(nullptr);

    m_childrenWithDirtyLayout.remove(childElement);
    m_childrenWithDirtyStyle.remove(childElement);
    ASSERT(!m_childrenWithDirtyStyle.contains(childElement));
    ASSERT(!m_childrenWithDirtyLayout.contains(childElement));

    // remove from list
    childElement->parent(nullptr);
    childElement->unlinkFromChildrenList(m_firstChild, m_lastChild);
    ASSERT(childElement->m_listNext == nullptr);
    ASSERT(childElement->m_listPrev == nullptr);

    // remove from internal dirty lists
    ASSERT(!m_childrenWithDirtyStyle.contains(childElement));
    ASSERT(!m_childrenWithDirtyLayout.contains(childElement));

    // overlay child
    if (childElement->m_overlay)
    {
        if (m_overlayElements)
        {
            m_overlayElements->remove(childElement);
            if (m_overlayElements->empty())
            {
                delete m_overlayElements;
                m_overlayElements = nullptr;
            }
        }
    }

    // release previous reference
    // NOTE: this may delete the element
    childElement->releaseRef();

    // finally, notify whoever that we have changed the children list
    handleChildrenChange();
}

void IElement::bindNativeWindowRenderer(Renderer* windowRenderer)
{
    if (m_renderer != windowRenderer)
    {
        if (m_renderer)
        {
            for (auto* cur = m_timers; cur; cur = cur->m_next)
                cur->detach();
        }

        m_renderer = windowRenderer;

        for (auto child = childrenList(); child; ++child)
        {
            ASSERT_EX(child->parentElement() == this, "Child was already removed from this parent");
            child->bindNativeWindowRenderer(windowRenderer);
        }

        if (windowRenderer)
        {
            for (auto* cur = m_timers; cur; cur = cur->m_next)
                cur->attach();
        }
    }
}

uint64_t IElement::windowNativeHandle() const
{
    if (renderer())
        if (auto window = findParentWindow())
            return window->renderer()->queryWindowNativeHandle(window);

    return 0;
}

void IElement::focus()
{
    if (m_renderer)// && m_hitTest == HitTestState::Enabled)
        m_renderer->requestFocus(this);
}

//------

std::atomic<uint32_t> GStatPrepareStyle = 0;

void IElement::adjustCustomOverlayElementsPixelScale(float& scale) const
{

}

bool IElement::adjustBackgroundStyle(canvas::RenderStyle& outStyle, float& outBorderWidth) const
{
    return false;
}

bool IElement::adjustBorderStyle(canvas::RenderStyle& outStyle, float& outBorderWidth) const
{
    return false;
}

void IElement::prepareStyle(StyleStack& stack, float pixelScale, bool allChildren)
{
    if (parentElement() != nullptr)
        GStatPrepareStyle++;

    // make sure we have a valid style selector
    const auto* selectorPtr = styleSelector();

    // push this element on the stack
    stack.push(selectorPtr);

    // pixel scale for children
    float overlayChildrenPixelScale = pixelScale;
    if (!m_renderOverlayElements)
        adjustCustomOverlayElementsPixelScale(overlayChildrenPixelScale);

    // if the style is changing it's mostly will change for ALL children
    auto pixelScaleChanged = (m_cachedStyle.pixelScale != pixelScale);
    const auto styleSelectorChanged = (m_cachedStyle.selectorKey != stack.selectorKey()) || pixelScaleChanged;
    if (allChildren || styleSelectorChanged)
    {
        m_childrenWithDirtyStyle.reset();
        for (auto child = childrenList(); child; ++child)
        {
            ASSERT_EX(child->parentElement() == this, "Child was already removed from this parent");
            child->prepareStyle(stack, child->isOverlay() ? overlayChildrenPixelScale : pixelScale, true);
        }
    }
    else
    {
        // only update dirty children
        for (auto *child : m_childrenWithDirtyStyle.keys())
        {
            ASSERT_EX(child->parent() == this, "Child was already removed from this parent");
            child->prepareStyle(stack, child->isOverlay() ? overlayChildrenPixelScale : pixelScale, allChildren);
        }
        m_childrenWithDirtyStyle.reset();
    }

    // generate the parameter table for this element
    if (styleSelectorChanged || allChildren)
    {
        m_cachedStyle.pixelScale = pixelScale;
        m_cachedStyle.selectorKey = stack.selectorKey();
        if (stack.buildTable(m_cachedStyle.paramsKey, m_cachedStyle.params) || pixelScaleChanged)
        {
            m_cachedStyle.opacity = evalStyleValue<float>("opacity"_id, 1.0f);
            invalidateLayout();
            invalidateGeometry();
        }
    }

    // remove ourselves from stack
    stack.pop(selectorPtr);

    // make sure we have a valid styles in the end
    DEBUG_CHECK(m_cachedStyle.params);
}

std::atomic<uint32_t> GStatComputeLayout = 0;
       
void IElement::computeLayout(ElementLayout& outLayout)
{
    GStatComputeLayout++;

    // extract padding
    {
        float left = evalStyleValue<float>("padding-left"_id) * m_cachedStyle.pixelScale;
        float top = evalStyleValue<float>("padding-top"_id) * m_cachedStyle.pixelScale;
        float right = evalStyleValue<float>("padding-right"_id) * m_cachedStyle.pixelScale;
        float bottom = evalStyleValue<float>("padding-bottom"_id) * m_cachedStyle.pixelScale;
        outLayout.m_padding = Offsets(left, top, right, bottom);
    }

    // extract margin
    {
        float left = evalStyleValue<float>("margin-left"_id) * m_cachedStyle.pixelScale;
        float top = evalStyleValue<float>("margin-top"_id) * m_cachedStyle.pixelScale;
        float right = evalStyleValue<float>("margin-right"_id) * m_cachedStyle.pixelScale;
        float bottom = evalStyleValue<float>("margin-bottom"_id) * m_cachedStyle.pixelScale;
        outLayout.m_margin = Offsets(left, top, right, bottom);
    }

    // default alignment depends on the element "auto expand" feature
    auto defaultHorizontalAlignment = m_autoExpandX ? ElementHorizontalLayout::Expand : ElementHorizontalLayout::Left;
    auto defaultVerticalAlignment = m_autoExpandY ? ElementVerticalLayout::Expand : ElementVerticalLayout::Top;

    // extract alignment from styles
    outLayout.m_verticalAlignment = evalStyleValue("vertical-align"_id, defaultVerticalAlignment);
    outLayout.m_horizontalAlignment = evalStyleValue("horizontal-align"_id, defaultHorizontalAlignment);

    // extract inner alignment
    outLayout.m_internalHorizontalAlignment = evalStyleValue("inner-horizontal-align"_id, ElementHorizontalLayout::Left);
    outLayout.m_internalVerticalAlignment = evalStyleValue("inner-vertical-align"_id, ElementVerticalLayout::Top);

    ElementVerticalLayout m_internalVerticalAlignment = ElementVerticalLayout::Top;
    ElementHorizontalLayout m_internalHorizontalAlignment = ElementHorizontalLayout::Left;

    // HACK - force expand the column containers
    if (m_layout == LayoutMode::Columns)
        outLayout.m_horizontalAlignment = ElementHorizontalLayout::Expand;

    outLayout.m_relative.x = evalStyleValue("relative-x"_id, 0.0f) * m_cachedStyle.pixelScale;
    outLayout.m_relative.y = evalStyleValue("relative-x"_id, 0.0f) * m_cachedStyle.pixelScale;

    // extract proportion information for sizers
    outLayout.m_proportion = evalStyleValue("proportion"_id, 0.0f);

    // extract basic size
    computeSize(outLayout.m_innerSize);

    // override the size with the style
    {
        float val = 0.0;

        // limit X size
        if (evalStyleValueIfPresent("width"_id, val))
        {
            outLayout.m_innerSize.x = val * m_cachedStyle.pixelScale;
        }
        else
        {
            auto minSize = evalStyleValue<float>("min-width"_id, 0.0f) * m_cachedStyle.pixelScale;
            if (outLayout.m_innerSize.x < minSize)
            {
                outLayout.m_innerSize.x = minSize;
            }
            else
            {
                auto maxSize = evalStyleValue<float>("max-width"_id, FLT_MAX) * m_cachedStyle.pixelScale;
                if (outLayout.m_innerSize.x > maxSize)
                    outLayout.m_innerSize.x = maxSize;
            }
        }

        // absolute limit X size
        {
            auto minAbsoluteSize = evalStyleValue<float>("min-absolute-width"_id, 0.0f); // NOT DPI scaled
            auto maxAbsoluteSize = evalStyleValue<float>("max-absolute-width"_id, FLT_MAX); // NOT DPI scaled
            outLayout.m_innerSize.x = std::clamp<float>(outLayout.m_innerSize.x, minAbsoluteSize, maxAbsoluteSize);
        }

        // limit Y size
        if (evalStyleValueIfPresent("height"_id, val))
        {
            outLayout.m_innerSize.y = val * m_cachedStyle.pixelScale;
        }
        else
        {
            auto minSize = evalStyleValue<float>("min-height"_id, 0.0f) * m_cachedStyle.pixelScale;
            if (outLayout.m_innerSize.y < minSize)
            {
                outLayout.m_innerSize.y = minSize;
            }
            else
            {
                auto maxSize = evalStyleValue<float>("max-height"_id, FLT_MAX) * m_cachedStyle.pixelScale;
                if (outLayout.m_innerSize.y > maxSize)
                    outLayout.m_innerSize.y = maxSize;
            }
        }

        // absolute limit Y size
        {
            auto minAbsoluteSize = evalStyleValue<float>("min-absolute-height"_id, 0.0f); // NOT DPI scaled
            auto maxAbsoluteSize = evalStyleValue<float>("max-absolute-height"_id, FLT_MAX); // NOT DPI scaled
            outLayout.m_innerSize.y = std::clamp<float>(outLayout.m_innerSize.y, minAbsoluteSize, maxAbsoluteSize);
        }
    }

    // TODO: border
}

void IElement::prepareLayout(bool& outLayoutRecomputed, bool force, bool initial)
{
    ASSERT(m_cachedStyle.params);
    //ASSERT_EX(cls()->is<Window>() || m_weakParent != nullptr, "Recursive method called without a parent");

    // refresh visibility state, do not update layout if we are not visible
    if (m_visibility == VisibilityState::Hidden)
        return;

    // prepare layouts of children first
    bool childLayoutRecomputed = false;
    if (force)
    {
        m_childrenWithDirtyLayout.reset();
        for (auto child = childrenList(); child; ++child)
        {
            ASSERT_EX(child->parentElement() == this, "Child was already removed from this parent");
            child->prepareLayout(childLayoutRecomputed, force, initial);
        }
    }
    else
    {
        for (auto* child : m_childrenWithDirtyLayout.keys())
        {
            ASSERT_EX(child->parent() == this, "Child was already removed from this parent");
            child->prepareLayout(childLayoutRecomputed, force, initial);
        }
        m_childrenWithDirtyLayout.reset();
    }

    // extract basic information from style
    if (!m_cachedLayoutValid || childLayoutRecomputed || force)
    {
        ElementLayout layout;
        computeLayout(layout);

        if (initial)
        {
            if (auto initialWidth = evalStyleValueIfPresentPtr<float>("initial-width"_id))
                layout.m_innerSize.x = *initialWidth;
            if (auto initialHeight = evalStyleValueIfPresentPtr<float>("initial-height"_id))
                layout.m_innerSize.y = *initialHeight;
        }

        m_cachedLayout = layout;
        m_cachedLayoutValid = true;
        outLayoutRecomputed = true;
    }
}

void IElement::prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const
{
    // draw the dimming rect as big as the clip area
    if (auto fadeoutPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("fadeout"_id))
    {
        ElementArea drawRect(-10000.0f, -10000.0f, 20000.0f, 20000.0f);
        auto style = fadeoutPtr->evaluate(stash, pixelScale, drawRect);

        builder.beginPath();
        builder.fillPaint(style);
        builder.rect(drawRect.left(), drawRect.top(), drawRect.size().x, drawRect.size().y);
        builder.fill();
    }

    // draw the shadow
    if (auto shadowPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("shadow"_id))
    {
        float margin = evalStyleValue<float>("shadow-margin"_id, 0.0f) * pixelScale;
        float padding = evalStyleValue<float>("shadow-padding"_id, 0.0f) * pixelScale;
        if (padding > 0.0f)
        {
            auto shadowArea = drawArea.extend(padding, padding, padding, padding);
            auto style = shadowPtr->evaluate(stash, pixelScale, drawArea);

            builder.beginPath();
            prepareBoundaryGeometry(stash, shadowArea, pixelScale, builder, -margin);
            builder.fillPaint(style);
            builder.fill();
        }
    }
}

void IElement::prepareBoundaryGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder, float inset) const
{
    auto ox = drawArea.absolutePosition().x + inset;
    auto oy = drawArea.absolutePosition().y + inset;
    auto sx = drawArea.size().x - inset * 2.0f;
    auto sy = drawArea.size().y - inset * 2.0f;

    float borderRadius = evalStyleValue<float>("border-radius"_id, 0.0f) * pixelScale;
    if (borderRadius > 0.0f)
        builder.roundedRect(ox, oy, sx, sy, borderRadius);
    else
        builder.rect(ox, oy, sx, sy);

    // TODO: slanted rect, circle, etc
}

void IElement::prepareBackgroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const
{
    if (auto shadowPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("background"_id))
    {
        float borderWidth = evalStyleValue<float>("border-width"_id, 0.0f);// *pixelScale;

        auto style = shadowPtr->evaluate(stash, pixelScale, drawArea);
        adjustBackgroundStyle(style, borderWidth);

        if (style.innerColor.a > 0 || style.outerColor.a > 0)
        {
            builder.beginPath();
            prepareBoundaryGeometry(stash, drawArea, pixelScale, builder, borderWidth * 0.5f); // inset the fill by half the border size to help with AA
            builder.fillPaint(style);
            builder.fill();
        }
    }
}

void IElement::prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const
{

}

void IElement::prepareOverlayGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const
{
    if (auto borderStylePtr = evalStyleValueIfPresentPtr<Color>("border-color"_id))
    {
        if (borderStylePtr->a > 0)
        {
            float borderWidth = evalStyleValue<float>("border-width"_id, 0.0f);// *pixelScale;
            if (borderWidth > 0.0f)
            {
                auto style = canvas::SolidColor(*borderStylePtr);
                adjustBorderStyle(style, borderWidth);

                builder.beginPath();
                prepareBoundaryGeometry(stash, drawArea, pixelScale, builder, borderWidth * 0.5f);
                builder.strokePaint(style, borderWidth);
                builder.stroke();
            }
            else
            {
                auto borderLeft = evalStyleValue<float>("border-left"_id, 0.0f);// *pixelScale;
                auto borderTop = evalStyleValue<float>("border-top"_id, 0.0f);// *pixelScale;
                auto borderRight = evalStyleValue<float>("border-right"_id, 0.0f);// *pixelScale;
                auto borderBottom = evalStyleValue<float>("border-bottom"_id, 0.0f);// *pixelScale;
                if (borderLeft > 0.0f || borderTop > 0.0f || borderRight > 0.0f || borderBottom > 0.0f)
                {
                    auto style = canvas::SolidColor(*borderStylePtr);
                    adjustBorderStyle(style, borderWidth);

                    if (borderLeft > 0.0f)
                    {
                        builder.beginPath();
                        builder.moveTo(drawArea.left() + (borderLeft/2.0f), drawArea.top());
                        builder.lineTo(drawArea.left() + (borderLeft / 2.0f), drawArea.bottom());
						builder.strokePaint(style, borderLeft);
                        builder.stroke();
                    }

                    if (borderTop > 0.0f)
                    {
                        builder.beginPath();
                        builder.moveTo(drawArea.left(), drawArea.top() + (borderTop/2.0f));
                        builder.lineTo(drawArea.right(), drawArea.top() + (borderTop / 2.0f));
						builder.strokePaint(style, borderTop);
                        builder.stroke();
                    }

                    if (borderRight > 0.0f)
                    {
                        builder.beginPath();
                        builder.moveTo(drawArea.right() - (borderRight/2.0f), drawArea.top());
                        builder.lineTo(drawArea.right() - (borderRight / 2.0f), drawArea.bottom());
						builder.strokePaint(style, borderRight);
                        builder.stroke();
                    }

                    if (borderBottom > 0.0f)
                    {
                        builder.beginPath();
                        builder.moveTo(drawArea.left(), drawArea.bottom() - (borderBottom/2.0f));
                        builder.lineTo(drawArea.right(), drawArea.bottom() - (borderBottom / 2.0f));
						builder.strokePaint(style, borderBottom);
                        builder.stroke();
                    }
                }
            }
        }
    }

    if (auto overlayStylePtr = evalStyleValueIfPresentPtr<style::RenderStyle>("overlay"_id))
    {
        auto style = overlayStylePtr->evaluate(stash, pixelScale, drawArea);

        builder.beginPath();
        prepareBoundaryGeometry(stash, drawArea, pixelScale, builder, 0.0f);
        builder.fillPaint(style);
        builder.fill();
    }
}

void IElement::renderShadow(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    if (m_cachedGeometry && m_cachedGeometry->shadow)
        canvas.place(drawArea.absolutePosition(), *m_cachedGeometry->shadow, mergedOpacity);
}

void IElement::renderForeground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    if (m_cachedGeometry && m_cachedGeometry->foreground)
		canvas.place(drawArea.absolutePosition(), *m_cachedGeometry->foreground, mergedOpacity);
}

void IElement::renderBackground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    if (m_cachedGeometry && m_cachedGeometry->background)
		canvas.place(drawArea.absolutePosition(), *m_cachedGeometry->background, mergedOpacity);
}

void IElement::renderOverlay(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    if (m_cachedGeometry && m_cachedGeometry->overlay)
		canvas.place(drawArea.absolutePosition(), *m_cachedGeometry->overlay, mergedOpacity);
}

std::atomic<uint32_t> GStatGeometryRebuild = 0;

void IElement::rebuildCachedGeometry(DataStash& stash, const ElementArea& drawArea)
{
    GStatGeometryRebuild += 1;

	if (!m_cachedGeometry)
		m_cachedGeometry = new ElementCachedGeometry();

	if (!m_cachedGeometry->shadow)
		m_cachedGeometry->shadow = new canvas::Geometry();

	if (!m_cachedGeometry->background)
		m_cachedGeometry->background = new canvas::Geometry;

	if (!m_cachedGeometry->foreground)
		m_cachedGeometry->foreground = new canvas::Geometry;

	if (!m_cachedGeometry->overlay)
		m_cachedGeometry->overlay = new canvas::Geometry;

	m_cachedGeometry->shadow->reset();
	m_cachedGeometry->background->reset();
	m_cachedGeometry->foreground->reset();
	m_cachedGeometry->overlay->reset();

    canvas::GeometryBuilder ShadowGeometry(*m_cachedGeometry->shadow);
    canvas::GeometryBuilder BackgroundGeometry(*m_cachedGeometry->background);
    canvas::GeometryBuilder ForegroundGeometry(*m_cachedGeometry->foreground);
    canvas::GeometryBuilder OverlayGeometry(*m_cachedGeometry->overlay);

    // prepare shadow geometry
    prepareShadowGeometry(stash, drawArea, cachedStyleParams().pixelScale, ShadowGeometry);
    prepareBackgroundGeometry(stash, drawArea, cachedStyleParams().pixelScale, BackgroundGeometry);
    prepareForegroundGeometry(stash, drawArea, cachedStyleParams().pixelScale, ForegroundGeometry);
    prepareOverlayGeometry(stash, drawArea, cachedStyleParams().pixelScale, OverlayGeometry);

    /*// create the cached geometry container only if we have geometry (many layout items don't)
    if (ShadowGeometry.empty() && BackgroundGeometry.empty() && ForegroundGeometry.empty() && OverlayGeometry.empty())
    {
        // remove container
        delete m_cachedGeometry;
        m_cachedGeometry = nullptr;
    }*/

    // update cache params
    m_cachedGeometrySize = drawArea.size(); // WE ASSUME that the geometry only depends on size
    m_cachedGeometryStyleHash = m_cachedStyle.paramsKey; 
}

void IElement::prepareCachedGeometry(DataStash& stash, const ElementArea& drawArea)
{
    auto fetchNewGeometry = (m_cachedGeometryStyleHash != m_cachedStyle.paramsKey) || (m_cachedGeometrySize != drawArea.size());
    if (fetchNewGeometry)
        rebuildCachedGeometry(stash, drawArea.resetOffset());
}

void IElement::prepareDynamicSizing(const ElementArea& drawArea, const ElementDynamicSizing*& dataPtr) const
{
}

void IElement::renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float mergedOpacity)
{

}

void IElement::render(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float parentMergedOpacity, const ElementDynamicSizing* parentDynamicSizing)
{
    ASSERT(m_visibility != VisibilityState::Hidden);

    // calculate opacity for this element and following elements
    auto mergedOpacity = m_opacity * parentMergedOpacity * m_cachedStyle.opacity;
    if (mergedOpacity <= 0.0f)
        return;

    // get draw area for this element
    auto drawArea = m_cachedLayout.calcDrawAreaFromOuterArea(outerArea);
    m_cachedDrawArea = drawArea;

    // don't draw if we are outside the clip area
    auto clipArea = drawArea.clipTo(outerClipArea);
    if (clipArea.empty())
        return;

    // register in the hit test area
    // NOTE: margins of the element are NOT registered for hit testing
    if (m_hitTest == HitTestState::Enabled)
    {
        auto clippedDrawArea = drawArea.clipTo(outerClipArea);
        hitCache.storeElement(clippedDrawArea, this);
    }
    else if (m_hitTest == HitTestState::DisabledTree)
    {
        hitCache.disableHitTestCollection();
    }

    // prepare cached geometry
    prepareCachedGeometry(stash, drawArea);

    // draw element shadows - NOTE - drawing with parent clipping rect
    renderShadow(stash, drawArea, canvas, mergedOpacity);

    // clip when rendering the background/foreground
    canvas.pushScissorRect();
    if (canvas.scissorRect(clipArea.left(), clipArea.top(), clipArea.size().x, clipArea.size().y))
    {
        // draw all background geometry
        renderBackground(stash, drawArea, canvas, mergedOpacity);

        // draw all foreground geometry
        renderForeground(stash, drawArea, canvas, mergedOpacity);

        // draw children
        {
            // get inner area for children layout
            auto innerArea = m_cachedLayout.calcInnerAreaFromDrawArea(drawArea);

            // arrange children
            ArrangedChildren arrangedChildren;
            arrangeChildren(innerArea, clipArea, arrangedChildren, parentDynamicSizing);
            if (!arrangedChildren.empty())
            {
                // TODO: sort by z-order

                // draw shadows + elements
                for (const auto& child : arrangedChildren)
                {
                    // filter interactions
                    ASSERT_EX(child.m_element->parentElement() == this, "Trying to draw elements that is not parented here");
                    child.m_element->render(hitCache, stash, child.m_drawArea, child.m_clipArea, canvas, mergedOpacity, parentDynamicSizing);

                    // prepare sizing data
                    if (child.m_element->isProvidingDynamicSizingData())
                        child.m_element->prepareDynamicSizing(child.m_drawArea, parentDynamicSizing);
                }
            }
        }

        // custom overlay
        if (!m_renderOverlayElements)
            renderCustomOverlayElements(hitCache, stash, drawArea, clipArea, canvas, mergedOpacity);
    }

    // restore clipping rect to parent one
    canvas.popScissorRect();

    // draw overlay - NOTE - drawing with parent clipping rect
    renderOverlay(stash, drawArea, canvas, mergedOpacity);

    // reenable disabled child hit test
    if (m_hitTest == HitTestState::DisabledTree)
        hitCache.enableHitTestCollection();
}

StringBuf IElement::queryTooltipString() const
{
    if (auto tooltipStringPtr = evalStyleValueIfPresentPtr<StringBuf>("tooltip"_id))
        return *tooltipStringPtr;

    return "";
}

ElementPtr IElement::queryTooltipElement(const Position& absolutePosition, ElementArea& outTooltipArea) const
{   
    if (const auto tooltipText = queryTooltipString())
    {    
        outTooltipArea = cachedDrawArea();
        return RefNew<TextLabel>(tooltipText);
    }

    return nullptr;
}

DragDropDataPtr IElement::queryDragDropData(const BaseKeyFlags& keys, const Position& position) const
{
    return nullptr;
}

//--

Timer::Timer(IElement* owner, StringID name /*= StringID::EMPTY()*/)
    : m_owner(owner)
    , m_name(name)
{
    link();
}

Timer::Timer(IElement* owner, float timeInterval, bool oneShot /*= false*/, StringID name /*= StringID::EMPTY()*/)
    : m_owner(owner)
    , m_name(name)
{
    m_scheduleTime.resetToNow();
    m_nextTick = m_scheduleTime + (double)timeInterval;
    m_tickInterval = oneShot ? NativeTimeInterval() : NativeTimeInterval(timeInterval);

    link();
}

Timer::~Timer()
{
    stop();
    unlink();
}

void Timer::link()
{
    if (m_owner)
    {
        m_next = m_owner->m_timers;
        m_owner->m_timers = this;

        attach();
    }
}

void Timer::unlink()
{
    if (m_owner)
    {
        detach();

        auto** prev = &m_owner->m_timers;
        while (*prev != nullptr)
        {
            if (*prev == this)
            {
                *prev = m_next;
                break;
            }
            prev = &((*prev)->m_next);
        }

        m_next = nullptr;
    }
}

void Timer::detach()
{
    if (m_renderer)
    {
        m_renderer->detachTimer(m_owner, this);
        m_renderer = nullptr;
    }
}

void Timer::attach()
{
    if (m_renderer)
        detach();

    if (m_owner && m_owner->renderer() && scheduled())
    {
        m_renderer = m_owner->renderer();
        m_renderer->attachTimer(m_owner, this);
    }
}
    
void Timer::stop()
{
    detach();
    
    m_tickInterval = NativeTimeInterval();
    m_scheduleTime = NativeTimePoint();
    m_nextTick = NativeTimePoint();
}

void Timer::startOneShot(float delay)
{
    m_tickInterval = NativeTimeInterval();
    m_scheduleTime.resetToNow();
    m_nextTick = m_scheduleTime + (double)delay;

    attach();
}

void Timer::startRepeated(float interval)
{
    m_tickInterval = NativeTimeInterval(interval);
    m_scheduleTime.resetToNow();
    m_nextTick = m_scheduleTime + (double)interval;

    attach();
}

bool Timer::running() const
{
    return m_nextTick.valid() && m_renderer;
}

bool Timer::scheduled() const
{
    return m_nextTick.valid();
}

NativeTimePoint Timer::nextTick() const
{
    return m_nextTick;
}

bool Timer::call()
{
    auto func = m_func;

    // detach one shot timer before calling stop
    float timeElapsed = m_scheduleTime.timeTillNow().toSeconds();
    if (m_tickInterval.isZero())
    {
        stop();
    }
    else
    {
        m_scheduleTime.resetToNow();
        m_nextTick = m_scheduleTime + m_tickInterval;
    }

    // call the timer function
    if (func && func(m_name ? m_name : "OnTimer"_id, m_owner, m_owner, CreateVariant(timeElapsed)))
        return true;

    // if we have owning element than maybe it it interested
    if (m_owner)
        return m_owner->handleTimer(m_name, timeElapsed);

    // no callback
    return false;
}

//--

EventFunctionBinder IElement::bind(StringID name, IElement* owner/* = nullptr*/)
{
    if (!m_eventTable)
        m_eventTable = new EventTable;
    return m_eventTable->bind(name, owner);
}

void IElement::unbind(StringID name, IElement* owner)
{
    if (m_eventTable)
        m_eventTable->unbind(name, owner);
}

bool IElement::callGeneric(IElement* originator, StringID name, const Variant& data)
{
    if (m_eventTable)
        return m_eventTable->callGeneric(name, originator, data);
    return false;
}

//--

KeyShortcutTable& IElement::shortcuts()
{
    if (!m_keyShortctuts)
        m_keyShortctuts = new KeyShortcutTable(this);
    return *m_keyShortctuts;
}

const KeyShortcutTable& IElement::shortcuts() const
{
    static KeyShortcutTable theEmptyTable(nullptr);
    return m_keyShortctuts ? *m_keyShortctuts : theEmptyTable;
}

EventFunctionBinder IElement::bindShortcut(KeyShortcut key)
{
    return shortcuts().bindShortcut(key);
}

//--

void IElement::InvalidateAllDataEverywhere()
{
    // TODO
}

void IElement::configLoad(const ConfigBlock& block)
{}

void IElement::configSave(const ConfigBlock& block) const
{}

//--

IClipboard& IElement::clipboard() const
{
    return m_renderer ? *m_renderer : IClipboard::LocalClipboard();
}

//--

void PrintElementStats()
{
    auto numStyleUpdate = GStatPrepareStyle.exchange(0);
    auto numLayoutUpdate = GStatComputeLayout.exchange(0);
    auto numGeometryUpdate = GStatGeometryRebuild.exchange(0);
    if (numStyleUpdate || numLayoutUpdate || numGeometryUpdate)
    {
        //TRACE_INFO("UI Update: {} style, {} layout, {} geom", numStyleUpdate, numLayoutUpdate, numGeometryUpdate);
    }
}

END_BOOMER_NAMESPACE_EX(ui)
