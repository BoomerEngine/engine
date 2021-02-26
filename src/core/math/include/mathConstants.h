/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

//---
// Global math constants, no namespace

#undef PI
#define PI (3.14159265358979323846)

#undef HALFPI
#define HALFPI (1.57079632679489661923)

#undef TWOPI
#define TWOPI (6.28318530717958647692)

#undef DEG2RAD
#define DEG2RAD (0.01745329251f)

#undef RAD2DEG
#define RAD2DEG (57.2957795131f)

#undef SMALL_EPSILON
#define SMALL_EPSILON (1e-6)

#undef NORMALIZATION_EPSILON
#define NORMALIZATION_EPSILON (1e-8)

#undef PLANE_EPSILON
#define PLANE_EPSILON (1e-6)

#undef VERY_LARGE_FLOAT
#define VERY_LARGE_FLOAT (std::numeric_limits<float>::max())
