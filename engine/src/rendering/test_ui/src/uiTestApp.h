/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: test #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/app/include/application.h"

namespace rendering
{
    namespace test
    {
        /// boilerplate for rendering scene test
        /// contains basic scene initialization and other shit
        class UIApp : public base::app::IApplication
        {
        public:
            UIApp();
            virtual ~UIApp();

        protected:
            virtual bool initialize(const base::app::CommandLine& commandline) override;
            virtual void update() override;

            base::UniquePtr<ui::Renderer> m_renderer;
            base::UniquePtr<ui::DataStash> m_dataStash;
            base::UniquePtr<NativeWindowRenderer> m_nativeRenderer;
            base::NativeTimePoint m_lastUpdateTime;
        };      

    } // test
} // scene