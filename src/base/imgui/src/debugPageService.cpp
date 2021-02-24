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
#include "debugPageService.h"

#include "base/app/include/localService.h"
#include "base/app/include/localServiceContainer.h"
#include "base/canvas/include/canvas.h"
#include "base/imgui/include/imgui.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

RTTI_BEGIN_TYPE_CLASS(DebugPageService);
RTTI_END_TYPE();

//--

DebugPageService::DebugPageService()
    : m_lastTimeDelta(0.0f)
{
}

DebugPageService::~DebugPageService()
{
}

app::ServiceInitializationResult DebugPageService::onInitializeService(const app::CommandLine& cmdLine)
{
    m_lastTickTime.resetToNow();
    return app::ServiceInitializationResult::Finished;
}

void DebugPageService::onShutdownService()
{
    m_pages.clear();
    m_menu = MenuEntry();
}

void DebugPageService::onSyncUpdate()
{
    m_lastTimeDelta = std::clamp<float>(m_lastTickTime.timeTillNow().toSeconds(), 0.0001f, 0.1f);
    m_lastTickTime.resetToNow();

    if (m_visible)
        for (const auto& page : m_pages)
            page->handleTick(m_lastTimeDelta);
}

void DebugPageService::visiblity(bool visible)
{
    m_visible = visible;
}

void DebugPageService::render()
{
    if (m_visible)
    {
        if (m_pages.empty())
            createDebugPages();

        if (m_menu.children.empty())
            createMenu();

        renderMenu();

        for (const auto& page : m_pages)
            page->handleRender();
    }
}

void DebugPageService::createDebugPages()
{
    Array<SpecificClassType<IDebugPage>> debugPageClasses;
    RTTI::GetInstance().enumClasses(debugPageClasses);
    TRACE_INFO("Found {} global debug page classes", debugPageClasses.size());

    for (auto pageClass : debugPageClasses)
        if (auto page = pageClass.create())
            if (page->handleInitialize())
                m_pages.pushBack(page);
}

DebugPageService::MenuEntry* DebugPageService::GetOrCreateEntry(Array<MenuEntry>& entries, StringView name)
{
    for (auto& entry : entries)
        if (entry.name == name)
            return &entry;

    auto& entry = entries.emplaceBack();
    entry.name = StringBuf(name);
    return &entry;
}

void DebugPageService::createMenu()
{
    // reset current state
    m_menu.children.clear();

    // get all properties
    Array<ConfigPropertyBase*> properties;
    properties.reserve(512);
    ConfigPropertyBase::GetAllProperties(properties);

    // look for properties in group "DebugPage"
    for (const auto* prop : properties)
    {
        // look for the "IsVisible" property in the "DebugPage.XXX.YYY" group
        const auto groupName = prop->group().view();
        if (prop->name() == "IsVisible" && groupName.beginsWith("DebugPage.") && prop->type() == reflection::GetTypeObject<bool>())
        {
            InplaceArray<StringView, 4> nameParts;
            groupName.afterFirst("DebugPage.").slice(".", false, nameParts);

            auto* cur = &m_menu;
            for (uint32_t i = 0; i < nameParts.size() && cur; ++i)
                cur = GetOrCreateEntry(cur->children, nameParts[i]);

            if (cur)
            {
                cur->configGroupName = prop->group();
                cur->configProperties.pushBack(prop);
            }
        }
    }
}

void DebugPageService::RenderEntry(const MenuEntry& entry)
{
    if (entry.children.empty())
    {
        bool flag = config::ValueBool(entry.configGroupName, "IsVisible", false);
        if (ImGui::MenuItem(entry.name.c_str(), "", &flag, true))
        {
            config::WriteBool(entry.configGroupName, "IsVisible", flag);
            ConfigPropertyBase::RefreshPropertyValue(entry.configGroupName, "IsVisible");
        }
    }
    else
    {
        if (ImGui::BeginMenu(entry.name.c_str(), true))
        {
            for (const auto& child : entry.children)
                RenderEntry(child);
            ImGui::EndMenu();
        }
    }
}

void DebugPageService::renderMenu()
{
    auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("Engine Debug", &m_visible, flags))
    {
        for (const auto& child : m_menu.children)
            RenderEntry(child);

        ImGui::End();
    }
}

//--

bool DebugPagesVisible()
{
    if (auto service = base::GetService<DebugPageService>())
        return service->visible();
    return false;
}

void DebugPagesRender()
{
    if (auto service = base::GetService<DebugPageService>())
        service->render();        
}

void DebugPagesVisibility(bool visible)
{
    if (auto service = base::GetService<DebugPageService>())
        service->visiblity(visible);
}

//--

END_BOOMER_NAMESPACE(base)