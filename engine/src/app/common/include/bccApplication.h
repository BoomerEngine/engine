/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/app/include/application.h"

namespace application
{

    // commandline processor application
    class APP_COMMON_API BCCApp : public base::app::IApplication
    {
    public:
        virtual bool initialize(const base::app::CommandLine& commandline) override final;
        virtual void update() override final;
    };

} // application
