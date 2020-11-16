/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: app #]
***/

#pragma once

#include "base/system/include/timing.h"
#include "localServiceContainer.h"

namespace base
{
    namespace app
    {
        // the main root application like object that controls the main flow of the system
        // the main role of the App object is to hold on to global singleton-like object called "services"
        class BASE_APP_API IApplication : public NoCopy
        {
        public:
            IApplication();
            virtual ~IApplication();

            /// called after all local services were initialized
            virtual bool initialize(const base::app::CommandLine& commandline);

            /// cleanup, called before local services are closed
            virtual void cleanup();

            /// update application state
            virtual void update();
        };

    } // app
} // base
