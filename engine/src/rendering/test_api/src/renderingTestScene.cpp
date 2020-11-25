/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingTest.h"
#include "renderingTestScene.h"

#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
    namespace test
    {

        //--

        SceneCamera::SceneCamera()
            : cameraPosition(-10.0f, 1.0f, 3.0f)
            , cameraTarget(0.0f, 0.0f, 0.5f)
        {
            calcMatrices();
        }


        void SceneCamera::calcMatrices(bool renderToTexture /*= false*/)
        {
            auto position = cameraPosition;
            auto rotation = (cameraTarget - cameraPosition).toRotator().toQuat();

            static base::Matrix CameraToView(base::Vector4(0, 1, 0, 0), base::Vector4(0, 0, -1, 0), base::Vector4(1, 0, 0, 0), base::Vector4(0, 0, 0, 1));
            static base::Matrix CameraToViewYFlip(base::Vector4(0, 1, 0, 0), base::Vector4(0, 0, 1, 0), base::Vector4(1, 0, 0, 0), base::Vector4(0, 0, 0, 1));
            const base::Matrix ConvMatrix = renderToTexture ? CameraToViewYFlip : CameraToView;

            const base::Matrix ViewToCamera = ConvMatrix.inverted();

            m_WorldToCamera = rotation.inverted().toMatrix();
            m_WorldToCamera.translation(-m_WorldToCamera.transformVector(position));

            if (fov == 0.0f)
                m_ViewToScreen = base::Matrix::BuildOrtho(zoom * aspect, zoom, -50.0f, 50.0f);
            else
                m_ViewToScreen = base::Matrix::BuildPerspectiveFOV(fov, aspect, 0.01f, 20.0f);

            m_ScreenToView = m_ViewToScreen.inverted();
            m_CameraToScreen = ConvMatrix * m_ViewToScreen;

            m_WorldToScreen = m_WorldToCamera * m_CameraToScreen;
            m_ScreenToWorld = m_WorldToScreen.inverted();            
        }

        //--

        SimpleScene::SimpleScene()
            : m_lightPosition(1.0f, 1.0f, 1.0f)
            , m_lightColor(1.2f, 1.2f, 1.2f, 1.0f)
            , m_ambientColor(0.2f, 0.2f, 0.2f, 1.0f)
        {}

        void SimpleScene::draw(command::CommandWriter& cmd, const ShaderLibrary* func, const SceneCamera& cameraParams)
        {
            // setup global params
            SceneGlobalParams constsData;
            constsData.WorldToScreen = cameraParams.m_WorldToScreen.transposed();
            constsData.AmbientColor = m_ambientColor;
            constsData.LightColor = m_lightColor;
            constsData.LightDirection = m_lightPosition.normalized();
            constsData.CameraPosition = cameraParams.cameraPosition;

			DescriptorEntry params[1];
			params[0].constants(constsData);

            cmd.opBindDescriptor("SceneGlobalParams"_id, params);

            // draw objects
            for (auto& obj : m_objects)
            {
                if (obj.m_mesh)
                {
                    SceneObjectParams objectConstData;
                    objectConstData.UVScale = obj.m_params.UVScale;
                    objectConstData.DiffuseColor = obj.m_params.DiffuseColor;
                    objectConstData.LocalToWorld = obj.m_params.LocalToWorld.transposed();
                    objectConstData.SpecularColor = obj.m_params.SpecularColor;

					DescriptorEntry objectParams[2];
                    objectParams[0].view(obj.m_params.Texture);
					objectParams[1].constants(objectConstData);
                    cmd.opBindDescriptor("SceneObjectParams"_id, objectParams);

                    obj.m_mesh->drawMesh(cmd, func);
                }
            }
        }

        //--

        SimpleScenePtr CreatePlatonicScene(IRenderingTest& owner)
        {
            auto ret = base::RefNew<SimpleScene>();

            // plane as a base
            {
                MeshSetup meshSetup;
                meshSetup.m_swapYZ = true;

                auto mesh = owner.loadMesh("plane.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4::ONE();
					
					auto texture = owner.loadImage2D("checker_d.png", true);
					auto sampler = Globals().SamplerWrapTriLinear;
					obj.m_params.Texture = texture->createView(sampler);
                }
            }

            // cube
            {
                MeshSetup meshSetup;
                //meshSetup.m_computeFaceNormals = true;

                auto mesh = owner.loadMesh("cube.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(1.0f, 0.2f, 0.2f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, 20.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(0.0f, -3.0f, -mesh->m_bounds.min.z);
                }
            }

            // teapot
            {
                MeshSetup meshSetup;
                meshSetup.m_loadTransform = base::Matrix::BuildScale(3.0f / 1.0f);
                //meshSetup.m_loadTransform *= base::Matrix::BuildRotationX(-90.0f);
                auto mesh = owner.loadMesh("teapot.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(0.6f, 1.2f, 0.2f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, -30.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(0.0f, 0.0f, -mesh->m_bounds.min.z);
                }
            }

            // piramid
            {
                MeshSetup meshSetup;

                auto mesh = owner.loadMesh("tetrahedron.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(1.3f, 1.2f, 0.2f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, 80.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(0.0f, 2.4f, -mesh->m_bounds.min.z);
                }
            }

            // octa
            {
                MeshSetup meshSetup;

                auto mesh = owner.loadMesh("octahedron.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(0.9f, 0.2f, 1.2f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, 10.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(-2.0f, -2.0f, -mesh->m_bounds.min.z);
                }
            }

            // dodeca
            {
                MeshSetup meshSetup;

                auto mesh = owner.loadMesh("dodecahedron.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(0.0f, 0.6f, 1.2f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, 10.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(-2.0f, 0.0f, -mesh->m_bounds.min.z);
                }
            }

            // icosahedron
            {
                MeshSetup meshSetup;

                auto mesh = owner.loadMesh("icosahedron.obj", meshSetup);
                if (mesh)
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(0.2f, 1.2f, 1.2f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, 10.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(-2.0f, 1.7f, -mesh->m_bounds.min.z);
                }
            }


            return ret;
        }

        //--

        SimpleScenePtr CreateTeapotScene(IRenderingTest& owner)
        {
            auto ret = base::RefNew<SimpleScene>();

            // plane as a base
            {
                MeshSetup meshSetup;
                meshSetup.m_swapYZ = true;

                if (auto mesh = owner.loadMesh("plane.obj", meshSetup))
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4::ONE();
                    
					auto texture = owner.loadImage2D("checker_d.png", true);
					auto sampler = Globals().SamplerWrapTriLinear;
					obj.m_params.Texture = texture->createView(sampler);
                }
            }

            // teapot
            {
                MeshSetup meshSetup;
                meshSetup.m_loadTransform = base::Matrix::BuildScale(6.0f / 1.0f);
                //meshSetup.m_loadTransform *= base::Matrix::BuildRotationX(-90.0f);

                if (auto mesh = owner.loadMesh("teapot.obj", meshSetup))
                {
                    auto& obj = ret->m_objects.emplaceBack();
                    obj.m_mesh = mesh;
                    obj.m_params.DiffuseColor = base::Vector4(0.8f, 1.2f, 0.8f, 1.0f);

                    obj.m_params.LocalToWorld = base::Matrix::BuildRotation(0.0f, -30.0f, 0.0f);
                    obj.m_params.LocalToWorld.translation(0.0f, 0.0f, -mesh->m_bounds.min.z);
                }
            }

            return ret;
        }

        //--


    } // test
} // rendering