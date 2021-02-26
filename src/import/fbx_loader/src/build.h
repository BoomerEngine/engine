/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

#define FBXSDK_SHARED
#include <fbxsdk.h>

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

struct FBXDataNode;
struct FBXSkeletonBone;
struct FBXSkeletonBuilder;
struct FBXMaterialMapper;

class FBXFile;

//--

static Vector3 ToVector(const fbxsdk::FbxVector4& v)
{
    return Vector3((float)v[0], (float)v[1], (float)v[2]);
}

static Matrix ToMatrix(const fbxsdk::FbxMatrix& m)
{
    Matrix ret;
    ret.identity();
    ret.m[0][0] = (float)m.Get(0,0);
    ret.m[0][1] = (float)m.Get(1,0);
    ret.m[0][2] = (float)m.Get(2,0);
    ret.m[0][3] = (float)m.Get(3,0);
    ret.m[1][0] = (float)m.Get(0,1);
    ret.m[1][1] = (float)m.Get(1,1);
    ret.m[1][2] = (float)m.Get(2,1);
    ret.m[1][3] = (float)m.Get(3,1);
    ret.m[2][0] = (float)m.Get(0,2);
    ret.m[2][1] = (float)m.Get(1,2);
    ret.m[2][2] = (float)m.Get(2,2);
    ret.m[2][3] = (float)m.Get(3,2);
    return ret;
}

END_BOOMER_NAMESPACE_EX(assets)
