/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector3 #]
***/

#include "build.h"

namespace base
{

    //--

    RTTI_BEGIN_TYPE_STRUCT(Vector3);
        RTTI_BIND_NATIVE_COMPARE(Vector3);
        RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();

        RTTI_PROPERTY(x).editable();
        RTTI_PROPERTY(y).editable();
        RTTI_PROPERTY(z).editable();

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
        RTTI_FUNCTION("LargestAxis", largestAxis);
        RTTI_FUNCTION("SmallestAxis", smallestAxis);
        RTTI_FUNCTION("ToAngles", toRotator);

        RTTI_FUNCTION("xx", xx);
        RTTI_FUNCTION("xy", _xy);
        RTTI_FUNCTION("xz", xz);
        RTTI_FUNCTION("yx", yx);
        RTTI_FUNCTION("yy", yy);
        RTTI_FUNCTION("yz", _yz);
        RTTI_FUNCTION("zx", zx);
        RTTI_FUNCTION("zy", zy);
        RTTI_FUNCTION("zz", zz);

        RTTI_FUNCTION("xxx", xxx);
        RTTI_FUNCTION("yyy", yyy);
        RTTI_FUNCTION("zzz", zzz);
        RTTI_FUNCTION("xyz", _xyz);
        RTTI_FUNCTION("zyx", zyx);

        RTTI_FUNCTION("xyzw", xyzw);

        //RTTI_STATIC_FUNCTION("Random", Rand);
    RTTI_END_TYPE();

    //-----------------------------------------------------------------------------

    Angles Vector3::toRotator() const
    {
        Angles ret;
        if (!x && !y)
        {
            ret.yaw = 0.0f;
            ret.pitch = (z > 0) ? -90.0f : 90.0f;
            ret.roll = 0.0f;
        }
        else
        {
            ret.yaw =  RAD2DEG * atan2(y, x);
            ret.pitch =  RAD2DEG * atan2(-z, sqrt(x*x + y*y));
            ret.roll = 0.0f;
        }
        return ret;
    }

    float Dot(const Vector3 &a, const Vector3 &b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    Vector3 Cross(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    Vector3 Snap(const Vector3 &a, float grid)
    {
        return Vector3(Snap(a.x, grid), Snap(a.y, grid), Snap(a.z, grid));
    }

    Vector3 Lerp(const Vector3 &a, const Vector3 &b, float frac)
    {
        return Vector3(Lerp(a.x, b.x, frac), Lerp(a.y, b.y, frac), Lerp(a.z, b.z, frac));
    }

    Vector3 Min(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }

    Vector3 Max(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }

    Vector3 Clamp(const Vector3 &a, const Vector3 &minV, const Vector3 &maxV)
    {
        return Vector3(std::clamp(a.x, minV.x, maxV.x), std::clamp(a.y, minV.y, maxV.y), std::clamp(a.z, minV.z, maxV.z));
    }

    Vector3 Clamp(const Vector3 &a, float minF, float maxF)
    {
        return Vector3(std::clamp(a.x, minF, maxF), std::clamp(a.y, minF, maxF), std::clamp(a.z, minF, maxF));
    }

    //--

    static Vector3 Vector3_ZERO(0,0,0);
    static Vector3 Vector3_ONE(1,1,1);
    static Vector3 Vector3_EX(1,0,0);
    static Vector3 Vector3_EY(0,1,0);
    static Vector3 Vector3_EZ(0,0,1);
    static Vector3 Vector3_INF(VERY_LARGE_FLOAT, VERY_LARGE_FLOAT, VERY_LARGE_FLOAT);

    const Vector3& Vector3::ZERO() { return Vector3_ZERO; }
    const Vector3& Vector3::ONE() { return Vector3_ONE; }
    const Vector3& Vector3::EX() { return Vector3_EX; }
    const Vector3& Vector3::EY() { return Vector3_EY; }
    const Vector3& Vector3::EZ() { return Vector3_EZ; }
    const Vector3& Vector3::INF() { return Vector3_INF; }

    //--

    static Vector3 opNegV3(const Vector3& a) { return -a; }
    static Vector3 opAddV3(const Vector3& a, const Vector3& b) { return a + b; }
    static Vector3 opSubV3(const Vector3& a, const Vector3& b) { return a - b; }
    static Vector3 opMulV3(const Vector3& a, const Vector3& b) { return a * b; }
    static Vector3 opDivV3(const Vector3& a, const Vector3& b) { return a / b; }
    static float opDotV3(const Vector3& a, const Vector3& b) { return a | b; }
    static Vector3 opCrossV3(const Vector3& a, const Vector3& b) { return a ^ b; }

    static Vector3 opAsssignAddV3(Vector3& a, const Vector3& b) { return a += b; }
    static Vector3 opAsssignSubV3(Vector3& a, const Vector3& b) { return a -= b; }
    static Vector3 opAsssignMulV3(Vector3& a, const Vector3& b) { return a *= b; }
    static Vector3 opAsssignDivV3(Vector3& a, const Vector3& b) { return a /= b; }

    static Vector3 opAsssignAddV3F( Vector3& a, float b) { return a += b; }
    static Vector3 opAsssignSubV3F(Vector3& a, float b) { return a -= b; }
    static Vector3 opAsssignMulV3F(Vector3& a, float b) { return a *= b; }
    static Vector3 opAsssignDivV3F(Vector3& a, float b) { return a /= b; }

    static Vector3 opAddV3F(const Vector3& a, float b) { return a + b; }
    static Vector3 opSubV3F(const Vector3& a, float b) { return a - b; }
    static Vector3 opMulV3F(const Vector3& a, float b) { return a * b; }
    static Vector3 opDivV3F(const Vector3& a, float b) { return a / b; }
    static Vector3 opMulFV3(float a, const Vector3& b) { return b * a; }

    static Vector3 AbsV3(const Vector3& a) { return a.abs(); }
    static Vector3 MinV3(const Vector3& a, const Vector3& b) { return Min(a,b); }
    static Vector3 MaxV3(const Vector3& a, const Vector3& b) { return Max(a,b); }
    static float DotV3(const Vector3& a, const Vector3& b) { return Dot(a,b); }
    static Vector3 SnapV3(const Vector3& a, float b) { return Snap(a,b); }
    static Vector3 LerpV3(const Vector3& a, const Vector3& b, float f) { return Lerp(a,b, f); }
    static Vector3 ClampV3(const Vector3& a, const Vector3& minV, const Vector3& maxV) { return Clamp(a,minV,maxV); }
    static Vector3 ClampV3F(const Vector3& a, float minV, float maxV) { return Clamp(a,minV,maxV); }
    static Vector3 NormalV3(const Vector3 &a, const Vector3 &normal) { return NormalPart(a, normal); }
    static Vector3 TangentV3(const Vector3 &a, const Vector3 &normal) { return TangentPart(a, normal); }
    static Vector3 ClampLengthV3(const Vector3& a, float maxLength) { return ClampLength(a, maxLength); }
    static Vector3 SetLengthV3(const Vector3& a, float maxLength) { return SetLength(a, maxLength); }

    RTTI_GLOBAL_FUNCTION(AbsV3, "Core.AbsV3");
    RTTI_GLOBAL_FUNCTION(MinV3, "Core.MinV3");
    RTTI_GLOBAL_FUNCTION(MaxV3, "Core.MaxV3");
    RTTI_GLOBAL_FUNCTION(DotV3, "Core.DotV3");
    RTTI_GLOBAL_FUNCTION(SnapV3, "Core.SnapV3");
    RTTI_GLOBAL_FUNCTION(ClampV3, "Core.ClampV3");
    RTTI_GLOBAL_FUNCTION(ClampV3F, "Core.ClampV3F");
    RTTI_GLOBAL_FUNCTION(LerpV3, "Core.LerpV3");
    RTTI_GLOBAL_FUNCTION(NormalV3, "Core.NormalV3");
    RTTI_GLOBAL_FUNCTION(TangentV3, "Core.TangentV3");
    RTTI_GLOBAL_FUNCTION(ClampLengthV3, "Core.ClampLengthV3");
    RTTI_GLOBAL_FUNCTION(SetLengthV3, "Core.SetLengthV3");
    RTTI_GLOBAL_FUNCTION(opNegV3, "Core.opNegate_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAddV3, "Core.opAdd_ref_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opSubV3, "Core.opSubtract_ref_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opMulV3, "Core.opMultiply_ref_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opDivV3, "Core.opDivide_ref_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opDotV3, "Core.opBinaryOr_ref_Vector3_ref_Vector3_float");
    RTTI_GLOBAL_FUNCTION(opCrossV3, "Core.opHat_ref_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignAddV3, "Core.opAddAssign_out_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignSubV3, "Core.opSubAssign_out_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignMulV3, "Core.opMulAssign_out_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignDivV3, "Core.opDivAssign_out_Vector3_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAddV3F, "Core.opAdd_ref_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opSubV3F, "Core.opSubtract_ref_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opMulV3F, "Core.opMultiply_ref_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opDivV3F, "Core.opDivide_ref_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opMulFV3, "Core.opMultiply_float_ref_Vector3_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignAddV3F, "Core.opAddAssign_out_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignSubV3F, "Core.opSubAssign_out_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignMulV3F, "Core.opMulAssign_out_Vector3_float_Vector3");
    RTTI_GLOBAL_FUNCTION(opAsssignDivV3F, "Core.opDivAssign_out_Vector3_float_Vector3");

    RTTI_GLOBAL_FUNCTION(TriangleNormal, "Core.TriangleNormal");
    RTTI_GLOBAL_FUNCTION(SafeTriangleNormal, "Core.SafeTriangleNormal");

    //--

} // base
