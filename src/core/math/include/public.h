/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#pragma once

// STD headers
#include <math.h>

// Glue code
#include "core_math_glue.inl"

// Math is global
#include "mathConstants.h" // TODO: remove!
#include "mathCommon.h"

// Math classes
#include "rotation.h"
#include "vector3.h"
#include "position.h"
#include "vector2.h"
#include "vector4.h"
#include "box.h"
#include "plane.h"
#include "matrix.h"
#include "quat.h"
#include "color.h"
#include "point.h"
#include "rect.h"
#include "range.h"
#include "float16.h"
#include "xform2D.h"
#include "transform.h"
#include "eulerTransform.h"
#include "matrix33.h"
#include "simd.h"
#include "stack2D.h"
#include "stack3D.h"
#include "mathRandom.h"
#include "obb.h"
#include "sphere.h"
#include "camera.h"
#include "lerp.h"
#include "trans.h"

// Inlined part of math classes
#include "vector2.inl"
#include "vector3.inl"
#include "vector4.inl"
#include "color.inl"
#include "box.inl"
#include "matrix.inl"
#include "rotation.inl"
#include "quat.inl"
#include "plane.inl"
#include "point.inl"
#include "rect.inl"
#include "range.inl"
#include "xform2D.inl"
#include "transform.inl"
#include "eulerTransform.inl"
#include "matrix33.inl"
#include "position.inl"
#include "simd.inl"
#include "stack2D.inl"
#include "stack3D.inl"
#include "lerp.inl"
#include "trans.inl"
#include "mathCommon.inl"
#include "mathRandom.inl"

//--

// Shims for scripting

BEGIN_BOOMER_NAMESPACE();

BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Vector2)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Vector3)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Vector4)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Matrix)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Matrix33)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Quat)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Transform)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(EulerTransform)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Angles)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Point)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Rect)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Color)
BOOMER_MONO_SHIM_VALUE_STRUCT_TYPE(Box)

END_BOOMER_NAMESPACE();
