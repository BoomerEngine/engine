/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector2 #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//-----

RTTI_BEGIN_TYPE_STRUCT(Vector2);
    RTTI_BIND_NATIVE_COMPARE(Vector2);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();

    RTTI_PROPERTY(x).editable();
    RTTI_PROPERTY(y).editable();

    RTTI_NATIVE_CLASS_FUNCTION_EX("MinValue", minValue);
    RTTI_NATIVE_CLASS_FUNCTION_EX("MaxValue", maxValue);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Abs", abs);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Trunc", trunc);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Frac", frac);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Round", round);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Ceil", ceil);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Floor", floor);
    RTTI_NATIVE_CLASS_FUNCTION_EX("SquareLength", squareLength);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Length", length);
    RTTI_NATIVE_CLASS_FUNCTION_EX("InvLength", invLength);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Sum", sum);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Trace", trace);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Normalize", normalize);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Normalized", normalized);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Distance", distance);
    RTTI_NATIVE_CLASS_FUNCTION_EX("SquareDistance", squareDistance);
    RTTI_NATIVE_CLASS_FUNCTION_EX("LargestAxis", largestAxis);
    RTTI_NATIVE_CLASS_FUNCTION_EX("SmallestAxis", smallestAxis);

RTTI_END_TYPE();

//-----

static Vector2 Vector2_ZERO(0,0);
static Vector2 Vector2_ONE(1,1);
static Vector2 Vector2_EX(1,0);
static Vector2 Vector2_EY(0,1);
static Vector2 Vector2_INF(VERY_LARGE_FLOAT, VERY_LARGE_FLOAT);

const Vector2& Vector2::ZERO()
{
    return Vector2_ZERO;
}

const Vector2& Vector2::ONE()
{
    return Vector2_ONE;
}

const Vector2& Vector2::EX()
{
    return Vector2_EX;
}

const Vector2& Vector2::EY()
{
    return Vector2_EY;
}

const Vector2& Vector2::INF()
{
    return Vector2_INF;
}

//--

void Vector2::print(IFormatStream& f) const
{
    f.appendf("[{},{}]", x, y);
}

//--

/*static Vector2 opNegV2(const Vector2& a) { return -a; }
static Vector2 opPrepV2(const Vector2& a) { return ~a; }
static Vector2 opAddV2(const Vector2& a, const Vector2& b) { return a + b; }
static Vector2 opSubV2(const Vector2& a, const Vector2& b) { return a - b; }
static Vector2 opMulV2(const Vector2& a, const Vector2& b) { return a * b; }
static Vector2 opDivV2(const Vector2& a, const Vector2& b) { return a / b; }
static float opDotV2(const Vector2& a, const Vector2& b) { return a | b; }

static Vector2 opAsssignAddV2(Vector2& a, const Vector2& b) { return a += b; }
static Vector2 opAsssignSubV2(Vector2& a, const Vector2& b) { return a -= b; }
static Vector2 opAsssignMulV2(Vector2& a, const Vector2& b) { return a *= b; }
static Vector2 opAsssignDivV2(Vector2& a, const Vector2& b) { return a /= b; }

static Vector2 opAsssignAddV2F( Vector2& a, float b) { return a += b; }
static Vector2 opAsssignSubV2F(Vector2& a, float b) { return a -= b; }
static Vector2 opAsssignMulV2F(Vector2& a, float b) { return a *= b; }
static Vector2 opAsssignDivV2F(Vector2& a, float b) { return a /= b; }

static Vector2 opAddV2F(const Vector2& a, float b) { return a + b; }
static Vector2 opSubV2F(const Vector2& a, float b) { return a - b; }
static Vector2 opMulV2F(const Vector2& a, float b) { return a * b; }
static Vector2 opDivV2F(const Vector2& a, float b) { return a / b; }
static Vector2 opMulFV2(float a, const Vector2& b) { return b * a; }

static Vector2 AbsV2(const Vector2& a) { return a.abs(); }
static Vector2 MinV2(const Vector2& a, const Vector2& b) { return Min(a,b); }
static Vector2 MaxV2(const Vector2& a, const Vector2& b) { return Max(a,b); }
static float DotV2(const Vector2& a, const Vector2& b) { return Dot(a,b); }
static Vector2 SnapV2(const Vector2& a, float b) { return Snap(a,b); }
static Vector2 LerpV2(const Vector2& a, const Vector2& b, float f) { return Lerp(a,b, f); }
static Vector2 ClampV2(const Vector2& a, const Vector2& minV, const Vector2& maxV) { return Clamp(a,minV,maxV); }
static Vector2 ClampV2F(const Vector2& a, float minV, float maxV) { return Clamp(a,minV,maxV); }
static Vector2 NormalV2(const Vector2 &a, const Vector2 &normal) { return NormalPart(a, normal); }
static Vector2 TangentV2(const Vector2 &a, const Vector2 &normal) { return TangentPart(a, normal); }
static Vector2 ClampLengthV2(const Vector2& a, float maxLength) { return ClampLength(a, maxLength); }
static Vector2 SetLengthV2(const Vector2& a, float maxLength) { return SetLength(a, maxLength); }

RTTI_GLOBAL_FUNCTION(AbsV2, "Core.AbsV2");
RTTI_GLOBAL_FUNCTION(MinV2, "Core.MinV2");
RTTI_GLOBAL_FUNCTION(MaxV2, "Core.MaxV2");
RTTI_GLOBAL_FUNCTION(DotV2, "Core.DotV2");
RTTI_GLOBAL_FUNCTION(SnapV2, "Core.SnapV2");
RTTI_GLOBAL_FUNCTION(ClampV2, "Core.ClampV2");
RTTI_GLOBAL_FUNCTION(ClampV2F, "Core.ClampV2F");
RTTI_GLOBAL_FUNCTION(LerpV2, "Core.LerpV2");
RTTI_GLOBAL_FUNCTION(NormalV2, "Core.NormalV2");
RTTI_GLOBAL_FUNCTION(TangentV2, "Core.TangentV2");
RTTI_GLOBAL_FUNCTION(ClampLengthV2, "Core.ClampLengthV2");
RTTI_GLOBAL_FUNCTION(SetLengthV2, "Core.SetLengthV2");
RTTI_GLOBAL_FUNCTION(opNegV2, "Core.opNegate_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opPrepV2, "Core.opBinaryNot_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opAddV2, "Core.opAdd_ref_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opSubV2, "Core.opSubtract_ref_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opMulV2, "Core.opMultiply_ref_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opDivV2, "Core.opDivide_ref_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opDotV2, "Core.opBinaryOr_ref_Vector2_ref_Vector2_float");
RTTI_GLOBAL_FUNCTION(opAsssignAddV2, "Core.opAddAssign_out_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignSubV2, "Core.opSubAssign_out_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignMulV2, "Core.opMulAssign_out_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignDivV2, "Core.opDivAssign_out_Vector2_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opAddV2F, "Core.opAdd_ref_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opSubV2F, "Core.opSubtract_ref_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opMulV2F, "Core.opMultiply_ref_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opDivV2F, "Core.opDivide_ref_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opMulFV2, "Core.opMultiply_float_ref_Vector2_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignAddV2F, "Core.opAddAssign_out_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignSubV2F, "Core.opSubAssign_out_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignMulV2F, "Core.opMulAssign_out_Vector2_float_Vector2");
RTTI_GLOBAL_FUNCTION(opAsssignDivV2F, "Core.opDivAssign_out_Vector2_float_Vector2");
*/

//--

END_BOOMER_NAMESPACE()
