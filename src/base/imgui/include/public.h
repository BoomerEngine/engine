/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_imgui_glue.inl"

#include "imgui.h"
#include "imgui_integration.h"

BEGIN_BOOMER_NAMESPACE(base)

class IDebugPage;
typedef RefPtr<IDebugPage> DebugPagePtr;

END_BOOMER_NAMESPACE(base)