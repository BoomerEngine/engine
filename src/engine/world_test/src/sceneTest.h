/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/object/include/rttiMetadata.h"
#include "core/resource/include/resourceLoader.h"

#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/style.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"

#include "engine/font/include/font.h"
#include "engine/font/include/fontInputText.h"
#include "engine/font/include/fontGlyphCache.h"
#include "engine/font/include/fontGlyphBuffer.h"

#include "engine/rendering/include/params.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

class FlyCameraEntity;

//---

// order of test
class SceneTestOrderMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTestOrderMetadata, rtti::IMetadata);

public:
    INLINE SceneTestOrderMetadata()
        : m_order(-1)
    {}

    SceneTestOrderMetadata& order(int val)
    {
        m_order = val;
        return *this;
    }

    int m_order;
};

//---

class FlyCamera : public IReferencable
{
public:
    FlyCamera();
    ~FlyCamera();

    void update(float dt);

    void place(const Vector3& initialPos, const Angles& initialRotation);

    void compute(rendering::FrameParams& params) const;

    bool processRawInput(const input::BaseEvent& evt);

private:
    float m_buttonForwardBackward = 0.0f;
    float m_buttonLeftRight = 0.0f;
    float m_buttonUpDown = 0.0f;
    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;

    Angles m_rotation;
    Vector3 m_position;

    rendering::CameraContextPtr m_context;
};

//---

/// a basic rendering test for the scene
class ISceneTest : public NoCopy
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISceneTest);

public:
    ISceneTest();

    bool processInitialization();

    virtual void initialize();
    virtual void configure();
    virtual void render(rendering::FrameParams& info);
    virtual void update(float dt);
    virtual bool processInput(const input::BaseEvent& evt);

    void reportError(StringView msg);

    MeshRef loadMesh(StringView meshName);

protected:
    WorldPtr m_world;
    bool m_failed = false;

    RefPtr<FlyCamera> m_camera;

    //--

    void configureLocalAdjustments();

    static rendering::FilterFlags st_FrameFilterFlags;
    static rendering::FrameRenderMode st_FrameMode;

    static Angles st_GlobalLightingRotation;

    static rendering::FrameParams_ShadowCascades st_CascadeAdjust;
    static bool st_CascadeAdjustEnabled;

    struct GlobalLightingParams
    {
        float globalLightColor[3];
        float globalLightBrightness = 1.0f;
        bool enableLightOverride = false;

        float globalAmbientZenithColor[3];
        float globalAmbientHorizonColor[3];
        float globalAmbientBrightness = 1.0f;
        bool enableAmbientOverride = false;

        GlobalLightingParams();

        void pack(rendering::FrameParams_GlobalLighting& outParams) const;
    };

    static GlobalLightingParams st_GlobalLightingAdjust;

    static rendering::FrameParams_ExposureAdaptation st_GlobalExposureAdaptationAdjust;
    static bool st_GlobalExposureAdaptationAdjustEnabled;

    static rendering::FrameParams_ToneMapping st_GlobalTonemappingAdjust;
    static bool st_GlobalTonemappingAdjustEnabled;

    static rendering::FrameParams_ColorGrading st_GlobalColorGradingAdjust;
    static bool st_GlobalColorGradingAdjustEnabled;
};

///---

/// a basic test with empty initial world
class ISceneTestEmptyWorld : public ISceneTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(ISceneTestEmptyWorld, ISceneTest);

public:
    ISceneTestEmptyWorld();

    void recreateWorld();
    virtual void createWorldContent();

protected:
    virtual void initialize() override;

    Vector3 m_initialCameraPosition;
    Angles m_initialCameraRotation;
};

///---

END_BOOMER_NAMESPACE_EX(test)
