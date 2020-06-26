/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tools #]
***/

#pragma once

#include "ui/toolkit/include/uiWindowOverlay.h"

namespace ed
{
    namespace world
    {

        ///----

        /// helper window to select node class
        class EDITOR_SCENE_MAIN_API NodeClassSelector : public ui::PopupWindowOverlay
        {
            RTTI_DECLARE_VIRTUAL_CLASS(NodeClassSelector, ui::PopupWindowOverlay);

        public:
            NodeClassSelector();

            /// get selected class
            base::ClassType selectedClass() const;

            ///--

            typedef std::function<void(base::ClassType)> TOnClassSelectedEvent;
            TOnClassSelectedEvent OnClassSelected;

        private:
            base::ClassType m_rootClass;

            base::RefPtr<ui::ListView> m_classList;

            //--

            void refreshClassList();
        };

        ///----

    } // mesh
} // ed