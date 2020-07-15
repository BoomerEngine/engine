/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector4 #]
***/

#include "build.h"

namespace base
{
    //-----------------------------------------------------------------------------

    RTTI_BEGIN_TYPE_STRUCT(Vector4);
        RTTI_BIND_NATIVE_COMPARE(Vector4);
        RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();

        RTTI_PROPERTY(x).editable();
        RTTI_PROPERTY(y).editable();
        RTTI_PROPERTY(z).editable();
        RTTI_PROPERTY(w).editable();

        RTTI_FUNCTION("MinValue", minValue);
        RTTI_FUNCTION("MaxValue", maxValue);
        RTTI_FUNCTION("Abs",abs);
        RTTI_FUNCTION("Trunc", trunc);
        RTTI_FUNCTION("Frac", frac);
        RTTI_FUNCTION("Round", round);
        RTTI_FUNCTION("Ceil", ceil);
        RTTI_FUNCTION("Floor", floor);
        RTTI_FUNCTION("SquareLength", squareLength);
        RTTI_FUNCTION("Length", length);
        RTTI_FUNCTION("InvLength", invLength);
        RTTI_FUNCTION("Sum", sum);
        RTTI_FUNCTION("Trace", trace);
        RTTI_FUNCTION("Normalize", normalize);
        RTTI_FUNCTION("Normalized", normalized);
        RTTI_FUNCTION("Distance", distance);
        RTTI_FUNCTION("SquareDistance", squareDistance);
        RTTI_FUNCTION("Project", project);
        RTTI_FUNCTION("Projected", projected);

        RTTI_FUNCTION("xx", xx);
        RTTI_FUNCTION("xz", xz);
        RTTI_FUNCTION("xw", xw);
        RTTI_FUNCTION("yx", yx);
        RTTI_FUNCTION("yy", yy);
        RTTI_FUNCTION("yw", yw);
        RTTI_FUNCTION("zx", zx);
        RTTI_FUNCTION("zy", zy);
        RTTI_FUNCTION("zz", zz);
        RTTI_FUNCTION("wx", wx);
        RTTI_FUNCTION("wy", wy);
        RTTI_FUNCTION("wz", wz);
        RTTI_FUNCTION("ww", ww);
        RTTI_FUNCTION("xy", _xy);
        RTTI_FUNCTION("yz", _yz);
        RTTI_FUNCTION("zw", _zw);
        RTTI_FUNCTION("xxx", xxx);
        RTTI_FUNCTION("yyy", yyy);
        RTTI_FUNCTION("zzz", zzz);
        RTTI_FUNCTION("www", www);
        RTTI_FUNCTION("xyz", _xyz);
        RTTI_FUNCTION("zyx", zyx);
        RTTI_FUNCTION("xxxx", xxxx);
        RTTI_FUNCTION("yyyy", yyyy);
        RTTI_FUNCTION("zzzz", zzzz);
        RTTI_FUNCTION("wwww", wwww);
        RTTI_FUNCTION("xyzw", _xyzw);
        RTTI_FUNCTION("wzyx", wzyx);

        //RTTI_STATIC_FUNCTION("Random", Rand);
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

    /*Vector4 Vector4::Rand(float min, float max)
    {
        return Vector4(RandRange(min, max), RandRange(min, max), RandRange(min, max), RandRange(min, max));
    }*/

    //--

    float Dot(const Vector4 &a, const Vector4 &b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }

    Vector3 Cross(const Vector4 &a, const Vector4 &b)
    {
        return Cross((const Vector3&)a, (const Vector3&)b);
    }

    Vector4 Snap(const Vector4 &a, float grid)
    {
        return Vector4(Snap(a.x, grid), Snap(a.y, grid), Snap(a.z, grid), Snap(a.w, grid));
    }

    Vector4 Lerp(const Vector4 &a, const Vector4 &b, float frac)
    {
        return Vector4(Lerp(a.x, b.x, frac), Lerp(a.y, b.y, frac), Lerp(a.z, b.z, frac), Lerp(a.w, b.w, frac));
    }

    Vector4 Min(const Vector4 &a, const Vector4 &b)
    {
        return Vector4(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w));
    }

    Vector4 Max(const Vector4 &a, const Vector4 &b)
    {
        return Vector4(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w));
    }

    Vector4 Clamp(const Vector4 &a, const Vector4 &minV, const Vector4 &maxV)
    {
        return Vector4(std::clamp(a.x, minV.x, maxV.x), std::clamp(a.y, minV.y, maxV.y), std::clamp(a.z, minV.z, maxV.z), std::clamp(a.w, minV.w, maxV.w));
    }

    Vector4 Clamp(const Vector4 &a, float minF, float maxF)
    {
        return Vector4(std::clamp(a.x, minF, maxF), std::clamp(a.y, minF, maxF), std::clamp(a.z, minF, maxF), std::clamp(a.w, minF, maxF));
    }

    //--

    static Vector4 opNegV4(const Vector4& a) { return -a; }
    static Vector4 opAddV4(const Vector4& a, const Vector4& b) { return a + b; }
    static Vector4 opSubV4(const Vector4& a, const Vector4& b) { return a - b; }
    static Vector4 opMulV4(const Vector4& a, const Vector4& b) { return a * b; }
    static Vector4 opDivV4(const Vector4& a, const Vector4& b) { return a / b; }
    static float opDotV4(const Vector4& a, const Vector4& b) { return Dot(a,b); }

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

    //--

} // base