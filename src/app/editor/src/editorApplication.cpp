/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "editorApplication.h"

#include "core/app/include/launcherPlatform.h"

BEGIN_BOOMER_NAMESPACE()

//---

bool EditorApp::initialize(const CommandLine& commandline)
{
    return GetService<ed::EditorService>()->start();
}

void EditorApp::cleanup()
{
}

void EditorApp::update()
{
}

//---

END_BOOMER_NAMESPACE()

