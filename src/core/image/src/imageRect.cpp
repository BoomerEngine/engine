/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#include "build.h"
#pragma hdrstop

#include "imageRect.h"

BEGIN_BOOMER_NAMESPACE_EX(image)

//--

ImageRect ImageRect::downsampled() const
{
	ImageRect ret;

	if (!empty())
	{
		ret.offsetX = offsetX / 2;
		ret.offsetY = offsetY / 2;
		ret.offsetZ = offsetZ / 2;
		ret.sizeX = std::max<uint32_t>(1, sizeX / 2);
		ret.sizeY = std::max<uint32_t>(1, sizeY / 2);
		ret.sizeZ = std::max<uint32_t>(1, sizeZ / 2);
	}

	return ret;
}

ImageRect& ImageRect::merge(const ImageRect& other)
{
	if (other)
	{
		if (!empty())
		{
			auto maxX = std::max<uint32_t>(offsetX + sizeX, other.offsetX + other.sizeX);
			auto maxY = std::max<uint32_t>(offsetY + sizeY, other.offsetY + other.sizeY);
			auto maxZ = std::max<uint32_t>(offsetZ + sizeZ, other.offsetZ + other.sizeZ);

			offsetX = std::min<uint32_t>(offsetX, other.offsetX);
			offsetY = std::min<uint32_t>(offsetY, other.offsetY);
			offsetZ = std::min<uint32_t>(offsetZ, other.offsetZ);

			sizeX = maxX - offsetX;
			sizeY = maxY - offsetY;
			sizeZ = maxZ - offsetZ;
		}
		else
		{
			*this = other;
		}
	}

	return *this;
}

bool ImageRect::touches(const ImageRect& other) const
{
	auto maxX = offsetX + sizeX;
	auto maxY = offsetY + sizeY;
	auto maxZ = offsetZ + sizeZ;

	auto otherMaxX = other.offsetX + other.sizeX;
	auto otherMaxY = other.offsetY + other.sizeY;
	auto otherMaxZ = other.offsetZ + other.sizeZ;

	if (maxX <= other.offsetX || maxY <= other.offsetY || maxZ <= other.offsetZ)
		return false;

	if (otherMaxX <= offsetX || otherMaxY <= offsetY || otherMaxZ <= offsetZ)
		return false;

	return true;
}

bool ImageRect::contains(int x, int y, int z) const
{
	return (x >= offsetX && x < offsetX + sizeX) &&
		(y >= offsetY && y < offsetY + sizeY) &&
		(z >= offsetZ && z < offsetZ + sizeZ);
}

bool ImageRect::within(uint32_t width /*= 1*/, uint32_t height /*= 1*/, uint32_t depth /*= 1*/) const
{
	return (offsetX < width && offsetY < height && offsetZ < depth) &&
		(offsetX + sizeX <= width && offsetY + sizeY <= height && offsetZ + sizeZ <= depth);
}

//--

void ImageRect::print(IFormatStream& f) const
{
	if (offsetX == 0 && offsetY == 0 && offsetZ == 0)
		f.appendf("[{}x{}x{}]", sizeX, sizeY, sizeZ);
	else
		f.appendf("[{}x{}x{}] @ ({},{},{})", sizeX, sizeY, sizeZ, offsetX, offsetY, offsetZ);
}

//--

END_BOOMER_NAMESPACE_EX(image)
