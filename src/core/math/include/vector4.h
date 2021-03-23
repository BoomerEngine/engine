/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector4 #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// 4 component vector
TYPE_ALIGN(16, class) CORE_MATH_API Vector4
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Vector4);

public:
    float x,y,z,w;

    //--

    INLINE Vector4();
    INLINE Vector4(float inX, float inY, float inZ, float inW);
    INLINE Vector4(const Vector3 &other, float inW = 1.0f);
    INLINE Vector4(const Vector4 &other) = default;
    INLINE Vector4(Vector4&& other) = default;
    INLINE Vector4& operator=(const Vector4 &other) = default;
    INLINE Vector4& operator=(Vector4&& other) = default;

    INLINE Vector4 operator-() const;

    INLINE Vector4 operator+(const Vector4 &other) const;
    INLINE Vector4 operator+(float value) const;
    INLINE Vector4 operator-(const Vector4 &other) const;
    INLINE Vector4 operator-(float value) const;
    INLINE Vector4 operator*(const Vector4 &other) const;
    INLINE Vector4 operator*(float value) const;
    friend Vector4 operator*(float value, const Vector4 &other);
    INLINE Vector4 operator/(const Vector4 &other) const;
    INLINE Vector4 operator/(float value) const;

    INLINE Vector4 &operator+=(const Vector4 &other);
    INLINE Vector4 &operator+=(float value);
    INLINE Vector4 &operator-=(const Vector4 &other);
    INLINE Vector4 &operator-=(float value);
    INLINE Vector4 &operator*=(const Vector4 &other);
    INLINE Vector4 &operator*=(float value);
    INLINE Vector4 &operator/=(const Vector4 &other);
    INLINE Vector4 &operator/=(float value);

    INLINE bool operator==(const Vector4 &other) const;
    INLINE bool operator!=(const Vector4 &other) const;

    INLINE float operator|(const Vector4 &other) const;
    INLINE float operator|(const Vector3 &other) const;
    INLINE float operator|(const Vector2 &other) const;

    //--

    //! returns minimal from all vector components
    INLINE float minValue() const;

    //! returns maximal from all vector components
    INLINE float maxValue() const;

    //! returns sum of all components
    INLINE float sum() const;

    //! returns product of all components
    INLINE float trace() const;

    //! get vector with absolute components
    INLINE Vector4 abs() const;

    //! round to nearest integer
    INLINE Vector4 round() const;

    //! get fractional part
    INLINE Vector4 frac() const;

    //! truncate fractional part
    INLINE Vector4 trunc() const;

    //! round to lower integer
    INLINE Vector4 floor() const;

    //! round to higher integer
    INLINE Vector4 ceil() const;

    //! square length of vector
    INLINE float squareLength() const;

    //! length of the vector
    INLINE float length() const;

    //! one over length, fast, returns 0 if length is zero
    INLINE float invLength() const;

    //! check if vector is close to zero vector
    INLINE bool isNearZero(float epsilon = SMALL_EPSILON) const;

    //! check if vector is near enough to another one
    INLINE bool isSimilar(const Vector4 &other, float epsilon = SMALL_EPSILON) const;

    //! check if vector is exactly equal to zero vector
    INLINE bool isZero() const;

    //! normalize vector, does nothing if vector is of zero length
    INLINE float normalize();

    //! normalize vector, no checks, returns length
    INLINE float normalizeFast();

    //! return normalized vector, returns the same vector if it can't be normalized
    INLINE Vector4 normalized() const;

    //! return fast normalized vector, no checks
    INLINE Vector4 normalizedFast() const;

    //! project vector back to w=1 plane if w != 0
    INLINE void project();

    //! project vector back to w=1 plane if W is non zero
    INLINE void projectFast();

    //! project vector back to w=1 plane if w != 0
    INLINE Vector3 projected() const;

    //! project vector back to w=1 plane if W is non zero
    INLINE Vector3 projectedFast() const;

    //! get the 1/vector, safe
    INLINE Vector4 oneOver() const;

    //! get the 1/vector, unsafe
    INLINE Vector4 oneOverFast() const;

    //---

    //! Calculate 4D distance to other point
    INLINE float distance(const Vector4 &other) const;

    //! calculate 4D square distance to other point
    INLINE float squareDistance(const Vector4 &other) const;

    //---

    //! extract sub vectors
    INLINE Vector2 xx() const;
    INLINE Vector2 xz() const;
    INLINE Vector2 xw() const;
    INLINE Vector2 yx() const;
    INLINE Vector2 yy() const;
    INLINE Vector2 yw() const;
    INLINE Vector2 zx() const;
    INLINE Vector2 zy() const;
    INLINE Vector2 zz() const;
    INLINE Vector2 wx() const;
    INLINE Vector2 wy() const;
    INLINE Vector2 wz() const;
    INLINE Vector2 ww() const;

    INLINE Vector3 xxx() const;
    INLINE Vector3 xxy() const;
    INLINE Vector3 xxz() const;
    INLINE Vector3 xxw() const;
    INLINE Vector3 xyx() const;
    INLINE Vector3 xyy() const;
    INLINE Vector3 xyw() const;
    INLINE Vector3 xzx() const;
    INLINE Vector3 xzy() const;
    INLINE Vector3 xzz() const;
    INLINE Vector3 xzw() const;
    INLINE Vector3 xwx() const;
    INLINE Vector3 xwy() const;
    INLINE Vector3 xwz() const;
    INLINE Vector3 xww() const;
    INLINE Vector3 yxx() const;
    INLINE Vector3 yxy() const;
    INLINE Vector3 yxz() const;
    INLINE Vector3 yxw() const;
    INLINE Vector3 yyx() const;
    INLINE Vector3 yyy() const;
    INLINE Vector3 yyz() const;
    INLINE Vector3 yyw() const;
    INLINE Vector3 yzx() const;
    INLINE Vector3 yzy() const;
    INLINE Vector3 yzz() const;
    INLINE Vector3 ywx() const;
    INLINE Vector3 ywy() const;
    INLINE Vector3 ywz() const;
    INLINE Vector3 yww() const;
    INLINE Vector3 zxx() const;
    INLINE Vector3 zxy() const;
    INLINE Vector3 zxz() const;
    INLINE Vector3 zxw() const;
    INLINE Vector3 zyx() const;
    INLINE Vector3 zyy() const;
    INLINE Vector3 zyz() const;
    INLINE Vector3 zyw() const;
    INLINE Vector3 zzx() const;
    INLINE Vector3 zzy() const;
    INLINE Vector3 zzz() const;
    INLINE Vector3 zzw() const;
    INLINE Vector3 zwx() const;
    INLINE Vector3 zwy() const;
    INLINE Vector3 zwz() const;
    INLINE Vector3 zww() const;
    INLINE Vector3 wxx() const;
    INLINE Vector3 wxy() const;
    INLINE Vector3 wxz() const;
    INLINE Vector3 wxw() const;
    INLINE Vector3 wyx() const;
    INLINE Vector3 wyy() const;
    INLINE Vector3 wyz() const;
    INLINE Vector3 wyw() const;
    INLINE Vector3 wzx() const;
    INLINE Vector3 wzy() const;
    INLINE Vector3 wzz() const;
    INLINE Vector3 wzw() const;
    INLINE Vector3 wwx() const;
    INLINE Vector3 wwy() const;
    INLINE Vector3 wwz() const;
    INLINE Vector3 www() const;

    INLINE Vector4 xxxx() const;
    INLINE Vector4 xxxy() const;
    INLINE Vector4 xxxz() const;
    INLINE Vector4 xxxw() const;
    INLINE Vector4 xxyx() const;
    INLINE Vector4 xxyy() const;
    INLINE Vector4 xxyz() const;
    INLINE Vector4 xxyw() const;
    INLINE Vector4 xxzx() const;
    INLINE Vector4 xxzy() const;
    INLINE Vector4 xxzz() const;
    INLINE Vector4 xxzw() const;
    INLINE Vector4 xxwx() const;
    INLINE Vector4 xxwy() const;
    INLINE Vector4 xxwz() const;
    INLINE Vector4 xxww() const;
    INLINE Vector4 xyxx() const;
    INLINE Vector4 xyxy() const;
    INLINE Vector4 xyxz() const;
    INLINE Vector4 xyxw() const;
    INLINE Vector4 xyyx() const;
    INLINE Vector4 xyyy() const;
    INLINE Vector4 xyyz() const;
    INLINE Vector4 xyyw() const;
    INLINE Vector4 xyzx() const;
    INLINE Vector4 xyzy() const;
    INLINE Vector4 xyzz() const;
    INLINE Vector4 xywx() const;
    INLINE Vector4 xywy() const;
    INLINE Vector4 xywz() const;
    INLINE Vector4 xyww() const;
    INLINE Vector4 xzxx() const;
    INLINE Vector4 xzxy() const;
    INLINE Vector4 xzxz() const;
    INLINE Vector4 xzxw() const;
    INLINE Vector4 xzyx() const;
    INLINE Vector4 xzyy() const;
    INLINE Vector4 xzyz() const;
    INLINE Vector4 xzyw() const;
    INLINE Vector4 xzzx() const;
    INLINE Vector4 xzzy() const;
    INLINE Vector4 xzzz() const;
    INLINE Vector4 xzzw() const;
    INLINE Vector4 xzwx() const;
    INLINE Vector4 xzwy() const;
    INLINE Vector4 xzwz() const;
    INLINE Vector4 xzww() const;
    INLINE Vector4 xwxx() const;
    INLINE Vector4 xwxy() const;
    INLINE Vector4 xwxz() const;
    INLINE Vector4 xwxw() const;
    INLINE Vector4 xwyx() const;
    INLINE Vector4 xwyy() const;
    INLINE Vector4 xwyz() const;
    INLINE Vector4 xwyw() const;
    INLINE Vector4 xwzx() const;
    INLINE Vector4 xwzy() const;
    INLINE Vector4 xwzz() const;
    INLINE Vector4 xwzw() const;
    INLINE Vector4 xwwx() const;
    INLINE Vector4 xwwy() const;
    INLINE Vector4 xwwz() const;
    INLINE Vector4 xwww() const;
    INLINE Vector4 yxxx() const;
    INLINE Vector4 yxxy() const;
    INLINE Vector4 yxxz() const;
    INLINE Vector4 yxxw() const;
    INLINE Vector4 yxyx() const;
    INLINE Vector4 yxyy() const;
    INLINE Vector4 yxyz() const;
    INLINE Vector4 yxyw() const;
    INLINE Vector4 yxzx() const;
    INLINE Vector4 yxzy() const;
    INLINE Vector4 yxzz() const;
    INLINE Vector4 yxzw() const;
    INLINE Vector4 yxwx() const;
    INLINE Vector4 yxwy() const;
    INLINE Vector4 yxwz() const;
    INLINE Vector4 yxww() const;
    INLINE Vector4 yyxx() const;
    INLINE Vector4 yyxy() const;
    INLINE Vector4 yyxz() const;
    INLINE Vector4 yyxw() const;
    INLINE Vector4 yyyx() const;
    INLINE Vector4 yyyy() const;
    INLINE Vector4 yyyz() const;
    INLINE Vector4 yyyw() const;
    INLINE Vector4 yyzx() const;
    INLINE Vector4 yyzy() const;
    INLINE Vector4 yyzz() const;
    INLINE Vector4 yyzw() const;
    INLINE Vector4 yywx() const;
    INLINE Vector4 yywy() const;
    INLINE Vector4 yywz() const;
    INLINE Vector4 yyww() const;
    INLINE Vector4 yzxx() const;
    INLINE Vector4 yzxy() const;
    INLINE Vector4 yzxz() const;
    INLINE Vector4 yzxw() const;
    INLINE Vector4 yzyx() const;
    INLINE Vector4 yzyy() const;
    INLINE Vector4 yzyz() const;
    INLINE Vector4 yzyw() const;
    INLINE Vector4 yzzx() const;
    INLINE Vector4 yzzy() const;
    INLINE Vector4 yzzz() const;
    INLINE Vector4 yzzw() const;
    INLINE Vector4 yzwx() const;
    INLINE Vector4 yzwy() const;
    INLINE Vector4 yzwz() const;
    INLINE Vector4 yzww() const;
    INLINE Vector4 ywxx() const;
    INLINE Vector4 ywxy() const;
    INLINE Vector4 ywxz() const;
    INLINE Vector4 ywxw() const;
    INLINE Vector4 ywyx() const;
    INLINE Vector4 ywyy() const;
    INLINE Vector4 ywyz() const;
    INLINE Vector4 ywyw() const;
    INLINE Vector4 ywzx() const;
    INLINE Vector4 ywzy() const;
    INLINE Vector4 ywzz() const;
    INLINE Vector4 ywzw() const;
    INLINE Vector4 ywwx() const;
    INLINE Vector4 ywwy() const;
    INLINE Vector4 ywwz() const;
    INLINE Vector4 ywww() const;
    INLINE Vector4 zxxx() const;
    INLINE Vector4 zxxy() const;
    INLINE Vector4 zxxz() const;
    INLINE Vector4 zxxw() const;
    INLINE Vector4 zxyx() const;
    INLINE Vector4 zxyy() const;
    INLINE Vector4 zxyz() const;
    INLINE Vector4 zxyw() const;
    INLINE Vector4 zxzx() const;
    INLINE Vector4 zxzy() const;
    INLINE Vector4 zxzz() const;
    INLINE Vector4 zxzw() const;
    INLINE Vector4 zxwx() const;
    INLINE Vector4 zxwy() const;
    INLINE Vector4 zxwz() const;
    INLINE Vector4 zxww() const;
    INLINE Vector4 zyxx() const;
    INLINE Vector4 zyxy() const;
    INLINE Vector4 zyxz() const;
    INLINE Vector4 zyxw() const;
    INLINE Vector4 zyyx() const;
    INLINE Vector4 zyyy() const;
    INLINE Vector4 zyyz() const;
    INLINE Vector4 zyyw() const;
    INLINE Vector4 zyzx() const;
    INLINE Vector4 zyzy() const;
    INLINE Vector4 zyzz() const;
    INLINE Vector4 zyzw() const;
    INLINE Vector4 zywx() const;
    INLINE Vector4 zywy() const;
    INLINE Vector4 zywz() const;
    INLINE Vector4 zyww() const;
    INLINE Vector4 zzxx() const;
    INLINE Vector4 zzxy() const;
    INLINE Vector4 zzxz() const;
    INLINE Vector4 zzxw() const;
    INLINE Vector4 zzyx() const;
    INLINE Vector4 zzyy() const;
    INLINE Vector4 zzyz() const;
    INLINE Vector4 zzyw() const;
    INLINE Vector4 zzzx() const;
    INLINE Vector4 zzzy() const;
    INLINE Vector4 zzzz() const;
    INLINE Vector4 zzzw() const;
    INLINE Vector4 zzwx() const;
    INLINE Vector4 zzwy() const;
    INLINE Vector4 zzwz() const;
    INLINE Vector4 zzww() const;
    INLINE Vector4 zwxx() const;
    INLINE Vector4 zwxy() const;
    INLINE Vector4 zwxz() const;
    INLINE Vector4 zwxw() const;
    INLINE Vector4 zwyx() const;
    INLINE Vector4 zwyy() const;
    INLINE Vector4 zwyz() const;
    INLINE Vector4 zwyw() const;
    INLINE Vector4 zwzx() const;
    INLINE Vector4 zwzy() const;
    INLINE Vector4 zwzz() const;
    INLINE Vector4 zwzw() const;
    INLINE Vector4 zwwx() const;
    INLINE Vector4 zwwy() const;
    INLINE Vector4 zwwz() const;
    INLINE Vector4 zwww() const;
    INLINE Vector4 wxxx() const;
    INLINE Vector4 wxxy() const;
    INLINE Vector4 wxxz() const;
    INLINE Vector4 wxxw() const;
    INLINE Vector4 wxyx() const;
    INLINE Vector4 wxyy() const;
    INLINE Vector4 wxyz() const;
    INLINE Vector4 wxyw() const;
    INLINE Vector4 wxzx() const;
    INLINE Vector4 wxzy() const;
    INLINE Vector4 wxzz() const;
    INLINE Vector4 wxzw() const;
    INLINE Vector4 wxwx() const;
    INLINE Vector4 wxwy() const;
    INLINE Vector4 wxwz() const;
    INLINE Vector4 wxww() const;
    INLINE Vector4 wyxx() const;
    INLINE Vector4 wyxy() const;
    INLINE Vector4 wyxz() const;
    INLINE Vector4 wyxw() const;
    INLINE Vector4 wyyx() const;
    INLINE Vector4 wyyy() const;
    INLINE Vector4 wyyz() const;
    INLINE Vector4 wyyw() const;
    INLINE Vector4 wyzx() const;
    INLINE Vector4 wyzy() const;
    INLINE Vector4 wyzz() const;
    INLINE Vector4 wyzw() const;
    INLINE Vector4 wywx() const;
    INLINE Vector4 wywy() const;
    INLINE Vector4 wywz() const;
    INLINE Vector4 wyww() const;
    INLINE Vector4 wzxx() const;
    INLINE Vector4 wzxy() const;
    INLINE Vector4 wzxz() const;
    INLINE Vector4 wzxw() const;
    INLINE Vector4 wzyx() const;
    INLINE Vector4 wzyy() const;
    INLINE Vector4 wzyz() const;
    INLINE Vector4 wzyw() const;
    INLINE Vector4 wzzx() const;
    INLINE Vector4 wzzy() const;
    INLINE Vector4 wzzz() const;
    INLINE Vector4 wzzw() const;
    INLINE Vector4 wzwx() const;
    INLINE Vector4 wzwy() const;
    INLINE Vector4 wzwz() const;
    INLINE Vector4 wzww() const;
    INLINE Vector4 wwxx() const;
    INLINE Vector4 wwxy() const;
    INLINE Vector4 wwxz() const;
    INLINE Vector4 wwxw() const;
    INLINE Vector4 wwyx() const;
    INLINE Vector4 wwyy() const;
    INLINE Vector4 wwyz() const;
    INLINE Vector4 wwyw() const;
    INLINE Vector4 wwzx() const;
    INLINE Vector4 wwzy() const;
    INLINE Vector4 wwzz() const;
    INLINE Vector4 wwzw() const;
    INLINE Vector4 wwwx() const;
    INLINE Vector4 wwwy() const;
    INLINE Vector4 wwwz() const;
    INLINE Vector4 wwww() const;

    INLINE Vector2& xy();
    INLINE Vector2& yz();
    INLINE Vector2& zw();
    INLINE Vector3& xyz();
    INLINE Vector3& yzw();
    INLINE Vector4& xyzw();

    INLINE const Vector2& xy() const;
    INLINE const Vector2& yz() const;
    INLINE const Vector2& zw() const;
    INLINE const Vector3& xyz() const;
    INLINE const Vector3& yzw() const;
    INLINE const Vector4& xyzw() const;

    //---

    static const Vector4& ZERO();
    static const Vector4& ZEROH(); // 0 0 0 1
    static const Vector4& ONE();
    static const Vector4& EX();
    static const Vector4& EY();
    static const Vector4& EZ();
    static const Vector4& EW();
    static const Vector4& INF();

    //---

    //! Position based interface for polygons
    INLINE Vector3& position() { return *(Vector3*) this; }
    INLINE const Vector3& position() const { return *(const Vector3*) this; }

    //---

    void print(IFormatStream& f) const;

private:
    INLINE Vector2 _xy() const;
    INLINE Vector2 _yz() const;
    INLINE Vector2 _zw() const;
    INLINE Vector3 _xyz() const;
    INLINE Vector3 _yzw() const;
    INLINE Vector4 _xyzw() const;
};

//--

END_BOOMER_NAMESPACE()
