/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingBufferView.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {

        //---

        /// create buffer upload data from an array
        template< typename T >
        INLINE static SourceData CreateSourceData(const base::Array<T>& sourceData)
        {
            SourceData ret;
            ret.data = base::Buffer::Create(POOL_TEMP, sourceData.dataSize(), 1, sourceData.data());
            return ret;
        }

        /// create buffer upload data from an array
        template< typename T >
        INLINE static SourceData CreateSourceDataRaw(const T& sourceData)
        {
            SourceData ret;
            ret.data = base::Buffer::Create(POOL_TEMP, sizeof(sourceData), 1, &sourceData);
            return ret;
        }

        //---

        struct Simple2DVertex
        {
            base::Vector2 VertexPosition;
            base::Vector2 VertexUV;
            base::Color VertexColor;

            INLINE void set(float x, float y, float u, float v, base::Color _color)
            {
                VertexPosition = base::Vector2(x, y);
                VertexUV = base::Vector2(u, v);
                VertexColor = _color;
            }
        };

        struct Simple3DVertex
        {
            base::Vector3 VertexPosition;
            base::Vector2 VertexUV;
            base::Color VertexColor;

            INLINE Simple3DVertex()
            {}

            INLINE Simple3DVertex(float x, float y, float z, float u = 0.0f, float v = 0.0f, base::Color _color = base::Color::BLACK)
            {
                VertexPosition = base::Vector3(x, y, z);
                VertexUV = base::Vector2(u, v);
                VertexColor = _color;
            }

            INLINE void set(float x, float y, float z, float u=0.0f, float v=0.0f, base::Color _color = base::Color::BLACK)
            {
                VertexPosition = base::Vector3(x, y, z);
                VertexUV = base::Vector2(u, v);
                VertexColor = _color;
            }
        };

        struct Mesh3DVertex
        {
            base::Vector3 VertexPosition;
            base::Vector3 VertexNormal;
            base::Vector2 VertexUV;
            base::Color VertexColor;

            INLINE Mesh3DVertex()
                : VertexPosition(0,0,0)
                , VertexNormal(0,0,1)
                , VertexUV(0,0)
                , VertexColor(base::Color::WHITE)
            {}
        };

        //---

        // draw a quad in the screen space, uses the Simple3DVertex
        extern void DrawQuad(command::CommandWriter& cmd, const ShaderLibrary* func, float x, float y, float w, float h, float u0=0.0f, float v0 = 0.0f, float u1=1.0f, float v1 = 1.0f, base::Color color = base::Color::WHITE);

        // bind params for drawing a screen rect in specific part of screen
        extern void SetQuadParams(command::CommandWriter& cmd, const ImageView& rt, const base::Rect& rect);

        // bind params for drawing a screen rect relative to screen size
        extern void SetQuadParams(command::CommandWriter& cmd, float x, float y, float w, float h);

        //---

    } // test
} // rendering