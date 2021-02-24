/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiNotebook.h"
#include "uiTextLabel.h"
#include "uiScrollArea.h"
#include "uiButton.h"

BEGIN_BOOMER_NAMESPACE(ui)

//---

RTTI_BEGIN_TYPE_CLASS(Notebook);
    RTTI_METADATA(ElementClassNameMetadata).name("Notebook");
RTTI_END_TYPE();

Notebook::Notebook(NotebookFlags flags)
    : m_flags(flags)
{
    layoutVertical();
    enableAutoExpand(true, true);

    auto outsideArea = createInternalChildWithType<>("NotebookHeaderArea"_id);
    outsideArea->layoutHorizontal();
    outsideArea->customHorizontalAligment(ElementHorizontalLayout::Expand);
    outsideArea->customVerticalAligment(ElementVerticalLayout::Bottom);
    m_headerTotalArea = outsideArea;

    auto header = outsideArea->createChildWithType<ScrollArea>("NotebookHeader"_id, ScrollMode::None, ScrollMode::Hidden);// TODO: Auto);
    header->layoutHorizontal();
    header->customHorizontalAligment(ElementHorizontalLayout::Expand);
    header->customVerticalAligment(ElementVerticalLayout::Middle);
    m_headerScrollArea = header;
    //header->customProportion(1.0f);

    if (m_flags.test(NotebookFlagBit::TabListPopupButton))
    {
        m_headerTotalArea = header->createChildWithType<Button>("NotebookHeaderPopupButton"_id, ButtonModeBit::EventOnClick);
        m_headerTotalArea->customHorizontalAligment(ElementHorizontalLayout::Left);
        m_headerTotalArea->customVerticalAligment(ElementVerticalLayout::Bottom);
    }

    m_header = header->createChildWithType<>("NotebookHeaderBar"_id);
    m_header->layoutHorizontal();
    m_header->customHorizontalAligment(ElementHorizontalLayout::Left);
    m_header->customVerticalAligment(ElementVerticalLayout::Bottom);

    m_container = createInternalChildWithType<>("NotebookContainer"_id);
    m_container->customHorizontalAligment(ElementHorizontalLayout::Expand);
    m_container->customVerticalAligment(ElementVerticalLayout::Expand);
}

Notebook::~Notebook()
{}

void Notebook::attachHeaderElement(IElement* element)
{
    DEBUG_CHECK_RETURN_EX(element, "Invalid element");
    DEBUG_CHECK_RETURN_EX(element->parent() == nullptr, "Element already part of a hierarchy");
    DEBUG_CHECK_RETURN_EX(!m_customHeaderElements.contains(element), "Element already in header");

    m_customHeaderElements.pushBack(element);
    m_headerTotalArea->attachChild(element);
}

void Notebook::dettachHeaderElement(IElement* element)
{
    if (m_customHeaderElements.remove(element))
    {
        m_headerTotalArea->detachChild(element);
    }
}

void Notebook::removeAllTabs()
{
    m_activeTab = nullptr;

    for (int i = m_tabs.lastValidIndex(); i >= 0; --i)
    {
        m_container->detachChild(m_tabs[i]);
        m_header->detachChild(m_buttons[i]);
    }

    m_tabs.clear();
    m_buttons.clear();
}

void Notebook::tab(IElement* tab)
{
    if (!tab)
        return;

    DEBUG_CHECK_EX(m_tabs.contains(tab), "Element is not a tab in this notebook");
    if (!m_tabs.contains(tab))
        tab = nullptr;

    if (tab != m_activeTab)
    {
        if (m_activeTab)
        {
            m_activeTab->visibility(false);
            m_activeTab->call(this, "OnTabDectivated"_id);

            auto tabIndex = m_tabs.find(m_activeTab);
            if (tabIndex != INDEX_NONE)
            {
                const auto& button = m_buttons[tabIndex];
                button->removeStyleClass("active"_id);
            }
        }

        m_activeTab = tab;

        if (m_activeTab)
        {
            auto tabIndex = m_tabs.find(m_activeTab);
            if (tabIndex != INDEX_NONE)
            {
                const auto& button = m_buttons[tabIndex];
                button->addStyleClass("active"_id);

                m_headerScrollArea->scrollToMakeElementVisible(button);
            }

            m_activeTab->visibility(true);
            m_activeTab->call(this, "OnTabActivated"_id);
            m_activeTab->focus();
        }
    }
}

bool Notebook::containsTab(IElement* tab)
{
    return m_tabs.contains(tab);
}

static int FindElementIndex(IElement* container, IElement* searchFor)
{
    int index = 0;
    for (ElementChildIterator it(container->childrenList()); it; ++it, ++index)
        if (*it == searchFor)
            return index;

    return -1;
}

void Notebook::attachTab(IElement* tab, IElement* afterTab /*= nullptr*/, bool activate /*= true*/)
{
    if (!tab)
        return;

    DEBUG_CHECK_EX(!afterTab || m_tabs.contains(afterTab), "Element is not a tab in this notebook");

    // tab must always fill the notebook area
    tab->customHorizontalAligment(ElementHorizontalLayout::Expand);
    tab->customVerticalAligment(ElementVerticalLayout::Expand);

    // find where to add
    auto insertTabIndex = m_tabs.find(afterTab);
    if (insertTabIndex == -1)
        insertTabIndex = m_tabs.size();

    // create header button
    auto button = createHeaderButtonForTab(tab);

    // add to list of tabs
    m_tabs.insert(insertTabIndex, tab);
    m_buttons.insert(insertTabIndex, button);

    // add the tab to container
    m_container->attachChild(tab);
    tab->visibility(false);

    // activate the tab
    if (activate || m_tabs.size() == 1)
        this->tab(tab);
}

base::StringBuf Notebook::tabTitle(IElement* tab)
{
    return tab->evalStyleValue<base::StringBuf>("title"_id, "Tab");
}

bool Notebook::tabIsVisible(IElement* tab)
{
    return true;
}

bool Notebook::tabHasCloseButton(IElement* tab)
{
    return tab->evalStyleValue<bool>("close-button"_id, false);
}

void Notebook::tabHandleCloseRequest(IElement* tab)
{
    detachTab(tab);
}

bool Notebook::tabHandleContextMenu(IElement* tab, const Position& pos)
{
    if (tab)
        return tab->call(EVENT_TAB_CONTEXT_MENU, pos);
    return false;
}

IElement* Notebook::tabForButton(IElement* button)
{
    auto index = m_buttons.find(button);
    if (index >= 0 && index <= m_tabs.lastValidIndex())
        return m_tabs[index];
    return nullptr;
}

ButtonPtr Notebook::createHeaderButtonForTab(IElement* tab)
{
    auto tabButtonFlags = { ButtonModeBit::EventOnClick, ButtonModeBit::SecondEventOnMiddleClick };
    auto tabButton = m_header->createChildWithType<Button>("NotebookHeaderTabButton"_id, tabButtonFlags);
    tabButton->bind(EVENT_CLICKED, this) = [this](Button* tabButton)
    {
        if (auto* newTab = tabForButton(tabButton))
            this->tab(newTab);
    };
    tabButton->bind(EVENT_CLICKED_SECONDARY, this) = [this](Button* tabButton)
    {
        if (auto* newTab = tabForButton(tabButton))
            tabHandleCloseRequest(newTab);
    };

    tabButton->createNamedChild<TextLabel>("Title"_id, tabTitle(tab));

    tabButton->bind(EVENT_CONTEXT_MENU) = [this](ui::Button* button, Position position)
    {
        if (auto* tab = tabForButton(button))
            return tabHandleContextMenu(tab, position);

        return false;
    };

    if (m_flags.test(NotebookFlagBit::AllowCloseTabs))
    {
        auto closeButton = tabButton->createNamedChildWithType<Button>("CloseButton"_id, "NotebookHeaderTabCloseButton"_id);
        closeButton->createChild<TextLabel>();
        closeButton->visibility(tabHasCloseButton(tab));

        closeButton->bind(EVENT_CLICKED, this) = [this](Button* closeButton)
        {
            if (auto tabButton = closeButton->findParent<Button>())
                if (auto* tab = tabForButton(tabButton))
                    tabHandleCloseRequest(tab);
        };
    }

    updateHeaderButtonData(tabButton, tab);
    return tabButton;
}

void Notebook::updateHeaderButtonData(Button* button, IElement* tab)
{
    if (auto label = button->findChildByName<TextLabel>("Title"_id))
    {
        auto caption = tabTitle(tab);
        label->text(caption);
    }

    if (m_flags.test(NotebookFlagBit::AllowCloseTabs))
    {
        if (auto closeButton = button->findChildByName<TextLabel>("CloseButton"_id))
        {
            auto canClose = tabHasCloseButton(tab);
            closeButton->visibility(canClose);
        }
    }
}

void Notebook::detachTab(IElement* tabToDetach, IElement* otherTabToActive /*= nullptr*/)
{
    if (!tabToDetach)
        return;

    DEBUG_CHECK_EX(!otherTabToActive || m_tabs.contains(otherTabToActive), "Element is not a tab in this notebook");

    auto index = m_tabs.find(tabToDetach);
    DEBUG_CHECK_EX(index != INDEX_NONE, "Element is not a tab in this notebook");
    if (index != INDEX_NONE)
    {
        if (m_activeTab == tabToDetach)
        {
            m_activeTab = nullptr;

            if (!otherTabToActive)
            {
                if (index == m_tabs.lastValidIndex() && m_tabs.size() != 1)
                    otherTabToActive = m_tabs[index - 1];
                else if (index < m_tabs.lastValidIndex())
                    otherTabToActive = m_tabs[index + 1];
            }
        }

        // remove tab
        m_container->detachChild(tabToDetach);
        m_tabs.erase(index);

        // remove button
        m_header->detachChild(m_buttons[index]);
        m_buttons.erase(index);
    }

    if (otherTabToActive)
        tab(otherTabToActive);
}

void Notebook::updateHeaderButtons()
{
    uint32_t count = std::min<uint32_t>(m_tabs.size(), m_buttons.size());

    uint32_t i = 0;
    for (; i < count; ++i)
        updateHeaderButtonData(m_buttons[i], m_tabs[i]);

    while (i < m_buttons.size())
    {
        m_header->detachChild(m_buttons[i]);
        i += 1;
    }

    m_buttons.resize(count);
}

void Notebook::attachChild(IElement* childElement)
{
    attachTab(childElement);
}

void Notebook::detachChild(IElement* childElement)
{
    detachTab(childElement);
}

void Notebook::computeSize(Size& outSize) const
{
    outSize = m_header->cachedLayoutParams().calcTotalSize();

    if (m_button)
    {
        auto buttonSize = m_header->cachedLayoutParams().calcTotalSize();
        outSize.x += buttonSize.x;
        outSize.y = std::max<float>(buttonSize.y, outSize.y);
    }

    auto containerSize = m_container->cachedLayoutParams().calcTotalSize();
    auto maxContainerWidth = containerSize.x;
    auto maxContainerHeight = containerSize.y;

    for (auto* tab : m_tabs)
    {
        auto tabSize = m_container->cachedLayoutParams().calcTotalSize();
        maxContainerWidth = std::max<float>(tabSize.x, maxContainerWidth);
        maxContainerHeight = std::max<float>(tabSize.y, maxContainerHeight);
    }

    outSize.x = std::max<float>(maxContainerWidth, outSize.x);
    outSize.y += maxContainerHeight;
}

//---

END_BOOMER_NAMESPACE(ui)