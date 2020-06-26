/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: app #]
***/

#include "build.h"
#include "application.h"
#include "localService.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/absolutePathBuilder.h"
#include "base/containers/include/stringBuilder.h"
#include "base/system/include/thread.h"
#include "base/memory/include/poolStats.h"

static bool GReportMemoryUsage = false;

namespace base
{
    namespace app
    {

        //-----

        /*ConfigProperty<float> cvMinTimeDelta("Engine", "MinTimeDelta", 0.002f);
        ConfigProperty<float> cvMaxTimeDelta("Engine", "MaxTimeDelta", 0.1f);
        ConfigProperty<float> cvForceTimeDelta("Engine", "ForceTimeDelta", 0.0f);
        ConfigProperty<float> cvMaxFPS("Engine", "MaxFPS", 0.0f);*/

        //-----

        IApplication::IApplication()
        {}

        IApplication::~IApplication()
        {}

        bool IApplication::initialize(const CommandLine& commandline)
        {
            return true;
        }

        void IApplication::cleanup()
        {
        }

        void IApplication::update()
        {
        }

        //---

    } // app
} // base