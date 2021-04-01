/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector4 #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

RTTI_BEGIN_TYPE_STRUCT(Vector4);
    RTTI_BIND_NATIVE_COMPARE(Vector4);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();

    RTTI_PROPERTY(x).editable();
    RTTI_PROPERTY(y).editable();
    RTTI_PROPERTY(z).editable();
    RTTI_PROPERTY(w).editable();

    RTTI_NATIVE_CLASS_FUNCTION_EX("MinValue", minValue);
    RTTI_NATIVE_CLASS_FUNCTION_EX("MaxValue", maxValue);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Abs",abs);
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
    RTTI_NATIVE_CLASS_FUNCTION_EX("Project", project);
    RTTI_NATIVE_CLASS_FUNCTION_EX("Projected", projected);
RTTI_END_TYPE();

//--

static Vector4 ZERO_V4(0,0,0,0);
static Vector4 ZEROH_V4(0,0,0,1);
static Vector4 ONE_V4(1,1,1,1);
static Vector4 EX_V4(1,0,0,0);
static Vector4 EY_V4(0,1,0,0);
static Vector4 EZ_V4(0,0,1,0);
static Vector4 EW_V4(0,0,0,1);
static Vector4 INF_V4(VERY_LARGE_FLOAT, VERY_LARGE_FLOAT, VERY_LARGE_FLOAT, VERY_LARGE_FLOAT);

const Vector4& Vector4::ZERO() { return ZERO_V4; }
const Vector4& Vector4::ZEROH() { return ZEROH_V4; }
const Vector4& Vector4::ONE() { return ONE_V4; }
const Vector4& Vector4::EX() { return EX_V4; }
const Vector4& Vector4::EY() { return EY_V4; }
const Vector4& Vector4::EZ() { return EZ_V4; }
const Vector4& Vector4::EW() { return EW_V4; }
const Vector4& Vector4::INF() { return INF_V4; }

//--

void Vector4::print(IFormatStream& f) const
{
    f.appendf("[{},{},{},{}]", x, y, z, w);
}

//--

/*static Vector4 opNegV4(const Vector4& a) { return -a; }
static Vector4 opAddV4(const Vector4& a, const Vector4& b) { return a + b; }
static Vector4 opSubV4(const Vector4& a, const Vector4& b) { return a - b; }
static Vector4 opMulV4(const Vector4& a, const Vector4& b) { return a * b; }
static Vector4 opDivV4(const Vector4& a, const Vector4& b) { return a / b; }
static float opDotV4(const Vector4& a, const Vector4& b) { return a | b; }

static Vector4 opAsssignAddV4(Vector4& a, const Vector4& b) { return a += b; }
static Vector4 opAsssignSubV4(Vector4& a, const Vector4& b) { return a -= b; }
static Vector4 opAsssignMulV4(Vector4& a, const Vector4& b) { return a *= b; }
static Vector4 opAsssignDivV4(Vector4& a, const Vector4& b) { return a /= b; }

static Vector4 opAsssignAddV4F( Vector4& a, float b) { return a += b; }
static Vector4 opAsssignSubV4F(Vector4& a, float b) { return a -= b; }
static Vector4 opAsssignMulV4F(Vector4& a, float b) { return a *= b; }
static Vector4 opAsssignDivV4F(Vector4& a, float b) { return a /= b; }

static Vector4 opAddV4F(const Vector4& a, float b) { return a + b; }
static Vector4 opSubV4F(const Vector4& a, float b) { return a - b; }
static Vector4 opMulV4F(const Vector4& a, float b) { return a * b; }
static Vector4 opDivV4F(const Vector4& a, float b) { return a / b; }
static Vector4 opMulFV4(float a, const Vector4& b) { return b * a; }

static Vector4 AbsV4(const Vector4& a) { return a.abs(); }
static Vector4 MinV4(const Vector4& a, const Vector4& b) { return Min(a,b); }
static Vector4 MaxV4(const Vector4& a, const Vector4& b) { return Max(a,b); }
static float DotV4(const Vector4& a, const Vector4& b) { return Dot(a,b); }
static Vector4 SnapV4(const Vector4& a, float b) { return Snap(a,b); }
static Vector4 LerpV4(const Vector4& a, const Vector4& b, float f) { return Lerp(a,b,f); }
static Vector4 ClampV4(const Vector4& a, const Vector4& minV, const Vector4& maxV) { return Clamp(a, minV, maxV); }
static Vector4 ClampV4F(const Vector4& a, float minF, float maxF) { return Clamp(a, minF, maxF); }
static Vector4 NormalV4(const Vector4 &a, const Vector4 &normal) { return NormalPart(a, normal); }
static Vector4 TangentV4(const Vector4 &a, const Vector4 &normal) { return TangentPart(a, normal); }
static Vector4 ClampLengthV4(const Vector4& a, float maxLength) { return ClampLength(a, maxLength); }
static Vector4 SetLengthV4(const Vector4& a, float maxLength) { return SetLength(a, maxLength); }

RTTI_GLOBAL_FUNCTION(AbsV4, "Core.AbsV4");
RTTI_GLOBAL_FUNCTION(DotV4, "Core.DotV4");
RTTI_GLOBAL_FUNCTION(SnapV4, "Core.SnapV4");
RTTI_GLOBAL_FUNCTION(MinV4, "Core.MinV4");
RTTI_GLOBAL_FUNCTION(MaxV4, "Core.MaxV4");
RTTI_GLOBAL_FUNCTION(LerpV4, "Core.LerpV4");
RTTI_GLOBAL_FUNCTION(ClampV4, "Core.ClampV4");
RTTI_GLOBAL_FUNCTION(ClampV4F, "Core.ClampV4F");
RTTI_GLOBAL_FUNCTION(NormalV4, "Core.NormalV4");
RTTI_GLOBAL_FUNCTION(TangentV4, "Core.TangentV4");
RTTI_GLOBAL_FUNCTION(ClampLengthV4, "Core.ClampLengthV4");
RTTI_GLOBAL_FUNCTION(SetLengthV4, "Core.SetLengthV4");
RTTI_GLOBAL_FUNCTION(opNegV4, "Core.opNegate_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opAddV4, "Core.opAdd_ref_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opSubV4, "Core.opSubtract_ref_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opMulV4, "Core.opMultiply_ref_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opDivV4, "Core.opDivide_ref_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opDotV4, "Core.opBinaryOr_ref_Vector4_ref_Vector4_float");
RTTI_GLOBAL_FUNCTION(opAsssignAddV4, "Core.opAddAssign_out_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignSubV4, "Core.opSubAssign_out_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignMulV4, "Core.opMulAssign_out_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignDivV4, "Core.opDivAssign_out_Vector4_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opAddV4F, "Core.opAdd_ref_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opSubV4F, "Core.opSubtract_ref_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opMulV4F, "Core.opMultiply_ref_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opDivV4F, "Core.opDivide_ref_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opMulFV4, "Core.opMultiply_float_ref_Vector4_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignAddV4F, "Core.opAddAssign_out_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignSubV4F, "Core.opSubAssign_out_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignMulV4F, "Core.opMulAssign_out_Vector4_float_Vector4");
RTTI_GLOBAL_FUNCTION(opAsssignDivV4F, "Core.opDivAssign_out_Vector4_float_Vector4");
*/

//--

END_BOOMER_NAMESPACE()
