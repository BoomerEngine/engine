/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

namespace rendering
{
    namespace test
    {

        ///---

        /// scene camera
        struct SceneCamera
        {
            base::Vector3 cameraPosition;
            base::Vector3 cameraTarget;
            float aspect = 1.0f;
            float fov = 90.0f;
            float zoom = 1.0f;

            SceneCamera();

            void calcMatrices(bool renderToTexture=false);

            base::Matrix m_WorldToCamera;
            base::Matrix m_CameraToScreen;
            base::Matrix m_ViewToScreen;
            base::Matrix m_WorldToScreen;

            base::Matrix m_ScreenToWorld;
            base::Matrix m_ScreenToView;
        };

        ///---

        /// global params for scene shading
        struct SceneGlobalParams
        {
            struct ConstData
            {
                base::Matrix WorldToScreen;
                base::Vector4 CameraPosition;
                base::Vector4 LightDirection;
                base::Vector4 LightColor;
                base::Vector4 AmbientColor;

                INLINE ConstData()
                {
                    WorldToScreen = base::Matrix::IDENTITY();
                    LightDirection = base::Vector3(1, 1, 1).normalized();
                    CameraPosition = base::Vector3(0, 0, 0);
                }
            };

            ConstantsView Consts;
        };

        ///---

        /// params for scene object
        struct SceneObjectParams
        {
            struct ConstsData
            {
                base::Matrix LocalToWorld;
                base::Vector2 UVScale;
                base::Vector2 Dummy;
                base::Vector4 DiffuseColor;
                base::Vector4 SpecularColor;

                INLINE ConstsData()
                {
                    LocalToWorld = base::Matrix::IDENTITY();
                    UVScale = base::Vector2(1, 1);
                    DiffuseColor = base::Vector4(0.5f, 0.5f, 0.5f, 1.0f);
                    SpecularColor = base::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
                }
            };

            ConstantsView Consts;
            ImageView Texture;
            
            INLINE SceneObjectParams()
            {
                Texture = ImageView::DefaultWhite();
            }
        };

        ///---

        /// simple scene as collection of objects
        class SimpleScene : public base::IReferencable
        {
        public:
            struct Object
            {
                SimpleRenderMeshPtr m_mesh;

                struct Params
                {
                    base::Matrix LocalToWorld;
                    base::Vector2 UVScale;
                    base::Vector4 DiffuseColor;
                    base::Vector4 SpecularColor;
                    ImageView Texture;

                    INLINE Params()
                    {
                        LocalToWorld = base::Matrix::IDENTITY();
                        UVScale = base::Vector2(1, 1);
                        DiffuseColor = base::Vector4(0.5f, 0.5f, 0.5f, 1.0f);
                        SpecularColor = base::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
                        Texture = ImageView::DefaultWhite();
                    }
                };

                Params m_params;
            };

            base::Array<Object> m_objects;

            base::Vector3 m_lightPosition;
            base::Vector4 m_lightColor;
            base::Vector4 m_ambientColor;

            //--

            SimpleScene();

            void draw(command::CommandWriter& cmd, const ShaderLibrary* func, const SceneCamera& camera);
        };

        typedef base::RefPtr<SimpleScene> SimpleScenePtr;

        class IRenderingTest;

        // create a simple platonic solids scene
        extern SimpleScenePtr CreatePlatonicScene(IRenderingTest& owner);

        // create a single teapot scene
        extern SimpleScenePtr CreateTeapotScene(IRenderingTest& owner);

    } // test
} // rendering