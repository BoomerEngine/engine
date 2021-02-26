/*
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "core/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(PointerComparisons);

BEGIN_BOOMER_NAMESPACE()

namespace tests
{

    struct Base {};
    struct DerivedA : public Base {};
    struct DerivedA2 : public DerivedA {};
    struct DerivedB : public Base {};

} // test

#if 0

TEST(PointerComparisons, EmptyTestComapre)
{
    RefPtr<tests::Base> strong;
    RefWeakPtr<tests::Base> weak;
    UntypedSharedPtr untypedStrong;
    UntypedSharedPtr untypedWeak;
    tests::Base* ptr = nullptr;
    const tests::Base* constPtr = nullptr;

    EXPECT_TRUE(strong == strong);
    EXPECT_TRUE(strong == weak);
    EXPECT_TRUE(strong == untypedStrong);
    EXPECT_TRUE(strong == untypedWeak);
    EXPECT_TRUE(strong == ptr);
    EXPECT_TRUE(strong == constPtr);
    EXPECT_TRUE(strong == nullptr);

    EXPECT_TRUE(weak == strong);
    EXPECT_TRUE(weak == weak);
    EXPECT_TRUE(weak == untypedStrong);
    EXPECT_TRUE(weak == untypedWeak);
    EXPECT_TRUE(weak == ptr);
    EXPECT_TRUE(weak == constPtr);
    EXPECT_TRUE(weak == nullptr);

    EXPECT_TRUE(untypedStrong == strong);
    EXPECT_TRUE(untypedStrong == weak);
    EXPECT_TRUE(untypedStrong == untypedStrong);
    EXPECT_TRUE(untypedStrong == untypedWeak);
    EXPECT_TRUE(untypedStrong == ptr);
    EXPECT_TRUE(untypedStrong == constPtr);
    EXPECT_TRUE(untypedStrong == nullptr);

    EXPECT_TRUE(untypedWeak == strong);
    EXPECT_TRUE(untypedWeak == weak);
    EXPECT_TRUE(untypedWeak == untypedStrong);
    EXPECT_TRUE(untypedWeak == untypedWeak);
    EXPECT_TRUE(untypedWeak == ptr);
    EXPECT_TRUE(untypedWeak == constPtr);
    EXPECT_TRUE(untypedWeak == nullptr);

    EXPECT_TRUE(ptr == strong);
    EXPECT_TRUE(ptr == weak);
    EXPECT_TRUE(ptr == untypedStrong);
    EXPECT_TRUE(ptr == untypedWeak);
    EXPECT_TRUE(ptr == ptr);
    EXPECT_TRUE(ptr == constPtr);
    EXPECT_TRUE(ptr == nullptr);

    EXPECT_TRUE(constPtr == strong);
    EXPECT_TRUE(constPtr == weak);
    EXPECT_TRUE(constPtr == untypedStrong);
    EXPECT_TRUE(constPtr == untypedWeak);
    EXPECT_TRUE(constPtr == ptr);
    EXPECT_TRUE(constPtr == constPtr);
    EXPECT_TRUE(constPtr == nullptr);

    EXPECT_TRUE(nullptr == strong);
    EXPECT_TRUE(nullptr == weak);
    EXPECT_TRUE(nullptr == untypedStrong);
    EXPECT_TRUE(nullptr == untypedWeak);
    EXPECT_TRUE(nullptr == ptr);
    EXPECT_TRUE(nullptr == constPtr);
    EXPECT_TRUE(nullptr == nullptr);
}

TEST(PointerComparisons, EmptyTestNotComapre)
{
    RefPtr<tests::Base> strong;
    RefWeakPtr<tests::Base> weak;
    UntypedSharedPtr untypedStrong;
    UntypedSharedPtr untypedWeak;
    tests::Base* ptr = nullptr;
    const tests::Base* constPtr = nullptr;

    EXPECT_FALSE(strong != strong);
    EXPECT_FALSE(strong != weak);
    EXPECT_FALSE(strong != untypedStrong);
    EXPECT_FALSE(strong != untypedWeak);
    EXPECT_FALSE(strong != ptr);
    EXPECT_FALSE(strong != constPtr);
    EXPECT_FALSE(strong != nullptr);

    EXPECT_FALSE(weak != strong);
    EXPECT_FALSE(weak != weak);
    EXPECT_FALSE(weak != untypedStrong);
    EXPECT_FALSE(weak != untypedWeak);
    EXPECT_FALSE(weak != ptr);
    EXPECT_FALSE(weak != constPtr);
    EXPECT_FALSE(weak != nullptr);

    EXPECT_FALSE(untypedStrong != strong);
    EXPECT_FALSE(untypedStrong != weak);
    EXPECT_FALSE(untypedStrong != untypedStrong);
    EXPECT_FALSE(untypedStrong != untypedWeak);
    EXPECT_FALSE(untypedStrong != ptr);
    EXPECT_FALSE(untypedStrong != constPtr);
    EXPECT_FALSE(untypedStrong != nullptr);

    EXPECT_FALSE(untypedWeak != strong);
    EXPECT_FALSE(untypedWeak != weak);
    EXPECT_FALSE(untypedWeak != untypedStrong);
    EXPECT_FALSE(untypedWeak != untypedWeak);
    EXPECT_FALSE(untypedWeak != ptr);
    EXPECT_FALSE(untypedWeak != constPtr);
    EXPECT_FALSE(untypedWeak != nullptr);

    EXPECT_FALSE(ptr != strong);
    EXPECT_FALSE(ptr != weak);
    EXPECT_FALSE(ptr != untypedStrong);
    EXPECT_FALSE(ptr != untypedWeak);
    EXPECT_FALSE(ptr != ptr);
    EXPECT_FALSE(ptr != constPtr);
    EXPECT_FALSE(ptr != nullptr);

    EXPECT_FALSE(constPtr != strong);
    EXPECT_FALSE(constPtr != weak);
    EXPECT_FALSE(constPtr != untypedStrong);
    EXPECT_FALSE(constPtr != untypedWeak);
    EXPECT_FALSE(constPtr != ptr);
    EXPECT_FALSE(constPtr != constPtr);
    EXPECT_FALSE(constPtr != nullptr);

    EXPECT_FALSE(nullptr != strong);
    EXPECT_FALSE(nullptr != weak);
    EXPECT_FALSE(nullptr != untypedStrong);
    EXPECT_FALSE(nullptr != untypedWeak);
    EXPECT_FALSE(nullptr != ptr);
    EXPECT_FALSE(nullptr != constPtr);
    EXPECT_FALSE(nullptr != nullptr);
}

TEST(PointerComparisons, UnrelatedPointersStillCompare)
{
    RefPtr<tests::Base> base;
    RefPtr<tests::DerivedA> derA = base.staticCast<tests::DerivedA>();
    RefPtr<tests::DerivedB> derB = base.staticCast<tests::DerivedB>();

    EXPECT_TRUE(derA == derB);
}

#endif

END_BOOMER_NAMESPACE()