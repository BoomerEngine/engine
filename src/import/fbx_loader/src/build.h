/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

//#define FBXSDK_SHARED
//#include <fbxsdk.h>

#include "ofbx/ofbx.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

struct FBXDataNode;
struct FBXSkeletonBone;
struct FBXSkeletonBuilder;
struct FBXMaterialMapper;

class FBXFile;

//--

static Vector3 ToVector(const ofbx::Vec4& v)
{
    return Vector3((float)v.x, (float)v.y, (float)v.z);
}

static Vector3 ToVector(const ofbx::Vec3& v)
{
    return Vector3((float)v.x, (float)v.y, (float)v.z);
}

static Matrix ToMatrix(const ofbx::Matrix& m)
{
    return Matrix(TRANSPOSED_FLAG, m.m);
}

END_BOOMER_NAMESPACE_EX(assets)
