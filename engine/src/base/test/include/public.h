/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_test_glue.inl"

#ifdef BUILD_AS_LIBS
    #define GTEST_CREATE_SHARED_LIBRARY 0
    #define GTEST_LINKED_AS_SHARED_LIBRARY 0
#else
    #ifdef BASE_TEST_EXPORTS
        #define GTEST_CREATE_SHARED_LIBRARY 1
        #define GTEST_LINKED_AS_SHARED_LIBRARY 0
    #else
        #define GTEST_CREATE_SHARED_LIBRARY 0
        #define GTEST_LINKED_AS_SHARED_LIBRARY 1
    #endif
#endif

#define DECLARE_TEST_FILE(x) void InitTests_##x() {};
