/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: test #]
***/

#pragma once

#include "base/app/include/application.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

/// boilerplate for test app
/// contains basic scene initialization and other shit
class UIApp : public base::app::IApplication
{
public:
    UIApp();
    virtual ~UIApp();

protected:
    virtual bool initialize(const base::app::CommandLine& commandline) override;
    virtual void update() override;
    virtual void cleanup() override;

    base::UniquePtr<ui::Renderer> m_renderer;
    base::RefPtr<ui::DataStash> m_dataStash;
    base::UniquePtr<NativeWindowRenderer> m_nativeRenderer;
    base::NativeTimePoint m_lastUpdateTime;
};      

END_BOOMER_NAMESPACE(rendering::test)