/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

inline static const double PI_D = 3.14159265358979323846;
inline static const float PI = 3.14159265358979323846f;

inline static const double HALFPI_D = 1.57079632679489661923;
inline static const float HALFPI = 1.57079632679489661923f;

inline static const double TWOPI_D = 6.28318530717958647692;
inline static const float TWOPI = 6.28318530717958647692f;

inline static const float DEG2RAD = 0.01745329251f;
inline static const float RAD2DEG = 57.2957795131f;

inline static const float SMALL_EPSILON = 1e-6;

inline static const float NORMALIZATION_EPSILON = 1e-8;

inline static const float PLANE_EPSILON = 1e-6;

inline static const float VERY_LARGE_FLOAT = std::numeric_limits<float>::max();

END_BOOMER_NAMESPACE()