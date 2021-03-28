/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

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

#pragma once

#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// canvas geometry rendering state
/// contains ALL informations necessary for rendering the object
struct ENGINE_CANVAS_API CanvasRenderStyle
{
    XForm2D xform;

    Vector2 base;
    Vector2 extent;

	float radius = 0.0f;
	float feather = 0.0f;
	float margin = 0.0f;

	CanvasImagePtr image;

	Color innerColor = Color::WHITE;
	Color outerColor = Color::WHITE;

    bool wrapU:1;
    bool wrapV:1;
    bool customUV:1;
	bool xformNeeded:1;
	bool attributesNeeded : 1;

	//--

	CanvasRenderStyle();
};

//--

// Supported render style: linear gradient, box gradient, radial gradient and image pattern.
// These can be used as paints for strokes and fills.

// Creates and returns a linear gradient. Parameters (sx,sy)-(ex,ey) specify the start and end coordinates
// of the linear gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_LinearGradient(const Vector2& start, const Vector2& end, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_LinearGradient(float sx, float sy, float ex, float ey, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_LinearGradienti(int sx, int sy, int ex, int ey, const Color& icol, const Color& ocol);

// Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
// drop shadows or highlights for boxes. Parameters (x,y) define the top-left corner of the rectangle,
// (w,h) define the size of the rectangle, r defines the corner radius, and f feather. Feather defines how blurry
// the border of the rectangle is. Parameter icol specifies the inner color and ocol the outer color of the gradient.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_BoxGradient(const Vector2& pos, float w, float h, float r, float f, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_BoxGradient(const Vector2& start, const Vector2& end, float r, float f, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_BoxGradient(float x, float y, float w, float h, float r, float f, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_BoxGradienti(int x, int y, int w, int h, int r, int f, const Color& icol, const Color& ocol);

// Creates and returns a radial gradient. Parameters (cx,cy) specify the center, inr and outr specify
// the inner and outer radius of the gradient, icol specifies the start color and ocol the end color.
// The gradient is transformed by the current transform when it is passed to fillPaint() or strokePaint().
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_RadialGradient(const Vector2& center, float inr, float outr, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_RadialGradient(float cx, float cy, float inr, float outr, const Color& icol, const Color& ocol);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_RadialGradienti(int cx, int cy, int inr, int outr, const Color& icol, const Color& ocol);

// image projection settings
struct ENGINE_CANVAS_API ImagePatternSettings
{
	CanvasImagePtr m_image;
    float m_angle = 0.0f; // rotation around pivot
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    float m_scaleX = 1.0f; // scaling of the image pattern
    float m_scaleY = 1.0f; // scaling of the image pattern
    float m_pivotX = 0.0f;
    float m_pivotY = 0.0f;
    bool m_wrapU = false;
    bool m_wrapV = false;
	bool m_customUV = false; // use UVs in the vertices, not the ones computed from style
    uint8_t m_alpha = 255;
	uint8_t m_margin = 0;

    INLINE ImagePatternSettings() {};
	INLINE ImagePatternSettings(const CanvasImage* image) : m_image(AddRef(image)) {};
    INLINE ImagePatternSettings(const CanvasImagePtr& imagePtr) : m_image(imagePtr) {};
    INLINE ImagePatternSettings& scale(float s) { m_scaleX = s; m_scaleY = s; return *this; }
    INLINE ImagePatternSettings& offset(float x, float y) { m_offsetX = x; m_offsetY = y;  return *this; }
    INLINE ImagePatternSettings& angle(float a) { m_angle = a;  return *this; }
    INLINE ImagePatternSettings& pivot(float x, float y) { m_pivotX = x; m_pivotY = y; return *this; }
    INLINE ImagePatternSettings& alpha(uint8_t alpha) { m_alpha = alpha; return *this; }
    INLINE ImagePatternSettings& wrap() { m_wrapU = m_wrapV = true; return *this; }
    INLINE ImagePatternSettings& clamp() { m_wrapU = m_wrapV = false; return *this; }
    INLINE ImagePatternSettings& wrapU(bool u = true) { m_wrapU = u; return *this; }
    INLINE ImagePatternSettings& wrapV(bool u = true) { m_wrapV = u; return *this; }
	INLINE ImagePatternSettings& customUV(bool flag = true) { m_customUV = flag; return *this; }
	INLINE ImagePatternSettings& image(const CanvasImage* img) { m_image = AddRef(img); return *this; }
	INLINE ImagePatternSettings& margin(uint8_t m) { m_margin = m; return *this; }

	XForm2D calcPixelToUVTransform() const;
};

// Creates and returns an image patter. Parameters (ox,oy) specify the left-top location of the image pattern,
// (ex,ey) the size of one image, angle rotation around the top-left corner, image is handle to the image to render.
// The gradient is transformed by the current transform when it is passed to nvgFillPaint() or nvgStrokePaint().
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_ImagePattern(const CanvasImage* image, const ImagePatternSettings& pattern);
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_ImagePattern(const ImagePatternSettings& pattern);

// Create a style with solid paint color
extern ENGINE_CANVAS_API CanvasRenderStyle CanvasStyle_SolidColor(const Color& color);

//--

END_BOOMER_NAMESPACE()
