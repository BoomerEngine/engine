/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\rotation #]
***/

#include "build.h"

namespace base
{
    //-----------------------------------------------------------------------------

    RTTI_BEGIN_TYPE_STRUCT(Angles);
        RTTI_BIND_NATIVE_COMPARE(Angles);
        RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();
        RTTI_PROPERTY(pitch).editable("Pitch around Y axis").metadata<PropertyNumberRangeMetadata>(-360.0f, 360.0f).metadata<PropertyHasDragMetadata>(true);
        RTTI_PROPERTY(yaw).editable("Yaw around Z axis").metadata<PropertyNumberRangeMetadata>(-360.0f, 360.0f).metadata<PropertyHasDragMetadata>(true);
        RTTI_PROPERTY(roll).editable("Roll around X axis").metadata<PropertyNumberRangeMetadata>(-360.0f, 360.0f).metadata<PropertyHasDragMetadata>(true);
        RTTI_FUNCTION("ForwardVector", forward);
        RTTI_FUNCTION("SideVector", side);
        RTTI_FUNCTION("UpVector", up);
        RTTI_FUNCTION("IsZero", isZero);
        RTTI_FUNCTION("IsNearZero", isNearZero);
        RTTI_FUNCTION("ToMatrix", toMatrix);
        RTTI_FUNCTION("ToMatrix33", toMatrix33);
        RTTI_FUNCTION("ToQuat", toQuat);
        RTTI_FUNCTION("ToAxes", angleVectors);
        RTTI_FUNCTION("Distance", distance);
        RTTI_FUNCTION("Normalize", normalize);
        RTTI_FUNCTION("Normalized", normalized);
//        RTTI_STATIC_FUNCTION("Random", Rand);
        RTTI_STATIC_FUNCTION("Approach", Approach);
    RTTI_END_TYPE();

    //-----------------------------------------------------------------------------

    void Angles::normalize()
    {
        pitch = AngleNormalize(pitch);
        yaw = AngleNormalize(yaw);
        roll = AngleNormalize(roll);
    }

    Angles Angles::normalized() const
    {
        return Angles(AngleNormalize(pitch), AngleNormalize(yaw), AngleNormalize(roll));
    }

    Angles Angles::distance(const Angles &other) const
    {
        float localPitch = AngleDistance(pitch, other.pitch);
        float localYaw = AngleDistance(yaw, other.yaw);
        float localRoll = AngleDistance(roll, other.roll);
        return Angles(localPitch, localYaw, localRoll);
    }

    Angles Angles::Approach(const Angles &a, const Angles &b, float move)
    {
        return Angles(AngleReach(a.pitch, b.pitch, move),
                AngleReach(a.yaw, b.yaw, move),
                AngleReach(a.roll, b.roll, move));
    }

    /*Angles Angles::Rand(float minAngle, float maxAngle, bool withRoll)
    {
        return Angles(RandRange(minAngle, maxAngle),
               RandRange(minAngle, maxAngle),
         withRoll ? RandRange(minAngle, maxAngle) : 0.0f);
    }*/

    void Angles::angleVectors(Vector3& forward, Vector3& right, Vector3& up) const
    {
        return someAngleVectors(&forward, &right, &up);
    }

    void Angles::someAngleVectors(Vector3 *forward, Vector3 *right, Vector3 *up) const
    {
        // Rotation order: X(Roll) Y(Pitch) Z(Yaw)
        // All rotations are CW
        // All equations derived using Mathematica

        float yawRad = DEG2RAD * yaw;
        float pitchRad = DEG2RAD * pitch;
        float rollRad = DEG2RAD * roll;

        float sinYaw = sin(yawRad);
        float cosYaw = cos(yawRad);
        float sinPitch = sin(pitchRad);
        float cosPitch = cos(pitchRad);
        float sinRoll = sin(rollRad);
        float cosRoll = cos(rollRad);

        if (forward)
            *forward = Vector3(cosPitch*cosYaw, cosPitch*sinYaw, -sinPitch);

        if (right)
            *right = Vector3(sinRoll*sinPitch*cosYaw - cosRoll*sinYaw, cosRoll*cosYaw + sinRoll*sinPitch*sinYaw, sinRoll*cosPitch);

        if (up)
            *up = Vector3(cosRoll*sinPitch*cosYaw + sinRoll*sinYaw, cosRoll*sinPitch*sinYaw - sinRoll*cosYaw, cosRoll*cosPitch);
    }


    Vector3 Angles::forward() const
    {
        float yawRad = DEG2RAD * yaw;
        float pitchRad = DEG2RAD * pitch;
        float rollRad = DEG2RAD * roll;

        float sinYaw = sin(yawRad);
        float cosYaw = cos(yawRad);
        float sinPitch = sin(pitchRad);
        float cosPitch = cos(pitchRad);

        return Vector3( cosPitch*cosYaw, cosPitch*sinYaw, -sinPitch );
    }

    Vector3 Angles::side() const
    {
        float yawRad = DEG2RAD * yaw;
        float pitchRad = DEG2RAD * pitch;
        float rollRad = DEG2RAD * roll;

        float sinYaw = sin(yawRad);
        float cosYaw = cos(yawRad);
        float sinPitch = sin(pitchRad);
        float cosPitch = cos(pitchRad);
        float sinRoll = sin(rollRad);
        float cosRoll = cos(rollRad);

        return Vector3(sinRoll*sinPitch*cosYaw - cosRoll*sinYaw, cosRoll*cosYaw + sinRoll*sinPitch*sinYaw, sinRoll*cosPitch);
    }

    Vector3 Angles::up() const
    {
        float yawRad = DEG2RAD * yaw;
        float pitchRad = DEG2RAD * pitch;
        float rollRad = DEG2RAD * roll;

        float sinYaw = sin(yawRad);
        float cosYaw = cos(yawRad);
        float sinPitch = sin(pitchRad);
        float cosPitch = cos(pitchRad);
        float sinRoll = sin(rollRad);
        float cosRoll = cos(rollRad);

        return Vector3(cosRoll*sinPitch*cosYaw + sinRoll*sinYaw, cosRoll*sinPitch*sinYaw - sinRoll*cosYaw, cosRoll*cosPitch);
    }

    Matrix Angles::toMatrix() const
    {
        Vector3 x,y,z;
        angleVectors(x,y,z);

        return Matrix(x.x, y.x, z.x,
                      x.y, y.y, z.y,
                      x.z, y.z, z.z);
    }

    Matrix Angles::toMatrixTransposed() const
    {
        return toMatrix().transposed();
    }

    Matrix33 Angles::toMatrix33() const
    {
        Vector3 x,y,z;
        angleVectors(x,y,z);

        return Matrix33(x.x, y.x, z.x,
                      x.y, y.y, z.y,
                      x.z, y.z, z.z);
    }

    Matrix33 Angles::toMatrixTransposed33() const
    {
        return toMatrix33().transposed();
    }

    Quat Angles::toQuat() const
    {
        Quat ret;

        // Calculate half angles
        float halfR = DEG2RAD * roll * 0.5f;
        float halfY = DEG2RAD * yaw * 0.5f;
        float halfP = DEG2RAD * pitch * 0.5f;

        // Calculate rotation components
        float sr = sin( halfR );
        float cr = cos( halfR );
        float sy = sin( halfY );
        float cy = cos( halfY );
        float sp = sin( halfP );
        float cp = cos( halfP );

        // Assemble quaternion
        ret.x = sr*cp*cy-cr*sp*sy; // X
        ret.y = cr*sp*cy+sr*cp*sy; // Y
        ret.z = cr*cp*sy-sr*sp*cy; // Z
        ret.w = cr*cp*cy+sr*sp*sy; // W

        // Make sure output quaternion is normalized
        return ret;
    }

    Quat Angles::toQuatInverted() const
    {
        return toQuat().inverted();
    }

    static Angles Angles_ZERO(0,0,0);
    static Angles Angles_X90_CW(0.0f, 0.0f, 90.0f);
    static Angles Angles_X90_CCW(0.0f, 0.0f, -90.0f);
    static Angles Angles_Y90_CW(90.0f, 0.0f, 0.0f);
    static Angles Angles_Y90_CCW(-90.0f, 0.0f, 0.0f);
    static Angles Angles_Z90_CW(0.0f, 90.0f, 0.0f);
    static Angles Angles_Z90_CCW(0.0f, -90.0f, 0.0f);

    const Angles& Angles::ZERO() { return Angles_ZERO; }
    const Angles& Angles::X90_CW() { return Angles_X90_CW; };
    const Angles& Angles::X90_CCW() { return Angles_X90_CCW; };
    const Angles& Angles::Y90_CW() { return Angles_Y90_CW; };
    const Angles& Angles::Y90_CCW() { return Angles_Y90_CCW; };
    const Angles& Angles::Z90_CW() { return Angles_Z90_CW; };
    const Angles& Angles::Z90_CCW() { return Angles_Z90_CCW; };

    //--

    Angles Min(const Angles &a, const Angles &b)
    {
        return Angles(std::min(a.pitch, b.pitch), std::min(a.yaw, b.yaw), std::min(a.roll, b.roll));
    }

    Angles Max(const Angles &a, const Angles &b)
    {
        return Angles(std::max(a.pitch, b.pitch), std::max(a.yaw, b.yaw), std::max(a.roll, b.roll));
    }

    float Dot(const Angles &a, const Angles &b)
    {
        return Dot(a.forward(), b.forward());
    }

    Angles Snap(const Angles &a, float grid)
    {
        return Angles(Snap(a.pitch, grid), Snap(a.yaw, grid), Snap(a.roll, grid));
    }

    Angles LerpNormalized(const Angles &a, const Angles &b, float frac)
    {
        return Angles(
                a.pitch + frac * AngleDistance(a.pitch, b.pitch),
                a.yaw + frac * AngleDistance(a.yaw, b.yaw),
                a.roll + frac * AngleDistance(a.roll, b.roll));
    }

    Angles Lerp(const Angles &a, const Angles &b, float frac)
    {
        return Angles(Lerp(a.pitch, b.pitch, frac), Lerp(a.yaw, b.yaw, frac), Lerp(a.roll, b.roll, frac));
    }

    Angles Clamp(const Angles &a, const Angles &minV, const Angles &maxV)
    {
        return Angles(std::clamp(a.pitch, minV.pitch, maxV.pitch), std::clamp(a.yaw, minV.yaw, maxV.yaw), std::clamp(a.roll, minV.roll, maxV.roll));
    }

    Angles Clamp(const Angles &a, float minF, float maxF)
    {
        return Angles(std::clamp(a.pitch, minF, maxF), std::clamp(a.yaw, minF, maxF), std::clamp(a.roll, minF, maxF));
    }

    //--

    static Angles opNegA(const Angles& a) { return -a; }
    static Angles opAddA(const Angles& a, const Angles& b) { return a + b; }
    static Angles opSubA(const Angles& a, const Angles& b) { return a - b; }
    static Angles opMulAF(const Angles& a, float b) { return a * b; }
    static Angles opDivAF(const Angles& a, float b) { return a / b; }
    static float opDotA(const Angles& a, const Angles& b) { return Dot(a,b); }

    static Angles opAsssignAddA(Angles& a, const Angles& b) { return a += b; }
    static Angles opAsssignSubA(Angles& a, const Angles& b) { return a -= b; }
    static Angles opAsssignMulAF(Angles& a, float b) { return a *= b; }
    static Angles opAsssignDivAF(Angles& a, float b) { return a /= b; }

    static Angles AbsA(const Angles& a) { return a.abs(); }
    static float DotA(const Angles& a, const Angles& b) { return Dot(a,b); }
    static Angles MinA(const Angles& a, const Angles& b) { return Min(a,b); }
    static Angles MaxA(const Angles& a, const Angles& b) { return Max(a,b); }
    static Angles LerpA(const Angles& a, const Angles& b, float f) { return Lerp(a,b, f); }
    static Angles LerpNormalizedA(const Angles& a, const Angles& b, float f) { return Lerp(a,b, f); }
    static Angles ClampA(const Angles& a, const Angles& minV, const Angles& maxV) { return Clamp(a,minV,maxV); }
    static Angles ClampAF(const Angles& a, float minV, float maxV) { return Clamp(a,minV,maxV); }

    RTTI_GLOBAL_FUNCTION(AbsA, "Core.AbsA");
    RTTI_GLOBAL_FUNCTION(MinA, "Core.MinA");
    RTTI_GLOBAL_FUNCTION(MaxA, "Core.MaxA");
    RTTI_GLOBAL_FUNCTION(ClampA, "Core.ClampA");
    RTTI_GLOBAL_FUNCTION(ClampAF, "Core.ClampAF");
    RTTI_GLOBAL_FUNCTION(LerpA, "Core.LerpA");
    RTTI_GLOBAL_FUNCTION(LerpNormalizedA, "Core.LerpNormalizedA");
    RTTI_GLOBAL_FUNCTION(opNegA, "Core.opNegate_ref_Angles_Angles");
    RTTI_GLOBAL_FUNCTION(opAddA, "Core.opAdd_ref_Angles_ref_Angles_Angles");
    RTTI_GLOBAL_FUNCTION(opSubA, "Core.opSubtract_ref_Angles_ref_Angles_Angles");
    RTTI_GLOBAL_FUNCTION(opMulAF, "Core.opMultiply_ref_Angles_float_Angles");
    RTTI_GLOBAL_FUNCTION(opDivAF, "Core.opDivide_ref_Angles_float_Angles");
    RTTI_GLOBAL_FUNCTION(opDotA, "Core.opBinaryOr_ref_Angles_ref_Angles_float");
    RTTI_GLOBAL_FUNCTION(opAsssignAddA, "Core.opAddAssign_out_Angles_ref_Angles_Angles");
    RTTI_GLOBAL_FUNCTION(opAsssignSubA, "Core.opSubAssign_out_Angles_ref_Angles_Angles");
    RTTI_GLOBAL_FUNCTION(opAsssignMulAF, "Core.opMulAssign_out_Angles_float_Angles");
    RTTI_GLOBAL_FUNCTION(opAsssignDivAF, "Core.opDivAssign_out_Angles_float_Angles");

    //--

} // base
