/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// TRS (translation-rotation-scale) transform of a node in the node chain
/// uses Euler angles for rotation for easier editing
class CORE_MATH_API EulerTransform
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(EulerTransform);

public:
    /// base types for the transform class
    typedef Vector3 Translation;
    typedef Vector3 Scale;
    typedef Angles Rotation;

    INLINE EulerTransform();
    INLINE EulerTransform(const Translation & pos);
    INLINE EulerTransform(const Translation & pos, const Rotation & rot);
    INLINE EulerTransform(const Translation & pos, const Rotation & rot, const Scale & scale);

    INLINE EulerTransform(const EulerTransform & other) = default;
    INLINE EulerTransform(EulerTransform && other) = default;
    INLINE EulerTransform& operator=(const EulerTransform & other) = default;
    INLINE EulerTransform& operator=(EulerTransform && other) = default;
    INLINE ~EulerTransform() = default;

    INLINE bool operator==(const EulerTransform& other) const;
    INLINE bool operator!=(const EulerTransform& other) const;

    //--

    Translation T;
    Rotation R;
    Scale S;

    //---

    /// set identity
    INLINE EulerTransform& identity();

    /// check if transform is identity
    bool isIdentity() const;

    //--

    // compute transform 
    Transform toTransform() const;

    /// get a matrix representation of the transform
    Matrix toMatrix() const;

    /// get a matrix representation of the transform without the scaling
    Matrix toMatrixNoScale() const;

    //--

    // print to debug stream
    void print(IFormatStream& f) const;

    //--

    // identity placement (at origin)
    static const EulerTransform& IDENTITY();
};

END_BOOMER_NAMESPACE()
