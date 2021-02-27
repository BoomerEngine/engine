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

#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

//--

SceneCamera::SceneCamera()
    : position(-10.0f, 1.0f, 3.0f)
{
    calcMatrices();
}

static const Angles CUBE_ROTATIONS[6] = {
	Angles(0,0,-90), // X+
	Angles(0,180,90), // X-
	Angles(0,90,180), // Y+
	Angles(0,-90,0), // Y-
	Angles(-90,0,-90), // Z+
	Angles(90,0,-90), // Z-
};

void SceneCamera::setupCubemap(const Vector3& pos, int face, float limit /*= 20.0f*/)
{
	rotation = CUBE_ROTATIONS[face].toQuat();
	position = pos;
	aspect = 1.0f;
	zoom = 1.0f;
	fov = 90.0f;
	farPlane = limit;
}

void SceneCamera::setupDirectionalShadowmap(const Vector3& target, const Vector3& source, float _zoom /*= 10.0f*/, float _farPlane /*= 50.0f*/)
{
	aspect = 1.0f;
	fov = 0.0f;
	zoom = _zoom;
	farPlane = _farPlane;
	position = target;
	rotation = (target - source).toRotator().toQuat();
}

void SceneCamera::calcMatrices(bool flipY)
{
    //auto position = (fov == 0.0f) ? cameraTarget : cameraPosition;
    //auto rotation = (cameraTarget - cameraPosition).toRotator().toQuat();

    static Matrix CameraToView(Vector4(0, 1, 0, 0), Vector4(0, 0, -1, 0), Vector4(1, 0, 0, 0), Vector4(0, 0, 0, 1));
    static Matrix CameraToViewYFlip(Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0), Vector4(1, 0, 0, 0), Vector4(0, 0, 0, 1));
    const Matrix& ConvMatrix = flipY ? CameraToViewYFlip : CameraToView;

    const Matrix ViewToCamera = ConvMatrix.inverted();

    m_WorldToCamera = rotation.inverted().toMatrix();
    m_WorldToCamera.translation(-m_WorldToCamera.transformVector(position));

    if (fov == 0.0f)
        m_ViewToScreen = Matrix::BuildOrtho(zoom * aspect, zoom, -farPlane, farPlane);
    else
        m_ViewToScreen = Matrix::BuildPerspectiveFOV(fov, aspect, 0.01f, farPlane);

    m_ScreenToView = m_ViewToScreen.inverted();
    m_CameraToScreen = ConvMatrix * m_ViewToScreen;

    m_WorldToScreen = m_WorldToCamera * m_CameraToScreen;
    m_ScreenToWorld = m_WorldToScreen.inverted();   

	if (fov == 0.0f)
	{
		auto screenPosCamera = m_WorldToScreen.transformPoint(position);
		auto cameraZDist = std::fabsf(screenPosCamera.z - 0.5f);
		ASSERT(cameraZDist < 0.01f);
	}
}

//--

SimpleScene::SimpleScene()
    : m_lightPosition(1.0f, 1.0f, 1.0f)
    , m_lightColor(1.2f, 1.2f, 1.2f, 1.0f)
    , m_ambientColor(0.2f, 0.2f, 0.2f, 1.0f)
{}

void SimpleScene::draw(CommandWriter& cmd, const GraphicsPipelineObject* func, const SceneCamera& cameraParams)
{
	draw(cmd, func, &cameraParams, 1);
}

void SimpleScene::draw(CommandWriter& cmd, const GraphicsPipelineObject* func, const SceneCamera* cameras, uint32_t numCameras)
{
	SceneGlobalParams constsData;
	memzero(&constsData, sizeof(constsData));

    // setup global params
    constsData.AmbientColor = m_ambientColor;
    constsData.LightColor = m_lightColor;
    constsData.LightDirection = m_lightPosition.normalized();
	constsData.NumCameras = numCameras;

	// setup cameras
	for (uint32_t i = 0; i < numCameras; ++i)
	{
		constsData.Cameras[i].WorldToScreen = cameras[i].m_WorldToScreen.transposed();
		constsData.Cameras[i].CameraPosition = cameras[i].position;
	}

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
			objectParams[0].constants(objectConstData);
			objectParams[1].view(obj.m_params.Texture);
			cmd.opBindDescriptor("SceneObjectParams"_id, objectParams);

            obj.m_mesh->drawMesh(cmd, func);
        }
    }
}

//--

SimpleScenePtr CreatePlatonicScene(IRenderingTest& owner)
{
    auto ret = RefNew<SimpleScene>();

    // plane as a base
    {
        MeshSetup meshSetup;
        meshSetup.m_swapYZ = true;

        auto mesh = owner.loadMesh("plane.obj", meshSetup);
        if (mesh)
        {
            auto& obj = ret->m_objects.emplaceBack();
            obj.m_mesh = mesh;
            obj.m_params.DiffuseColor = Vector4::ONE();
			obj.m_params.UVScale = Vector2(10, 10);

			auto texture = owner.loadImage2D("checker_d.png", true, true);
			obj.m_params.Texture = texture->createSampledView();
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
            obj.m_params.DiffuseColor = Vector4(1.0f, 0.2f, 0.2f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, 20.0f, 0.0f);
            obj.m_params.LocalToWorld.translation(0.0f, -3.0f, -mesh->m_bounds.min.z);
        }
    }

    // teapot
    {
        MeshSetup meshSetup;
        meshSetup.m_loadTransform = Matrix::BuildScale(3.0f / 1.0f);
        //meshSetup.m_loadTransform *= Matrix::BuildRotationX(-90.0f);
        auto mesh = owner.loadMesh("teapot.obj", meshSetup);
        if (mesh)
        {
            auto& obj = ret->m_objects.emplaceBack();
            obj.m_mesh = mesh;
            obj.m_params.DiffuseColor = Vector4(0.6f, 1.2f, 0.2f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, -30.0f, 0.0f);
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
            obj.m_params.DiffuseColor = Vector4(1.3f, 1.2f, 0.2f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, 80.0f, 0.0f);
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
            obj.m_params.DiffuseColor = Vector4(0.9f, 0.2f, 1.2f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, 10.0f, 0.0f);
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
            obj.m_params.DiffuseColor = Vector4(0.0f, 0.6f, 1.2f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, 10.0f, 0.0f);
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
            obj.m_params.DiffuseColor = Vector4(0.2f, 1.2f, 1.2f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, 10.0f, 0.0f);
            obj.m_params.LocalToWorld.translation(-2.0f, 1.7f, -mesh->m_bounds.min.z);
        }
    }


    return ret;
}

//--

SimpleScenePtr CreateSphereScene(IRenderingTest& owner, const Vector3& initialPos, float size)
{
	auto ret = RefNew<SimpleScene>();

	// teapot
	{
		MeshSetup meshSetup;
		meshSetup.m_loadTransform = Matrix::BuildScale(size);
		//meshSetup.m_loadTransform *= Matrix::BuildRotationX(-90.0f);

		if (auto mesh = owner.loadMesh("sphere.obj", meshSetup))
		{
			auto& obj = ret->m_objects.emplaceBack();
			obj.m_mesh = mesh;
			obj.m_initialPos = initialPos - mesh->m_bounds.center();
			obj.m_params.DiffuseColor = Vector4(1, 1, 1, 1);
			obj.m_params.LocalToWorld.identity();
			obj.m_params.LocalToWorld.translation(initialPos);
		}
	}

	return ret;
}

//--

SimpleScenePtr CreateTeapotScene(IRenderingTest& owner)
{
    auto ret = RefNew<SimpleScene>();

    // plane as a base
    {
        MeshSetup meshSetup;
        meshSetup.m_swapYZ = true;

        if (auto mesh = owner.loadMesh("plane.obj", meshSetup))
        {
            auto& obj = ret->m_objects.emplaceBack();
            obj.m_mesh = mesh;
            obj.m_params.DiffuseColor = Vector4::ONE();
			obj.m_params.UVScale = Vector2(10, 10);
                    
			auto texture = owner.loadImage2D("checker_d.png", true, true);
			obj.m_params.Texture = texture->createSampledView();
        }
    }

    // teapot
    {
        MeshSetup meshSetup;
        meshSetup.m_loadTransform = Matrix::BuildScale(6.0f / 1.0f);
        //meshSetup.m_loadTransform *= Matrix::BuildRotationX(-90.0f);

        if (auto mesh = owner.loadMesh("teapot.obj", meshSetup))
        {
            auto& obj = ret->m_objects.emplaceBack();
            obj.m_mesh = mesh;
            obj.m_params.DiffuseColor = Vector4(0.8f, 1.2f, 0.8f, 1.0f);

            obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, -30.0f, 0.0f);
            obj.m_params.LocalToWorld.translation(0.0f, 0.0f, -mesh->m_bounds.min.z);
        }
    }

    return ret;
}

//--

SimpleScenePtr CreateManyTeapotsScene(IRenderingTest& owner, int gridSize)
{
	auto ret = RefNew<SimpleScene>();

	// plane as a base
	{
		MeshSetup meshSetup;
		meshSetup.m_swapYZ = true;

		if (auto mesh = owner.loadMesh("plane.obj", meshSetup))
		{
			auto& obj = ret->m_objects.emplaceBack();
			obj.m_mesh = mesh;
			obj.m_params.DiffuseColor = Vector4::ONE();
			obj.m_params.UVScale = Vector2(10.0f, 10.0f);

			auto texture = owner.loadImage2D("checker_d.png", true, true);
			obj.m_params.Texture = texture->createSampledView();
		}
	}

	// teapots
	{
		MeshSetup meshSetup;
		meshSetup.m_loadTransform = Matrix::BuildScale(1.6f / 1.0f);
		//meshSetup.m_loadTransform *= Matrix::BuildRotationX(-90.0f);

		FastRandState rng;

		if (auto mesh = owner.loadMesh("teapot.obj", meshSetup))
		{
			float spacing = 1.6f;
			for (int y = -gridSize; y <= gridSize; ++y)
			{
				for (int x = -gridSize; x <= gridSize; ++x)
				{
					auto& obj = ret->m_objects.emplaceBack();
					obj.m_mesh = mesh;
					obj.m_params.DiffuseColor.x = rng.range(0.4f, 1.0f);
					obj.m_params.DiffuseColor.y = rng.range(0.4f, 1.0f);
					obj.m_params.DiffuseColor.z = rng.range(0.4f, 1.0f);
					obj.m_params.DiffuseColor.w = 1.0f;

					obj.m_initialYaw = rng.range(0, 360);
					obj.m_initialPos.x = x * spacing;
					obj.m_initialPos.y = y * spacing;
					obj.m_initialPos.z = -mesh->m_bounds.min.z;
					obj.m_params.LocalToWorld = Matrix::BuildRotation(0.0f, obj.m_initialYaw, 0.0f);
					obj.m_params.LocalToWorld.translation(obj.m_initialPos);
				}
			}					
		}
	}

	return ret;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::test)
