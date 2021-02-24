/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/object/include/rttiClassRef.h"
#include "reflectionTypeName.h"

DECLARE_TEST_FILE(RttiClassRef);

BEGIN_BOOMER_NAMESPACE(base::test);

TEST(ClassRef, EmptyClassIsNull)
{
    ClassType base;
    EXPECT_EQ(nullptr, base.ptr());
}

TEST(ClassRef, EmptyClassNameIsEmpty)
{
    ClassType base;
    StringBuilder txt;
    txt << base.name();
    EXPECT_STREQ("", txt.c_str());
}

TEST(ClassRef, EmptyClassPrintsNull)
{
    ClassType base;
    StringBuilder txt;
    txt << base;
    EXPECT_STREQ("null", txt.c_str());
}

TEST(ClassRef, TypeRecognized)
{
    auto typeName  = reflection::GetTypeName< SpecificClassType<IObject> >();
    EXPECT_FALSE(typeName.empty()); 
    auto typeObject  = RTTI::GetInstance().findType(typeName);
    EXPECT_NE(nullptr, typeObject.ptr());
}

TEST(ClassRef, TypeOnlyAcceptsClasses)
{
    auto typeName  = reflection::GetTypeName< SpecificClassType< float > >();
    EXPECT_FALSE(typeName.empty());
    auto typeObject  = RTTI::GetInstance().findType(typeName);
    EXPECT_EQ(nullptr, typeObject.ptr());
}

TEST(ClassRef, EmptyClassIsValidBase)
{
    ClassType base;
    ClassType x = reflection::ClassID<IObject>();
    ClassType y = reflection::ClassID<rtti::IMetadata>();
    EXPECT_TRUE(x.is(base));
    EXPECT_TRUE(y.is(base));
}

TEST(ClassRef, UnrelatedTypesDontMatch)
{
    ClassType x = reflection::ClassID<IObject>();
    ClassType y = reflection::ClassID<rtti::IMetadata>();
    EXPECT_FALSE(x.is(y));
    EXPECT_FALSE(y.is(x));
}

//--

class ClassRefA : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ClassRefA, IObject);

public:
};

RTTI_BEGIN_TYPE_CLASS(ClassRefA);
RTTI_END_TYPE();

class ClassRefA2 : public ClassRefA
{
    RTTI_DECLARE_VIRTUAL_CLASS(ClassRefA2, ClassRefA);

public:
};

RTTI_BEGIN_TYPE_CLASS(ClassRefA2);
RTTI_END_TYPE();

class ClassRefB : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ClassRefB, IObject);

public:
};

RTTI_BEGIN_TYPE_CLASS(ClassRefB);
RTTI_END_TYPE();

//--

TEST(ClassRef, ObjectOfProperClassIsCreated)
{
    SpecificClassType<ClassRefA> x = test::ClassRefA2::GetStaticClass();
    auto obj = x.create();

    EXPECT_EQ(obj->cls(), ClassRefA2::GetStaticClass());
}

TEST(ClassRef, ObjectOfUnrelatedClassIsNotCreated)
{
    ClassType x = ClassRefA2::GetStaticClass();
    RefPtr<ClassRefB> obj = x.create<test::ClassRefB>();

    EXPECT_TRUE(obj.empty());
}

END_BOOMER_NAMESPACE(base::test)