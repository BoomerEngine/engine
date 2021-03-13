/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"

#include "launcherApplication.h"

#include "core/app/include/application.h"

DECLARE_MODULE(PROJECT_NAME)
{
    // custom module initialization code
}

boomer::IApplication& GetApplicationInstance()
{
    static boomer::LauncherApp theApp;
    return theApp;
}