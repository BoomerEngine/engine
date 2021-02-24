/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_fibers_glue.inl"

#include "fiberSystem.h"

BEGIN_BOOMER_NAMESPACE(base::fibers)

class SyncPoint;
class WaitList;
class WorkQueue;
class CommandQueue;

END_BOOMER_NAMESPACE(base::fibers)


BEGIN_BOOMER_NAMESPACE(base)

class IBackgroundJob;

END_BOOMER_NAMESPACE(base)
