/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\voxels #]
***/

#include "build.h"
#include "voxels.h"

BEGIN_BOOMER_NAMESPACE(base)

///---

static void SetInertiaTensor(double xx, double yy, double zz, double xy, double xz, double yz, Matrix33& outTensor)
{
    outTensor.m[0][0] = xx;
    outTensor.m[0][1] = xy;
    outTensor.m[0][2] = xz;
    outTensor.m[1][0] = xy;
    outTensor.m[1][1] = yy;
    outTensor.m[1][2] = yz;
    outTensor.m[2][0] = xz;
    outTensor.m[2][1] = yz;
    outTensor.m[2][2] = zz;
}
        
///---

VoxelizedGridSettings::VoxelizedGridSettings(uint32_t gridSize, const Box& bounds)
    : gridSize(gridSize)
    , gridSize2(gridSize * gridSize)
    , bounds(bounds)
{
    gridOffset = -bounds.min;

    auto boundSize = std::max<float>(0.1f, (bounds.max - bounds.min).maxValue());
    gridScale = (float)gridSize / boundSize;
    gridInvScale = boundSize / (float)gridSize;

    voxelVolume = (double)gridInvScale;
    voxelVolume = voxelVolume * voxelVolume * voxelVolume;
}

Vector3 VoxelizedGridSettings::transformPointToVoxelSpace(const Vector3& src) const
{
    Vector3 ret;
    ret.x = (src.x + gridOffset.x) * gridScale;
    ret.y = (src.y + gridOffset.y) * gridScale;
    ret.z = (src.z + gridOffset.z) * gridScale;
    return ret;
}

Vector3 VoxelizedGridSettings::transformPointFromVoxelSpace(const Vector3& src) const
{
    Vector3 ret;
    ret.x = bounds.min.x + (src.x * gridInvScale);
    ret.y = bounds.min.y + (src.y * gridInvScale);
    ret.z = bounds.min.z + (src.z * gridInvScale);
    return ret;
}

///---

VoxelizedShape::VoxelizedShape(uint32_t gridSize)
{
    voxels.resize(gridSize * gridSize * gridSize / 64);
    clear();
}

void VoxelizedShape::clear()
{
    memset(voxels.data(), 0, voxels.dataSize());
}

void VoxelizedShape::merge(const VoxelizedShape& other)
{
    ASSERT(other.voxels.size() == voxels.size());
    auto ptr  = voxels.typedData();
    auto otherPtr  = other.voxels.typedData();
    auto otherEndPtr  = otherPtr + other.voxels.size();
    while (otherPtr < otherEndPtr)
        *ptr++ |= *otherPtr++;
}

void VoxelizedShape::intersect(const VoxelizedShape& other)
{
    ASSERT(other.voxels.size() == voxels.size());
    auto ptr  = voxels.typedData();
    auto otherPtr  = other.voxels.typedData();
    auto otherEndPtr  = otherPtr + other.voxels.size();
    while (otherPtr < otherEndPtr)
        *ptr++ &= *otherPtr++;
}

//--

struct VoxelPos : public base::NoCopy
{
    uint32_t x,y,z;

    uint32_t voxelIndex;
    uint32_t voxelIndexOfZChange;

    const uint32_t size;
    const uint32_t size2;

    INLINE VoxelPos(uint32_t gridSize)
        : size(gridSize)
        , size2(gridSize * gridSize)
    {
        x = 0;
        y = 0;
        z = 0;
        voxelIndex = 0;
        voxelIndexOfZChange = size2;
    }

    INLINE void step()
    {
        voxelIndex += 1;
        x += 1;

        if (x == size)
        {
            x = 0;
            y += 1;

            if (y == size)
            {
                y = 0;
                z += 1;
                voxelIndexOfZChange += size2;
            }
        }
    }

    void advanceTo(uint32_t newVoxelIndex)
    {
        auto dist = newVoxelIndex - voxelIndex;
        voxelIndex = newVoxelIndex;

        if (dist <= size)
        {
            x += dist;
            if (x > size)
            {
                x -= size;
                y += 1;
            }
        }
        else
        {
            x = voxelIndex % size;
            y = (voxelIndex / size) % size;
        }

        if (voxelIndex >= voxelIndexOfZChange)
        {
            voxelIndexOfZChange += size2;
            z += 1;

            if (voxelIndex >= voxelIndexOfZChange)
            {
                z = voxelIndex / size2;
                voxelIndexOfZChange = (z+1) * size2;
            }
        }
    }

    Vector3 minPos() const
    {
        return Vector3((float)x, (float)y, (float)z);
    }

    Vector3 centerPos() const
    {
        return Vector3((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f);
    }

    Vector3 maxPos() const
    {
        return Vector3((float)x + 1, (float)y + 1, (float)z + 1);
    }
};

#ifdef PLATFORM_MSVC
static INLINE uint32_t  __builtin_ctzll(uint64_t val)
{
	unsigned long ret = 0;
	_BitScanForward64(&ret, val);
	return ret;
}

static INLINE uint32_t __builtin_clzll(uint64_t val)
{
	unsigned long ret = 0;
	_BitScanReverse64(&ret, val);
	return 63 - ret;			
}
#endif

#if 0
void VoxelizedShape::debugSave(const VoxelizedGridSettings& grid, StringView debugFile) const
{
    FILE* f = fopen(debugFile.ansi_str().c_str(), "w");
    if (f != NULL)
    {
        fprintf(f, "g Voxels\n");

        base::Array<uint32_t> cubeRoots;

        uint32_t vertexIndex = 1;

        {
            auto numWords = voxels.size();
            auto ptr  = voxels.typedData();
            auto voxelIndex = 0U;
            VoxelPos voxelPosition(grid.gridSize);
            for (uint32_t i=0; i<numWords; ++i, ++ptr, voxelIndex += 64)
            {
                if (0 != *ptr)
                {
					auto firstVoxelIndex = __builtin_ctzll(*ptr);
					auto lastVoxelIndex = 63 - __builtin_clzll(*ptr);
                    voxelPosition.advanceTo(firstVoxelIndex + voxelIndex);

                    auto localVoxelMask = 1ULL << firstVoxelIndex;
                    auto localVoxelIndex = voxelIndex + firstVoxelIndex;
                    for (uint32_t j=firstVoxelIndex; j<=lastVoxelIndex; ++j, ++localVoxelIndex, localVoxelMask <<= 1, voxelPosition.step())
                    {
                        if (0 != (*ptr & localVoxelMask))
                        {
                            cubeRoots.pushBack(vertexIndex);
                            vertexIndex += 8;

                            auto boxMin = grid.transformPointFromVoxelSpace(voxelPosition.minPos());
                            auto boxMax = grid.transformPointFromVoxelSpace(voxelPosition.maxPos());

                            fprintf(f, "v %f %f %f\n", boxMin.x, boxMin.y, boxMin.z);
                            fprintf(f, "v %f %f %f\n", boxMax.x, boxMin.y, boxMin.z);
                            fprintf(f, "v %f %f %f\n", boxMin.x, boxMax.y, boxMin.z);
                            fprintf(f, "v %f %f %f\n", boxMax.x, boxMax.y, boxMin.z);
                            fprintf(f, "v %f %f %f\n", boxMin.x, boxMin.y, boxMax.z);
                            fprintf(f, "v %f %f %f\n", boxMax.x, boxMin.y, boxMax.z);
                            fprintf(f, "v %f %f %f\n", boxMin.x, boxMax.y, boxMax.z);
                            fprintf(f, "v %f %f %f\n", boxMax.x, boxMax.y, boxMax.z);
                        }
                    }
                }
            }
        }

        for (auto start : cubeRoots)
        {
            fprintf(f, "f %d %d %d\n", start+2, start+1, start+0);
            fprintf(f, "f %d %d %d\n", start+2, start+3, start+1);
            fprintf(f, "f %d %d %d\n", start+6, start+4, start+5);
            fprintf(f, "f %d %d %d\n", start+7, start+6, start+5);
            fprintf(f, "f %d %d %d\n", start+4, start+2, start+0);
            fprintf(f, "f %d %d %d\n", start+6, start+2, start+4);
            fprintf(f, "f %d %d %d\n", start+3, start+5, start+1);
            fprintf(f, "f %d %d %d\n", start+7, start+5, start+3);
            fprintf(f, "f %d %d %d\n", start+6, start+3, start+2);
            fprintf(f, "f %d %d %d\n", start+6, start+7, start+3);
            fprintf(f, "f %d %d %d\n", start+1, start+4, start+0);
            fprintf(f, "f %d %d %d\n", start+5, start+4, start+1);
        }

        fclose(f);
    }
}
#endif

bool VoxelizedShape::calculateCenter(const VoxelizedGridSettings& grid, Vector3& outCenter, float& outVolume) const
{
    auto centerX = 0ULL;
    auto centerY = 0ULL;
    auto centerZ = 0ULL;
    auto count = 0;

    {
        auto numWords = voxels.size();
        auto ptr  = voxels.typedData();
        auto voxelIndex = 0U;
        VoxelPos voxelPosition(grid.gridSize);
        for (uint32_t i=0; i<numWords; ++i, ++ptr, voxelIndex += 64)
        {
            if (0 != *ptr)
            {
                auto firstVoxelIndex = __builtin_ctzll(*ptr);
                auto lastVoxelIndex = 63 - __builtin_clzll(*ptr);

                voxelPosition.advanceTo(firstVoxelIndex + voxelIndex);

                auto localVoxelMask = 1ULL << firstVoxelIndex;
                auto localVoxelIndex = voxelIndex + firstVoxelIndex;
                for (uint32_t j=firstVoxelIndex; j<=lastVoxelIndex; ++j, ++localVoxelIndex, localVoxelMask <<= 1, voxelPosition.step())
                {
                    if (0 != (*ptr & localVoxelMask))
                    {
                        centerX += voxelPosition.x;
                        centerY += voxelPosition.y;
                        centerZ += voxelPosition.z;
                        count += 1;
                    }
                }
            }
        }
    }

    if (count == 0)
        return false;

    centerX += count / 2;
    centerY += count / 2;
    centerZ += count / 2;

    Vector3 voxelCenter;
    voxelCenter.x = (float)((double)centerX / (double)count);
    voxelCenter.y = (float)((double)centerY / (double)count);
    voxelCenter.z = (float)((double)centerZ / (double)count);

    outCenter = grid.transformPointFromVoxelSpace(voxelCenter);
    outVolume = (float)((double)count * grid.voxelVolume);
    return true;
}

bool VoxelizedShape::calculateInertiaTensor(const VoxelizedGridSettings& grid, const Vector3& comPosition, Matrix33& outTensor) const
{
    double xx = 0.0;
    double yy = 0.0;
    double zz = 0.0;
    double xy = 0.0;
    double xz = 0.0;
    double yz = 0.0;

    auto count = 0;

    {
        auto numWords = voxels.size();
        auto ptr  = voxels.typedData();
        auto voxelIndex = 0U;
        VoxelPos voxelPosition(grid.gridSize);
        for (uint32_t i=0; i<numWords; ++i, ++ptr, voxelIndex += 64)
        {
            if (0 != *ptr)
            {
                auto firstVoxelIndex = __builtin_ctzll(*ptr);
                auto lastVoxelIndex = 63 - __builtin_clzll(*ptr);

                voxelPosition.advanceTo(firstVoxelIndex + voxelIndex);

                auto localVoxelMask = 1ULL << firstVoxelIndex;
                auto localVoxelIndex = voxelIndex + firstVoxelIndex;
                for (uint32_t j=firstVoxelIndex; j<=lastVoxelIndex; ++j, ++localVoxelIndex, localVoxelMask <<= 1, voxelPosition.step())
                {
                    if (0 != (*ptr & localVoxelMask))
                    {
                        auto bodyPos = grid.transformPointFromVoxelSpace(voxelPosition.centerPos());
                        auto d = bodyPos - comPosition;

                        float x2 = d.x*d.x;
                        float y2 = d.y*d.y;
                        float z2 = d.z*d.z;
                        xx += y2 + z2;
                        yy += x2 + z2;
                        zz += x2 + y2;
                        xy += d.x*d.y;
                        xz += d.x*d.z;
                        yz += d.y*d.z;
                        count += 1;
                    }
                }
            }
        }
    }

    if (count == 0)
        return false;

    // Normalize
    auto invCount = 1.0 / (double)count;
    xx *= invCount;
    yy *= invCount;
    zz *= invCount;
    xy *= invCount;
    xz *= invCount;
    yz *= invCount;

    // Compose matrix
    SetInertiaTensor(xx, yy, zz, xy, xz, yz, outTensor);
    return true;
}

END_BOOMER_NAMESPACE(base)