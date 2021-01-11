/***
* Inferno Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
* [# filter: icp #]
***/

#pragma once

namespace thirdparty
{
    namespace icp
    {
        // compute transform between two point clouds
        extern float CalcPoint2PointTransform(const float* targetPoints, const int numTargetPoints, const float* sourcePoint, const int numSourcePoints, float* outMatrix);

        // compute transform between two point clouds, returns maximum distance
        extern float CalcPoint2PointRigidTransform(const float* targetPoints, const int numTargetPoints, const float* sourcePoint, const int numSourcePoints, float* outMatrix);

    }
}