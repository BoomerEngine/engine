/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# dependency: app_common #]
* [# dependency: import_* #]
* [# dependency: editor_* #]
* [# app #]
* [# editor #]
* [# devonly #]
* [# engineonly #]
* [# generate_main #]
***/

#include "build.h"
#include "reflection.inl"

#include "app/common/include/editorApplication.h"
#include "base/app/include/application.h"

DECLARE_MODULE(PROJECT_NAME)
{
    // custom module initialization code
}

base::app::IApplication& GetApplicationInstance()
{
    static application::EditorApp theApp;
    return theApp;
}