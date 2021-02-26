/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_fibers_glue.inl"

#include "fiberSystem.h"

BEGIN_BOOMER_NAMESPACE_EX(fibers)

class SyncPoint;
class WaitList;
class WorkQueue;
class CommandQueue;

END_BOOMER_NAMESPACE_EX(fibers)


BEGIN_BOOMER_NAMESPACE()

class IBackgroundJob;

END_BOOMER_NAMESPACE()
