/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: app_common #]
* [# console #]
* [# devonly #]
* [# engineonly #]
* [# generate_main #]
***/

#include "build.h"
#include "reflection.inl"

#include "base/app/include/application.h"
#include "app/common/include/bccApplication.h"

DECLARE_MODULE(PROJECT_NAME)
{
    // custom module initialization code
}

base::app::IApplication& GetApplicationInstance()
{
    static application::BCCApp theApp;
    return theApp;
}