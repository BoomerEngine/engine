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

BEGIN_BOOMER_NAMESPACE()


//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDebugPage);
RTTI_END_TYPE();

IDebugPage::IDebugPage()
{}

IDebugPage::~IDebugPage()
{}

bool IDebugPage::handleInitialize()
{
    return true;
}

void IDebugPage::handleTick(float timeDelta)
{   
}

void IDebugPage::handleRender()
{
}

//--

END_BOOMER_NAMESPACE()
