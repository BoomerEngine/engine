/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#pragma once

#include "base/geometry/include/shape.h"

namespace base
{
    namespace shape
    {

        ///----

        /// voxelization settings
        /// NOTE: voxelization grid is IZOTROPIC
        struct BASE_GEOMETRY_API VoxelizedGridSettings
        {
            uint32_t gridSize = 0; // grid size
            uint32_t gridSize2 = 0; // grid squared
            float gridScale = 0.0f;
            float gridInvScale = 0.0f;
            Vector3 gridOffset;
            Box bounds;
            double voxelVolume = 0.0;

            //---

            VoxelizedGridSettings(uint32_t gridSize, const Box& bounds);

            // transform a point from outside space to grid space (voxel space)
            Vector3 transformPointToVoxelSpace(const Vector3& src) const;

            // transform a point from voxel space to outside space
            Vector3 transformPointFromVoxelSpace(const Vector3& src) const;
        };

        ///----

        /// voxelized shape
        struct BASE_GEOMETRY_API VozelizedShape
        {
            base::Array<uint64_t> voxels;

            //---

            VozelizedShape(uint32_t gridSize);

            // reset the shape to empty volume
            void clear();

            // inset shape into the volume (mark voxels inside the shape as occupied)
            void insert(const VoxelizedGridSettings& grid, const IShape& shape);

            // merge two shapes
            void merge(const VozelizedShape& other);

            // intersect two shapes
            void intersect(const VozelizedShape& other);

            //---

            // calculate center of mass of all shapes, returns data in
            // NOTE: can return false if shape is empty
            bool calculateCenter(const VoxelizedGridSettings& grid, Vector3& outCenter, float& outVolume) const;

            // calculate inertia tensor for given center of mass
            // NOTE: returned tensor is NOT diagonalized
            // NOTE: can return false if shape is empty
            bool calculateInertiaTensor(const VoxelizedGridSettings& grid, const Vector3& comPosition, Matrix33& outTensor) const;

            //---

            // save a debug OBJ file
            void debugSave(const VoxelizedGridSettings& grid, const io::AbsolutePath& debugFile) const;
        };

    } // shape
} // base
