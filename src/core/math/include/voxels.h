/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\voxels #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

///----

/// voxelization settings
/// NOTE: voxelization grid is IZOTROPIC
struct CORE_MATH_API VoxelizedGridSettings
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

/// binary (bit-size) voxel grid that can be used to voxelize shapes
struct CORE_MATH_API VoxelizedShape
{
    Array<uint64_t> voxels; // TODO: swizzle into 4x4x4 cells instead of

    //---

    VoxelizedShape(uint32_t gridSize);

    // reset the shape to empty volume
    void clear();

    // merge two shapes
    void merge(const VoxelizedShape& other);

    // intersect two shapes
    void intersect(const VoxelizedShape& other);

    //---

    // inset shape into the volume (mark voxels inside the shape as occupied)
    template< typename T >
    void insert(const VoxelizedGridSettings& grid, const T& shape);

    //---

    // calculate center of mass of all shapes, returns data in
    // NOTE: can return false if shape is empty
    bool calculateCenter(const VoxelizedGridSettings& grid, Vector3& outCenter, float& outVolume) const;

    // calculate inertia tensor for given center of mass
    // NOTE: returned tensor is NOT diagonalized
    // NOTE: can return false if shape is empty
    bool calculateInertiaTensor(const VoxelizedGridSettings& grid, const Vector3& comPosition, Matrix33& outTensor) const;

    //---
};

///----

template< typename T >
void VoxelizedShape::insert(const VoxelizedGridSettings& grid, const T& shape)
{
    auto shapeBounds = shape.bounds();
    if (!shapeBounds.touches(grid.bounds))
        return;

    auto voxelMin = grid.transformPointToVoxelSpace(shapeBounds.min);
    auto voxelMax = grid.transformPointToVoxelSpace(shapeBounds.max);

    auto voxelMinX = std::clamp<uint32_t>(std::floor(voxelMin.x), 0, grid.gridSize - 1);
    auto voxelMaxX = std::clamp<uint32_t>(std::ceil(voxelMax.x), 0, grid.gridSize - 1);
    auto voxelMinY = std::clamp<uint32_t>(std::floor(voxelMin.y), 0, grid.gridSize - 1);
    auto voxelMaxY = std::clamp<uint32_t>(std::ceil(voxelMax.y), 0, grid.gridSize - 1);
    auto voxelMinZ = std::clamp<uint32_t>(std::floor(voxelMin.z), 0, grid.gridSize - 1);
    auto voxelMaxZ = std::clamp<uint32_t>(std::ceil(voxelMax.z), 0, grid.gridSize - 1);

    auto voxelStepX = grid.gridInvScale;
    auto voxelStepY = grid.gridInvScale;
    auto voxelStepZ = grid.gridInvScale;
    auto voxelStartX = grid.bounds.min.x + grid.gridInvScale * (voxelMinX + 0.5f);
    auto voxelStartY = grid.bounds.min.y + grid.gridInvScale * (voxelMinY + 0.5f);
    auto voxelStartZ = grid.bounds.min.z + grid.gridInvScale * (voxelMinZ + 0.5f);

    // iterate over space
    auto voxelPosZ = voxelStartZ;
    auto voxelIndex = (voxelMinZ * grid.gridSize2) + (voxelMinY * grid.gridSize) + voxelMinX;
    for (uint32_t z = voxelMinZ; z <= voxelMaxZ; ++z, voxelPosZ += voxelStepZ)
    {
        auto voxelPosY = voxelStartY;
        auto voxelIndexSave = voxelIndex;
        for (uint32_t y = voxelMinY; y <= voxelMaxY; ++y, voxelPosY += voxelStepY)
        {
            auto writePos = voxels.typedData() + (voxelIndex / 64);
            auto writeMask = 1ULL << (voxelIndex % 64);

            auto voxelPosX = voxelStartX;
            for (uint32_t x = voxelMinX; x <= voxelMaxX; ++x, voxelPosX += voxelStepX)
            {
                Vector3 voxelPos(voxelPosX, voxelPosY, voxelPosZ);
                if (shape.contains(voxelPos))
                    *writePos |= writeMask;

                // advance mask
                writeMask <<= 1;
                if (writeMask == 0)
                {
                    writePos += 1;
                    writeMask = 1;
                }
            }

            voxelIndex += grid.gridSize;
        }

        voxelIndex = voxelIndexSave + grid.gridSize2;
    }
}

///----

END_BOOMER_NAMESPACE()
