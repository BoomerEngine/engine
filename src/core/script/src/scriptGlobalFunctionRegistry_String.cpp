/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE_EX(script)

//---

static StringBuf ConcatString(const StringBuf& a, const StringBuf & b)
{
    StringBuilder ret;
    ret << a << b;
    return ret.toString();
}

static StringBuf AssignConcatString(StringBuf& a, const StringBuf& b)
{
    StringBuilder ret;
    ret << a << b;
    a = ret.toString();
    return a;
}

static StringBuf Int8ToString(char val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Int16ToString(short val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Int32ToString(int val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Int64ToString(int64_t val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Uint8ToString(uint8_t val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Uint16ToString(uint16_t val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Uint32ToString(uint32_t val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf Uint64ToString(uint64_t val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf FloatToString(float val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf DoubleToString(double val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf BoolToString(bool val)
{
    return StringBuf(TempString("{}", val));
}

static StringBuf NameToString(StringID val)
{
    return StringBuf(val.view());
}

static StringBuf LeftString(const StringBuf& str, int count)
{
	return str.leftPart((uint32_t)count);
}

static StringBuf RightString(const StringBuf& str, int count)
{
	return str.rightPart((uint32_t)count);
}

static StringBuf SubString(const StringBuf& str, int start, int count)
{
	return str.subString(start, (uint32_t)count);
}

static int StrLen(const StringBuf& str)
{
	return str.length();
}

RTTI_GLOBAL_FUNCTION(ConcatString, "Core.opAdd_ref_string_ref_string_string");
RTTI_GLOBAL_FUNCTION(AssignConcatString, "Core.opAddAssign_out_string_ref_string_string");
RTTI_GLOBAL_FUNCTION(Int8ToString, "Core.cast_int8_string");
RTTI_GLOBAL_FUNCTION(Int16ToString, "Core.cast_int16_string");
RTTI_GLOBAL_FUNCTION(Int32ToString, "Core.cast_int_string");
RTTI_GLOBAL_FUNCTION(Int64ToString, "Core.cast_int64_string");
RTTI_GLOBAL_FUNCTION(Uint8ToString, "Core.cast_uint8_string");
RTTI_GLOBAL_FUNCTION(Uint16ToString, "Core.cast_uint16_string");
RTTI_GLOBAL_FUNCTION(Uint32ToString, "Core.cast_uint_string");
RTTI_GLOBAL_FUNCTION(Uint64ToString, "Core.cast_uint64_string");
RTTI_GLOBAL_FUNCTION(FloatToString, "Core.cast_float_string");
RTTI_GLOBAL_FUNCTION(DoubleToString, "Core.cast_double_string");
RTTI_GLOBAL_FUNCTION(BoolToString, "Core.cast_bool_string");
RTTI_GLOBAL_FUNCTION(NameToString, "Core.cast_strid_string");
RTTI_GLOBAL_FUNCTION(LeftString, "Core.StrLeft");
RTTI_GLOBAL_FUNCTION(RightString, "Core.StrRight");
RTTI_GLOBAL_FUNCTION(SubString, "Core.StrSub");
RTTI_GLOBAL_FUNCTION(StrLen, "Core.StrLen");

//---

END_BOOMER_NAMESPACE_EX(script)
