/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "canvasStyle.h"

#include "base/image/include/image.h"
#include "base/containers/include/crc.h"

// The Canvas class is copied from nanovg project by Mikko Mononen
// Adaptations were made to fit the rest of the source code in here

//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

BEGIN_BOOMER_NAMESPACE(base::canvas)

//--

RenderStyle::RenderStyle()
	: wrapU(0)
	, wrapV(0)
	, customUV(0)
	, xformNeeded(0)
	, attributesNeeded(0)
{
}

/*bool RenderStyle::operator==(const RenderStyle& other) const
{
    return (key == other.key) && (extent == other.extent) && (base == other.base) && (radius == other.radius) && 
        (wrapU == other.wrapU) && (wrapV == other.wrapV) && (customUV == other.customUV) &&
		(feather == other.feather) && (innerColor == other.innerColor) && (outerColor == other.outerColor) && (image == other.image);
        //(uvMin == other.uvMin) && (uvMax == other.uvMax);
}

bool RenderStyle::operator!=(const RenderStyle& other) const
{
    return !operator==(other);
}

uint64_t RenderStyle::ComputeKey(const RenderStyle& style)
{
	CRC64 crc;

	crc << style.radius;
	crc << style.feather;
	crc << style.innerColor.toNative();
	crc << style.outerColor.toNative();
	crc << style.image;

	if (style.image)
	{
		crc << style.wrapU;
		crc << style.wrapV;
		crc << style.customUV;
		crc << style.base.x;
		crc << style.base.x;
		crc << style.extent.y;
		crc << style.extent.y;
	}

	return crc;
}*/

//--
        
RenderStyle LinearGradienti(int sx, int sy, int ex, int ey, const Color& icol, const Color& ocol)
{
    return LinearGradient((float)sx, (float)sy, (float)ex, (float)ey, icol, ocol);
}

RenderStyle LinearGradient(float sx, float sy, float ex, float ey, const Color& icol, const Color& ocol)
{
    return LinearGradient(Vector2(sx, sy), Vector2(ex, ey), icol, ocol);
}

RenderStyle LinearGradient(const Vector2& start, const Vector2& end, const Color& icol, const Color& ocol)
{
    float large = 1e5;

    auto dx = end - start;
    auto dl = dx.length();
    auto dxdl = (dl > 0.001f) ? (dx / dl) : Vector2(0, 1);

    RenderStyle ret;
	ret.xformNeeded = true;
	ret.attributesNeeded = true;
    ret.xform.t[0] = dxdl.y; 
    ret.xform.t[1] = -dxdl.x;
    ret.xform.t[2] = dxdl.x;
    ret.xform.t[3] = dxdl.y;
    ret.xform.t[4] = start.x - dxdl.x*large;
    ret.xform.t[5] = start.y - dxdl.y*large;
    ret.xform = ret.xform.inverted();
    ret.extent.x = large;
    ret.extent.y = large + dl*0.5f;
    ret.radius = 0.0f;
    ret.feather = std::max(1.0f, dl);
    ret.innerColor = icol;
    ret.outerColor = ocol;           
    return ret;
}

RenderStyle BoxGradienti(int x, int y, int w, int h, int r, int f, const Color& icol, const Color& ocol)
{
    return BoxGradient((float)x, (float)y, (float)w, (float)h, (float)r, (float)f, icol, ocol);
}

RenderStyle BoxGradient(const Vector2& pos, float w, float h, float r, float f, const Color& icol, const Color& ocol)
{
    RenderStyle ret;
	ret.xformNeeded = true;
	ret.attributesNeeded = true;
	ret.extent = Vector2(w, h) * 0.5f;
    ret.xform = XForm2D::BuildTranslation(-(pos + ret.extent));
    ret.radius = r;
    ret.feather = std::max(1.0f, f);
    ret.innerColor = icol;
    ret.outerColor = ocol;
    return ret;
}

RenderStyle BoxGradient(const Vector2& start, const Vector2& end, float r, float f, const Color& icol, const Color& ocol)
{
    return BoxGradient(start, end.x - start.x, end.y - start.x, r, f, icol, ocol);
}

RenderStyle BoxGradient(float x, float y, float w, float h, float r, float f, const Color& icol, const Color& ocol)
{
    return BoxGradient(Vector2(x, y), w, h, r, f, icol, ocol);
}

RenderStyle RadialGradient(const Vector2& center, float inr, float outr, const Color& icol, const Color& ocol)
{
    auto r = (inr + outr) * 0.5f;
    auto f = (outr - inr);

    RenderStyle ret;
	ret.xformNeeded = true;
	ret.attributesNeeded = true;
	ret.xform = XForm2D::BuildTranslation(-center);
    ret.extent = Vector2(r, r);
    ret.radius = r;
    ret.feather = std::max(1.0f, f);
    ret.innerColor = icol;
    ret.outerColor = ocol;
	return ret;
}

RenderStyle RadialGradient(float cx, float cy, float inr, float outr, const Color& icol, const Color& ocol)
{
    return RadialGradient(Vector2(cx, cy), inr, outr, icol, ocol);
}

RenderStyle RadialGradienti(int cx, int cy, int inr, int outr, const Color& icol, const Color& ocol)
{
    return RadialGradient((float)cx, (float)cy, (float)inr, (float)outr, icol, ocol);
}

XForm2D ImagePatternSettings::calcPixelToUVTransform() const
{
	XForm2D ret;

	float ox = m_offsetX;
	float oy = m_offsetY;

	ret.identity();

	ret *= XForm2D::BuildTranslation(-ox, -oy);
	ret *= XForm2D::BuildScale(m_scaleX, m_scaleY);

	if (m_angle)
	{
		if (m_pivotX || m_pivotY)
		{
			// TODO: compute final params directly in one go
			ret *= XForm2D::BuildTranslation(-m_pivotX, -m_pivotY);
			ret *= XForm2D::BuildRotation(m_angle);
			ret *= XForm2D::BuildTranslation(m_pivotX, m_pivotY);
		}
		else
		{
			ret *= XForm2D::BuildRotation(m_angle);
		}
	}

	return ret;
}

RenderStyle ImagePattern(const ImagePatternSettings& pattern)
{
	if (!pattern.m_image)
		return SolidColor(Color::WHITE);

	RenderStyle ret;
	ret.attributesNeeded = false; // images are stored in vertices any way, don't need to have extra style for that
	ret.xformNeeded = true;
	ret.image = pattern.m_image;
	ret.innerColor = Color(255, 255, 255, pattern.m_alpha);
	ret.outerColor = Color(255, 255, 255, pattern.m_alpha);
	ret.base.x = 0.0f;
	ret.base.y = 0.0f;
	ret.extent.x = pattern.m_image.width;
	ret.extent.y = pattern.m_image.height;
	ret.wrapU = pattern.m_wrapU;
	ret.wrapV = pattern.m_wrapV;
	ret.customUV = pattern.m_customUV;
	ret.margin = pattern.m_margin;
	ret.xform = pattern.calcPixelToUVTransform();
	return ret;
}

RenderStyle ImagePattern(ImageEntry image, const ImagePatternSettings& pattern)
{
	ImagePatternSettings ret = pattern;
	ret.m_image = image;
	return ImagePattern(ret);
}        

RenderStyle SolidColor(const Color& color)
{
    RenderStyle ret;
    ret.radius = 0.0f;
    ret.feather = 1.0f;
    ret.outerColor = color;
    ret.innerColor = color;
    return ret;
}

//--

END_BOOMER_NAMESPACE(base::canvas)
