/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#pragma once

namespace example
{
    namespace helper
    {

        struct BoxVertex
        {
            Vector3 pos;
            Color color;
        };

        static_assert(sizeof(BoxVertex) == 16, "Change size in shader as well");

        void BuildBoxFace(Vector3 base, Vector3 u, Vector3 v, Color color, Array<BoxVertex>& outVertices, Array<uint16_t>& outIndices)
        {
            auto baseVertex = (uint16_t)outVertices.size();

            auto* vb = outVertices.allocateUninitialized(4);
            vb[0].pos = base;
            vb[1].pos = base + u;
            vb[2].pos = base + u + v;
            vb[3].pos = base + v;
            vb[0].color = color;
            vb[1].color = color;
            vb[2].color = color;
            vb[3].color = color;

            auto* ib = outIndices.allocateUninitialized(6);
            ib[0] = baseVertex + 0;
            ib[1] = baseVertex + 1;
            ib[2] = baseVertex + 2;
            ib[3] = baseVertex + 0;
            ib[4] = baseVertex + 2;
            ib[5] = baseVertex + 3;
        }

        void BuildBox(float size, Array<BoxVertex>& outVertices, Array<uint16_t>& outIndices)
        {
            Vector3 x(size, 0, 0);
            Vector3 y(0, size, 0);
            Vector3 z(0, 0, size);

            const float half = size * 0.5f;
            Vector3 vmin(-half, -half, -half);
            Vector3 vmax(half, half, half);

            BuildBoxFace(vmin, x, y, Color(128, 128, 0), outVertices, outIndices);
            BuildBoxFace(vmin, z, x, Color(128, 0, 128), outVertices, outIndices);
            BuildBoxFace(vmin, y, z, Color(0, 128, 128), outVertices, outIndices);
            BuildBoxFace(vmax, -y, -x, Color(128, 128, 255), outVertices, outIndices);
            BuildBoxFace(vmax, -x, -z, Color(128, 255, 128), outVertices, outIndices);
            BuildBoxFace(vmax, -z, -y, Color(255, 128, 128), outVertices, outIndices);
        }

        struct CameraMatrices
        {
            Vector3 CameraPosition;
            Vector3 CameraDirection;

            Matrix WorldToCamera;
            Matrix CameraToScreen;
            Matrix ViewToScreen;
            Matrix WorldToScreen;
            Matrix ScreenToWorld;
            Matrix ScreenToView;

            void calculate(const Vector3& from, const Vector3& to, float fov, float aspect)
            {
                auto rotation = (to - from).toRotator().toQuat();

                static base::Matrix CameraToView(base::Vector4(0, 1, 0, 0), base::Vector4(0, 0, -1, 0), base::Vector4(1, 0, 0, 0), base::Vector4(0, 0, 0, 1));
                static const base::Matrix ViewToCamera = CameraToView.inverted();

                WorldToCamera = rotation.inverted().toMatrix();
                WorldToCamera.translation(-WorldToCamera.transformVector(from));

                ViewToScreen = base::Matrix::BuildPerspectiveFOV(fov, aspect, 0.01f, 20.0f);
                ScreenToView = ViewToScreen.inverted();

                CameraToScreen = CameraToView * ViewToScreen;

                WorldToScreen = WorldToCamera * CameraToScreen;
                ScreenToWorld = WorldToScreen.inverted();

                CameraPosition = from;
                CameraDirection = (to - from).normalized();
            }
        };

    } // helper
} // example

