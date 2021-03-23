/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix33 #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// 3x3 rotation-sheer-scale matrix
/// can be used for 2D stuff as well
class CORE_MATH_API Matrix33
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Matrix33);

public:
    float m[3][3];

    //--

    INLINE Matrix33();
    INLINE Matrix33(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22);
    INLINE Matrix33(const Vector3& x, const Vector3& y, const Vector3& z);
    INLINE Matrix33(const Matrix33 &other) = default;
    INLINE Matrix33(Matrix33&& other) = default;
    INLINE Matrix33& operator=(const Matrix33 &other) = default;
    INLINE Matrix33& operator=(Matrix33&& other) = default;

    INLINE Matrix33 operator~() const;
    INLINE Matrix33 operator*(const Matrix33 &other) const;
    INLINE Matrix33 &operator*=(const Matrix33 &other);

    INLINE bool operator==(const Matrix33& other) const;
    INLINE bool operator!=(const Matrix33& other) const;

    //--

    //! Reset matrix to zeros
    INLINE void zero();

    //! Reset matrix to identity matrix
    INLINE void identity();

    //! get matrix row lengths (scale factors)
    Vector3 rowLengths() const;

    //! get matrix columnt length (scale factors)
    Vector3 columnLengths() const;

    //! scale matrix rows
    void scaleRows(const Vector3 &scale);

    //! scale matrix columns
    void scaleColumns(const Vector3 &scale);

    //! scale inner matrix
    void scaleInner(float scale);

    //! normalize lengths of the 3 major rows
    void normalizeRows();

    //! normalize length of the 3 major columns
    void normalizeColumns();

    //! Calculate matrix determinant
    double det() const;

    //! Transpose this matrix
    void transpose();

    //! Transpose matrix
    Matrix33 transposed() const;

    //! Invert matrix
    void invert();

    //! Return inverted matrix
    Matrix33 inverted() const;

    //! Get row
    Vector3 row(int i) const;

    //! Get column of matrix
    Vector3 column(int i) const;

    //! Get trace (diagonal) of the matrix
    Vector3 trace() const;

    //! Diagonalize this matrix, returns the rotation to apply and modifies the current matrix to only leave elements on the diagonal
    void diagonalise(Matrix33& outRotation, double limit = 0.01f, uint32_t maxIterations = 64);

    //--

    //! Concatenate matrices
    static Matrix33 Concat(const Matrix33 &a, const Matrix33 &b);

    //! Build scale matrix
    static Matrix33 BuildScale(const Vector3 &scale);

    //! Build uniform scale matrix
    static Matrix33 BuildScale(float scale);

    //! Build rotation matrix from Euler angles
    static Matrix33 BuildRotation(const Angles& rotation);

    //! Build rotation matrix from Euler angles
    static Matrix33 BuildRotation(float pitch, float yaw, float roll);

    //! Build rotation matrix from quaternion
    static Matrix33 BuildRotation(const Quat& quat);

    //! Build scale-rotation matrix
    static Matrix33 BuildRotationScale(const Angles& rotation, const Vector3& scale);
    static Matrix33 BuildRotationScale(const Quat& quat, const Vector3& scale);
    static Matrix33 BuildRotationScale(const Angles& rotation, float scale);
    static Matrix33 BuildRotationScale(const Quat& quat, float scale);

    //--

    //! convert to rotation, assumes orthonormal 3x3 matrix
    Angles toRotator() const;

    //! convert to quaternion (matrix should be orthonormal)
    Quat toQuat() const;

    //! Convert to TRS transform (rotation + translation + scale)
    //! NOTE: sheering is NOT preserved
    class Transform toTransform(const Vector3& position = Vector3::ZERO()) const;

    //! Convert to full matrix
    Matrix toMatrix() const;

    //--

    //! get identity matrix
    static const Matrix33& IDENTITY();

    //! get matrix with zeros
    static const Matrix33& ZERO();

private:
    double coFactor(int row, int col) const;
};

END_BOOMER_NAMESPACE()
