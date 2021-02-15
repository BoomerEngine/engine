/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#pragma once

namespace base
{
    namespace image
    {

		//--

		// region in image (extension of a rectangle to 3D space)
		struct BASE_IMAGE_API ImageRect
		{
			uint32_t offsetX = 0;
			uint32_t offsetY = 0;
			uint32_t offsetZ = 0;
			uint32_t sizeX = 0;
			uint32_t sizeY = 0;
			uint32_t sizeZ = 0;

			INLINE ImageRect() {};
			INLINE ImageRect(const ImageRect& other) = default;
			INLINE ImageRect& operator=(const ImageRect& other) = default;

			INLINE ImageRect(uint32_t offset, uint32_t size)
				: offsetX(offset)
				, sizeX(size)
				, sizeY(1)
				, sizeZ(1)
			{}

			INLINE ImageRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
				: offsetX(x)
				, offsetY(y)
				, sizeX(width)
				, sizeY(height)
				, sizeZ(1)
			{}

			INLINE ImageRect(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth)
				: offsetX(x)
				, offsetY(y)
				, offsetZ(z)
				, sizeX(width)
				, sizeY(height)
				, sizeZ(depth)
			{}

			INLINE ImageRect(const base::Rect& rect)
			{
				if (!rect.empty())
				{
					DEBUG_CHECK_RETURN(rect.min.x >= 0 && rect.min.y >= 0);
					offsetX = (uint32_t)rect.min.x;
					offsetY = (uint32_t)rect.min.y;
					sizeX = (uint32_t)rect.width();
					sizeY = (uint32_t)rect.height();
				}
			}

			INLINE operator bool() const { return sizeX && sizeY && sizeZ; }
			INLINE bool empty() const { return sizeX == 0 || sizeY == 0 || sizeZ == 0; }

			// down sample region (for next mip)
			ImageRect downsampled() const;

			// merge two regions into one
			ImageRect& merge(const ImageRect& other);

			// check if two regions touch
			bool touches(const ImageRect& rect) const;

			// check if region contains given pixel coordinates 
			bool contains(int x, int y = 0, int z = 0) const;

			// check if region lies within given image size
			bool within(uint32_t width = 1, uint32_t height = 1, uint32_t depth = 1) const;

			//--

			void print(base::IFormatStream & f) const;
		};

        //--

    }  // image
} // base

///--------------------------------------------------------------------------
