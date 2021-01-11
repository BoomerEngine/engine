///////////////////////////////////////////////////////////////////////////////
///   "Sparse Iterative Closest Point"
///   by Sofien Bouaziz, Andrea Tagliasacchi, Mark Pauly
///   Copyright (C) 2013  LGG, EPFL
///   [# filter: icp #]
///////////////////////////////////////////////////////////////////////////////
///   1) This file contains different implementations of the ICP algorithm.
///   2) This code requires EIGEN and NANOFLANN.
///   3) If OPENMP is activated some part of the code will be parallelized.
///   4) This code is for now designed for 3D registration
///   5) Two main input types are Eigen::Matrix3Xd or Eigen::Map<Eigen::Matrix3Xd>
///////////////////////////////////////////////////////////////////////////////
///   namespace nanoflann: NANOFLANN KD-tree adaptor for EIGEN
///   namespace RigidMotionEstimator: functions to compute the rigid motion
///   namespace SICP: sparse ICP implementation
///   namespace ICP: reweighted ICP implementation
///////////////////////////////////////////////////////////////////////////////

#include "build.h"
#include "icpImpl.h"

#include "base/math/include/Eigen/Dense"
#include "base/math/include/Eigen/Sparse"

namespace thirdparty
{
    namespace icp
    {

        typedef double Scalar;
        typedef Eigen::Matrix<Scalar, 3, Eigen::Dynamic> Vertices;

        static void CopyVertices(const float* targetPoints, const int numTargetPoints, Vertices& outVertices)
        {
            const auto limit = 1000000.0f;

            outVertices.resize(Eigen::NoChange, numTargetPoints);
            for (int i=0; i<numTargetPoints; ++i)
            {
                DEBUG_CHECK(!std::isnan(targetPoints[0]));
                DEBUG_CHECK(!std::isnan(targetPoints[1]));
                DEBUG_CHECK(!std::isnan(targetPoints[2]));

                DEBUG_CHECK(targetPoints[0] >= -limit && targetPoints[0] <= limit);
                DEBUG_CHECK(targetPoints[1] >= -limit && targetPoints[1] <= limit);
                DEBUG_CHECK(targetPoints[2] >= -limit && targetPoints[2] <= limit);

                outVertices(0,i) = targetPoints[0];
                outVertices(1,i) = targetPoints[1];
                outVertices(2,i) = targetPoints[2];
                targetPoints += 3;
            }

            for (int i = 0; i < numTargetPoints; ++i)
            {
                auto x = outVertices(0, i);
                auto y = outVertices(1, i);
                auto z = outVertices(2, i);

                DEBUG_CHECK(!std::isnan(x));
                DEBUG_CHECK(!std::isnan(y));
                DEBUG_CHECK(!std::isnan(z));

                DEBUG_CHECK(x >= -limit && x <= limit);
                DEBUG_CHECK(y >= -limit && y <= limit);
                DEBUG_CHECK(z >= -limit && z <= limit);
            }
        }

        static void CopyTransform(const Eigen::Affine3d& transform, float* outMatrix)
        {
            for (int i=0; i<4; ++i)
                for (int j=0; j<4; ++j)
                    *outMatrix++ = (float)transform(i,j);
        }

        float CalcPoint2PointTransform(const float* targetPoints, const int numTargetPoints, const float* sourcePoints, const int numSourcePoints, float* outMatrix)
        {
            Vertices vertices_target;
            CopyVertices(targetPoints, numTargetPoints, vertices_target);

            Vertices vertices_source;
            CopyVertices(sourcePoints, numSourcePoints, vertices_source);

            SICP::Parameters pars;
            pars.p = .5;
            pars.max_icp = 5;
            pars.use_penalty = false;
            pars.print_icpn = true;
            Eigen::Affine3d transform;
            const auto dist = SICP::point_to_point(vertices_source, vertices_target, pars, transform);
            fprintf(stdout, "Final dist: %f\n", dist);

            CopyTransform(transform, outMatrix);
            return dist;
        }

        // compute transform between two point clouds, returns maximum distance
        float CalcPoint2PointRigidTransform(const float* targetPoints, const int numTargetPoints, const float* sourcePoints, const int numSourcePoints, float* outMatrix)
        {
            Vertices vertices_target;
            CopyVertices(targetPoints, numTargetPoints, vertices_target);

            Vertices vertices_source;
            CopyVertices(sourcePoints, numSourcePoints, vertices_source);

            float distance = std::numeric_limits<float>::max();
            const auto transform = RigidMotionEstimator::point_to_point(vertices_source, vertices_target, &distance);
            CopyTransform(transform, outMatrix);

            return distance;;
        }


    }
}