/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"

#include "rttiTypeSystem.h"
#include "rttiType.h"
#include "rttiEnumType.h"
#include "rttiHandleType.h"
#include "rttiClassRefType.h"

#include "core/containers/include/stringBuf.h"
#include "core/containers/include/stringID.h"
#include "rttiResourceReferenceType.h"

BEGIN_BOOMER_NAMESPACE_EX(rtti)

namespace prv
{
    /// no conversion at all
    static bool NoConversion(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        return false;
    }

    /// simple conversion based on range cast
    template< typename SrcType, typename DestType >
    static bool RangeConversion(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        *(DestType*)destData = range_cast<DestType>(*(const SrcType*)srcData);
        return true;
    }

    /// simple conversion to bool based on comparison with default element
    template< typename SrcType >
    static bool BoolConversion(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        SrcType zero = SrcType();
        *(bool*)destData = ((*(const SrcType*)srcData) != zero);
        return true;
    }

    /// simple conversion to string
    static bool ToStringConversion(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        StringBuilder txt;
        srcType->printToText(txt, srcData);

        auto& dest = *(StringBuf*)destData;
        dest = txt.toString();
        return true;
    }

    /// simple conversion from string
    static bool FromStringConversion(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto& src = *(const StringBuf*)srcData;
        return destType->parseFromString(src.view(), destData);
    }

    /// strong handle to bool conversion
    static bool Conversion_StrongHandle_bool(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto& srcHandle = *(const RefPtr<IObject>*)srcData;
        *(bool*)destData = (srcHandle.get() != nullptr);
        return true;
    }

    /// weak handle to bool conversion
    static bool Conversion_WeakHandle_bool(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto& srcHandle = *(const RefWeakPtr<IObject>*)srcData;
        *(bool*)destData = !srcHandle.expired();
        return true;
    }

    /// enum to string
    static bool Conversion_Enum_StringBuf(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto srcEnum  = static_cast<const EnumType*>(srcType);
        ASSERT_EX(srcType->metaType() == MetaType::Enum, "Expected enum type");
        return srcEnum->toString(srcData, *(StringBuf*)destData);
    }

    /// enum to string ID
    static bool Conversion_Enum_StringID(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto srcEnum  = static_cast<const EnumType*>(srcType);
        ASSERT_EX(srcType->metaType() == MetaType::Enum, "Expected enum type");
        return srcEnum->toStringID(srcData, *(StringID*)destData);
    }

    /// enum to number
    template< typename DestType >
    static bool Conversion_Enum_Number(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto srcEnum  = static_cast<const EnumType*>(srcType);
        ASSERT_EX(srcType->metaType() == MetaType::Enum, "Expected enum type");

        int64_t number = 0;
        if (!srcEnum->toNumber(srcData, number))
            return false;

        *(DestType*)destData = range_cast<DestType>(number);
        return true;
    }

    /// enum from string
    static bool Conversion_StringBuf_Enum(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto destEnum  = static_cast<const EnumType*>(destType);
        ASSERT_EX(destEnum->metaType() == MetaType::Enum, "Expected enum type");

        return destEnum->fromString(*(const StringBuf*)srcData, destData);
    }

    /// enum from string id
    static bool Conversion_StringID_Enum(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto destEnum  = static_cast<const EnumType*>(destType);
        ASSERT_EX(destEnum->metaType() == MetaType::Enum, "Expected enum type");

        return destEnum->fromStringID(*(const StringID*)srcData, destData);
    }

    /// enum from number
    template< typename SrcType >
    static bool Conversion_Number_Enum(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto destEnum  = static_cast<const EnumType*>(destType);
        ASSERT_EX(destEnum->metaType() == MetaType::Enum, "Expected enum type");

        int64_t value = range_cast<int64_t>(*(const SrcType*)srcData);
        return destEnum->fromNumber(value, destData);
    }

    /// dynamic cast of strong handles
    static bool Conversion_DynamicHandleCast(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto srcHandleType  = static_cast<const IHandleType*>(srcType);
        auto destHandleType  = static_cast<const IHandleType*>(destType);
        return IHandleType::CastHandle(srcData, srcHandleType, destData, destHandleType);
    }

    /// class ref meta cast
    static bool Conversion_ClassRefCast(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        auto srcClassRefType  = static_cast<const ClassRefType*>(srcType);
        ASSERT_EX(srcClassRefType->metaType() == MetaType::ClassRef, "Expected class ref type");

        auto destClassRefType  = static_cast<const ClassRefType*>(destType);
        ASSERT_EX(destClassRefType->metaType() == MetaType::ClassRef, "Expected class ref type");

        return ClassRefType::CastClassRef(srcData, srcClassRefType, destData, destClassRefType);
    }

    /// sync ref to async ref
/*            static bool Conversion_SyncHandleToAsyncHandle(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        srcType->copy(destData, srcData);
        return true;
    }*/

    static bool Conversion_ResourceHandle(const void* srcData, const IType* srcType, void* destData, const IType* destType)
    {
        DEBUG_CHECK(srcType->metaType() == MetaType::ResourceRef || srcType->metaType() == MetaType::AsyncResourceRef);
        DEBUG_CHECK(destType->metaType() == MetaType::ResourceRef || destType->metaType() == MetaType::AsyncResourceRef);

        const auto* destRefType = static_cast<const IResourceReferenceType*>(destType);
        const auto* srcRefType = static_cast<const IResourceReferenceType*>(srcType);

        const auto destResourceClass = destRefType->referenceResourceClass();
        const auto srcResourceClass = destRefType->referenceResourceClass();

        if (destResourceClass.is(srcResourceClass))
        {
            srcType->copy(destData, srcData);
            return true;
        }

        return false;
    }

    #define SET_CAST(_srcType, _destType)  m_functions[(uint8_t)TypeConversionClass::Type##_srcType][(uint8_t)TypeConversionClass::Type##_destType] = &Conversion_##_srcType##_##_destType;
    #define SET_CAST_RAW(_srcType, _destType, _func)  m_functions[(uint8_t)TypeConversionClass::Type##_srcType][(uint8_t)TypeConversionClass::Type##_destType] = &_func;

    #define SET_RANGE_CAST(_srcType, _destType)  m_functions[(uint8_t)TypeConversionClass::Type##_srcType][(uint8_t)TypeConversionClass::Type##_destType] = &RangeConversion<_srcType, _destType>;
    #define SET_TO_BOOL_CAST(_srcType)  m_functions[(uint8_t)TypeConversionClass::Type##_srcType][(uint8_t)TypeConversionClass::Typebool] = &BoolConversion<_srcType>;

    #define SET_TO_STRING_CAST(_srcType)  m_functions[(uint8_t)TypeConversionClass::Type##_srcType][(uint8_t)TypeConversionClass::TypeStringBuf] = &ToStringConversion;
    #define SET_FROM_STRING_CAST(_destType)  m_functions[(uint8_t)TypeConversionClass::TypeStringBuf][(uint8_t)TypeConversionClass::Type##_destType] = &FromStringConversion;

    #define SET_NUMBER_TO_ENUM_CAST(_srcType)  m_functions[(uint8_t)TypeConversionClass::Type##_srcType][(uint8_t)TypeConversionClass::TypeEnum] = &Conversion_Number_Enum<_srcType>;
    #define SET_ENUM_TO_NUMBER_CAST(_destType)  m_functions[(uint8_t)TypeConversionClass::TypeEnum][(uint8_t)TypeConversionClass::Type##_destType] = &Conversion_Enum_Number<_destType>;

    /// aggregation of all conversion functions
    class ConversionMatrix
    {
    public:
        ConversionMatrix()
        {
            for (uint32_t i = 0; i < NUM_TYPES; ++i)
                for (uint32_t j = 0; j < NUM_TYPES; ++j)
                    m_functions[i][j] = &NoConversion;

            SET_RANGE_CAST(char, uint8_t);
            SET_RANGE_CAST(char, uint16_t);
            SET_RANGE_CAST(char, uint32_t);
            SET_RANGE_CAST(char, uint64_t);
            SET_RANGE_CAST(char, char);
            SET_RANGE_CAST(char, short);
            SET_RANGE_CAST(char, int);
            SET_RANGE_CAST(char, int64_t);
            SET_RANGE_CAST(char, float);
            SET_RANGE_CAST(char, double);

            SET_RANGE_CAST(short, uint8_t);
            SET_RANGE_CAST(short, uint16_t);
            SET_RANGE_CAST(short, uint32_t);
            SET_RANGE_CAST(short, uint64_t);
            SET_RANGE_CAST(short, char);
            SET_RANGE_CAST(short, short);
            SET_RANGE_CAST(short, int);
            SET_RANGE_CAST(short, int64_t);
            SET_RANGE_CAST(short, float);
            SET_RANGE_CAST(short, double);

            SET_RANGE_CAST(int, uint8_t);
            SET_RANGE_CAST(int, uint16_t);
            SET_RANGE_CAST(int, uint32_t);
            SET_RANGE_CAST(int, uint64_t);
            SET_RANGE_CAST(int, char);
            SET_RANGE_CAST(int, short);
            SET_RANGE_CAST(int, int);
            SET_RANGE_CAST(int, int64_t);
            SET_RANGE_CAST(int, float);
            SET_RANGE_CAST(int, double);

            SET_RANGE_CAST(int64_t, uint8_t);
            SET_RANGE_CAST(int64_t, uint16_t);
            SET_RANGE_CAST(int64_t, uint32_t);
            SET_RANGE_CAST(int64_t, uint64_t);
            SET_RANGE_CAST(int64_t, char);
            SET_RANGE_CAST(int64_t, short);
            SET_RANGE_CAST(int64_t, int);
            SET_RANGE_CAST(int64_t, int64_t);
            SET_RANGE_CAST(int64_t, float);
            SET_RANGE_CAST(int64_t, double);

            SET_RANGE_CAST(uint8_t, uint8_t);
            SET_RANGE_CAST(uint8_t, uint16_t);
            SET_RANGE_CAST(uint8_t, uint32_t);
            SET_RANGE_CAST(uint8_t, uint64_t);
            SET_RANGE_CAST(uint8_t, char);
            SET_RANGE_CAST(uint8_t, short);
            SET_RANGE_CAST(uint8_t, int);
            SET_RANGE_CAST(uint8_t, int64_t);
            SET_RANGE_CAST(uint8_t, float);
            SET_RANGE_CAST(uint8_t, double);
                                   
            SET_RANGE_CAST(uint16_t, uint8_t);
            SET_RANGE_CAST(uint16_t, uint16_t);
            SET_RANGE_CAST(uint16_t, uint32_t);
            SET_RANGE_CAST(uint16_t, uint64_t);
            SET_RANGE_CAST(uint16_t, char);
            SET_RANGE_CAST(uint16_t, short);
            SET_RANGE_CAST(uint16_t, int);
            SET_RANGE_CAST(uint16_t, int64_t);
            SET_RANGE_CAST(uint16_t, float);
            SET_RANGE_CAST(uint16_t, double);
                                   
            SET_RANGE_CAST(uint32_t, uint8_t);
            SET_RANGE_CAST(uint32_t, uint16_t);
            SET_RANGE_CAST(uint32_t, uint32_t);
            SET_RANGE_CAST(uint32_t, uint64_t);
            SET_RANGE_CAST(uint32_t, char);
            SET_RANGE_CAST(uint32_t, short);
            SET_RANGE_CAST(uint32_t, int);
            SET_RANGE_CAST(uint32_t, int64_t);
            SET_RANGE_CAST(uint32_t, float);
            SET_RANGE_CAST(uint32_t, double);
                                   
            SET_RANGE_CAST(uint64_t, uint8_t);
            SET_RANGE_CAST(uint64_t, uint16_t);
            SET_RANGE_CAST(uint64_t, uint32_t);
            SET_RANGE_CAST(uint64_t, uint64_t);
            SET_RANGE_CAST(uint64_t, char);
            SET_RANGE_CAST(uint64_t, short);
            SET_RANGE_CAST(uint64_t, int);
            SET_RANGE_CAST(uint64_t, int64_t);
            SET_RANGE_CAST(uint64_t, float);
            SET_RANGE_CAST(uint64_t, double);

            SET_RANGE_CAST(float, uint8_t);
            SET_RANGE_CAST(float, uint16_t);
            SET_RANGE_CAST(float, uint32_t);
            SET_RANGE_CAST(float, uint64_t);
            SET_RANGE_CAST(float, char);
            SET_RANGE_CAST(float, short);
            SET_RANGE_CAST(float, int);
            SET_RANGE_CAST(float, int64_t);
            SET_RANGE_CAST(float, float);
            SET_RANGE_CAST(float, double);

            SET_RANGE_CAST(double, uint8_t);
            SET_RANGE_CAST(double, uint16_t);
            SET_RANGE_CAST(double, uint32_t);
            SET_RANGE_CAST(double, uint64_t);
            SET_RANGE_CAST(double, char);
            SET_RANGE_CAST(double, short);
            SET_RANGE_CAST(double, int);
            SET_RANGE_CAST(double, int64_t);
            SET_RANGE_CAST(double, float);
            SET_RANGE_CAST(double, double);

            SET_TO_BOOL_CAST(bool);
            SET_TO_BOOL_CAST(uint8_t);
            SET_TO_BOOL_CAST(uint16_t);
            SET_TO_BOOL_CAST(uint32_t);
            SET_TO_BOOL_CAST(uint64_t);
            SET_TO_BOOL_CAST(char);
            SET_TO_BOOL_CAST(short);
            SET_TO_BOOL_CAST(int);
            SET_TO_BOOL_CAST(int64_t);
            SET_TO_BOOL_CAST(float);
            SET_TO_BOOL_CAST(double);
            SET_TO_BOOL_CAST(StringBuf);
            SET_TO_BOOL_CAST(StringID);

            SET_CAST(StrongHandle, bool);
            SET_CAST(WeakHandle, bool);

            SET_TO_STRING_CAST(bool);
            SET_TO_STRING_CAST(uint8_t);
            SET_TO_STRING_CAST(uint8_t);
            SET_TO_STRING_CAST(uint16_t);
            SET_TO_STRING_CAST(uint32_t);
            SET_TO_STRING_CAST(uint64_t);
            SET_TO_STRING_CAST(char);
            SET_TO_STRING_CAST(short);
            SET_TO_STRING_CAST(int);
            SET_TO_STRING_CAST(int64_t);
            SET_TO_STRING_CAST(float);
            SET_TO_STRING_CAST(double);
            SET_TO_STRING_CAST(StringBuf);
            SET_TO_STRING_CAST(StringID);

            SET_FROM_STRING_CAST(bool);
            SET_FROM_STRING_CAST(uint8_t);
            SET_FROM_STRING_CAST(uint16_t);
            SET_FROM_STRING_CAST(uint32_t);
            SET_FROM_STRING_CAST(uint64_t);
            SET_FROM_STRING_CAST(char);
            SET_FROM_STRING_CAST(short);
            SET_FROM_STRING_CAST(int);
            SET_FROM_STRING_CAST(int64_t);
            SET_FROM_STRING_CAST(float);
            SET_FROM_STRING_CAST(double);
            SET_FROM_STRING_CAST(StringBuf);
            SET_FROM_STRING_CAST(StringID);

            SET_CAST(Enum, StringBuf);
            SET_CAST(Enum, StringID);
            SET_CAST(StringBuf, Enum);
            SET_CAST(StringID, Enum);

            SET_ENUM_TO_NUMBER_CAST(uint8_t);
            SET_ENUM_TO_NUMBER_CAST(uint16_t);
            SET_ENUM_TO_NUMBER_CAST(uint32_t);
            SET_ENUM_TO_NUMBER_CAST(uint64_t);
            SET_ENUM_TO_NUMBER_CAST(char);
            SET_ENUM_TO_NUMBER_CAST(short);
            SET_ENUM_TO_NUMBER_CAST(int);
            SET_ENUM_TO_NUMBER_CAST(int64_t);

            SET_NUMBER_TO_ENUM_CAST(uint8_t);
            SET_NUMBER_TO_ENUM_CAST(uint16_t);
            SET_NUMBER_TO_ENUM_CAST(uint32_t);
            SET_NUMBER_TO_ENUM_CAST(uint64_t);
            SET_NUMBER_TO_ENUM_CAST(char);
            SET_NUMBER_TO_ENUM_CAST(short);
            SET_NUMBER_TO_ENUM_CAST(int);
            SET_NUMBER_TO_ENUM_CAST(int64_t);

            SET_CAST_RAW(StrongHandle, StrongHandle, Conversion_DynamicHandleCast);
            SET_CAST_RAW(StrongHandle, WeakHandle, Conversion_DynamicHandleCast);
            SET_CAST_RAW(WeakHandle, StrongHandle, Conversion_DynamicHandleCast);

            SET_CAST_RAW(SyncRef, SyncRef, Conversion_ResourceHandle);
            SET_CAST_RAW(AsyncRef, AsyncRef, Conversion_ResourceHandle);

            SET_CAST_RAW(ClassRef, ClassRef, Conversion_ClassRefCast);

            m_functions[(int)TypeConversionClass::TypeUnknown][(int)TypeConversionClass::TypeStringBuf] = &ToStringConversion;
            m_functions[(int)TypeConversionClass::TypeStringBuf][(int)TypeConversionClass::TypeUnknown] = &FromStringConversion;
        }

        INLINE bool convertData(const void* srcData, const IType* srcType, void* destData, const IType* destType)
        {
            if (!srcType || !destType)
                return false;

            auto srcTypeIndex = (uint8_t)srcType->traits().convClass;
            auto destTypeIndex = (uint8_t)destType->traits().convClass;

            auto convFunc = m_functions[srcTypeIndex][destTypeIndex];
            return convFunc(srcData, srcType, destData, destType);
        }

    private:
        static const uint32_t NUM_TYPES = (uint32_t)TypeConversionClass::MAX;

        typedef bool (*TCastFunction)(const void* srcData, const IType* srcType, void* destData, const IType* destType);
        TCastFunction m_functions[NUM_TYPES][NUM_TYPES];
    };

} // prv

bool ConvertData(const void* srcData, Type srcType, void* destData, Type destType)
{
    // invalid types
    if (!srcType || !destType)
        return false;

    // same type is always valid
    if (destType == srcType)
    {
        srcType->copy(destData, srcData);
        return true;
    }

    // use the conversion matrix
    static prv::ConversionMatrix convMatrix;
    return convMatrix.convertData(srcData, srcType.ptr(), destData, destType.ptr());
}

END_BOOMER_NAMESPACE_EX(rtti)
