/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: test #]
***/

#pragma once

#include "core/app/include/application.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

/// boilerplate for test app
/// contains basic scene initialization and other shit
class UIApp : public app::IApplication
{
public:
    UIApp();
    virtual ~UIApp();

protected:
    virtual bool initialize(const app::CommandLine& commandline) override;
    virtual void update() override;
    virtual void cleanup() override;

    UniquePtr<ui::Renderer> m_renderer;
    RefPtr<ui::DataStash> m_dataStash;
    UniquePtr<ui::NativeWindowRenderer> m_nativeRenderer;
    NativeTimePoint m_lastUpdateTime;
};      

END_BOOMER_NAMESPACE_EX(test)
