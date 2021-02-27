/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "gpu/device/include/globalObjects.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

///---

/// scene camera
struct SceneCamera
{
    Quat rotation;
    Vector3 position;
    float aspect = 1.0f;
    float fov = 90.0f;	
    float zoom = 1.0f;
	float farPlane = 50.0f;

    SceneCamera();

	void setupCubemap(const Vector3& pos, int face, float farPlane = 20.0f);
	void setupDirectionalShadowmap(const Vector3& target, const Vector3& source, float zoom = 10.0f, float farPlane = 50.0f);

    void calcMatrices(bool flipCamera = false);

    Matrix m_WorldToCamera;
    Matrix m_CameraToScreen;
    Matrix m_ViewToScreen;
    Matrix m_WorldToScreen;

    Matrix m_ScreenToWorld;
    Matrix m_ScreenToView;
};

///---

/// global params for scene shading
struct SceneCameraParams
{
	Matrix WorldToScreen;
	Vector4 CameraPosition;

	INLINE SceneCameraParams()
	{
		WorldToScreen = Matrix::IDENTITY();
		CameraPosition = Vector3(0, 0, 0);
	}
};

/// global params for scene shading
struct SceneGlobalParams
{            
	SceneCameraParams Cameras[6];
    Vector4 LightDirection;
    Vector4 LightColor;
    Vector4 AmbientColor;

	uint32_t NumCameras;
	uint32_t _Padding0;
	uint32_t _Padding1;
	uint32_t _Padding2;

    INLINE SceneGlobalParams()
    {
        LightDirection = Vector3(1, 1, 1).normalized();
		LightColor = Vector4(1, 1, 1, 1);
		AmbientColor = Vector4(0.2, 0.2, 0.2, 1);
		NumCameras = 1;
	}
};

///---

/// params for scene object
struct SceneObjectParams
{
    Matrix LocalToWorld;
    Vector2 UVScale;
    Vector2 Dummy;
    Vector4 DiffuseColor;
    Vector4 SpecularColor;

    INLINE SceneObjectParams()
    {
        LocalToWorld = Matrix::IDENTITY();
        UVScale = Vector2(1, 1);
        DiffuseColor = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
        SpecularColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }
};

///---

/// simple scene as collection of objects
class SimpleScene : public IReferencable
{
public:
    struct Object
    {
        SimpleRenderMeshPtr m_mesh;

		Vector3 m_initialPos;
		float m_initialYaw = 0.0f;

        struct Params
        {
            Matrix LocalToWorld;
            Vector2 UVScale;
            Vector4 DiffuseColor;
            Vector4 SpecularColor;
			ImageSampledViewPtr Texture;
			SamplerObjectPtr Sampler;

            INLINE Params()
            {
                LocalToWorld = Matrix::IDENTITY();
                UVScale = Vector2(1, 1);
                DiffuseColor = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
                SpecularColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
				Texture = gpu::Globals().TextureWhite;
				Sampler = gpu::Globals().SamplerWrapTriLinear;
            }
        };

        Params m_params;
    };

    Array<Object> m_objects;

    Vector3 m_lightPosition;
    Vector4 m_lightColor;
    Vector4 m_ambientColor;

    //--

    SimpleScene();

    void draw(CommandWriter& cmd, const GraphicsPipelineObject* func, const SceneCamera& camera);
	void draw(CommandWriter& cmd, const GraphicsPipelineObject* func, const SceneCamera* cameras, uint32_t numCameras);
};

typedef RefPtr<SimpleScene> SimpleScenePtr;

class IRenderingTest;

extern SimpleScenePtr CreatePlatonicScene(IRenderingTest& owner);
extern SimpleScenePtr CreateTeapotScene(IRenderingTest& owner);
extern SimpleScenePtr CreateManyTeapotsScene(IRenderingTest& owner, int gridSize);
extern SimpleScenePtr CreateSphereScene(IRenderingTest& owner, const Vector3& initialPos, float size = 0.1f);

END_BOOMER_NAMESPACE_EX(gpu::test)
