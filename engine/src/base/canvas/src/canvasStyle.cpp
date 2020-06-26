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

namespace base
{
    namespace canvas
    {
        //--

        bool RenderStyle::operator==(const RenderStyle& other) const
        {
            return (extent == other.extent) && (base == other.base) && (radius == other.radius) && 
                (wrapU == other.wrapU) && (wrapV == other.wrapV) && (customUV == other.customUV) &&
                (feather == other.feather) && (innerColor == other.innerColor) && (outerColor == other.outerColor) && (image == other.image) &&
                (uvMin == other.uvMin) && (uvMax == other.uvMax);
        }

        bool RenderStyle::operator!=(const RenderStyle& other) const
        {
            return !operator==(other);
        }

        void RenderStyle::recomputeHash()
        {
            CRC64 crc;

            crc << radius;
            crc << feather;
            crc << innerColor.toNative();
            crc << outerColor.toNative();

            if (image)
            {
                crc << wrapU;
                crc << wrapV;
                crc << customUV;

                crc.append(&base, sizeof(base));
                crc.append(&extent, sizeof(extent));
                crc.append(&uvMin, sizeof(uvMin));
                crc.append(&uvMax, sizeof(uvMax));

                crc << image->runtimeUniqueId();
            }
            
            hash = crc.crc();
        }

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
            ret.xform.t[0] = dxdl.y; 
            ret.xform.t[1] = -dxdl.x;
            ret.xform.t[2] = dxdl.x;
            ret.xform.t[3] = dxdl.y;
            ret.xform.t[4] = start.x - dxdl.x*large;
            ret.xform.t[5] = start.y - dxdl.y*large;
            ret.xform = ret.xform.inverted();
            ret.xformNeeded = true;
            ret.extent.x = large;
            ret.extent.y = large + dl*0.5f;
            ret.radius = 0.0f;
            ret.feather = std::max(1.0f, dl);
            ret.innerColor = icol;
            ret.outerColor = ocol;
            ret.recomputeHash();
            return ret;
        }

        RenderStyle BoxGradienti(int x, int y, int w, int h, int r, int f, const Color& icol, const Color& ocol)
        {
            return BoxGradient((float)x, (float)y, (float)w, (float)h, (float)r, (float)f, icol, ocol);
        }

        RenderStyle BoxGradient(const Vector2& pos, float w, float h, float r, float f, const Color& icol, const Color& ocol)
        {
            RenderStyle ret;
            ret.extent = Vector2(w, h) * 0.5f;
            ret.xform = XForm2D::BuildTranslation(-(pos + ret.extent));
            ret.xformNeeded = true;
            ret.radius = r;
            ret.feather = std::max(1.0f, f);
            ret.innerColor = icol;
            ret.outerColor = ocol;
            ret.recomputeHash();
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
            ret.xform = XForm2D::BuildTranslation(-center);
            ret.xformNeeded = true;
            ret.extent = Vector2(r, r);
            ret.radius = r;
            ret.feather = std::max(1.0f, f);
            ret.innerColor = icol;
            ret.outerColor = ocol;
            ret.recomputeHash();
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

        RenderStyle ImagePattern(const base::image::Image* image, const ImagePatternSettings& pattern)
        {
            if (!image)
                return SolidColor(Color::WHITE);

            RenderStyle ret;
            ret.image = image;
            ret.innerColor = Color(255, 255, 255, pattern.m_alpha);
            ret.outerColor = Color(255, 255, 255, pattern.m_alpha);

            float ox = pattern.m_offsetX;
            float oy = pattern.m_offsetY;

            // pixel->uv
            {
                ret.xformNeeded = true;

                ret.xform.identity();

                // TODO: compute final params directly in one go
                ret.xform *= XForm2D::BuildTranslation(-ox, -oy);
                ret.xform *= XForm2D::BuildScale(pattern.m_scaleX, pattern.m_scaleY);

                ret.xform *= XForm2D::BuildTranslation(-pattern.m_pivotX, -pattern.m_pivotY);
                ret.xform *= XForm2D::BuildRotation(pattern.m_angle);
                ret.xform *= XForm2D::BuildTranslation(pattern.m_pivotX, pattern.m_pivotY);                
            }

            // uv wrapping
            ret.wrapU = pattern.m_wrapU;
            ret.wrapV = pattern.m_wrapV;

            ret.extent.x = image->width();
            ret.extent.y = image->height();

            // uv calculation
            if (pattern.m_rect.empty())
            {
                ret.base.x = 0.0f;// 0.5f * image->invWidth();
                ret.base.y = 0.0f;//0.5f * image->invHeight();

                if (pattern.m_wrapU)
                {
                    ret.uvMin.x = 0.0f;
                    ret.uvMax.x = 1.0f;
                }
                else
                {
                    ret.uvMin.x = 0.5f * image->invWidth();
                    ret.uvMax.x = 1.0f - 0.5f * image->invWidth();
                }

                if (pattern.m_wrapV)
                {
                    ret.uvMin.y = 0.0f;
                    ret.uvMax.y = 1.0f;
                }
                else
                {
                    ret.uvMin.y = 0.5f * image->invHeight();
                    ret.uvMax.y = 1.0f - 0.5f * image->invHeight();
                }
            }
            else
            {
                ret.base.x = (pattern.m_rect.min.x + 0.5f) * image->invWidth();
                ret.base.y = (pattern.m_rect.min.y + 0.5f)* image->invHeight();
                
                //ret.extent.x = pattern.m_rect.width();// pattern.m_rect.width();
                //ret.extent.y = pattern.m_rect.height();// pattern.m_rect.height();
                ret.uvMin.x = (pattern.m_rect.min.x + 0.5f) * image->invWidth();
                ret.uvMin.y = (pattern.m_rect.min.y + 0.5f) * image->invHeight();
                ret.uvMax.x = (pattern.m_rect.max.x + 0.5f) * image->invWidth();
                ret.uvMax.y = (pattern.m_rect.max.y + 0.5f) * image->invHeight();

                /*ret.uvMin.x = (pattern.m_rect.min.x + 0.0f) * image->invWidth();
                ret.uvMin.y = (pattern.m_rect.min.y + 0.0f) * image->invHeight();
                ret.uvMax.x = (pattern.m_rect.max.x + 0.0f) * image->invWidth();
                ret.uvMax.y = (pattern.m_rect.max.y + 0.0f) * image->invHeight();*/
            }

            ret.recomputeHash();
            return ret;
        }        

        RenderStyle SolidColor(const Color& color)
        {
            RenderStyle ret;
            ret.radius = 0.0f;
            ret.feather = 1.0f;
            ret.outerColor = color;
            ret.innerColor = color;
            ret.recomputeHash();
            return ret;
        }

    } // canvas
} // base
