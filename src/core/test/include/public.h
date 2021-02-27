/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_test_glue.inl"

#ifdef BUILD_AS_LIBS
    #define GTEST_CREATE_SHARED_LIBRARY 0
    #define GTEST_LINKED_AS_SHARED_LIBRARY 0
#else
    #ifdef CORE_TEST_EXPORTS
        #define GTEST_CREATE_SHARED_LIBRARY 1
        #define GTEST_LINKED_AS_SHARED_LIBRARY 0
    #else
        #define GTEST_CREATE_SHARED_LIBRARY 0
        #define GTEST_LINKED_AS_SHARED_LIBRARY 1
    #endif
#endif

#undef DECLARE_TEST_FILE
#define DECLARE_TEST_FILE(x) void InitTests_##x() {};
