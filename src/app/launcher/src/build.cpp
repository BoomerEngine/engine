/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: app_common #]
* [# app #]
* [# engineonly #]
* [# generate_main #]
***/

#include "build.h"
#include "reflection.inl"
#include "app/common/include/launcherApplication.h"
#include "base/app/include/application.h"

DECLARE_MODULE(PROJECT_NAME)
{
    // custom module initialization code
}

base::app::IApplication& GetApplicationInstance()
{
    static application::LauncherApp theApp;
    return theApp;
}