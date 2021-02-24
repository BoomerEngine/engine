/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
*
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>::FlagsBase()
{
    m_data = FlagCalculator::Zero();
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>::FlagsBase(EnumType singleFlag)
{
    m_data = FlagCalculator::GetFlagValue(singleFlag);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>::FlagsBase(std::initializer_list<EnumType> list)
{
    m_data = FlagCalculator::Zero();
    for (auto singleFlag : list)
        m_data |= FlagCalculator::GetFlagValue(singleFlag);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>::FlagsBase(DataType initialFlags)
{
    m_data = initialFlags;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator|=(const EnumType singleFlag)
{
    return set(singleFlag);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator-=(const EnumType singleFlag)
{
    return clear(singleFlag);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator&=(const EnumType singleFlag)
{
    return mask(singleFlag);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator^=(const EnumType singleFlag)
{
    return toggle(singleFlag);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator|(const EnumType singleFlag) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) |= singleFlag;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator-(const EnumType singleFlag) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) -= singleFlag;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator&(const EnumType singleFlag) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) &= singleFlag;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator^(const EnumType singleFlag) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) ^= singleFlag;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::operator==(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return m_data == flags.m_data;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::operator!=(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return m_data != flags.m_data;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator|=(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    return set(flags);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator-=(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    return clear(flags);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator&=(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    return mask(flags);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::operator^=(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    return toggle(flags);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator|(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) |= flags;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator-(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) -= flags;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator&(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) &= flags;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::operator^(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this) ^= flags;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::empty() const
{
    return m_data == 0;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::test(EnumType flag) const
{
    return 0 != (m_data & FlagCalculator::GetFlagValue(flag));
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::anySet(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return 0 != (m_data & flags.m_data);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::allSet(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return flags.m_data == (m_data & flags.m_data);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::noneSet(FlagsBase<EnumType, DataType, FlagCalculator> flags) const
{
    return 0 == (m_data & flags.m_data);
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE bool FlagsBase<EnumType, DataType, FlagCalculator>::exactlyOneSet(EnumType flag) const
{
    return FlagCalculator::GetFlagValue(flag) == (m_data & FlagCalculator::GetFlagValue(flag));
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::set(EnumType singleFlag)
{
    m_data |= FlagCalculator::GetFlagValue(singleFlag);
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::clear(EnumType singleFlag)
{
    m_data &= ~FlagCalculator::GetFlagValue(singleFlag);
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::mask(EnumType singleFlag)
{
    m_data &= FlagCalculator::GetFlagValue(singleFlag);
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::toggle(EnumType singleFlag)
{
    m_data ^= FlagCalculator::GetFlagValue(singleFlag);
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::configure(EnumType singleFlag, bool value)
{
	if (value)
		m_data |= FlagCalculator::GetFlagValue(singleFlag);
	else
		m_data &= ~FlagCalculator::GetFlagValue(singleFlag);
	return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::set(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    m_data |= flags.m_data;
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::clear(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    m_data &= ~flags.m_data;
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::mask(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    m_data &= flags.m_data;
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::toggle(FlagsBase<EnumType, DataType, FlagCalculator> flags)
{
    m_data ^= flags.m_data;
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::invert()
{
    m_data = ~m_data;
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator> FlagsBase<EnumType, DataType, FlagCalculator>::inverted()
{
    return FlagsBase<EnumType, DataType, FlagCalculator>(*this).invert();
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::EMPTY()
{
    static FlagsBase<EnumType, DataType, FlagCalculator> theFlagsBase(0);
    return theFlagsBase;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::clearAll()
{
    m_data = 0;
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::enableAll()
{
    m_data = ~FlagCalculator::Zero();
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE FlagsBase<EnumType, DataType, FlagCalculator>& FlagsBase<EnumType, DataType, FlagCalculator>::toggleAll()
{
    m_data ^= ~FlagCalculator::Zero();
    return *this;
}

template< typename EnumType, typename DataType, typename FlagCalculator >
INLINE DataType FlagsBase<EnumType, DataType, FlagCalculator>::rawValue() const
{
    return m_data;
}

END_BOOMER_NAMESPACE(base)
