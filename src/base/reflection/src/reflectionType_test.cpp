/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/object/include/object.h"
#include "base/object/include/rttiClassRefType.h"

DECLARE_TEST_FILE(ReflectionType);

BEGIN_BOOMER_NAMESPACE(base::test)

class BaseClassA : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(BaseClassA, IObject);
};

class DevClassB : public BaseClassA
{
    RTTI_DECLARE_VIRTUAL_CLASS(DevClassB, BaseClassA);
};

class DevClassC : public BaseClassA
{
    RTTI_DECLARE_VIRTUAL_CLASS(DevClassC, BaseClassA);
};

RTTI_BEGIN_TYPE_CLASS(BaseClassA);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(DevClassB);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(DevClassC);
RTTI_END_TYPE();

//--

template< typename SrcType, typename DestType>
static bool Convert(const SrcType& a, const DestType& b)
{
    return rtti::ConvertData(&a, reflection::GetTypeObject<SrcType>(), &b, reflection::GetTypeObject<DestType>());
}

template< typename SrcType, typename DestType>
static void TestConvert(const SrcType& val, const DestType& expected, bool result=true)
{
    DestType dest = DestType();

    ASSERT_EQ(result, rtti::ConvertData(&val, reflection::GetTypeObject<SrcType>(), &dest, reflection::GetTypeObject<DestType>()))
        << "Conversion should succeed for: " << reflection::GetTypeName<SrcType>().c_str() << " ->  " << reflection::GetTypeName<DestType>().c_str() << ", src = " << testing::PrintToString(val);

    ASSERT_EQ(expected, dest) << "Conversion value is different for: " << reflection::GetTypeName<SrcType>().c_str() << " ->  " << reflection::GetTypeName<DestType>().c_str() << "src = " << testing::PrintToString(val);

}

template< typename SrcType, typename DestType>
static void TestNotConvert(const SrcType& val)
{
    DestType dest = DestType();

    ASSERT_FALSE(rtti::ConvertData(&val, reflection::GetTypeObject<SrcType>(), &dest, reflection::GetTypeObject<DestType>()))
        << "Conversion should fail for: " << reflection::GetTypeName<SrcType>().c_str() << " ->  " << reflection::GetTypeName<DestType>().c_str() << ", src = " << testing::PrintToString(val);
}

TEST(TypeConversion, Int8ToBool)
{
    TestConvert<char, bool>(42, true);
    TestConvert<char, bool>(0, false);
}

TEST(TypeConversion, Int8ToInt8)
{
    TestConvert<char, short>(42, 42);
}

TEST(TypeConversion, Int8ToInt16)
{
    TestConvert<char, short>(42, 42);
}

TEST(TypeConversion, Int8ToInt32)
{
    TestConvert<char, int>(42, 42);
}

TEST(TypeConversion, Int8ToInt64)
{
    TestConvert<char, int64_t>(42, 42);
}
TEST(TypeConversion, Int8ToUint8)
{
    TestConvert<char, uint8_t>(42, 42);
}

TEST(TypeConversion, Int8ToUint16)
{
    TestConvert<char, uint16_t>(42, 42);
}

TEST(TypeConversion, Int8ToUint32)
{
    TestConvert<char, uint16_t>(42, 42);
}

TEST(TypeConversion, Int8ToUint64)
{
    TestConvert<char, uint16_t>(42, 42);
}

TEST(TypeConversion, Int8ToFloat)
{
    TestConvert<char, float>(42, 42);
}

TEST(TypeConversion, Int8ToDouble)
{
    TestConvert<char, double>(42, 42);
}

TEST(TypeConversion, Int8ToString)
{
    TestConvert<char, StringBuf>(42, StringBuf("42"));
}

//--

TEST(TypeConversion, Int16ToBool)
{
    TestConvert<short, bool>(42, true);
    TestConvert<short, bool>(0, false);
}

TEST(TypeConversion, Int16ToInt8)
{
    TestConvert<short, short>(42, 42);
}

TEST(TypeConversion, Int16ToInt16)
{
    TestConvert<short, short>(42, 42);
}

TEST(TypeConversion, Int16ToInt32)
{
    TestConvert<short, int>(42, 42);
}

TEST(TypeConversion, Int16ToInt64)
{
    TestConvert<short, int64_t>(42, 42);
}
TEST(TypeConversion, Int16ToUint8)
{
    TestConvert<short, uint8_t>(42, 42);
}

TEST(TypeConversion, Int16ToUint16)
{
    TestConvert<short, uint16_t>(42, 42);
}

TEST(TypeConversion, Int16ToUint32)
{
    TestConvert<short, uint16_t>(42, 42);
}

TEST(TypeConversion, Int16ToUint64)
{
    TestConvert<short, uint16_t>(42, 42);
}

TEST(TypeConversion, Int16ToFloat)
{
    TestConvert<short, float>(42, 42);
}

TEST(TypeConversion, Int16ToDouble)
{
    TestConvert<short, double>(42, 42);
}

TEST(TypeConversion, Int16ToString)
{
    TestConvert<short, StringBuf>(42, StringBuf("42"));
}

//--

TEST(TypeConversion, Int32ToBool)
{
    TestConvert<int, bool>(42, true);
    TestConvert<int, bool>(0, false);
}

TEST(TypeConversion, Int32ToInt8)
{
    TestConvert<int, int>(42, 42);
}

TEST(TypeConversion, Int32ToInt16)
{
    TestConvert<int, int>(42, 42);
}

TEST(TypeConversion, Int32ToInt32)
{
    TestConvert<int, int>(42, 42);
}

TEST(TypeConversion, Int32ToInt64)
{
    TestConvert<int, int64_t>(42, 42);
}
TEST(TypeConversion, Int32ToUint8)
{
    TestConvert<int, uint8_t>(42, 42);
}

TEST(TypeConversion, Int32ToUint16)
{
    TestConvert<int, uint16_t>(42, 42);
}

TEST(TypeConversion, Int32ToUint32)
{
    TestConvert<int, uint16_t>(42, 42);
}

TEST(TypeConversion, Int32ToUint64)
{
    TestConvert<int, uint16_t>(42, 42);
}

TEST(TypeConversion, Int32ToFloat)
{
    TestConvert<int, float>(42, 42);
}

TEST(TypeConversion, Int32ToDouble)
{
    TestConvert<int, double>(42, 42);
}

TEST(TypeConversion, Int32ToString)
{
    TestConvert<int, StringBuf>(42, StringBuf("42"));
}


//--

TEST(TypeConversion, Int64ToBool)
{
    TestConvert<int64_t, bool>(42, true);
    TestConvert<int64_t, bool>(0, false);
}

TEST(TypeConversion, Int64ToInt8)
{
    TestConvert<int64_t, int64_t>(42, 42);
}

TEST(TypeConversion, Int64ToInt16)
{
    TestConvert<int64_t, int64_t>(42, 42);
}

TEST(TypeConversion, Int64ToInt32)
{
    TestConvert<int64_t, int64_t>(42, 42);
}

TEST(TypeConversion, Int64ToInt64)
{
    TestConvert<int64_t, int64_t>(42, 42);
}
TEST(TypeConversion, Int64ToUint8)
{
    TestConvert<int64_t, uint8_t>(42, 42);
}

TEST(TypeConversion, Int64ToUint16)
{
    TestConvert<int64_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Int64ToUint32)
{
    TestConvert<int64_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Int64ToUint64)
{
    TestConvert<int64_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Int64ToFloat)
{
    TestConvert<int64_t, float>(42, 42);
}

TEST(TypeConversion, Int64ToDouble)
{
    TestConvert<int64_t, double>(42, 42);
}

TEST(TypeConversion, Int64ToString)
{
    TestConvert<int64_t, StringBuf>(42, StringBuf("42"));
}

//-----------------------------------------

TEST(TypeConversion, Uint8ToBool)
{
    TestConvert<uint8_t, bool>(42, true);
    TestConvert<uint8_t, bool>(0, false);
}

TEST(TypeConversion, Uint8ToInt8)
{
    TestConvert<uint8_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint8ToInt16)
{
    TestConvert<uint8_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint8ToInt32)
{
    TestConvert<uint8_t, uint32_t>(42, 42);
}

TEST(TypeConversion, Uint8ToInt64)
{
    TestConvert<uint8_t, uint64_t>(42, 42);
}
TEST(TypeConversion, Uint8ToUint8)
{
    TestConvert<uint8_t, uint8_t>(42, 42);
}

TEST(TypeConversion, Uint8ToUint16)
{
    TestConvert<uint8_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint8ToUint32)
{
    TestConvert<uint8_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint8ToUint64)
{
    TestConvert<uint8_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint8ToFloat)
{
    TestConvert<uint8_t, float>(42, 42);
}

TEST(TypeConversion, Uint8ToDouble)
{
    TestConvert<uint8_t, double>(42, 42);
}

TEST(TypeConversion, Uint8ToString)
{
    TestConvert<uint8_t, StringBuf>(42, StringBuf("42"));
}

//--

TEST(TypeConversion, Uint16ToBool)
{
    TestConvert<uint16_t, bool>(42, true);
    TestConvert<uint16_t, bool>(0, false);
}

TEST(TypeConversion, Uint16ToInt8)
{
    TestConvert<uint16_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint16ToInt16)
{
    TestConvert<uint16_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint16ToInt32)
{
    TestConvert<uint16_t, uint32_t>(42, 42);
}

TEST(TypeConversion, Uint16ToInt64)
{
    TestConvert<uint16_t, uint64_t>(42, 42);
}
TEST(TypeConversion, Uint16ToUint8)
{
    TestConvert<uint16_t, uint8_t>(42, 42);
}

TEST(TypeConversion, Uint16ToUint16)
{
    TestConvert<uint16_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint16ToUint32)
{
    TestConvert<uint16_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint16ToUint64)
{
    TestConvert<uint16_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint16ToFloat)
{
    TestConvert<uint16_t, float>(42, 42);
}

TEST(TypeConversion, Uint16ToDouble)
{
    TestConvert<uint16_t, double>(42, 42);
}

TEST(TypeConversion, Uint16ToString)
{
    TestConvert<uint16_t, StringBuf>(42, StringBuf("42"));
}

//--

TEST(TypeConversion, Uint32ToBool)
{
    TestConvert<uint32_t, bool>(42, true);
    TestConvert<uint32_t, bool>(0, false);
}

TEST(TypeConversion, Uint32ToInt8)
{
    TestConvert<uint32_t, uint32_t>(42, 42);
}

TEST(TypeConversion, Uint32ToInt16)
{
    TestConvert<uint32_t, uint32_t>(42, 42);
}

TEST(TypeConversion, Uint32ToInt32)
{
    TestConvert<uint32_t, uint32_t>(42, 42);
}

TEST(TypeConversion, Uint32ToInt64)
{
    TestConvert<uint32_t, uint64_t>(42, 42);
}
TEST(TypeConversion, Uint32ToUint8)
{
    TestConvert<uint32_t, uint8_t>(42, 42);
}

TEST(TypeConversion, Uint32ToUint16)
{
    TestConvert<uint32_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint32ToUint32)
{
    TestConvert<uint32_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint32ToUint64)
{
    TestConvert<uint32_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint32ToFloat)
{
    TestConvert<uint32_t, float>(42, 42);
}

TEST(TypeConversion, Uint32ToDouble)
{
    TestConvert<uint32_t, double>(42, 42);
}

TEST(TypeConversion, Uint32ToString)
{
    TestConvert<uint32_t, StringBuf>(42, StringBuf("42"));
}


//--

TEST(TypeConversion, Uint64ToBool)
{
    TestConvert<uint64_t, bool>(42, true);
    TestConvert<uint64_t, bool>(0, false);
}

TEST(TypeConversion, Uint64ToInt8)
{
    TestConvert<uint64_t, uint64_t>(42, 42);
}

TEST(TypeConversion, Uint64ToInt16)
{
    TestConvert<uint64_t, uint64_t>(42, 42);
}

TEST(TypeConversion, Uint64ToInt32)
{
    TestConvert<uint64_t, uint64_t>(42, 42);
}

TEST(TypeConversion, Uint64ToInt64)
{
    TestConvert<uint64_t, uint64_t>(42, 42);
}
TEST(TypeConversion, Uint64ToUint8)
{
    TestConvert<uint64_t, uint8_t>(42, 42);
}

TEST(TypeConversion, Uint64ToUint16)
{
    TestConvert<uint64_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint64ToUint32)
{
    TestConvert<uint64_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint64ToUint64)
{
    TestConvert<uint64_t, uint16_t>(42, 42);
}

TEST(TypeConversion, Uint64ToFloat)
{
    TestConvert<uint64_t, float>(42, 42);
}

TEST(TypeConversion, Uint64ToDouble)
{
    TestConvert<uint64_t, double>(42, 42);
}

TEST(TypeConversion, Uint64ToString)
{
    TestConvert<uint64_t, StringBuf>(42, StringBuf("42"));
}

//----

TEST(TypeConversion, StringToInt8)
{
    TestConvert<StringBuf, char>(StringBuf("42"), 42);
    TestConvert<StringBuf, char>(StringBuf("-42"), -42);
    TestNotConvert<StringBuf, char>(StringBuf("128"));
    TestNotConvert<StringBuf, char>(StringBuf("-129"));
}

TEST(TypeConversion, StringToUint8)
{
    TestConvert<StringBuf, uint8_t>(StringBuf("42"), 42);
    TestConvert<StringBuf, uint8_t>(StringBuf("255"), 255);
    TestNotConvert<StringBuf, uint8_t>(StringBuf("-42"));
    TestNotConvert<StringBuf, uint8_t>(StringBuf("256"));
}

TEST(TypeConversion, StringToInt16)
{
    TestConvert<StringBuf, short>(StringBuf("42"), 42);
    TestConvert<StringBuf, short>(StringBuf("-42"), -42);
    TestConvert<StringBuf, short>(StringBuf("128"), 128);
    TestConvert<StringBuf, short>(StringBuf("-129"), -129);
    TestNotConvert<StringBuf, short>(StringBuf("32768"));
    TestNotConvert<StringBuf, short>(StringBuf("-32769"));
}

TEST(TypeConversion, StringToUint16)
{
    TestConvert<StringBuf, uint16_t>(StringBuf("42"), 42);
    TestConvert<StringBuf, uint16_t>(StringBuf("255"), 255);
    TestConvert<StringBuf, uint16_t>(StringBuf("65535"), 65535);
    TestNotConvert<StringBuf, uint16_t>(StringBuf("-42"));
    TestNotConvert<StringBuf, uint16_t>(StringBuf("65536"));
}

TEST(TypeConversion, StringToInt32)
{
    TestConvert<StringBuf, int>(StringBuf("42"), 42);
    TestConvert<StringBuf, int>(StringBuf("-42"), -42);
    TestConvert<StringBuf, int>(StringBuf("128"), 128);
    TestConvert<StringBuf, int>(StringBuf("-129"), -129);
    TestConvert<StringBuf, int>(StringBuf("32768"), 32768);
    TestConvert<StringBuf, int>(StringBuf("-32769"), -32769);
    TestNotConvert<StringBuf, int>(StringBuf("2147483648"));
    TestNotConvert<StringBuf, int>(StringBuf("-2147483649"));
}

TEST(TypeConversion, StringToUint32)
{
    TestConvert<StringBuf, uint32_t>(StringBuf("42"), 42);
    TestConvert<StringBuf, uint32_t>(StringBuf("255"), 255);
    TestConvert<StringBuf, uint32_t>(StringBuf("65535"), 65535);
    TestConvert<StringBuf, uint32_t>(StringBuf("4294967295"), 4294967295);
    TestNotConvert<StringBuf, uint32_t>(StringBuf("-42"));
    TestNotConvert<StringBuf, uint32_t>(StringBuf("4294967296"));
}

TEST(TypeConversion, StringToInt64)
{
    TestConvert<StringBuf, int64_t>(StringBuf("42"), 42);
    TestConvert<StringBuf, int64_t>(StringBuf("-42"), -42);
    TestConvert<StringBuf, int64_t>(StringBuf("128"), 128);
    TestConvert<StringBuf, int64_t>(StringBuf("-129"), -129);
    TestConvert<StringBuf, int64_t>(StringBuf("32768"), 32768);
    TestConvert<StringBuf, int64_t>(StringBuf("-32769"), -32769);
    TestConvert<StringBuf, int64_t>(StringBuf("2147483648"), 2147483648LL);
    TestConvert<StringBuf, int64_t>(StringBuf("-2147483649"), -2147483649LL);
}

TEST(TypeConversion, StringToUint64)
{
    TestConvert<StringBuf, uint64_t>(StringBuf("42"), 42);
    TestConvert<StringBuf, uint64_t>(StringBuf("255"), 255);
    TestConvert<StringBuf, uint64_t>(StringBuf("65535"), 65535);
    TestConvert<StringBuf, uint64_t>(StringBuf("4294967295"), 4294967295);
    TestConvert<StringBuf, uint64_t>(StringBuf("18446744073709551615"), 18446744073709551615ULL);
    //TestNotConvert<StringBuf, uint64_t>("-42");
    //TestNotConvert<StringBuf, uint64_t>("18446744073709551616");
}

//----

TEST(TypeConversion, HandleToBool)
{
    ObjectPtr nullHandle;
    TestConvert<ObjectPtr, bool>(nullHandle, false);

    ObjectPtr validHandle = RefNew<BaseClassA>();
    TestConvert<ObjectPtr, bool>(validHandle, true);
}

TEST(TypeConversion, HandleUpCast)
{
    RefPtr<DevClassB> handleB = RefNew<DevClassB>();
    RefPtr<BaseClassA> handleBase(handleB);

    TestConvert(handleB, handleBase);
}

TEST(TypeConversion, HandleDownCast)
{
    RefPtr<BaseClassA> handleBase = RefNew<DevClassB>();
    RefPtr<DevClassB> handleB = rtti_cast<DevClassB>(handleBase);

    TestConvert(handleBase, handleB);
}

TEST(TypeConversion, HandleUnrelatedDownCast)
{
    RefPtr<BaseClassA> handleBase = RefNew<DevClassB>();
    RefPtr<DevClassC> handleC = rtti_cast<DevClassC>(handleBase);

    TestConvert(handleBase, handleC, false);
}

TEST(TypeConversion, HandleUnrelatedClasses)
{
    RefPtr<DevClassB> handleB = RefNew<DevClassB>();
    TestNotConvert< RefPtr<DevClassB>, RefPtr<DevClassC> >(handleB);
}

TEST(TypeConversion, BaseClassRefCast)
{
    SpecificClassType<BaseClassA> handleA(reflection::ClassID<DevClassB>());
    SpecificClassType<DevClassB> handleB = rtti_cast<DevClassB>(handleA);
    EXPECT_EQ(handleA.ptr(), handleB.ptr());
}

TEST(TypeConversion, BaseClassRefFailedCast)
{
    SpecificClassType<BaseClassA> handleA(reflection::ClassID<DevClassC>());
    SpecificClassType<DevClassB> handleB = rtti_cast<DevClassB>(handleA);
    EXPECT_EQ(nullptr, handleB.ptr());
}

TEST(TypeConversion, BaseClassUpCast)
{
    SpecificClassType<DevClassB> handleA(reflection::ClassID<DevClassB>());
    SpecificClassType<BaseClassA> handleB = rtti_cast<BaseClassA>(handleA);
    EXPECT_EQ(handleA.ptr(), handleB.ptr());
}

TEST(TypeConversion, BaseClassRefCastRTTI)
{
    SpecificClassType<BaseClassA> handleA(reflection::ClassID<DevClassB>());
    SpecificClassType<DevClassB> handleB = rtti_cast<DevClassB>(handleA);
    TestConvert(handleA, handleB);
}

TEST(TypeConversion, BaseClassRefNotRelatedCastRTTI)
{
    SpecificClassType<DevClassC> handleA(reflection::ClassID<DevClassC>());
    TestNotConvert< SpecificClassType<DevClassC>, SpecificClassType<DevClassB> >(handleA);
}

TEST(TypeConversion, BaseClassRefNotCompataibleCastRTTI)
{
    SpecificClassType<BaseClassA> handleA(reflection::ClassID<BaseClassA>());
    SpecificClassType<DevClassB> handleB = rtti_cast<DevClassB>(handleA); // null
    TestConvert(handleA, handleB, false);
}

TEST(TypeConversion, BaseClassUpCastRTTI)
{
    SpecificClassType<DevClassB> handleA(reflection::ClassID<DevClassB>());
    SpecificClassType<BaseClassA> handleB = rtti_cast<BaseClassA>(handleA);
    TestConvert(handleA, handleB);
}

END_BOOMER_NAMESPACE(base::test)