/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "rendering/device/include/renderingDeviceGlobalObjects.h"

namespace rendering
{
    namespace test
    {

        ///---

        /// scene camera
        struct SceneCamera
        {
            base::Quat rotation;
            base::Vector3 position;
            float aspect = 1.0f;
            float fov = 90.0f;	
            float zoom = 1.0f;
			float farPlane = 50.0f;
			bool flipY = false;

            SceneCamera();

			void setupCubemap(const base::Vector3& pos, int face, float farPlane = 20.0f);
			void setupDirectionalShadowmap(const base::Vector3& target, const base::Vector3& source, float zoom = 10.0f, float farPlane = 50.0f);

            void calcMatrices();

            base::Matrix m_WorldToCamera;
            base::Matrix m_CameraToScreen;
            base::Matrix m_ViewToScreen;
            base::Matrix m_WorldToScreen;

            base::Matrix m_ScreenToWorld;
            base::Matrix m_ScreenToView;
        };

        ///---

		/// global params for scene shading
		struct SceneCameraParams
		{
			base::Matrix WorldToScreen;
			base::Vector4 CameraPosition;

			INLINE SceneCameraParams()
			{
				WorldToScreen = base::Matrix::IDENTITY();
				CameraPosition = base::Vector3(0, 0, 0);
			}
		};

        /// global params for scene shading
        struct SceneGlobalParams
        {            
			SceneCameraParams Cameras[6];
            base::Vector4 LightDirection;
            base::Vector4 LightColor;
            base::Vector4 AmbientColor;

			uint32_t NumCameras;
			uint32_t _Padding0;
			uint32_t _Padding1;
			uint32_t _Padding2;

            INLINE SceneGlobalParams()
            {
                LightDirection = base::Vector3(1, 1, 1).normalized();
				LightColor = base::Vector4(1, 1, 1, 1);
				AmbientColor = base::Vector4(0.2, 0.2, 0.2, 1);
				NumCameras = 1;
			}
        };

        ///---

        /// params for scene object
        struct SceneObjectParams
        {
            base::Matrix LocalToWorld;
            base::Vector2 UVScale;
            base::Vector2 Dummy;
            base::Vector4 DiffuseColor;
            base::Vector4 SpecularColor;

            INLINE SceneObjectParams()
            {
                LocalToWorld = base::Matrix::IDENTITY();
                UVScale = base::Vector2(1, 1);
                DiffuseColor = base::Vector4(0.5f, 0.5f, 0.5f, 1.0f);
                SpecularColor = base::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
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

				base::Vector3 m_initialPos;
				float m_initialYaw = 0.0f;

                struct Params
                {
                    base::Matrix LocalToWorld;
                    base::Vector2 UVScale;
                    base::Vector4 DiffuseColor;
                    base::Vector4 SpecularColor;
					ImageSampledViewPtr Texture;
					SamplerObjectPtr Sampler;

                    INLINE Params()
                    {
                        LocalToWorld = base::Matrix::IDENTITY();
                        UVScale = base::Vector2(1, 1);
                        DiffuseColor = base::Vector4(0.5f, 0.5f, 0.5f, 1.0f);
                        SpecularColor = base::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
						Texture = rendering::Globals().TextureWhite;
						Sampler = rendering::Globals().SamplerWrapTriLinear;
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

            void draw(command::CommandWriter& cmd, const GraphicsPipelineObject* func, const SceneCamera& camera);
			void draw(command::CommandWriter& cmd, const GraphicsPipelineObject* func, const SceneCamera* cameras, uint32_t numCameras);
        };

        typedef base::RefPtr<SimpleScene> SimpleScenePtr;

        class IRenderingTest;

        extern SimpleScenePtr CreatePlatonicScene(IRenderingTest& owner);
        extern SimpleScenePtr CreateTeapotScene(IRenderingTest& owner);
		extern SimpleScenePtr CreateManyTeapotsScene(IRenderingTest& owner, int gridSize);
		extern SimpleScenePtr CreateSphereScene(IRenderingTest& owner, const base::Vector3& initialPos, float size = 0.1f);

    } // test
} // rendering