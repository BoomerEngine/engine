/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: pages #]
***/

#pragma once

namespace base
{
    namespace debug
    {

        //---

        /// container for the debug pages, pages are rendered using ImGUI
        class BASE_DEBUG_API DebugPageContainer : public base::NoCopy
        {
        public:
            DebugPageContainer();
            ~DebugPageContainer();

            //---

            /// process input, should return true if filtered
            bool processInputEvent(const base::input::BaseEvent& evt);

            /// update debug page
            void update(float dt);

            /// render into a canvas
            void render(canvas::Canvas& c);

        private:
            struct DebugPageInfo
            {
                bool opened = false;
                bool requestOpened = false;
                base::StringBuf windowTitle;
                base::StringBuf name;
                base::ClassType classType = nullptr;
                base::RefPtr<IDebugPage> page;
            };

            struct DebugPageCategories
            {
                base::StringBuf category;
                base::Array<int> children;
            };

            base::Array<DebugPageInfo> m_debugPages;
            base::Array<DebugPageCategories> m_debugCategories;
            float m_lastTimeDelta;

            //--

            void createDebugPages();
            void renderMenu();
            void updatePage(DebugPageInfo& page);
        };

        //---

    } // debug
} // plugin