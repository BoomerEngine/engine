/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: pages #]
*
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//----

/// GLOBAL (engine wide) debug page, just derive from this class and you will be rendered when debug panels are enabled
class ENGINE_IMGUI_API IDebugPage : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IDebugPage);

public:
    IDebugPage();
    virtual ~IDebugPage();

    //! initialize the page
    virtual bool handleInitialize();

    //! update page content
    virtual void handleTick(float timeDelta);

    //! generate ui using ImGui
    virtual void handleRender();
};

//----

END_BOOMER_NAMESPACE()
