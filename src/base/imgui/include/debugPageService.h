/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: pages #]
***/

#pragma once

namespace base
{
    //---

    /// container for global "debug pages" that are rendered using ImGui
    class BASE_IMGUI_API DebugPageService : public app::ILocalService
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DebugPageService, app::ILocalService);

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

        virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
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
    extern BASE_IMGUI_API bool DebugPagesVisible();

    // render debug pages
    extern BASE_IMGUI_API void DebugPagesRender();

    // toggle debug page visiblity
    extern BASE_IMGUI_API void DebugPagesVisibility(bool visible);

    //---

} // base