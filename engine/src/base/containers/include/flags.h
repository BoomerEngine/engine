/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
*
***/

#pragma once

namespace base
{
    /// FlagsBase backing data type
    template< uint32_t EnumTypeSize >
    struct FlagsBaseDataType
    {};

    template<>
    struct FlagsBaseDataType<1>
    {
        typedef uint8_t DataType;
    };

    template<>
    struct FlagsBaseDataType<2>
    {
        typedef uint16_t DataType;
    };

    template<>
    struct FlagsBaseDataType<4>
    {
        typedef uint32_t DataType;
    };

    template<>
    struct FlagsBaseDataType<8>
    {
        typedef uint64_t DataType;
    };

    /// flag container, allows for type-safe (and warning safe) operations on the "enum class" based FlagsBase
    template< typename EnumType, typename DataType, typename FlagCalculator>
    class FlagsBase
    {
    public:
        typedef EnumType ENUM_TYPE;
        static const uint32_t FLAGS_TYPE = FlagCalculator::FLAGS_TYPE;

        INLINE FlagsBase(); // zero initialized
        INLINE FlagsBase(EnumType singleFlag);
        explicit INLINE FlagsBase(DataType initialFlags);
        INLINE FlagsBase(std::initializer_list<EnumType> list);

        INLINE FlagsBase(const FlagsBase<EnumType, DataType, FlagCalculator>& other) = default;
        INLINE FlagsBase(FlagsBase<EnumType, DataType, FlagCalculator>&& other) = default;

        INLINE FlagsBase& operator=(const FlagsBase<EnumType, DataType, FlagCalculator>& other) = default;
        INLINE FlagsBase& operator=(FlagsBase<EnumType, DataType, FlagCalculator>&& other) = default;

        static INLINE FlagsBase& EMPTY();

        INLINE FlagsBase& clearAll();
        INLINE FlagsBase& enableAll();
        INLINE FlagsBase& toggleAll();

        INLINE FlagsBase& operator|=(EnumType singleFlag);
        INLINE FlagsBase& operator-=(EnumType singleFlag);
        INLINE FlagsBase& operator&=(EnumType singleFlag);
        INLINE FlagsBase& operator^=(EnumType singleFlag);

        INLINE FlagsBase operator|(EnumType singleFlag) const;
        INLINE FlagsBase operator-(EnumType singleFlag) const;
        INLINE FlagsBase operator&(EnumType singleFlag) const;
        INLINE FlagsBase operator^(EnumType singleFlag) const;

        INLINE bool operator==(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE bool operator!=(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;

        INLINE FlagsBase& operator|=(FlagsBase<EnumType, DataType, FlagCalculator> flags);
        INLINE FlagsBase& operator-=(FlagsBase<EnumType, DataType, FlagCalculator> flags);
        INLINE FlagsBase& operator&=(FlagsBase<EnumType, DataType, FlagCalculator> flags);
        INLINE FlagsBase& operator^=(FlagsBase<EnumType, DataType, FlagCalculator> flags);

        INLINE FlagsBase operator|(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE FlagsBase operator-(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE FlagsBase operator&(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE FlagsBase operator^(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;

        INLINE bool empty() const;

        INLINE bool test(EnumType flag) const;

        INLINE bool anySet(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE bool allSet(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE bool noneSet(FlagsBase<EnumType, DataType, FlagCalculator> flags) const;
        INLINE bool exactlyOneSet(EnumType flag) const;

        INLINE FlagsBase& set(EnumType singleFlag);
        INLINE FlagsBase& clear(EnumType singleFlag);
        INLINE FlagsBase& toggle(EnumType singleFlag);
        INLINE FlagsBase& mask(EnumType singleFlag);
		INLINE FlagsBase& configure(EnumType singleFlag, bool value);

        INLINE FlagsBase& set(FlagsBase<EnumType, DataType, FlagCalculator> flags);
        INLINE FlagsBase& clear(FlagsBase<EnumType, DataType, FlagCalculator> flags);
        INLINE FlagsBase& toggle(FlagsBase<EnumType, DataType, FlagCalculator> flags);
        INLINE FlagsBase& mask(FlagsBase<EnumType, DataType, FlagCalculator> flags);

        INLINE FlagsBase& invert();
        INLINE FlagsBase inverted();

        INLINE DataType rawValue() const;

    private:
        DataType m_data;
    };

    // direct bit flag calculator
    // used when the enum values are a bit masks themselves
    template< typename EnumType, typename DataType>
    struct DirectFlagCalculator
    {
        static const uint32_t FLAGS_TYPE = 1;

        INLINE static DataType Zero()
        {
            return (DataType)0;
        }

        INLINE static DataType GetFlagValue(EnumType val)
        {
            return (DataType) val;
        }
    };

    // shifted bit flag calculator
    // used when the enum values are numbers (0 to something)
    template< typename EnumType, typename DataType>
    struct BitShiftFlagCalculator
    {
        static const uint32_t FLAGS_TYPE = 2;

        INLINE static DataType Zero()
        {
            return (DataType)0;
        }

        INLINE static DataType GetFlagValue(EnumType val)
        {
            DEBUG_CHECK_SLOW_EX((int64_t)val >= 0 && ((int64_t)val < (int64_t)(sizeof(DataType)*8)), "Enum value cannot be represented as flag");
            return ((DataType)1) << (uint8_t)val;
        }
    };

    /// direct flags in which the enum value already contains the bits
    template< typename EnumType, typename DataType>
    using DirectFlagsBase = FlagsBase<EnumType, DataType, DirectFlagCalculator<EnumType, DataType> >;

    template< typename EnumType>
    using DirectFlags = DirectFlagsBase<EnumType, typename FlagsBaseDataType<sizeof(EnumType)>::DataType >;

    /// bit flags, when the enum value contains numbers, 0,1,2 etc
    template< typename EnumType, typename DataType>
    using BitFlagsBase = FlagsBase<EnumType, DataType, BitShiftFlagCalculator<EnumType, DataType> >;

    template< typename EnumType>
    using BitFlags = BitFlagsBase<EnumType, typename FlagsBaseDataType<sizeof(EnumType)>::DataType >;

} // base

#include "flags.inl"