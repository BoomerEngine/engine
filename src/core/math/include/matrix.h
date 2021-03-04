/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

enum ETransposedFlag
{
    TRANSPOSED_FLAG,
};

/// 4x4 full matrix
TYPE_ALIGN(16, class) CORE_MATH_API Matrix
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Matrix);

public:
    // unaligned matrix data
    float m[4][4];

    //--

    INLINE Matrix(); // identity
    INLINE Matrix(const Matrix33 &other, const Vector3& t = Vector3::ZERO());
    INLINE Matrix(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22);
    INLINE Matrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33);
    INLINE Matrix(const Vector3& x, const Vector3& y, const Vector3& z);
    INLINE Matrix(const Vector4& x, const Vector4& y, const Vector4& z, const Vector4& w);
    INLINE Matrix(const Matrix& other) = default;
    INLINE Matrix(Matrix&& other) = default;
    INLINE Matrix& operator=(const Matrix& other) = default;
    INLINE Matrix& operator=(Matrix&& other) = default;

    INLINE Matrix(const float* data); // memcpy
    INLINE Matrix(const double* data);
    INLINE Matrix(ETransposedFlag, const float* data); // load transposed
    INLINE Matrix(ETransposedFlag, const double* data); // convert from doubles and load transposed

    //--

    INLINE Matrix operator~() const;
    INLINE Matrix operator*(const Matrix &other) const;
    INLINE Matrix &operator*=(const Matrix &other);
    INLINE bool operator==(const Matrix& other) const;
    INLINE bool operator!=(const Matrix& other) const;

    //---

    //! reset to zeros
    INLINE void zero();

    //! reset matrix to identity matrix
    INLINE void identity();

    //! set translation part of the matrix
    INLINE void translation(const Vector3 &trans);

    //! set translation part of the matrix
    INLINE void translation(float x, float y, float z);

    //! get translation part
    INLINE Vector3 translation() const;

    //! build column vector
    INLINE Vector4 column(int i) const;

    //! build row vector
    INLINE Vector4& row(int i);

    //! build row vector
    INLINE const Vector4& row(int i) const;

    //! get matrix row lengths (scale factors)
    Vector3 rowLengths() const;

    //! get matrix columnt length (scale factors)
    Vector3 columnLengths() const;

    //! scale matrix rows
    Matrix& scaleRows(const Vector3 &scale);

    //! scale matrix columns
    Matrix& scaleColumns(const Vector3 &scale);

    //! scale inner matrix
    Matrix& scaleInner(float scale);

    //! scale matrix translation
    Matrix& scaleTranslation(const Vector3 &scale);

    //! scale matrix translation
    Matrix& scaleTranslation(float scale);

    //! normalize lengths of the 3 major rows
    Matrix& normalizeRows();

    //! normalize length of the 3 major columns
    Matrix& normalizeColumns();

    //--

    //! Calculate matrix determinant
    double det() const;

    //! Calculate determinant of 3x3 matrix mart
    double det3() const;

    //! Transpose matrix
    Matrix transposed() const;

    //! Return inverted matrix
    Matrix inverted() const;

    //! Transpose this matrix
    void transpose();

    //! Invert matrix
    void invert();

    ///--

    //! Concatenate matrices
    static Matrix Concat(const Matrix &a, const Matrix &b);

    //! Build scale matrix
    static Matrix BuildScale(const Vector3 &scale);

    //! Build uniform scale matrix
    static Matrix BuildScale(float scale);

    //! Build translation matrix
    static Matrix BuildTranslation(const Vector3 &trans);

    //! Build rotation matrix
    static Matrix BuildRotation(const Angles& rotation);

    //! Build rotation matrix around the X axis
    static Matrix BuildRotationX(float degress);

    //! Build rotation matrix around the Y axis
    static Matrix BuildRotationY(float degress);

    //! Build rotation matrix around the Z axis
    static Matrix BuildRotationZ(float degress);

    //! Build rotation matrix
    static Matrix BuildRotation(float pitch, float yaw, float roll);

    //! Build rotation matrix from quaternion
    static Matrix BuildRotation(const Quat& rotation);

    //! Build reflection matrix
    static Matrix BuildReflection(const Plane &plane);

    //! Build matrix from translation, rotation and scale (TRS order)
    static Matrix BuildTRS(const Vector3& t, const Angles& r, const Vector3& s = Vector3::ONE());

    //! Build matrix from translation, rotation and scale (TRS order)
    static Matrix BuildTRS(const Vector3& t, const Quat& r, const Vector3& s = Vector3::ONE());

    //! Build matrix from translation, rotation and scale (TRS order)
    static Matrix BuildTRS(const Vector3& t, const Angles& r, float s);

    //! Build matrix from translation, rotation and scale (TRS order)
    static Matrix BuildTRS(const Vector3& t, const Quat& r, float s);

    //! Build matrix from translation, rotation and scale (TRS order)
    static Matrix BuildTRS(const Vector3& t, const Matrix33& rs);

    //! Build perspective projection matrix using viewport scales directly
    //! NOTE: in the engine we use inverted depth buffer Z values, so Z=0 at far plane and Z=1 at near plane
    //! NOTE: assumes left handled coordinate system
    static Matrix BuildPerspective(float xScale, float yScale, float zNear, float zFar, float offsetX = 0.0f, float offsetY = 0.0f);

    //! Build perspective projection matrix using FOV and aspect ratio
    //! NOTE: in the engine we use inverted depth buffer Z values, so Z=0 at far plane and Z=1 at near plane
    //! NOTE: assumes left handled coordinate system
    static Matrix BuildPerspectiveFOV(float fovDeg, float aspectWidthToHeight, float zNear, float zFar, float offsetX = 0.0f, float offsetY = 0.0f);

    //! Build orthographic project matrix using specified width and height of the viewport as well as the zNear and zFar planes
    static Matrix BuildOrtho(float width, float height, float zNear, float zFar);

    //---

    //! Transform point by matrix
    Vector3 transformPoint(const Vector3 &point) const;

    //! Transform point by matrix
    void transformPoint(const Vector3 &point, Vector3 &out) const;

    //! Transform point by matrix, output also W component
    void transformPoint(const Vector3 &point, Vector3 &out, float &w) const;

    //! Transform point by inverted matrix (only TR matrices are supported)
    Vector3 transformInvPoint(const Vector3 &point) const;

    //! Transform point by inverted matrix (only TR matrices are supported)
    void transformInvPoint(const Vector3 &point, Vector3 &out) const;

    //! Transform vector by matrix
    Vector3 transformVector(const Vector3 &point) const;

    //! Transform vector by matrix
    void transformVector(const Vector3 &point, Vector3 &out) const;

    //! Transform vector by transposed matrix
    Vector3 transformInvVector(const Vector3 &point) const;

    //! Transform vector by transposed matrix
    void transformInvVector(const Vector3 &point, Vector3 &out) const;

    //! Transform vector4 by matrix
    Vector4 transformVector4(const Vector4 &point) const;

    //! Transform vector4 by transposed matrix
    Vector4 transformInvVector4(const Vector4 &point) const;

    //! Transform plane
    Plane transformPlane(const Plane &plane) const;

    //! Transform plane
    void transformPlane(const Plane &plane, Plane &out) const;

    //! Transform box
    Box transformBox(const Box &box) const;

    //! Transform box
    void transformBox(const Box &box, Box &out) const;

    ///---

    //! convert to rotation, assumes orthonormal 3x3 matrix
    Angles toRotator() const;

    //! convert to quaternion (matrix should be orthonormal)
    Quat toQuat() const;

    //! convert to TRS transform (rotation + translation + scale)
    //! NOTE: sheering is NOT preserved
    Transform toTransform() const;

    //! export to table of floats
    void toFloats(float* outData) const;

    //! export to table of doubles
    void toDoubles(double* outData) const;

    //! export to TRANSPOSED table of floats
    void toFloatsTransposed(float* outData) const;

    //! export to TRANSPOSED table of doubles
    void toDoublesTransposed(float* outData) const;

    //---

    //! get identity matrix
    static const Matrix& IDENTITY();

    //! get zero matrix
    static const Matrix& ZERO();

    //---

    //! print matrix
    void print(IFormatStream& f) const;

private:
    double coFactor(int row, int col) const;
};

//--

END_BOOMER_NAMESPACE()
