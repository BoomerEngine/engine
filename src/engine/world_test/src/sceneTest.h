/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/object/include/rttiMetadata.h"
#include "core/resource/include/loader.h"

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

#include "engine/world/include/worldEntity.h"
#include "engine/world/include/world.h"
#include "engine/world/include/worldViewEntity.h"
#include "engine/world_entities/include/entityFreeCamera.h"

#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

class FlyCameraEntity;

//---

// order of test
class SceneTestOrderMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTestOrderMetadata, IMetadata);

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
class ISceneTest : public NoCopy
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISceneTest);

public:
    ISceneTest();

    bool processInitialization();

    virtual void initialize();
    virtual void configure();
    virtual void prepareFrame(FrameParams& info);
    virtual void renderDebug(DebugGeometryCollector& debug);
    virtual void renderFrame(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, FrameStats& outStats, CameraSetup& outCamera);
    virtual void update(float dt);
    virtual bool processInput(const InputEvent& evt);

    virtual CompiledWorldDataPtr createStaticContent();
    virtual void createDynamicContent(World* world);

    void reportError(StringView msg);

    INLINE World* world() const { return m_world; }

    INLINE FreeCameraEntity* camera() const { return m_camera; }

protected:
    WorldPtr m_world;
    bool m_failed = false;

    Vector3 m_initialCameraPosition;
    Angles m_initialCameraRotation;

    RefPtr<FreeCameraEntity> m_camera;
    CameraContextPtr m_cameraContext;

    //--

    void configureLocalAdjustments();
    void recreateWorld();

    static FrameFilterFlags st_FrameFilterFlags;
    static FrameRenderMode st_FrameMode;

    static Angles st_GlobalLightingRotation;

    static FrameParams_ShadowCascades st_CascadeAdjust;
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

        void pack(FrameParams_GlobalLighting& outParams) const;
    };

    static GlobalLightingParams st_GlobalLightingAdjust;

    static FrameParams_ExposureAdaptation st_GlobalExposureAdaptationAdjust;
    static bool st_GlobalExposureAdaptationAdjustEnabled;

    static FrameParams_ToneMapping st_GlobalTonemappingAdjust;
    static bool st_GlobalTonemappingAdjustEnabled;

    static FrameParams_ColorGrading st_GlobalColorGradingAdjust;
    static bool st_GlobalColorGradingAdjustEnabled;
};

///---

END_BOOMER_NAMESPACE_EX(test)
