/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\common #]
***/

#pragma once

#include "launcherPlatform.h"
#include "core/system/include/timing.h"

BEGIN_BOOMER_NAMESPACE()

/// common launcher implementation
/// this class is usually wrapper by platform specific one
class CommonPlatform : public Platform
{
public:
    CommonPlatform();
    virtual ~CommonPlatform();

    virtual bool handleStart(const CommandLine& cmdline, IApplication* app) override;
    virtual void handleUpdate() override;
    virtual void handleCleanup() override;
};

END_BOOMER_NAMESPACE()
