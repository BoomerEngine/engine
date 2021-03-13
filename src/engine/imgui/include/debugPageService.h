/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: pages #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

/// container for global "debug pages" that are rendered using ImGui
class ENGINE_IMGUI_API DebugPageService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(DebugPageService, IService);

public:
    DebugPageService();
    virtual ~DebugPageService();

    //---

    /// are we visible ?
    INLINE bool visible() const { return m_visible; }

    //---

    void visiblity(bool visible);

    void render(); // ImGui context is assumed to be active at this point

    //---

private:
    bool m_visible = false;
    NativeTimePoint m_lastTickTime;
    float m_lastTimeDelta = 0.0f;

    struct MenuEntry
    {
        StringBuf name;
        StringBuf configGroupName;
        Array<const ConfigPropertyBase*> configProperties;
        Array<MenuEntry> children;
    };

    MenuEntry m_menu;
    Array<DebugPagePtr> m_pages;

    //--

    virtual bool onInitializeService(const CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    static MenuEntry* GetOrCreateEntry(Array<MenuEntry>& entries, StringView name);
    static void RenderEntry(const MenuEntry& entry);

    void createDebugPages();
    void createMenu();

    void renderMenu();
};

//---

// are debug pages visible ?
extern ENGINE_IMGUI_API bool DebugPagesVisible();

// render debug pages
extern ENGINE_IMGUI_API void DebugPagesRender();

// toggle debug page visiblity
extern ENGINE_IMGUI_API void DebugPagesVisibility(bool visible);

//---

END_BOOMER_NAMESPACE()
