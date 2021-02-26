/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: base_* #]
* [# dependency: rendering_* #]
* [# dependency: game_* #]
* [# app #]
* [# engineonly #]
* [# generate_main #]
***/

#include "build.h"
#include "reflection.inl"
#include "launcherApplication.h"
#include "core/app/include/application.h"

DECLARE_MODULE(PROJECT_NAME)
{
    // custom module initialization code
}

boomer::app::IApplication& GetApplicationInstance()
{
    static boomer::LauncherApp theApp;
    return theApp;
}