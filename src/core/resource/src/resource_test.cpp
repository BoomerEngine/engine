/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "core/test/include/gtest/gtest.h"
#include "core/containers/include/public.h"
#include "core/containers/include/hashMap.h"
#include "core/system/include/scopeLock.h"
#include "core/io/include/fileHandleMemory.h"

#include "resource.h"
#include "loader.h"
#include "path.h"
#include "tags.h"
#include "fileSaver.h"
#include "fileLoader.h"

DECLARE_TEST_FILE(Resource);



