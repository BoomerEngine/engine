/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

// The Canvas class is heavily based on nanovg project by Mikko Mononen
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

#include "build.h"
#include "canvasStyle.h"
#include "canvasGeometry.h"
#include "canvasGeometryBuilder.h"
#include "canvasGeometryPathCache.h"

#include "base/font/include/font.h"
#include "base/font/include/fontGlyph.h"
#include "base/font/include/fontGlyphBuffer.h"
#include "base/font/include/fontInputText.h"

namespace base
{
    namespace canvas
    {
        //--

        ConfigProperty<bool> cvAntiAliasEnabled("Engine.Canvas", "EnableAntiAlias", true);
        ConfigProperty<float> cvTessTollerance("Engine.Canvas", "TesselationTollerance", 0.25f);
        ConfigProperty<float> cvDistTollerance("Engine.Canvas", "DistanceTollerance", 0.01f);
        ConfigProperty<float> cvLineFringeWidth("Engine.Canvas", "FringeWidth", 1.0f);

        //--

        GeometryBuilder::GeometryBuilder(bool aantiAlias)
            : m_antiAlias(aantiAlias & cvAntiAliasEnabled.get())
            , m_antiAliasFringeWidth(cvLineFringeWidth.get())
            , m_distTollerance(cvDistTollerance.get())
            , m_tessTollerance(cvTessTollerance.get())
        {
            m_distTolleranceSquared = m_distTollerance * m_distTollerance;
            m_pathCache = MemNew(prv::PathCache, m_distTollerance, m_tessTollerance);
            reset();
        }

        GeometryBuilder::~GeometryBuilder()
        {
            MemDelete(m_pathCache);
        }

        GeometryBuilder::RenderState::RenderState()
        {
            fillStyle = SolidColor(Color::WHITE);
            strokeStyle = SolidColor(Color::BLACK);
        }

        void GeometryBuilder::reset()
        {
            // reset stacks
            m_styleStack.reset();
            m_transformStack.reset();

            // reset states
            m_style = RenderState();
            m_transform.identity();
            m_transformClass = XForm2DClass::Identity;

            // reset command buffer
            m_commands.reset();
            m_commands.reserve(256);
            m_prevPosition = Vector2::ZERO();

            // styles
            m_outStyles.reserve(32);
            m_outStylesMap.reserve(32);

            // reset output data
            m_outStylesMap.reset();
            m_outVertices.reset();
            m_outGlyphs.reset();
            m_outPaths.reset();
            m_outGroups.reset();
        }

        void GeometryBuilder::extractNoReset(Geometry& outGeometry) const
        {
            // nothing to extract
            if (m_outGroups.empty())
                return;

            // reset output
            outGeometry.reset();

            // copy vertices
            auto absoluteVertices  = outGeometry.m_vertices.allocateUninitialized(m_outVertices.size());
            memcpy(absoluteVertices, m_outVertices.typedData(), outGeometry.m_vertices.dataSize());

            // copy paths
            auto absolutePaths  = outGeometry.m_paths.allocateUninitialized(m_outPaths.size());
            memcpy(absolutePaths, m_outPaths.typedData(), outGeometry.m_paths.dataSize());

            // copy glyphs
            auto absoluteGlyphs  = outGeometry.m_glyphs.allocateUninitialized(m_outGlyphs.size());
            memcpy(absoluteGlyphs, m_outGlyphs.typedData(), outGeometry.m_glyphs.dataSize());

            // copy styles
            outGeometry.m_styles = m_outStyles;

            // fix up pointers
            for (uint32_t i = 0; i < m_outPaths.size(); ++i)
            {
                auto& outPath = absolutePaths[i];

                if (outPath.fillVertexCount > 0)
                    outPath.fillVertices = absoluteVertices + (uint64_t)outPath.fillVertices;

                if (outPath.strokeVertexCount > 0)
                    outPath.strokeVertices = absoluteVertices + (uint64_t)outPath.strokeVertices;
            }

            // copy and fix groups
            auto absoluteGroups  = outGeometry.m_groups.allocateUninitialized(m_outGroups.size());
            memzero(absoluteGroups, m_outGroups.dataSize());

            for (uint32_t i = 0; i < m_outGroups.size(); ++i)
            {
                auto& outGroup = absoluteGroups[i];
                outGroup = m_outGroups[i];

                if (outGroup.numPaths > 0)
                    outGroup.paths = absolutePaths + (uint64_t)outGroup.paths;
                else
                    outGroup.paths = nullptr;

                if (outGroup.numVertices > 0)
                    outGroup.vertices = absoluteVertices + (uint64_t)outGroup.vertices;
                else
                    outGroup.vertices = nullptr;

                if (outGroup.numGlyphs > 0)
                    outGroup.glyphs = absoluteGlyphs + (uint64_t)outGroup.glyphs;
                else
                    outGroup.glyphs = nullptr;
            }

            // update bounds
            outGeometry.calcBounds();

            // cache all referenced glyphs
            outGeometry.prepareGlyphsForRendering();
        }

        void GeometryBuilder::extract(Geometry& outGeometry)
        {
            // extract the geometry
            extractNoReset(outGeometry);

            // reset command buffer
            m_commands.clear();
            m_commands.reserve(256);

            // reset output data
            m_outVertices.reset();
            m_outGlyphs.reset();
            m_outPaths.reset();
            m_outGroups.reset();
            m_outStyles.reset();
            m_outStylesMap.reset();
        }

        void GeometryBuilder::compositeOperation(CompositeOperation op)
        {
            m_style.op = op;
        }

        void GeometryBuilder::pushState()
        {
            m_styleStack.pushBack(m_style);
        }

        void GeometryBuilder::popState()
        {
            if (!m_styleStack.empty())
            {
                m_style = m_styleStack.back();
                m_styleStack.popBack();
            }
        }

        void GeometryBuilder::resetState()
        {
            m_style = RenderState();
        }

        void GeometryBuilder::strokeColor(const Color& color)
        {
            m_style.strokeStyle = SolidColor(color);
            m_style.strokeStyleIndex = -1;
        }

        void GeometryBuilder::strokePaint(const RenderStyle& style)
        {
            m_style.strokeStyle = style;
            m_style.strokeStyleIndex = -1;
        }

        void GeometryBuilder::fillColor(const Color& color)
        {
            m_style.fillStyle = SolidColor(color);
            m_style.fillStyleIndex = -1;
        }

        void GeometryBuilder::fillPaint(const RenderStyle& style)
        {
            m_style.fillStyle = style;
            m_style.fillStyleIndex = -1;
        }

        void GeometryBuilder::miterLimit(float limit)
        {
            m_style.miterLimit = limit;
        }

        void GeometryBuilder::strokeWidth(float size)
        {
            m_style.strokeWidth = size;
        }

        void GeometryBuilder::lineCap(LineCap capStyle)
        {
            m_style.lineCap = capStyle;
        }

        void GeometryBuilder::lineJoin(LineJoin jointStyle)
        {
            m_style.lineJoint = jointStyle;
        }

        void GeometryBuilder::globalAlpha(float alpha)
        {
            m_style.alpha = alpha;
        }

        void GeometryBuilder::resetTransform()
        {
            m_transform.identity();
            m_transformClass = XForm2DClass::Identity;
        }

        void GeometryBuilder::pushTransform()
        {
            m_transformStack.pushBack(m_transform);
        }

        void GeometryBuilder::popTransform()
        {
            if (!m_transformStack.empty())
            {
                m_transform = m_transformStack.back();
                m_transformClass = m_transform.classify();
                m_transformInvertedValid = false;
                m_transformStack.popBack();
            }
        }

        void GeometryBuilder::cacheInvertexTransform()
        {
            if (!m_transformInvertedValid)
            {
                if (m_transformClass == XForm2DClass::Identity)
                    m_transformInverted = m_transform.identity();
                else if (m_transformClass == XForm2DClass::HasTransform)
                    m_transformInverted = XForm2D::BuildTranslation(-m_transform.translation());
                else
                    m_transformInverted = m_transform.inverted();

                m_transformInvertedValid = true;
            }
        }

        void GeometryBuilder::transform(float a, float b, float c, float d, float e, float f)
        {
            m_transform = XForm2D(a, b, c, d, e, f) * m_transform;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::offset(float x, float y)
        {
            m_transform.t[4] += x;
            m_transform.t[5] += y;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::offseti(int x, int y)
        {
            m_transform.t[4] += (float)x;
            m_transform.t[5] += (float)y;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::translate(float x, float y)
        {
            m_transform = XForm2D::BuildTranslation(x,y) * m_transform;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::translatei(int x, int y)
        {
            translate((float)x, (float)y);
        }

        void GeometryBuilder::rotate(float angle)
        {
            m_transform = XForm2D::BuildRotation(angle) * m_transform;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::skewX(float angle)
        {
            m_transform = XForm2D::BuildSkewX(angle) * m_transform;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::skewY(float angle)
        {
            m_transform = XForm2D::BuildSkewY(angle) * m_transform;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::scale(float x, float y)
        {
            m_transform = XForm2D::BuildScale(x,y) * m_transform;
            m_transformClass = m_transform.classify();
            m_transformInvertedValid = false;
        }

        void GeometryBuilder::beginPath()
        {
            m_commands.clear();
        }


        uint32_t GeometryBuilder::cacheFillStyle()
        {
            if (m_style.fillStyleIndex == -1)
            {
                uint32_t index = 0;
                if (!m_outStylesMap.find(m_style.fillStyle, index))
                {
                    index = m_outStyles.size();
                    m_outStylesMap[m_style.fillStyle] = index;
                    m_outStyles.emplaceBack(m_style.fillStyle);
                }
                else
                {
                    DEBUG_CHECK(m_outStyles[index] == m_style.fillStyle);
                }
                
                m_style.fillStyleIndex = index;
            }

            return m_style.fillStyleIndex;
        }

        uint32_t GeometryBuilder::cacheStrokeStyle()
        {
            if (m_style.strokeStyleIndex == -1)
            {
                uint32_t index = 0;
                if (!m_outStylesMap.find(m_style.strokeStyle, index))
                {
                    index = m_outStyles.size();
                    m_outStyles.emplaceBack(m_style.strokeStyle);
                    m_outStylesMap[m_style.strokeStyle] = index;
                }
                else
                {
                    DEBUG_CHECK(m_outStyles[index] == m_style.strokeStyle);
                }

                m_style.strokeStyleIndex = index;
            }

            return m_style.strokeStyleIndex;
        }

        void GeometryBuilder::beginGroup(uint32_t styleIndex, RenderGroup::Type type)
        {
            RenderGroup newGroup;
            newGroup.type = type;
            newGroup.paths = (RenderPath*)(uint64_t)m_outPaths.size(); // an index, finalized during final copying
            newGroup.numPaths = 0;
            newGroup.vertices = (RenderVertex *)(uint64_t)m_outVertices.size(); // an index, finalized during final copying
            newGroup.numVertices = 0;
            newGroup.op = m_style.op;
            newGroup.styleIndex = styleIndex;
            m_outGroups.pushBack(newGroup);
        }

        void GeometryBuilder::appendCommands(const float* vals, uint32_t numVals)
        {
            // update the reference position
            auto cmd = (int)vals[0];
            if (cmd != CMD_CLOSE && cmd != CMD_WINDING)
            {
                m_prevPosition.x = vals[numVals - 2];
                m_prevPosition.y = vals[numVals - 1];
            }

            // write commands to buffer
            auto ptr  = m_commands.allocateUninitialized(numVals);
            memcpy(ptr, vals, sizeof(float) * numVals);

            // save some time if we don't have rotations
            if (0 == (m_transformClass & XForm2DClass::HasScaleRotation))
            {
                // transform points in the commands by the current transform
                // NOTE: we don't have rotation/scale so we can make it a little bit faster
                auto endPtr  = ptr + numVals;
                while (ptr < endPtr)
                {
                    auto cmd = (int)*ptr;
                    switch (cmd)
                    {
                    case CMD_LINETO:
                    case CMD_MOVETO:
                        ptr[1] += m_transform.t[4];
                        ptr[2] += m_transform.t[5];
                        ptr += 3;
                        break;

                    case CMD_BEZIERTO:
                        ptr[1] += m_transform.t[4];
                        ptr[2] += m_transform.t[5];
                        ptr[3] += m_transform.t[4];
                        ptr[4] += m_transform.t[5];
                        ptr[5] += m_transform.t[4];
                        ptr[6] += m_transform.t[5];
                        ptr += 7;
                        break;

                    case CMD_CLOSE:
                        ptr += 1;
                        break;

                    case CMD_WINDING:
                        ptr += 2;
                        break;

                    default:
                        ptr += 1;
                    }
                }
            }
            else
            {
                // transform points in the commands by the current transform
                auto endPtr  = ptr + numVals;
                while (ptr < endPtr)
                {
                    float temp;
                    auto cmd = (int)*ptr;
                    switch (cmd)
                    {
                        case CMD_LINETO:
                        case CMD_MOVETO:
                            temp = m_transform.transformX(ptr[1], ptr[2]);
                            ptr[2] = m_transform.transformY(ptr[1], ptr[2]);
                            ptr[1] = temp;
                            ptr += 3;
                            break;

                        case CMD_BEZIERTO:
                            temp = m_transform.transformX(ptr[1], ptr[2]);
                            ptr[2] = m_transform.transformY(ptr[1], ptr[2]);
                            ptr[1] = temp;
                            temp = m_transform.transformX(ptr[3], ptr[4]);
                            ptr[4] = m_transform.transformY(ptr[3], ptr[4]);
                            ptr[3] = temp;
                            temp = m_transform.transformX(ptr[5], ptr[6]);
                            ptr[6] = m_transform.transformY(ptr[5], ptr[6]);
                            ptr[5] = temp;
                            ptr += 7;
                            break;

                        case CMD_CLOSE:
                            ptr += 1;
                            break;

                        case CMD_WINDING:
                            ptr += 2;
                            break;

                        default:
                            ptr += 1;
                    }
                }
            }
        }

        void GeometryBuilder::moveToi(int x, int y)
        {
            moveTo((float)x, (float)y);
        }

        void GeometryBuilder::moveTo(float x, float y)
        {
            float vals[] = { CMD_MOVETO, x, y };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::moveTo(const Vector2& pos)
        {
            moveTo(pos.x, pos.y);
        }

        void GeometryBuilder::lineToi(int x, int y)
        {
            lineTo((float)x, (float)y);
        }

        void GeometryBuilder::lineTo(float x, float y)
        {
            float vals[] = { CMD_LINETO, x, y };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::lineTo(const Vector2& pos)
        {
            lineTo(pos.x, pos.y);
        }

        void GeometryBuilder::bezierToi(int c1x, int c1y, int c2x, int c2y, int x, int y)
        {
            bezierTo((float)c1x, (float)c1y, (float)c2x, (float)c2y, (float)x, (float)y);
        }

        void GeometryBuilder::bezierTo(const Vector2& c1, const Vector2& c2, const Vector2& pos)
        {
            bezierTo(c1.x, c1.y, c2.x, c2.y, pos.x, pos.y);
        }

        void GeometryBuilder::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
        {
            float vals[] = { CMD_BEZIERTO, c1x, c1y, c2x, c2y, x, y };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::quadTo(const Vector2& c, const Vector2& pos)
        {
            quadTo(c.x, c.y, pos.x, pos.y);
        }

        void GeometryBuilder::quadTo(float cx, float cy, float x, float y)
        {
            float x0 = m_prevPosition.x;
            float y0 = m_prevPosition.y;
            float vals[] = { CMD_BEZIERTO,
                x0 + 2.0f / 3.0f*(cx - x0), y0 + 2.0f / 3.0f*(cy - y0),
                x + 2.0f / 3.0f*(cx - x), y + 2.0f / 3.0f*(cy - y),
                x, y };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        static INLINE int Near(float x1, float y1, float x2, float y2, float tol)
        {
            float dx = x2 - x1;
            float dy = y2 - y1;
            return dx*dx + dy*dy < tol*tol;
        }

        static INLINE float DistanceToSegment(float x, float y, float px, float py, float qx, float qy)
        {
            float pqx = qx - px;
            float pqy = qy - py;
            float dx = x - px;
            float dy = y - py;
            float d = pqx*pqx + pqy*pqy;
            float t = pqx*dx + pqy*dy;
            if (d > 0) t /= d;
            if (t < 0) t = 0;
            else if (t > 1) t = 1;

            {
                float dx = px + t*pqx - x;
                float dy = py + t*pqy - y;
                return dx*dx + dy*dy;
            }
        }

        static INLINE float Cross(float dx0, float dy0, float dx1, float dy1)
        {
            return dx1*dy0 - dx0*dy1;
        }

        void GeometryBuilder::arcTo(const Vector2& p1, const Vector2& p2, float radius)
        {
            arcTo(p1.x, p1.y, p2.x, p2.y, radius);
        }

        void GeometryBuilder::arcTo(float x1, float y1, float x2, float y2, float radius)
        {
            // we cannot arc as a first command
            if (m_commands.empty())
                return;

            float x0 = m_prevPosition.x;
            float y0 = m_prevPosition.y;

            // Handle degenerate cases.
            if (Near(x0, y0, x1, y1, m_distTollerance) ||
                Near(x1, y1, x2, y2, m_distTollerance) ||
                DistanceToSegment(x1, y1, x0, y0, x2, y2) < m_distTolleranceSquared ||
                radius < m_distTollerance)
            {
                lineTo(x1, y1);
                return;
            }

            // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
            Vector2 d0(x0 - x1, y0 - y1);
            Vector2 d1(x2 - x1, y2 - y1);
            d0.normalize();
            d1.normalize();

            float a = acos(d0.x*d1.x + d0.y*d1.y);
            float d = radius / tanf(a / 2.0f);

            if (d > 10000.0f) {
                lineTo(x1, y1);
                return;
            }

            if (Cross(d0.x, d0.y, d1.x, d1.y) > 0.0f)
            {
                auto cx = x1 + d0.x*d + d0.y*radius;
                auto cy = y1 + d0.y*d + -d0.x*radius;
                auto a0 = atan2(d0.x, -d0.y);
                auto a1 = atan2(-d1.x, d1.y);
                auto dir = Winding::CW;
                arc(cx, cy, radius, a0, a1, dir);
            }
            else
            {
                auto cx  = x1 + d0.x*d + -d0.y*radius;
                auto cy  = y1 + d0.y*d + d0.x*radius;
                auto a0  = atan2(-d0.x, d0.y);
                auto a1 = atan2(d1.x, -d1.y);
                auto dir = Winding::CCW;
                arc(cx, cy, radius, a0, a1, dir);
            }
        }

        void GeometryBuilder::closePath()
        {
            float vals[] = { CMD_CLOSE };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::pathWinding(Winding dir)
        {
            float vals[] = { CMD_WINDING, (float)dir };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::arci(int cx, int cy, int r, float a0, float a1, Winding dir)
        {
            arc((float)cx, (float)cy, (float)r, a0, a1, dir);
        }

        void GeometryBuilder::arc(const Vector2& center, float r, float a0, float a1, Winding dir)
        {
            arc(center.x, center.y, r, a0, a1, dir);
        }

        void GeometryBuilder::arc(float cx, float cy, float r, float a0, float a1, Winding dir)
        {
            // Clamp angles
            auto da = a1 - a0;
            if (dir == Winding::CW)
            {
                if (fabs(da) >= TWOPI)
                    da = TWOPI;
                else
                    while (da < 0.0f) da += TWOPI;
            }
            else
            {
                if (fabs(da) >= TWOPI)
                    da = -TWOPI;
                else
                    while (da > 0.0f) da -= TWOPI;
            }


            auto move = m_commands.empty() ? CMD_MOVETO : CMD_LINETO;

            // Split arc into max 90 degree segments.
            uint32_t ndivs = std::max(1, std::min((int)std::round(std::abs(da) / HALFPI), 5));
            float hda = (da / (float)ndivs) / 2.0f;
            float kappa = std::abs(4.0f / 3.0f * (1.0f - std::cos(hda)) / std::sin(hda)) * (dir == Winding::CCW ? -1.0f : 1.0f);

            float px = 0.0f, py = 0.0f, ptanx = 0.0f, ptany = 0.0f;
            float vals[3 + 5 * 7];
            uint32_t nvals = 0;
            for (uint32_t i=0; i<=ndivs; i++)
            {
                float a = a0 + da * (i / (float)ndivs);
                float dx = std::cos(a);
                float dy = std::sin(a);
                float x = cx + dx*r;
                float y = cy + dy*r;
                float tanx = -dy*r*kappa;
                float tany = dx*r*kappa;

                if (i == 0)
                {
                    vals[nvals++] = (float)move;
                    vals[nvals++] = x;
                    vals[nvals++] = y;
                }
                else
                {
                    vals[nvals++] = CMD_BEZIERTO;
                    vals[nvals++] = px + ptanx;
                    vals[nvals++] = py + ptany;
                    vals[nvals++] = x - tanx;
                    vals[nvals++] = y - tany;
                    vals[nvals++] = x;
                    vals[nvals++] = y;
                }

                px = x;
                py = y;
                ptanx = tanx;
                ptany = tany;
            }

            appendCommands(vals, nvals);
        }

        void GeometryBuilder::recti(int x, int y, int w, int h)
        {
            rect((float)x, (float)y, (float)w, (float)h);
        }

        void GeometryBuilder::rect(float x, float y, float w, float h)
        {
            float vals[] = {
                CMD_MOVETO, x,y,
                CMD_LINETO, x,y + h,
                CMD_LINETO, x + w,y + h,
                CMD_LINETO, x + w,y,
                CMD_CLOSE
            };
            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::rect(const Vector2& start, const Vector2& end)
        {
            rect(start.x, start.y, end.x - start.x, end.y - start.y);
        }

        void GeometryBuilder::rect(const Rect& r)
        {
            rect((float)r.min.x, (float)r.min.y, (float)r.width(), (float)r.height());
        }

        void GeometryBuilder::roundedRecti(int x, const int  y, const int  w, const int  h, int r)
        {
            roundedRect((float)x, (float)y, (float)w, (float)h, (float)r);
        }

        void GeometryBuilder::roundedRect(float x, float y, float w, float h, float r)
        {
            roundedRectVarying(x, y, w, h, r, r, r, r);
        }

        void GeometryBuilder::roundedRect(const Vector2& start, const Vector2& end, float r)
        {
            roundedRect(start.x, start.y, end.x - start.x, end.y - start.y, r);
        }

        void GeometryBuilder::roundedRect(const Rect& rect, float r)
        {
            roundedRect((float)rect.min.x, (float)rect.min.y, (float)rect.width(), (float)rect.height(), r);
        }

        void GeometryBuilder::roundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
        {
            // degenerate case
            if (radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f)
            {
                rect(x, y, w, h);
                return;
            }

            float halfw = fabs(w)*0.5f;
            float halfh = fabs(h)*0.5f;
            float signW = w < 0.0f ? -1.0f : 1.0f;
            float signH = h < 0.0f ? -1.0f : 1.0f;
            float rxBL = std::min(radBottomLeft, halfw) * signW;
            float ryBL = std::min(radBottomLeft, halfh) * signH;
            float rxBR = std::min(radBottomRight, halfw) * signW;
            float ryBR = std::min(radBottomRight, halfh) * signH;
            float rxTR = std::min(radTopRight, halfw) * signW;
            float ryTR = std::min(radTopRight, halfh) * signH;
            float rxTL = std::min(radTopLeft, halfw) * signW;
            float ryTL = std::min(radTopLeft, halfh) * signH;

            float kappa90 = 0.5522847493f; // Length proportional to radius of a cubic bezier handle for 90deg arcs.

            float vals[] =
            {
                CMD_MOVETO, x, y + ryTL,
                CMD_LINETO, x, y + h - ryBL,
                CMD_BEZIERTO, x, y + h - ryBL*(1 - kappa90), x + rxBL*(1 - kappa90), y + h, x + rxBL, y + h,
                CMD_LINETO, x + w - rxBR, y + h,
                CMD_BEZIERTO, x + w - rxBR*(1 - kappa90), y + h, x + w, y + h - ryBR*(1 - kappa90), x + w, y + h - ryBR,
                CMD_LINETO, x + w, y + ryTR,
                CMD_BEZIERTO, x + w, y + ryTR*(1 - kappa90), x + w - rxTR*(1 - kappa90), y, x + w - rxTR, y,
                CMD_LINETO, x + rxTL, y,
                CMD_BEZIERTO, x + rxTL*(1 - kappa90), y, x, y + ryTL*(1 - kappa90), x, y + ryTL,
                CMD_CLOSE
            };

            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::ellipsei(int cx, int cy, int rx, int ry)
        {
            ellipse((float)cx, (float)cy, (float)rx, (float)ry);
        }

        void GeometryBuilder::ellipse(float cx, float cy, float rx, float ry)
        {
            float kappa90 = 0.5522847493f; // Length proportional to radius of a cubic bezier handle for 90deg arcs.

            float vals[] = {
                CMD_MOVETO, cx - rx, cy,
                CMD_BEZIERTO, cx - rx, cy + ry*kappa90, cx - rx*kappa90, cy + ry, cx, cy + ry,
                CMD_BEZIERTO, cx + rx*kappa90, cy + ry, cx + rx, cy + ry*kappa90, cx + rx, cy,
                CMD_BEZIERTO, cx + rx, cy - ry*kappa90, cx + rx*kappa90, cy - ry, cx, cy - ry,
                CMD_BEZIERTO, cx - rx*kappa90, cy - ry, cx - rx, cy - ry*kappa90, cx - rx, cy,
                CMD_CLOSE
            };

            appendCommands(vals, ARRAY_COUNT(vals));
        }

        void GeometryBuilder::ellipse(const Vector2& center, float rx, float ry)
        {
            ellipse(center.x, center.y, rx, ry);
        }

        void GeometryBuilder::ellipse(const Vector2& tl, const Vector2& br)
        {
            float rx = (br.x - tl.x) * 0.5f;
            float ry = (br.y - tl.y) * 0.5f;
            float cx = tl.x + rx;
            float cy = tl.y + ry;
            ellipse(cx, cy, rx, ry);
        }

        void GeometryBuilder::ellipse(const Rect& rect)
        {
            float rx = (rect.max.x - rect.min.x) * 0.5f;
            float ry = (rect.max.y - rect.min.y) * 0.5f;
            float cx = rect.min.x + rx;
            float cy = rect.min.y + ry;
            ellipse(cx, cy, rx, ry);
        }

        void GeometryBuilder::circlei(int cx, int cy, int r)
        {
            circle((float)cx, (float)cy, (float)r);
        }

        void GeometryBuilder::circle(float cx, float cy, float r)
        {
            ellipse(cx, cy, r, r);
        }

        void GeometryBuilder::circle(const Vector2& center, float r)
        {
            ellipse(center.x, center.y, r, r);
        }

        ///---
                
        void GeometryBuilder::flattenPath(prv::PathCache& cache)
        {
            // Flatten
            uint32_t i = 0;
            auto cmdPtr  = m_commands.typedData();
            auto cmdEnd  = cmdPtr + m_commands.size();
            while (cmdPtr < cmdEnd)
            {
                uint32_t cmd = (uint32_t)cmdPtr[0];
                const Vector2* cmdPoints = (const Vector2*)(cmdPtr+1); // data embedded in the command stream

                switch (cmd)
                {
                case CMD_MOVETO:
                    m_pathCache->addPath();
                    m_pathCache->addPoint(cmdPoints[0], prv::PointTypeFlag::Corner);
                    cmdPtr += 3;
                    break;

                case CMD_LINETO:
                    m_pathCache->addPoint(cmdPoints[0], prv::PointTypeFlag::Corner);
                    cmdPtr += 3;
                    break;

                case CMD_BEZIERTO:
                    if (m_pathCache->lastPoint())
                        m_pathCache->addBezier(m_pathCache->lastPoint()->pos, cmdPoints[0], cmdPoints[1], cmdPoints[2], 0, prv::PointTypeFlag::Corner);
                    cmdPtr += 7;
                    break;

                case CMD_CLOSE:
                    m_pathCache->closePath();
                    cmdPtr += 1;
                    break;

                case CMD_WINDING:
                    m_pathCache->winding((Winding)(int)cmdPtr[1]);
                    cmdPtr += 2;
                    break;

                default:
                    cmdPtr += 1;
                }
            }
        }

        namespace helper
        {
            // helper class for writing output vertices
            class OutputVertexWriter : public base::NoCopy
            {
            public:
                INLINE OutputVertexWriter(Array<RenderVertex>& arr, uint32_t maxVertices)
                    : m_arr(arr)
                    , m_numWritten(0)
                    , m_boundsMin(FLT_MAX, FLT_MAX)
                    , m_boundsMax(-FLT_MAX, -FLT_MAX)
                {
                    m_startVertex = arr.allocateUninitialized(maxVertices);
                    m_curVertex = m_startVertex;
                    m_endVertex = m_startVertex + maxVertices;
                }

                INLINE uint32_t numWrittenVertices() const
                {
                    return m_numWritten;
                }

                INLINE ~OutputVertexWriter()
                {
                    auto baseVertex  = m_arr.typedData();
                    ASSERT(m_curVertex >= baseVertex);
                    auto actualWrittenCount = range_cast<uint32_t>(m_curVertex - baseVertex);
                    m_arr.resize(actualWrittenCount);
                }

                INLINE RenderVertex* currentPtr() const
                {
                    return m_curVertex;
                }

                INLINE RenderVertex* startVertexIndexMarker() const
                {
                    auto baseVertex  = m_arr.typedData();
                    return (RenderVertex*)(m_startVertex - baseVertex);
                }

                INLINE RenderVertex* startVertex() const
                {
                    return m_startVertex;
                }

                INLINE void add(float x, float y, float u=0.5f, float v=1.0f)
                {
                    ASSERT(m_curVertex < m_endVertex);
                    m_curVertex->pos.x = x;
                    m_curVertex->pos.y = y;
                    m_curVertex->uv.x = u;
                    m_curVertex->uv.y = v;
                    m_boundsMin = Min(m_boundsMin, m_curVertex->pos);
                    m_boundsMax = Max(m_boundsMax, m_curVertex->pos);
                    ++m_curVertex;
                    ++m_numWritten;
                }

                INLINE uint32_t finishAndGetCount()
                {
                    auto count = range_cast<uint32_t>(m_curVertex - m_startVertex);
                    m_startVertex = m_curVertex;
                    //boundsMin = Vector2(FLT_MAX, FLT_MAX);
                    //boundsMax = Vector2(-FLT_MAX, -FLT_MAX);
                    return count;
                }

                INLINE const Vector2& boundsMin() const { return m_boundsMin; }
                INLINE const Vector2& boundsMax() const { return m_boundsMax; }

            private:
                RenderVertex* m_startVertex; // current batch
                RenderVertex* m_curVertex; // current vertex
                RenderVertex* m_endVertex; // absolute
                uint32_t m_numWritten;
                 
                Vector2 m_boundsMin;
                Vector2 m_boundsMax;

                Array<RenderVertex>& m_arr;
            };

            static INLINE void ChooseBevel(bool bevel, const prv::PathPoint& p0, const prv::PathPoint& p1, float w, Vector2& out0, Vector2& out1)
            {
                if (bevel)
                {
                    out0 = p1.pos + p0.d.prep() * w;
                    out1 = p1.pos + p1.d.prep() * w;
                }
                else
                {
                    out0 = p1.pos + p1.dm * w;
                    out1 = p1.pos + p1.dm * w;
                }
            }

            static void AddBevelJoin(OutputVertexWriter& vertexWriter, const prv::PathPoint& p0, const prv::PathPoint& p1, float lw, float rw, float lu, float ru, float fringe)
            {
                auto dl0 = p0.d.prep();
                auto dl1 = p1.d.prep();

                if (p1.flags.test(prv::PointTypeFlag::Left))
                {
                    Vector2 l0, l1;
                    ChooseBevel(p1.flags.test(prv::PointTypeFlag::InnerBevel), p0, p1, lw, l0, l1);

                    vertexWriter.add(l0.x, l0.y, lu, 1.0f);
                    vertexWriter.add(p1.pos.x - dl0.x * rw, p1.pos.y - dl0.y * rw, ru, 1.0f);

                    if (p1.flags.test(prv::PointTypeFlag::Bevel))
                    {
                        vertexWriter.add(l0.x, l0.y, lu, 1.0f);
                        vertexWriter.add(p1.pos.x - dl0.x*rw, p1.pos.y - dl0.y * rw, ru, 1);

                        vertexWriter.add(l1.x, l1.y, lu, 1.0f);
                        vertexWriter.add(p1.pos.x - dl1.x * rw, p1.pos.y - dl1.y * rw, ru, 1.0f);
                    }
                    else
                    {
                        auto r0 = p1.pos - p1.dm * rw;

                        vertexWriter.add(p1.pos.x, p1.pos.y, 0.5f, 1);
                        vertexWriter.add(p1.pos.x - dl0.x*rw, p1.pos.y - dl0.y * rw, ru, 1);

                        vertexWriter.add(r0.x, r0.y, ru, 1);
                        vertexWriter.add(r0.x, r0.y, ru, 1);

                        vertexWriter.add(p1.pos.x, p1.pos.y, 0.5f, 1);
                        vertexWriter.add(p1.pos.x - dl1.x *rw, p1.pos.y - dl1.y * rw, ru, 1);
                    }

                    vertexWriter.add(l1.x, l1.y, lu, 1);
                    vertexWriter.add(p1.pos.x - dl1.x*rw, p1.pos.y - dl1.y * rw, ru, 1);
                }
                else
                {
                    Vector2 r0, r1;
                    ChooseBevel(p1.flags.test(prv::PointTypeFlag::InnerBevel), p0, p1, -rw, r0, r1);

                    vertexWriter.add(p1.pos.x + dl0.x *lw, p1.pos.y + dl0.y * lw, lu, 1);
                    vertexWriter.add(r0.x, r0.y, ru, 1);

                    if (p1.flags.test(prv::PointTypeFlag::Bevel))
                    {
                        vertexWriter.add(p1.pos.x + dl0.x * lw, p1.pos.y + dl0.y*lw, lu, 1.0f);
                        vertexWriter.add(r0.x, r0.y, ru, 1.0f);

                        vertexWriter.add(p1.pos.x + dl1.x *lw, p1.pos.y + dl1.y * lw, lu, 1.0f);
                        vertexWriter.add(r1.x, r1.y, ru, 1.0f);
                    }
                    else
                    {
                        auto l0 = p1.pos + p1.dm*lw;

                        vertexWriter.add(p1.pos.x + dl0.x*lw, p1.pos.y + dl0.y * lw, lu, 1.0f);
                        vertexWriter.add(p1.pos.x, p1.pos.y, 0.5f, 1.0f);

                        vertexWriter.add(l0.x, l0.y, lu, 1.0f);
                        vertexWriter.add(l0.x, l0.y, lu, 1.0f);

                        vertexWriter.add(p1.pos.x + dl1.x*lw, p1.pos.y + dl1.y * lw, lu, 1.0f);
                        vertexWriter.add(p1.pos.x, p1.pos.y, 0.5f, 1.0f);
                    }

                    vertexWriter.add(p1.pos.x + dl1.x*lw, p1.pos.y + dl1.y * lw, lu, 1.0f);
                    vertexWriter.add(r1.x, r1.y, ru, 1.0f);
                }
            }

            static void AddRoundJoin(OutputVertexWriter& vertexWriter, const prv::PathPoint& p0, const prv::PathPoint& p1, float lw, float rw, float lu, float ru, uint32_t ncap, float fringe)
            {
                auto dl0 = p0.d.prep();
                auto dl1 = p1.d.prep();

                if (p1.flags.test(prv::PointTypeFlag::Left))
                {
                    Vector2 l0, l1;
                    ChooseBevel(p1.flags.test(prv::PointTypeFlag::InnerBevel), p0, p1, lw, l0, l1);

                    auto a0 = atan2(-dl0.y, -dl0.x);
                    auto a1 = atan2(-dl1.y, -dl1.x);
                    if (a1 > a0) a1 -= TWOPI;

                    vertexWriter.add(l0.x, l0.y, lu, 1.0f);
                    vertexWriter.add(p1.pos.x - dl0.x *rw, p1.pos.y - dl0.y * rw, ru, 1.0f);

                    auto count = std::clamp<int>((int)ceil(((a0 - a1) / PI) * ncap), 2, ncap);
                    for (int i = 0; i < count; i++)
                    {
                        auto u = i / (float)(count - 1);
                        float a = a0 + u * (a1 - a0);
                        float rx = p1.pos.x + cos(a) * rw;
                        float ry = p1.pos.y + sin(a) * rw;

                        vertexWriter.add(p1.pos.x, p1.pos.y, 0.5f, 1.0f);
                        vertexWriter.add(rx, ry, ru, 1.0f);
                    }

                    vertexWriter.add(l1.x, l1.y, lu, 1.0f);
                    vertexWriter.add(p1.pos.x - dl1.x*rw, p1.pos.y - dl1.y * rw, ru, 1.0f);
                }
                else
                {
                    Vector2 r0, r1;
                    ChooseBevel(p1.flags.test(prv::PointTypeFlag::InnerBevel), p0, p1, -rw, r0, r1);

                    auto a0 = atan2(dl0.y, dl0.x);
                    auto a1 = atan2(dl1.y, dl1.x);
                    if (a1 < a0) a1 += TWOPI;

                    vertexWriter.add(p1.pos.x + dl0.x*rw, p1.pos.y + dl0.y * rw, lu, 1.0f);
                    vertexWriter.add(r0.x, r0.y, ru, 1.0f);

                    auto count = std::clamp<int>((int)ceil(((a0 - a1) / PI) * ncap), 2, ncap);
                    for (int i = 0; i < count; i++)
                    {
                        auto u = i / (float)(count - 1);
                        float a = a0 + u * (a1 - a0);
                        float lx = p1.pos.x + cos(a) * lw;
                        float ly = p1.pos.y + sin(a) * lw;

                        vertexWriter.add(lx, ly, lu, 1.0f);
                        vertexWriter.add(p1.pos.x, p1.pos.y, 0.5f, 1.0f);
                    }

                    vertexWriter.add(p1.pos.x + dl1.x*rw, p1.pos.y + dl1.y * rw, lu, 1.0f);
                    vertexWriter.add(r1.x, r1.y, ru, 1.0f);
                }
            }

            static void AddButtCapStart(OutputVertexWriter& vertexWriter, const prv::PathPoint& point, const Vector2 dv, float w, float d, float aa)
            {
                auto p = point.pos - dv*d;
                auto dl = dv.prep();
                vertexWriter.add(p.x + (dl.x * w) - (dv.x * aa), p.y + (dl.y * w) - (dv.y * aa), 0, 0);
                vertexWriter.add(p.x - (dl.x * w) - (dv.x * aa), p.y - (dl.y * w) - (dv.y * aa), 0, 0);
                vertexWriter.add(p.x + (dl.x * w), p.y + dl.y * w, 0, 1);
                vertexWriter.add(p.x - (dl.x * w), p.y - dl.y * w, 1, 1);
            }

            static void AddButtCapEnd(OutputVertexWriter& vertexWriter, const prv::PathPoint& point, const Vector2 dv, float w, float d, float aa)
            {
                auto p = point.pos + dv*d;
                auto dl = dv.prep();
                vertexWriter.add(p.x + (dl.x * w), p.y + (dl.y * w), 0, 1);
                vertexWriter.add(p.x - (dl.x * w), p.y - (dl.y * w), 0, 1);
                vertexWriter.add(p.x + (dl.x * w) + (dv.x * aa), p.y + (dl.y * w) + (dv.y * aa), 0, 0);
                vertexWriter.add(p.x - (dl.x * w) + (dv.x * aa), p.y - (dl.y * w) + (dv.y * aa), 1, 0);
            }

            static void AddRoundCapStart(OutputVertexWriter& vertexWriter, const prv::PathPoint& point, const Vector2 dv, float w, uint32_t ncap, float aa)
            {
                auto p = point.pos;
                auto dl = dv.prep();
                for (uint32_t i=0; i<ncap; i++)
                {
                    auto a = i / (float)(ncap - 1) * PI;
                    auto ax = cos(a) * w;
                    auto ay = sin(a) * w;

                    auto px = p.x - dl.x*ax - dv.x*ay;
                    auto py = p.y - dl.y*ax - dv.y*ay;
                    vertexWriter.add(px,py, 0, 1);
                    vertexWriter.add(p.x, p.y, 0.5f, 1);
                }
                vertexWriter.add(p.x + dl.x *w, p.y + dl.y * w, 0, 1);
                vertexWriter.add(p.x - dl.x *w, p.y - dl.y * w, 0, 1);
            }

            static void AddRoundCapEnd(OutputVertexWriter& vertexWriter, const prv::PathPoint& point, const Vector2 dv, float w, uint32_t ncap, float aa)
            {
                auto p = point.pos;
                auto dl = dv.prep();

                vertexWriter.add(p.x + dl.x *w, p.y + dl.y * w, 0, 1);
                vertexWriter.add(p.x - dl.x *w, p.y - dl.y * w, 1, 1);
                for (uint32_t i = 0; i < ncap; i++)
                {
                    auto a = i / (float)(ncap - 1) * PI;
                    auto ax = cos(a) * w;
                    auto ay = sin(a) * w;

                    auto px = p.x - dl.x*ax + dv.x*ay;
                    auto py = p.y - dl.y*ax + dv.y*ay;
                    vertexWriter.add(p.x, p.y, 0.5f, 1);
                    vertexWriter.add(px, py, 0, 1);
                }
            }

            static INLINE float CalcAverageScale(const XForm2D& xform)
            {
                float sx = sqrtf(xform.t[0] * xform.t[0] + xform.t[2] * xform.t[2]);
                float sy = sqrtf(xform.t[1] * xform.t[1] + xform.t[3] * xform.t[3]);
                return (sx + sy) * 0.5f;
            }

            static INLINE uint32_t CalcCurveDivs(float r, float arc, float tol)
            {
                float da =acos(r / (r + tol)) * 2.0f;
                return (uint32_t)std::max<int>(2, (int)ceil(arc / da));
            }


        } // helper

        void GeometryBuilder::applyPaintXForm(uint32_t firstVertex, uint32_t numVertices, const RenderStyle& style)
        {
            // compute final paint UV, skip for solid color shapes
            if (style.xformNeeded)
            {
                // make sure inverse transform is up to date
                cacheInvertexTransform();

                //auto paintXform = m_transform.inverted() * style.xform.inverted();
                auto paintXform = m_transformInverted * style.xform;

                auto* writePtr = m_outVertices.typedData() + firstVertex;
                auto* writeEndPtr = writePtr + numVertices;
                while (writePtr < writeEndPtr)
                {
                    writePtr->paintUV.x = paintXform.transformX(writePtr->pos.x, writePtr->pos.y);
                    writePtr->paintUV.y = paintXform.transformY(writePtr->pos.x, writePtr->pos.y);
                    ++writePtr;
                }
            }
        }

        void GeometryBuilder::stroke()
        {
            // initial count of paths and vertices, used to calculate the total resources used in a render group
            auto firstPath = m_outPaths.size();
            auto firstVertex = m_outVertices.size();

            // compute the rendering scale, related to current transformation
            // strokes are affected by the scale setting
            float scaleFactor = helper::CalcAverageScale(m_transform);

            // compute and limit the stoke width
            auto strokeRenderStyleIndex = cacheStrokeStyle();

            // If the stroke width is less than pixel size, use alpha to emulate coverage.
            // Since coverage is area, scale by alpha*alpha.
            float alphaScale = 1.0f;
            float rawStrokeWidth = std::clamp<float>(m_style.strokeWidth * scaleFactor, 0.0f, 200.0f);
            if (rawStrokeWidth < m_antiAliasFringeWidth)
            {
                float alpha = std::clamp<float>(rawStrokeWidth / m_antiAliasFringeWidth, 0.0f, 1.0f);
                alphaScale = alpha * alpha;
                rawStrokeWidth = m_antiAliasFringeWidth;
            }

            // apply global alpha
            beginGroup(strokeRenderStyleIndex, RenderGroup::Type::Stoke);

            // convert the commands into list of points and path segments
            m_pathCache->reset();
            flattenPath(*m_pathCache);

            // calculate inner deltas and other data, preparing path to be used
            m_pathCache->computeDeltas();

            // calculate joints, note: the fringe width is half of the stroke width
            const float halfStrokeWidth = (rawStrokeWidth + (m_antiAlias ? m_antiAliasFringeWidth : 0.0f)) * 0.5f;
            m_pathCache->computeJoints(halfStrokeWidth, m_style.lineJoint, m_style.miterLimit);

            // allocate output vertices
            auto numVertices = m_pathCache->computeStrokeVertexCount(m_style.lineJoint, m_style.lineCap, halfStrokeWidth) * 2;
            helper::OutputVertexWriter vertexWriter(m_outVertices, numVertices);

            // compute required curve divisions
            auto numCapVerts = helper::CalcCurveDivs(halfStrokeWidth, PI, m_tessTollerance);

            // convert the generated path data into renderable path data
            for (auto& path : m_pathCache->paths)
            {
                auto srcPtr  = m_pathCache->points.typedData() + path.first;

                // create output path element in the generated geometry
                RenderPath renderPath;
                renderPath.fillVertices = nullptr;
                renderPath.fillVertexCount = 0;

                // looping or not
                const prv::PathPoint* p0 = path.closed ? (srcPtr + path.count - 1) : (srcPtr);
                const prv::PathPoint* p1 = path.closed ? (srcPtr) : (srcPtr + 1);
                int startIndex = path.closed ? 0 : 1;
                int endIndex = path.closed ? path.count : path.count-1;

                // add starting cap in case of not closed path
                if (!path.closed)
                {
                    auto d = (p1->pos - p0->pos).normalized();

                    if (m_style.lineCap == LineCap::Butt)
                        helper::AddButtCapStart(vertexWriter, *p0, d, halfStrokeWidth, -m_antiAliasFringeWidth*0.5f, m_antiAliasFringeWidth);
                    else if (m_style.lineCap == LineCap::Square)
                        helper::AddButtCapStart(vertexWriter, *p0, d, halfStrokeWidth, halfStrokeWidth - m_antiAliasFringeWidth, m_antiAliasFringeWidth);
                    else if (m_style.lineCap == LineCap::Round)
                        helper::AddRoundCapStart(vertexWriter, *p0, d, halfStrokeWidth, numCapVerts, m_antiAliasFringeWidth);
                }

                // add internal vertices
                for (int j=startIndex; j<endIndex; ++j, p0 = p1++)
                {
                    if ((p1->flags.test(prv::PointTypeFlag::Bevel)) || (p1->flags.test(prv::PointTypeFlag::InnerBevel)))
                    {
                        if (m_style.lineJoint == LineJoin::Round)
                            helper::AddRoundJoin(vertexWriter, *p0, *p1, halfStrokeWidth, halfStrokeWidth, 0.0f, 1.0f, numCapVerts, m_antiAliasFringeWidth);
                        else
                            helper::AddBevelJoin(vertexWriter, *p0, *p1, halfStrokeWidth, halfStrokeWidth, 0.0f, 1.0f, m_antiAliasFringeWidth);
                    }
                    else
                    {
                        vertexWriter.add(p1->pos.x + p1->dm.x * halfStrokeWidth, p1->pos.y + p1->dm.y * halfStrokeWidth, 0.0f, 1.0f);
                        vertexWriter.add(p1->pos.x - p1->dm.x * halfStrokeWidth, p1->pos.y - p1->dm.y * halfStrokeWidth, 1.0f, 1.0f);
                    }
                }

                // add final cap in case the path is not closed
                if (path.closed)
                {
                    // create loop
                    auto writtenVerts  = vertexWriter.startVertex();
                    vertexWriter.add(writtenVerts[0].pos.x, writtenVerts[0].pos.y, 0.0f, 1.0f);
                    vertexWriter.add(writtenVerts[1].pos.x, writtenVerts[1].pos.y, 1.0f, 1.0f);
                }
                else
                {
                    auto d = (p1->pos - p0->pos).normalized();

                    if (m_style.lineCap == LineCap::Butt)
                        helper::AddButtCapEnd(vertexWriter, *p1, d, halfStrokeWidth, -m_antiAliasFringeWidth*0.5f, m_antiAliasFringeWidth);
                    else if (m_style.lineCap == LineCap::Square)
                        helper::AddButtCapEnd(vertexWriter, *p1, d, halfStrokeWidth, halfStrokeWidth - m_antiAliasFringeWidth, m_antiAliasFringeWidth);
                    else if (m_style.lineCap == LineCap::Round)
                        helper::AddRoundCapEnd(vertexWriter, *p1, d, halfStrokeWidth, numCapVerts, m_antiAliasFringeWidth);
                }

                // finalize stroke part
                renderPath.strokeVertices = vertexWriter.startVertexIndexMarker();
                renderPath.strokeVertexCount = vertexWriter.finishAndGetCount();

                // emit the path
                m_outPaths.pushBack(renderPath);
            }

            // reclaim unused vertices
            ((BaseArray*)&m_outVertices)->changeSize(firstVertex + vertexWriter.numWrittenVertices());

            // calculate the paint UV for all emitted vertices
            applyPaintXForm(firstVertex, vertexWriter.numWrittenVertices(), m_style.strokeStyle);

            // update last group with proper path count
            m_outGroups.back().numPaths = m_outPaths.size() - firstPath;
            m_outGroups.back().numVertices = vertexWriter.numWrittenVertices();
            //m_outGroups.back().fringeWidth = m_device.fringeWidth;
            //m_outGroups.back().strokeWidth = strokeWidth;
            m_outGroups.back().vertexBoundsMin = vertexWriter.boundsMin();
            m_outGroups.back().vertexBoundsMax = vertexWriter.boundsMax();
        }

        void GeometryBuilder::fill()
        {
            // initial count of paths and vertices, used to calculate the total resources used in a render group
            auto firstPath = m_outPaths.size();
            auto firstVertex = m_outVertices.size();

            // start group with fill render settings
            auto renderStateIndex = cacheFillStyle();
            beginGroup(renderStateIndex, RenderGroup::Type::Fill);

            // convert the commands into list of points and path segments
            m_pathCache->reset();
            flattenPath(*m_pathCache);

            // calculate inner deltas and other data, preparing path to be used
            m_pathCache->computeDeltas();

            // calculate joints, some special magic for MSAA mode
            auto fringe = m_antiAlias ? m_antiAliasFringeWidth : 0.0f;
            auto hasFringe = fringe > 0.0f;
            m_pathCache->computeJoints(fringe, LineJoin::Miter, 2.4f);

            // allocate output vertices
            bool convex = m_pathCache->paths.size() == 1 && m_pathCache->paths[0].convex;
            auto numVertices = m_pathCache->computeFillVertexCount(hasFringe) + (convex ? 0 : 4);
            helper::OutputVertexWriter vertexWriter(m_outVertices, numVertices);

            // convert the generated path data into renderable path data
            for (auto& path : m_pathCache->paths)
            {
                auto srcPtr  = m_pathCache->points.typedData() + path.first;

                // create output path element in the generated geometry
                RenderPath renderPath;

                // Calculate shape vertices.
                auto woff = 0.5f * m_antiAliasFringeWidth;
                if (hasFringe)
                {
                    auto lastPoint  = srcPtr + path.count - 1;
                    auto curPoint  = srcPtr;
                    for (uint32_t j = 0; j < path.count; ++j, lastPoint = curPoint++)
                    {
                        if (curPoint->flags.test(prv::PointTypeFlag::Bevel))
                        {
                            auto dl0 = lastPoint->d.prep();
                            auto dl1 = curPoint->d.prep();
                            if (curPoint->flags.test(prv::PointTypeFlag::Left))
                            {
                                vertexWriter.add(curPoint->pos.x + curPoint->dm.x * woff, curPoint->pos.y + curPoint->dm.y * woff);
                            }
                            else
                            {
                                vertexWriter.add(curPoint->pos.x + dl0.x * woff, curPoint->pos.y + dl0.y * woff);
                                vertexWriter.add(curPoint->pos.x + dl1.x * woff, curPoint->pos.y + dl1.y * woff);
                            }
                        }
                        else
                        {
                            vertexWriter.add(curPoint->pos.x + curPoint->dm.x * woff, curPoint->pos.y + curPoint->dm.y * woff);
                        }
                    }
                }
                else // no fringe
                {
                    for (uint32_t j = 0; j < path.count; ++j)
                        vertexWriter.add(srcPtr[j].pos.x, srcPtr[j].pos.y);
                }

                // setup fill part
                renderPath.fillVertices = vertexWriter.startVertexIndexMarker();
                renderPath.fillVertexCount = vertexWriter.finishAndGetCount();

                // render additional stroke to cover for the fringe
                if (hasFringe)
                {
                    float lw = fringe + woff;
                    float rw = fringe - woff;
                    float lu = 0;
                    float ru = 1;

                    // Create only half a fringe for convex shapes so that
                    // the shape can be rendered without stenciling.
                    if (m_pathCache->isConvex())
                    {
                        lw = woff;  // This should generate the same vertex as fill inset above.
                        lu = 0.5f;  // Set outline fade at middle.
                    }

                    // Looping
                    auto lastPoint  = srcPtr + path.count - 1;
                    auto curPoint  = srcPtr;
                    for (uint32_t j = 0; j < path.count; ++j, lastPoint == curPoint++)
                    {
                        if (curPoint->flags.test(prv::PointTypeFlag::Bevel) || curPoint->flags.test(prv::PointTypeFlag::InnerBevel))
                        {
                            helper::AddBevelJoin(vertexWriter, *lastPoint, *curPoint, lw, rw, lu, ru, fringe);
                        }
                        else
                        {
                            vertexWriter.add(curPoint->pos.x + (curPoint->dm.x * lw), curPoint->pos.y + (curPoint->dm.y * lw), lu, 1.0f);
                            vertexWriter.add(curPoint->pos.x - (curPoint->dm.x * rw), curPoint->pos.y - (curPoint->dm.y * rw), ru, 1.0f);
                        }
                    }

                    // Loop it
                    vertexWriter.add(vertexWriter.startVertex()[0].pos.x, vertexWriter.startVertex()[0].pos.y, lu, 1.0f);
                    vertexWriter.add(vertexWriter.startVertex()[1].pos.x, vertexWriter.startVertex()[1].pos.y, ru, 1.0f);

                    // export stroke data
                    renderPath.strokeVertices = vertexWriter.startVertexIndexMarker();
                    renderPath.strokeVertexCount = vertexWriter.finishAndGetCount();
                }

                // emit the path
                m_outPaths.pushBack(renderPath);
            }

            // extra vertices for concave masking
            if (!convex)
            {
                const auto& vertexBoundsMin = vertexWriter.boundsMin();
                const auto& vertexBoundsMax = vertexWriter.boundsMax();

                // write the vertices for the concave masking
                vertexWriter.add(vertexBoundsMin.x, vertexBoundsMin.y, 0.0f, 0.0f);
                vertexWriter.add(vertexBoundsMax.x, vertexBoundsMin.y, 1.0f, 0.0f);
                vertexWriter.add(vertexBoundsMax.x, vertexBoundsMax.y, 1.0f, 1.0f);
                vertexWriter.add(vertexBoundsMin.x, vertexBoundsMax.y, 0.0f, 1.0f);
            }

            // reclaim unused vertices
            ((BaseArray*)&m_outVertices)->changeSize(firstVertex + vertexWriter.numWrittenVertices());

            // calculate the paint UV for all emitted vertices
            applyPaintXForm(firstVertex, vertexWriter.numWrittenVertices(), m_style.fillStyle);

            // update last group with proper path count
            m_outGroups.back().convex = convex;
            m_outGroups.back().numPaths = m_outPaths.size() - firstPath;
            m_outGroups.back().numVertices = vertexWriter.numWrittenVertices();
            m_outGroups.back().vertexBoundsMin = vertexWriter.boundsMin();
            m_outGroups.back().vertexBoundsMax = vertexWriter.boundsMax();
        }

        void GeometryBuilder::print(const font::Font* font, int fontSize, StringView<char> txt, int hcenter/* = -1*/, int vcenter /*= -1*/, bool bold /*= false*/)
        {
            if (font)
            {
                font::GlyphBuffer buffer;

                font::FontStyleParams styleParams;
                styleParams.size = fontSize;
                styleParams.bold = bold;

                font::FontAssemblyParams assemblyParams;
                if (hcenter == 0)
                    assemblyParams.horizontalAlignment = font::FontAlignmentHorizontal::Center;
                else if (hcenter == 1)
                    assemblyParams.horizontalAlignment = font::FontAlignmentHorizontal::Right;
                else 
                    assemblyParams.horizontalAlignment = font::FontAlignmentHorizontal::Left;

                if (vcenter == 0)
                    assemblyParams.verticalAlignment = font::FontAlignmentVertical::Middle;
                else if (vcenter == 1)
                    assemblyParams.verticalAlignment = font::FontAlignmentVertical::Bottom;
                else if (vcenter == 2)
                    assemblyParams.verticalAlignment = font::FontAlignmentVertical::Baseline;
                else
                    assemblyParams.verticalAlignment = font::FontAlignmentVertical::Top;
                
                font::FontInputText inputText(txt.data(), txt.length());
                font->renderText(styleParams, assemblyParams, inputText, buffer);

                print(buffer);
            }
        }

        void GeometryBuilder::print(const font::GlyphBuffer& glyphs)
        {
            return print(glyphs.glyphs(), glyphs.size(), sizeof(font::GlyphBufferEntry));
        }

        void GeometryBuilder::print(const void* glyphEntries, uint32_t numGlyphs, uint32_t dataStride)
        {
            // initial count of glyphs
            auto firstGlyph = m_outGlyphs.size();

            // start group with fill render settings
            auto renderState = cacheFillStyle();
            beginGroup(renderState, RenderGroup::Type::Glyphs);

            // allocate glyphs
            auto firstWriteGlyph  = m_outGlyphs.allocateUninitialized(numGlyphs);
            auto readPtr  = (const font::GlyphBufferEntry*)glyphEntries;
            Vector2 boundsMin(FLT_MAX, FLT_MAX);
            Vector2 boundsMax(-FLT_MAX, -FLT_MAX);
            uint32_t numGlyphsWritten = 0;
            for (uint32_t i = 0; i < numGlyphs; ++i, readPtr = base::OffsetPtr(readPtr, dataStride))
            {
                auto& srcGlyph = *readPtr;

                // do not render glyphs with no bitmaps
                if (!srcGlyph.glyph || !srcGlyph.glyph->bitmap())
                    continue;

                // compute positions of the vertices
                float sizeX = (float)srcGlyph.glyph->size().x;
                float sizeY = (float)srcGlyph.glyph->size().y;
                float offsetX = (float)srcGlyph.pos.x;
                float offsetY = (float)srcGlyph.pos.y;
                float endX = offsetX + sizeX;
                float endY = offsetY + sizeY;

                // transform, if needed
                firstWriteGlyph->glyph = srcGlyph.glyph;
                if (m_transformClass & XForm2DClass::HasScaleRotation)
                {
                    firstWriteGlyph->coords[0].x = m_transform.transformX(offsetX, offsetY);
                    firstWriteGlyph->coords[0].y = m_transform.transformY(offsetX, offsetY);
                    firstWriteGlyph->coords[1].x = m_transform.transformX(endX, offsetY);
                    firstWriteGlyph->coords[1].y = m_transform.transformY(endX, offsetY);
                    firstWriteGlyph->coords[2].x = m_transform.transformX(endX, endY);
                    firstWriteGlyph->coords[2].y = m_transform.transformY(endX, endY);
                    firstWriteGlyph->coords[3].x = m_transform.transformX(offsetX, endY);
                    firstWriteGlyph->coords[3].y = m_transform.transformY(offsetX, endY);
                }
                else
                {
                    firstWriteGlyph->coords[0].x = offsetX;
                    firstWriteGlyph->coords[0].y = offsetY;
                    firstWriteGlyph->coords[1].x = endX;
                    firstWriteGlyph->coords[1].y = offsetY;
                    firstWriteGlyph->coords[2].x = endX;
                    firstWriteGlyph->coords[2].y = endY;
                    firstWriteGlyph->coords[3].x = offsetX;
                    firstWriteGlyph->coords[3].y = endY;
                }

                firstWriteGlyph->page = 0;
                firstWriteGlyph->color = srcGlyph.color;
                firstWriteGlyph->uvMin = Vector2::ZERO();
                firstWriteGlyph->uvMax = Vector2::ZERO();

                // update bounds
                boundsMin = Min(boundsMin, firstWriteGlyph->coords[0]);
                boundsMax = Max(boundsMax, firstWriteGlyph->coords[2]);

                ++firstWriteGlyph;
                ++numGlyphsWritten;
            }

            // update last group with proper path count
            m_outGroups.back().numGlyphs = numGlyphsWritten;
            m_outGroups.back().glyphs = (RenderGlyph*)(uint64_t)firstGlyph;
            //m_outGroups.back().fringeWidth = m_device.fringeWidth;
            m_outGroups.back().vertexBoundsMin = boundsMin;
            m_outGroups.back().vertexBoundsMax = boundsMax;
        }

    } // canvas
} // base
