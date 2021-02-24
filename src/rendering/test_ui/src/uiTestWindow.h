/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: test #]
***/

#pragma once

#include "base/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

//--

/// test window
class TestWindow : public ui::Window
{
public:
    TestWindow();

    virtual void queryInitialPlacementSetup(ui::WindowInitialPlacementSetup& outSetup) const override;
    virtual void handleExternalCloseRequest() override;

	base::canvas::DynamicAtlasPtr m_honklerAtlas;
};

//--

END_BOOMER_NAMESPACE(rendering::test)