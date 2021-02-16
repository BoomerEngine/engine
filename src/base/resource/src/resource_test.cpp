/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/containers/include/public.h"
#include "base/containers/include/hashMap.h"
#include "base/system/include/scopeLock.h"
#include "base/io/include/ioFileHandleMemory.h"

#include "resource.h"
#include "resourceLoader.h"
#include "resourcePath.h"
#include "resourceTags.h"
#include "resourceFileSaver.h"
#include "resourceFileLoader.h"

DECLARE_TEST_FILE(Resource);

using namespace base;

