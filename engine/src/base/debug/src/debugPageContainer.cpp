/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: pages #]
*
***/

#include "build.h"
#include "debugPage.h"
#include "debugPageContainer.h"

#include "base/app/include/localService.h"
#include "base/app/include/localServiceContainer.h"
#include "base/canvas/include/canvas.h"
#include "base/imgui/include/imgui.h"

namespace base
{
    namespace debug
    {

        //--

        DebugPageContainer::DebugPageContainer()
            : m_lastTimeDelta(0.0f)
        {
            createDebugPages();
        }

        DebugPageContainer::~DebugPageContainer()
        {
        }

        bool DebugPageContainer::processInputEvent(const base::input::BaseEvent& evt)
        {
            //ImGui::ProcessInputEvent(evt, ImGui::GetIO());
            return false;
        }

        void DebugPageContainer::update(float dt)
        {
            m_lastTimeDelta = dt;

            for (auto& entry : m_debugPages)
            {
                if (entry.page)
                {
                    // update the page
                    if (!entry.page->handleTick(*this, dt))
                    {
                        TRACE_INFO("Debug page {} closed on it's own request", entry.windowTitle);
                        entry.opened = false;
                        entry.page.reset();
                    }
                }
            }
        }

        void DebugPageContainer::render(canvas::Canvas& c)
        {
            // render the main menu
            renderMenu();

            // render the opened pages
            for (auto& entry : m_debugPages)
            {
                if (entry.page)
                {
                    //uiFrame.SetNextWindowSize(base::Vector2(520,600), ImGuiCond_FirstUseEver);
                    if (ImGui::Begin(entry.windowTitle.c_str(), &entry.requestOpened))
                    {
                        entry.page->handleRender(*this);
                    }

                    ImGui::End();

                    // window closed by a click
                    if (!entry.requestOpened)
                    {
                        TRACE_INFO("Debug page {} closed by user", entry.windowTitle);
                        entry.opened = false;
                        entry.page.reset();
                    }
                }
            }
        }

        void DebugPageContainer::createDebugPages()
        {
            base::Array<base::ClassType> debugPageClasses;
            RTTI::GetInstance().enumClasses(IDebugPage::GetStaticClass(), debugPageClasses);
            TRACE_INFO("Found {} debug page classes", debugPageClasses.size());

            for (auto pageClass  : debugPageClasses)
            {
                auto titleMetadata  = pageClass->findMetadata<DebugPageTitleMetadata>();
                auto categoryeMetadata  = pageClass->findMetadata<DebugPageCategoryMetadata>();
                if (titleMetadata && titleMetadata->title() && *titleMetadata->title() && categoryeMetadata && categoryeMetadata->category() && *categoryeMetadata->category())
                {
                    auto &page = m_debugPages.emplaceBack();
                    page.opened = false;
                    page.classType = pageClass;
                    page.name = titleMetadata->title();
                    page.windowTitle = base::TempString("{} - {}", categoryeMetadata->category(), titleMetadata->title());

                    bool categoryFound = false;
                    for (auto& cat : m_debugCategories)
                    {
                        if (cat.category == categoryeMetadata->category())
                        {
                            cat.children.pushBack(m_debugPages.lastValidIndex());
                            categoryFound = true;
                        }
                    }

                    if (!categoryFound)
                    {
                        auto& cat = m_debugCategories.emplaceBack();
                        cat.category = categoryeMetadata->category();
                        cat.children.pushBack(m_debugPages.lastValidIndex());
                    }
                }
            }
        }

        void DebugPageContainer::updatePage(DebugPageInfo& page)
        {
            if (page.requestOpened != page.opened)
            {
                if (page.requestOpened)
                {
                    auto pageObject = page.classType->create<IDebugPage>();
                    if (pageObject->handleInitialize(*this))
                    {
                        page.opened = true;
                        page.page = pageObject;
                    }
                }
                else if (page.page)
                {
                    page.opened = false;
                    page.page.reset();
                }
            }
        }

        void DebugPageContainer::renderMenu()
        {
            if (ImGui::BeginMainMenuBar())
            {
                for (auto& cat : m_debugCategories)
                {
                    if (ImGui::BeginMenu(cat.category.c_str()))
                    {
                        for (auto id : cat.children)
                        {
                            auto& page = m_debugPages[id];
                            if (ImGui::MenuItem(page.name.c_str(), "", &page.requestOpened, true))
                            {
                                updatePage(page);
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMainMenuBar();
            }
        }

        //--

    } // debug
} // plugin