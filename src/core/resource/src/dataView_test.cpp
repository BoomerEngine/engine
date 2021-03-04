/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "core/test/include/gtest/gtest.h"
#include "core/object/include/object.h"
#include "core/reflection/include/reflectionMacros.h"
#include "core/object/include/rttiDataView.h"

DECLARE_TEST_FILE(DataView);

BEGIN_BOOMER_NAMESPACE_EX(test)

class DataViewTestObject : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataViewTestObject, IObject);

public:
    char m_int8;
    short m_int16;
    int m_int32;
    int64_t m_int64;
    uint8_t m_uint8;
    uint16_t m_uint16;
    uint32_t m_uint32;
    uint64_t m_uint64;
    float m_float;
    double m_double;
    bool m_bool;

    StringBuf m_string;
    StringID m_stringID;

    Vector3 m_vector;
    Box m_box;

    Array<int> m_intArray;
    Array<Vector3 > m_vectorArray;
    Array<Array<int>> m_intArrayArray;
    Array<Array<Vector3>> m_vectorArrayArray;

     InplaceArray<int, 16> m_staticArray;

    float m_nativeArray[5];

    RefPtr<DataViewTestObject> m_ptr;
    RefPtr<DataViewTestObject> m_inlined;

    int m_readOnly;
    int m_nonEditable;

    //--


    DataHolder m_valueToOverride;
    mutable DataHolder m_lastPropertyChangingValue;
    mutable StringBuf m_lastPropertyChangingPath;
    StringBuf m_lastPropertyChangedPath;
    bool m_failPropertyChanging;

    DataViewTestObject()
    {
        m_int8 = -8;
        m_int16 = -16;
        m_int32 = -32;
        m_int64 = -64;
        m_uint8 = 8;
        m_uint16 = 16;
        m_uint32 = 32;
        m_uint64 = 64;
        m_float = 1.618f;
        m_double = 3.1415926535;
        m_bool = true;

        m_string = StringBuf("Ala ma kota");
        m_stringID = "test"_id;

        m_readOnly = 666;
        m_nonEditable = 123;

        m_vector = Vector3(1,2,3);
        m_box = Box(Vector3(-1,-2,-3), Vector3(4,5,6));

        m_intArray.pushBack(5);
        m_intArray.pushBack(10);
        m_intArray.pushBack(15);

        m_vectorArray.pushBack(Vector3(1,2,3));
        m_vectorArray.pushBack(Vector3(4,5,6));

        m_intArrayArray.emplaceBack();
        m_intArrayArray.back().pushBack(1);
        m_intArrayArray.back().pushBack(2);
        m_intArrayArray.back().pushBack(3);
        m_intArrayArray.back().pushBack(4);
        m_intArrayArray.emplaceBack();
        m_intArrayArray.back().pushBack(-3);
        m_intArrayArray.back().pushBack(-2);
        m_intArrayArray.back().pushBack(-1);

        m_vectorArrayArray.emplaceBack();
        m_vectorArrayArray.back().pushBack(Vector3(1,2,3));
        m_vectorArrayArray.back().pushBack(Vector3(4,5,6));
        m_vectorArrayArray.emplaceBack();
        m_vectorArrayArray.back().pushBack(Vector3(-1,-2,-3));
        m_vectorArrayArray.back().pushBack(Vector3(-4,-5,-6));

        m_staticArray.pushBack(5);
        m_staticArray.pushBack(10);
        m_staticArray.pushBack(15);

        m_nativeArray[0] = 1.0f;
        m_nativeArray[1] = 2.0f;
        m_nativeArray[2] = 3.0f;
        m_nativeArray[3] = 4.0f;
        m_nativeArray[4] = 5.0f;

        m_failPropertyChanging = false;
    }

    virtual bool onPropertyChanging(StringView path, const void* data, Type dataType) const override
    {
        m_lastPropertyChangingPath = StringBuf(path);
        m_lastPropertyChangingValue = DataHolder(dataType, data);

        /*if (!m_valueToOverride.empty())
            valueToSet = m_valueToOverride;*/

        if (m_failPropertyChanging)
            return false;

        return true;
    }

    virtual void onPropertyChanged(StringView path) override
    {
        m_lastPropertyChangedPath = StringBuf(path);
    }
};

RTTI_BEGIN_TYPE_CLASS(DataViewTestObject);
    RTTI_PROPERTY(m_int8).editable();
    RTTI_PROPERTY(m_int16).editable();
    RTTI_PROPERTY(m_int32).editable();
    RTTI_PROPERTY(m_int64).editable();
    RTTI_PROPERTY(m_uint8).editable();
    RTTI_PROPERTY(m_uint16).editable();
    RTTI_PROPERTY(m_uint32).editable();
    RTTI_PROPERTY(m_uint64).editable();
    RTTI_PROPERTY(m_float).editable();
    RTTI_PROPERTY(m_double).editable();
    RTTI_PROPERTY(m_bool).editable();
    RTTI_PROPERTY(m_string).editable();
    RTTI_PROPERTY(m_stringID).editable();
    RTTI_PROPERTY(m_vector).editable();
    RTTI_PROPERTY(m_box).editable();
    RTTI_PROPERTY(m_intArray).editable();
    RTTI_PROPERTY(m_vectorArray).editable();
    RTTI_PROPERTY(m_intArrayArray).editable();
    RTTI_PROPERTY(m_vectorArrayArray).editable();
    RTTI_PROPERTY(m_staticArray).editable();
    RTTI_PROPERTY(m_nativeArray).editable();
    RTTI_PROPERTY(m_ptr).editable();
    RTTI_PROPERTY(m_inlined).editable().inlined();
    RTTI_PROPERTY(m_readOnly).editable().readonly();
    RTTI_PROPERTY(m_nonEditable);
RTTI_END_TYPE();

template< typename T >
INLINE bool DescribeView(StringView viewPath, const T& data, DataViewInfo& outInfo)
{
    return GetTypeObject<T>()->describeDataView(viewPath, &data, outInfo).valid();
}

template< typename T, typename U >
INLINE bool ReadView(StringView viewPath, const T& data, U& outData)
{
    return GetTypeObject<T>()->readDataView(viewPath, &data, &outData, GetTypeObject<U>()).valid();
}

template< typename T, typename U >
INLINE bool WriteView(StringView viewPath, T& data, const U& inputData)
{
    return GetTypeObject<T>()->writeDataView(viewPath, &data, &inputData, GetTypeObject<U>()).valid();
}


TEST(EditView, SimpleIntViewDesribed)
{
    int value = 42;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("", value, info));
}

TEST(EditView, SimpleIntViewTypeReturned)
{
    int value = 42;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<int>());
}

TEST(EditView, SimpleIntViewPointerPointsToValue)
{
    int value = 42;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("", value, info));
    EXPECT_EQ(info.dataPtr, &value);
}

TEST(EditView, SimpleIntValueLikeReported)
{
    int value = 42;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("", value, info));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeValue));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeStruct));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeArray));
}

TEST(EditView, SimpleIntReadWorks)
{
    int value = 42;
    DataViewInfo info;
    int temp = 0;
    ASSERT_TRUE(ReadView("", value, temp));
    EXPECT_EQ(value, temp);
}

TEST(EditView, SimpleIntWriteWorks)
{
    int value = 0;
    DataViewInfo info;
    int temp = 24;
    ASSERT_TRUE(WriteView("", value, temp));
    EXPECT_EQ(value, temp);
}

TEST(EditView, SimpleIntReadIfMemberFails)
{
    int value = 42;
    DataViewInfo info;
    int temp = 0;
    ASSERT_FALSE(ReadView("x", value, temp));
    EXPECT_EQ(0, temp);
}

TEST(EditView, SimpleIntReadIfArrayFails)
{
    int value = 42;
    DataViewInfo info;
    int temp = 0;
    ASSERT_FALSE(ReadView("[0]", value, temp));
    EXPECT_EQ(0, temp);
}

TEST(EditView, SimpleIntWriteMemberFails)
{
    int value = 42;
    DataViewInfo info;
    int temp = 0;
    ASSERT_FALSE(WriteView("x", value, temp));
    EXPECT_EQ(42, value);
}

TEST(EditView, SimpleIntWriteOfArrayFails)
{
    int value = 42;
    DataViewInfo info;
    int temp = 0;
    ASSERT_FALSE(WriteView("[0]", value, temp));
    EXPECT_EQ(42, value);
}

TEST(EditView, SimpleIntReadToString)
{
    int value = 42;
    DataViewInfo info;
    StringBuf txt;
    ASSERT_TRUE(ReadView("", value, txt));
    EXPECT_STREQ("42", txt.c_str());
}

TEST(EditView, SimpleIntWriteFromString)
{
    int value = 42;
    DataViewInfo info;
    StringBuf txt("666");
    ASSERT_TRUE(WriteView("", value, txt));
    EXPECT_EQ(666, value);
}

///---

TEST(EditView, Vector3FlagsReported)
{
    Vector3 value;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("", value, info));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeValue));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeStruct));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeArray));
    EXPECT_EQ(info.dataType, GetTypeObject<Vector3>());
}

TEST(EditView, Vector3MembersNotReportedByDefault)
{
    Vector3 value;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("", value, info));
    EXPECT_EQ(0, info.members.size());
}

TEST(EditView, Vector3MembersReportedWhenAsked)
{
    Vector3 value;
    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::MemberList;
    ASSERT_TRUE(DescribeView("", value, info));
    EXPECT_EQ(3, info.members.size());
}

TEST(EditView, Vector3MembersNamesCorrect)
{
    Vector3 value;
    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::MemberList;
    ASSERT_TRUE(DescribeView("", value, info));
    ASSERT_EQ(3, info.members.size());
    EXPECT_STREQ("x", info.members[0].name.c_str());
    EXPECT_STREQ("y", info.members[1].name.c_str());
    EXPECT_STREQ("z", info.members[2].name.c_str());
}

TEST(EditView, Vector3MembersCanDescribeThemselves)
{
    Vector3 value;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("x", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<float>());
    ASSERT_TRUE(DescribeView("y", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<float>());
    ASSERT_TRUE(DescribeView("z", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<float>());
}

TEST(EditView, Vector3MembersReturnCorrectPointers)
{
    Vector3 value;
    DataViewInfo info;
    ASSERT_TRUE(DescribeView("x", value, info));
    EXPECT_EQ(info.dataPtr, &value.x);
    ASSERT_TRUE(DescribeView("y", value, info));
    EXPECT_EQ(info.dataPtr, &value.y);
    ASSERT_TRUE(DescribeView("z", value, info));
    EXPECT_EQ(info.dataPtr, &value.z);
}

TEST(EditView, Vector3MembersDescribeUnknownMemberFails)
{
    Vector3 value;
    DataViewInfo info;
    ASSERT_FALSE(DescribeView("w", value, info));
}

TEST(EditView, Vector3MembersDescribeArrayFails)
{
    Vector3 value;
    DataViewInfo info;
    ASSERT_FALSE(DescribeView("[0]", value, info));
}

TEST(EditView, Vector3MembersRead)
{
    Vector3 value(1,2,3);
    DataViewInfo info;
    float temp = 0.0f;
    ASSERT_TRUE(ReadView("x", value, temp));
    EXPECT_FLOAT_EQ(temp, value.x);
    ASSERT_TRUE(ReadView("y", value, temp));
    EXPECT_FLOAT_EQ(temp, value.y);
    ASSERT_TRUE(ReadView("z", value, temp));
    EXPECT_FLOAT_EQ(temp, value.z);
}

TEST(EditView, Vector3InvalidReadFails)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    float temp = 5.0f;
    ASSERT_FALSE(ReadView("w", value, temp));
    EXPECT_FLOAT_EQ(temp, 5.0f);
}

TEST(EditView, Vector3WholeValueAsVector3)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    Vector3 temp(0, 0, 0);
    ASSERT_TRUE(ReadView("", value, temp));
    EXPECT_FLOAT_EQ(temp.x, value.x);
    EXPECT_FLOAT_EQ(temp.y, value.y);
    EXPECT_FLOAT_EQ(temp.z, value.z);
}

TEST(EditView, Vector3WholeValueReadAsString)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    StringBuf txt;
    ASSERT_TRUE(ReadView("", value, txt));
    EXPECT_STREQ("(x=1.000000)(y=2.000000)(z=3.000000)", txt.c_str());
}

TEST(EditView, Vector3MembersWrite)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    ASSERT_TRUE(WriteView("x", value, 4.0f));
    ASSERT_TRUE(WriteView("y", value, 5.0f));
    ASSERT_TRUE(WriteView("z", value, 6.0f));

    EXPECT_FLOAT_EQ(4.0f, value.x);
    EXPECT_FLOAT_EQ(5.0f, value.y);
    EXPECT_FLOAT_EQ(6.0f, value.z);
}

TEST(EditView, Vector3WholeValueWriteAsVector3)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    Vector3 temp(4, 5, 6);
    ASSERT_TRUE(WriteView("", value, temp));
    EXPECT_FLOAT_EQ(temp.x, value.x);
    EXPECT_FLOAT_EQ(temp.y, value.y);
    EXPECT_FLOAT_EQ(temp.z, value.z);
}

TEST(EditView, Vector3WholeValueWriteAsString)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    StringBuf txt("(x=4)(y=5)(z=6)");
    ASSERT_TRUE(WriteView("", value, txt));
    EXPECT_FLOAT_EQ(4.0f, value.x);
    EXPECT_FLOAT_EQ(5.0f, value.y);
    EXPECT_FLOAT_EQ(6.0f, value.z);
}

TEST(EditView, Vector3PartialValueWriteAsString)
{
    Vector3 value(1, 2, 3);
    DataViewInfo info;
    StringBuf txt("(x=4)(z=6)");
    ASSERT_TRUE(WriteView("", value, txt));
    EXPECT_FLOAT_EQ(4.0f, value.x);
    EXPECT_FLOAT_EQ(2.0f, value.y);
    EXPECT_FLOAT_EQ(6.0f, value.z);
}

TEST(EditView, NestedStructureRead)
{
    Box value(Vector3(1, 2, 3), Vector3(4, 5, 6));
    DataViewInfo info;
    float temp = 0.0f;
    EXPECT_TRUE(ReadView("min.x", value, temp));
    EXPECT_FLOAT_EQ(1.0f, temp);
    EXPECT_TRUE(ReadView("min.y", value, temp));
    EXPECT_FLOAT_EQ(2.0f, temp);
    EXPECT_TRUE(ReadView("min.z", value, temp));
    EXPECT_FLOAT_EQ(3.0f, temp);
    EXPECT_TRUE(ReadView("max.x", value, temp));
    EXPECT_FLOAT_EQ(4.0f, temp);
    EXPECT_TRUE(ReadView("max.y", value, temp));
    EXPECT_FLOAT_EQ(5.0f, temp);
    EXPECT_TRUE(ReadView("max.z", value, temp));
    EXPECT_FLOAT_EQ(6.0f, temp);
}

TEST(EditView, NestedStructureWrite)
{
    Box value;
    DataViewInfo info;
    EXPECT_TRUE(WriteView("min.x", value, 1.0f));
    EXPECT_TRUE(WriteView("min.y", value, 2.0f));
    EXPECT_TRUE(WriteView("min.z", value, 3.0f));
    EXPECT_TRUE(WriteView("max.x", value, 4.0f));
    EXPECT_TRUE(WriteView("max.y", value, 5.0f));
    EXPECT_TRUE(WriteView("max.z", value, 6.0f));
    EXPECT_FLOAT_EQ(1.0f, value.min.x);
    EXPECT_FLOAT_EQ(2.0f, value.min.y);
    EXPECT_FLOAT_EQ(3.0f, value.min.z);
    EXPECT_FLOAT_EQ(4.0f, value.max.x);
    EXPECT_FLOAT_EQ(5.0f, value.max.y);
    EXPECT_FLOAT_EQ(6.0f, value.max.z);
}

//--

TEST(EditView, StaticArrayReportedAsArray)
{
    int value[4];
    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", value, info));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeArray));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeValue));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeStruct));
    EXPECT_EQ(0, info.members.size());
}

TEST(EditView, StaticArrayDoesNotReportMembers)
{
    int value[4];
    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::MemberList;
    EXPECT_TRUE(DescribeView("", value, info));
    EXPECT_EQ(0, info.members.size());
}

TEST(EditView, StaticArraySizeReported)
{
    int value[4];
    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", value, info));
    EXPECT_EQ(4, info.arraySize);
}

TEST(EditView, StaticArrayMembersDescribeThemselves)
{
    int value[4];
    DataViewInfo info;
    EXPECT_TRUE(DescribeView("[0]", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<int>());
    EXPECT_TRUE(DescribeView("[1]", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<int>());
    EXPECT_TRUE(DescribeView("[2]", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<int>());
    EXPECT_TRUE(DescribeView("[3]", value, info));
    EXPECT_EQ(info.dataType, GetTypeObject<int>());
}

TEST(EditView, StaticArrayMembersReturnPropertPointers)
{
    int value[4];
    DataViewInfo info;
    EXPECT_TRUE(DescribeView("[0]", value, info));
    EXPECT_EQ(info.dataPtr, &value[0]);
    EXPECT_TRUE(DescribeView("[1]", value, info));
    EXPECT_EQ(info.dataPtr, &value[1]);
    EXPECT_TRUE(DescribeView("[2]", value, info));
    EXPECT_EQ(info.dataPtr, &value[2]);
    EXPECT_TRUE(DescribeView("[3]", value, info));
    EXPECT_EQ(info.dataPtr, &value[3]);
}

TEST(EditView, StaticArrayMemberOutofRangeFailsDescribe)
{
    int value[4];
    DataViewInfo info;
    EXPECT_FALSE(DescribeView("[5]", value, info));
    EXPECT_FALSE(DescribeView("[-1]", value, info));
    EXPECT_FALSE(DescribeView("[aab]", value, info));
    EXPECT_FALSE(DescribeView("[1", value, info));
    EXPECT_FALSE(DescribeView("[[[[1223", value, info));
    EXPECT_FALSE(DescribeView("[1]]", value, info));
    EXPECT_FALSE(DescribeView("[1][", value, info));
    EXPECT_FALSE(DescribeView("[1].x", value, info));
    EXPECT_FALSE(DescribeView("[1][0]", value, info));
}

TEST(EditView, StaticArrayMembersRead)
{
    int value[4] = { 1,2,3,4 };
    DataViewInfo info;
    int temp = 0;
    EXPECT_TRUE(ReadView("[0]", value, temp));
    EXPECT_EQ(1, temp);
    EXPECT_TRUE(ReadView("[1]", value, temp));
    EXPECT_EQ(2, temp);
    EXPECT_TRUE(ReadView("[2]", value, temp));
    EXPECT_EQ(3, temp);
    EXPECT_TRUE(ReadView("[3]", value, temp));
    EXPECT_EQ(4, temp);
}


TEST(EditView, StaticArrayMembersWrite)
{
    int value[4] = { 1,2,3,4 };
    DataViewInfo info;
    EXPECT_TRUE(WriteView("[0]", value, 5));
    EXPECT_TRUE(WriteView("[1]", value, 6));
    EXPECT_TRUE(WriteView("[2]", value, 7));
    EXPECT_TRUE(WriteView("[3]", value, 8));
    EXPECT_EQ(5, value[0]);
    EXPECT_EQ(6, value[1]);
    EXPECT_EQ(7, value[2]);
    EXPECT_EQ(8, value[3]);
}

TEST(EditView, StaticArrayReadWholeAsString)
{
    int value[4] = { 1,2,3,4 };
    DataViewInfo info;
    StringBuf txt;
    EXPECT_TRUE(ReadView("", value, txt));
    EXPECT_STREQ(txt.c_str(), "[1][2][3][4]");
}

TEST(EditView, StaticArrayWriteWholeString)
{
    int value[4] = { 1,2,3,4 };
    DataViewInfo info;
    StringBuf txt("[5][6][7][8]");
    EXPECT_TRUE(WriteView("", value, txt));
    EXPECT_EQ(5, value[0]);
    EXPECT_EQ(6, value[1]);
    EXPECT_EQ(7, value[2]);
    EXPECT_EQ(8, value[3]);
}

TEST(EditView, StaticArrayPartialWriteString)
{
    int value[4] = { 1,2,3,4 };
    DataViewInfo info;
    StringBuf txt("[5][6]");
    EXPECT_TRUE(WriteView("", value, txt));
    EXPECT_EQ(5, value[0]);
    EXPECT_EQ(6, value[1]);
    EXPECT_EQ(0, value[2]);
    EXPECT_EQ(0, value[3]);
}

TEST(EditView, StaticArrayOverflowWriteStringDoesNotFail)
{
    int protectorA[2] = { 0,0 };
    int value[4] = { 1,2,3,4 };
    int protectorB[2] = { 0,0 };
    DataViewInfo info;
    StringBuf txt("[5][6][7][8][9]");
    EXPECT_TRUE(WriteView("", value, txt));
    EXPECT_EQ(5, value[0]);
    EXPECT_EQ(6, value[1]);
    EXPECT_EQ(7, value[2]);
    EXPECT_EQ(8, value[3]);
    EXPECT_EQ(0, protectorA[0]);
    EXPECT_EQ(0, protectorA[1]);
    EXPECT_EQ(0, protectorB[0]);
    EXPECT_EQ(0, protectorB[1]);
}

//--

TEST(EditView, PointerDescribedPropertlyWhenAsHandle)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeStruct));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeArray));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::Object));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeValue));
}

TEST(EditView, PointerEmptyDescribedAsNonStruct)
{
    RefPtr<DataViewTestObject> ptr;

    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeStruct));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeArray));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::Object));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeValue));
}

TEST(EditView, PointerDoesNotReportMembersWhenNotAsked)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_EQ(0, info.members.size());
}

TEST(EditView, PointerDoesNotSetObjectDataWhenNotAsked)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_EQ(nullptr, info.objectPtr);
    EXPECT_FALSE(info.objectClass);
}

TEST(EditView, PointerDescribedPointerValid)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", ptr, info));
    //EXPECT_EQ(info.dataPtr, ptr.get());
    EXPECT_EQ(info.dataPtr, &ptr);
}

TEST(EditView, PointerDescribedTypeIsHandleType)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_EQ(info.dataType, GetTypeObject<RefPtr<DataViewTestObject>>());
}

TEST(EditView, PointerDescribedAsObjectWhenAsked)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::ObjectInfo;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_FALSE(info.flags.test(DataViewInfoFlagBit::LikeArray));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeStruct));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::Object));
    EXPECT_TRUE(info.flags.test(DataViewInfoFlagBit::LikeValue));
}

TEST(EditView, PointerTypeStillResportedAsHandleEvenWhenObjectDataAsked)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::ObjectInfo;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_EQ(info.dataType, GetTypeObject<RefPtr<DataViewTestObject>>());
}

TEST(EditView, PointerDescribedObjectDataWhenAsked)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::ObjectInfo;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_EQ(info.objectPtr, ptr.get());
    EXPECT_EQ(info.objectClass, DataViewTestObject::GetStaticClass());
}

TEST(EditView, PointerDoesReportMembersWhenAsked)
{
    auto ptr = RefNew<DataViewTestObject>();

    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::MemberList;
    EXPECT_TRUE(DescribeView("", ptr, info));
    EXPECT_EQ(24, info.members.size());
}

TEST(EditView, PointerReadWholeValueReadsAPointer)
{
    auto ptr = RefNew<DataViewTestObject>();
    RefPtr<DataViewTestObject> ptr2;

    DataViewInfo info;
    EXPECT_TRUE(ReadView("", ptr, ptr2));
    EXPECT_EQ(ptr.get(), ptr2.get());
}

TEST(EditView, PointerReadWholeValueWriteWorks)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();

    DataViewInfo info;
    EXPECT_TRUE(WriteView("", ptr, ptr2));
    EXPECT_EQ(ptr.get(), ptr2.get());
}

TEST(EditView, PointerReadMember)
{
    auto ptr = RefNew<DataViewTestObject>();
    ptr->m_uint32 = 1234;

    uint32_t temp = 0;
    EXPECT_TRUE(ReadView("uint32", ptr, temp));
    EXPECT_EQ(ptr->m_uint32, temp);

    StringBuf txt;
    EXPECT_TRUE(ReadView("string", ptr, txt));
    EXPECT_STREQ(ptr->m_string.c_str(), txt.c_str());
}


TEST(EditView, PointerReadFollowsInlined)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();
    auto ptr3 = RefNew<DataViewTestObject>();
    ptr->m_inlined = ptr2;
    ptr2->m_uint32 = 666;
    ptr2->m_inlined = ptr3;
    ptr3->m_uint32 = 1234;

    uint32_t temp = 0;
    EXPECT_TRUE(ReadView("uint32", ptr, temp));
    EXPECT_EQ(32, temp);

    temp = 0;
    EXPECT_TRUE(ReadView("inlined.uint32", ptr, temp));
    EXPECT_EQ(666, temp);

    temp = 0;
    EXPECT_TRUE(ReadView("inlined.inlined.uint32", ptr, temp));
    EXPECT_EQ(1234, temp);
}

TEST(EditView, PointerReadReturnsPointer)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();
    ptr->m_inlined = ptr2;

    RefPtr<DataViewTestObject> temp;
    EXPECT_TRUE(ReadView("inlined", ptr, temp));
    EXPECT_EQ(temp.get(), ptr2.get());
}

TEST(EditView, PointerWriteChnagesPointer)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();
    auto ptr3 = RefNew<DataViewTestObject>();
    ptr->m_inlined = ptr2;

    RefPtr<DataViewTestObject> temp;
    EXPECT_TRUE(ReadView("inlined", ptr, temp));
    EXPECT_EQ(temp.get(), ptr2.get());
    EXPECT_TRUE(WriteView("inlined", ptr, ptr3));
    EXPECT_EQ(ptr->m_inlined.get(), ptr3.get());
    EXPECT_TRUE(ReadView("inlined", ptr, temp));
    EXPECT_EQ(temp.get(), ptr3.get());
}

TEST(EditView, PointerNullPtrReadFails)
{
    auto ptr = RefNew<DataViewTestObject>();

    uint32_t temp = 0;
    EXPECT_TRUE(ReadView("uint32", ptr, temp));
    EXPECT_EQ(32, temp);

    temp = 0;
    EXPECT_FALSE(ReadView("inlined.uint32", ptr, temp));
    EXPECT_EQ(0, temp);
}

/*TEST(EditView, PointerCantFollowNonInlined)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();
    ptr->m_ptr = ptr2;

    uint32_t temp = 0;
    EXPECT_FALSE(ReadView("ptr.uint32", ptr, temp));
    EXPECT_EQ(0, temp);
}*/

TEST(EditView, PointerCantStillReadNonInlined)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();
    ptr->m_ptr = ptr2;

    RefPtr<DataViewTestObject> temp;
    EXPECT_TRUE(ReadView("ptr", ptr, temp));
    EXPECT_EQ(temp.get(), ptr2.get());
}

TEST(EditView, PointerCantStillWriteNonInlined)
{
    auto ptr = RefNew<DataViewTestObject>();
    auto ptr2 = RefNew<DataViewTestObject>();
    auto ptr3 = RefNew<DataViewTestObject>();
    ptr->m_ptr = ptr2;

    RefPtr<DataViewTestObject> temp;
    EXPECT_TRUE(ReadView("ptr", ptr, temp));
    EXPECT_EQ(temp.get(), ptr2.get());
    EXPECT_TRUE(WriteView("ptr", ptr, ptr3));
    EXPECT_EQ(ptr->m_ptr.get(), ptr3.get());
    EXPECT_TRUE(ReadView("ptr", ptr, temp));
    EXPECT_EQ(temp.get(), ptr3.get());
}

//--

END_BOOMER_NAMESPACE_EX(test)
