/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"
#include "base/resource/include/resourceLoader.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"

#include "base/font/include/font.h"
#include "base/font/include/fontInputText.h"
#include "base/font/include/fontGlyphCache.h"
#include "base/font/include/fontGlyphBuffer.h"

#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingSimpleFlyCamera.h"

BEGIN_BOOMER_NAMESPACE(game::test)

//---

class FlyCameraEntity;

//---

// order of test
class SceneTestOrderMetadata : public base::rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTestOrderMetadata, base::rtti::IMetadata);

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

/// a basic rendering test for the scene
class ISceneTest : public base::NoCopy
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISceneTest);

public:
    ISceneTest();

    bool processInitialization();

    virtual void initialize();
    virtual void configure();
    virtual void render(rendering::scene::FrameParams& info);
    virtual void update(float dt);
    virtual bool processInput(const base::input::BaseEvent& evt);

    void reportError(base::StringView msg);

    rendering::MeshRef loadMesh(base::StringView meshName);

protected:
    base::world::WorldPtr m_world;
    bool m_failed = false;

    base::RefPtr<rendering::scene::FlyCamera> m_camera;

    //--

    void configureLocalAdjustments();

    static rendering::scene::FilterFlags st_FrameFilterFlags;
    static rendering::scene::FrameRenderMode st_FrameMode;

    static base::Angles st_GlobalLightingRotation;

    static rendering::scene::FrameParams_ShadowCascades st_CascadeAdjust;
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

        void pack(rendering::scene::FrameParams_GlobalLighting& outParams) const;
    };

    static GlobalLightingParams st_GlobalLightingAdjust;

    static rendering::scene::FrameParams_ExposureAdaptation st_GlobalExposureAdaptationAdjust;
    static bool st_GlobalExposureAdaptationAdjustEnabled;

    static rendering::scene::FrameParams_ToneMapping st_GlobalTonemappingAdjust;
    static bool st_GlobalTonemappingAdjustEnabled;

    static rendering::scene::FrameParams_ColorGrading st_GlobalColorGradingAdjust;
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

    base::Vector3 m_initialCameraPosition;
    base::Angles m_initialCameraRotation;
};

///---

END_BOOMER_NAMESPACE(game::test)