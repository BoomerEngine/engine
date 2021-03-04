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

#include "core/io/include/io.h"
#include "core/containers/include/stringBuilder.h"
#include "core/system/include/thread.h"
#include "core/memory/include/poolStats.h"

static bool GReportMemoryUsage = false;

BEGIN_BOOMER_NAMESPACE_EX(app)

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

END_BOOMER_NAMESPACE_EX(app)
