/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "sceneTest.h"

#include "engine/world/include/world.h"
#include "engine/world/include/entity.h"
#include "engine/mesh/include/mesh.h"
#include "engine/rendering/include/cameraContext.h"

#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

RTTI_BEGIN_TYPE_CLASS(SceneTestOrderMetadata);
RTTI_END_TYPE();

//--

static float MaxColor(const Vector3& color, float minMax = 1.0f)
{
    return std::max<float>(minMax, color.maxValue());
}

ISceneTest::GlobalLightingParams::GlobalLightingParams()
{
    auto params = rendering::FrameParams_GlobalLighting();

    globalLightBrightness = MaxColor(params.globalLightColor);
    globalLightColor[0] = params.globalLightColor.x / globalLightBrightness;
    globalLightColor[1] = params.globalLightColor.y / globalLightBrightness;
    globalLightColor[2] = params.globalLightColor.z / globalLightBrightness;

    globalLightBrightness = log2f(globalLightBrightness);

    globalAmbientBrightness = std::max<float>(MaxColor(params.globalAmbientColorHorizon, 0.1f), MaxColor(params.globalAmbientColorZenith, 0.1f));
    globalAmbientHorizonColor[0] = params.globalAmbientColorHorizon.x / globalAmbientBrightness;
    globalAmbientHorizonColor[1] = params.globalAmbientColorHorizon.y / globalAmbientBrightness;
    globalAmbientHorizonColor[2] = params.globalAmbientColorHorizon.z / globalAmbientBrightness;
    globalAmbientZenithColor[0] = params.globalAmbientColorZenith.x / globalAmbientBrightness;
    globalAmbientZenithColor[1] = params.globalAmbientColorZenith.y / globalAmbientBrightness;
    globalAmbientZenithColor[2] = params.globalAmbientColorZenith.z / globalAmbientBrightness;

    globalAmbientBrightness = log2f(globalLightBrightness / 0.1f);
}

void ISceneTest::GlobalLightingParams::pack(rendering::FrameParams_GlobalLighting& outParams) const
{
    if (enableLightOverride)
    {
        float actualScale = powf(2.0f, globalLightBrightness);
        outParams.globalLightColor.x = globalLightColor[0] * actualScale;
        outParams.globalLightColor.y = globalLightColor[1] * actualScale;
        outParams.globalLightColor.z = globalLightColor[2] * actualScale;
    }

    if (enableAmbientOverride)
    {
        float actualScale = powf(2.0f, globalAmbientBrightness) * 0.1f;
        outParams.globalAmbientColorHorizon.x = globalAmbientHorizonColor[0] * actualScale;
        outParams.globalAmbientColorHorizon.y = globalAmbientHorizonColor[1] * actualScale;
        outParams.globalAmbientColorHorizon.z = globalAmbientHorizonColor[2] * actualScale;
        outParams.globalAmbientColorZenith.x = globalAmbientZenithColor[0] * actualScale;
        outParams.globalAmbientColorZenith.y = globalAmbientZenithColor[1] * actualScale;
        outParams.globalAmbientColorZenith.z = globalAmbientZenithColor[2] * actualScale;
    }
}

//--

FlyCamera::FlyCamera()
{
    m_context = RefNew<rendering::CameraContext>();
}

FlyCamera::~FlyCamera()
{}

bool FlyCamera::processRawInput(const input::BaseEvent& evt)
{
    if (const auto* key = evt.toKeyEvent())
    {
        if (key->pressed() || key->released())
        {
            float value = key->pressed() ? 1.0f : -1.0f;
            switch (key->keyCode())
            {
            case input::KeyCode::KEY_A: m_buttonLeftRight -= value; return true;
            case input::KeyCode::KEY_D: m_buttonLeftRight += value; return true;
            case input::KeyCode::KEY_W: m_buttonForwardBackward += value; return true;
            case input::KeyCode::KEY_S: m_buttonForwardBackward -= value; return true;
            case input::KeyCode::KEY_Q: m_buttonUpDown -= value; return true;
            case input::KeyCode::KEY_E: m_buttonUpDown += value; return true;
            }
        }
    }
    else if (const auto* axis = evt.toAxisEvent())
    {
        switch (axis->axisCode())
        {
        case input::AxisCode::AXIS_MOUSEX: m_mouseDeltaX = axis->displacement() * 0.25f; return true;
        case input::AxisCode::AXIS_MOUSEY: m_mouseDeltaY = axis->displacement() * 0.25f; return true;
        }
    }

    return false;
}

void FlyCamera::compute(rendering::FrameParams& params) const
{
    CameraSetup cameraParams;
    cameraParams.position = m_position;
    cameraParams.rotation = m_rotation.toQuat();
    cameraParams.fov = 75.0f;
    cameraParams.aspect = params.resolution.aspect();
    cameraParams.nearPlane = 0.01f;
    cameraParams.farPlane = 500.0f;

    params.camera.camera.setup(cameraParams);
    params.camera.cameraContext = m_context;
}

void FlyCamera::place(const Vector3& initialPos, const Angles& initialRotation)
{
    m_position = initialPos;
    m_rotation = initialRotation;
}


void FlyCamera::update(float dt)
{
    Vector3 cameraMoveLocalDir(0, 0, 0);
    cameraMoveLocalDir.x = m_buttonForwardBackward;
    cameraMoveLocalDir.y = m_buttonLeftRight;
    cameraMoveLocalDir.z = m_buttonUpDown;

    float cameraSpeed = 5.0f;
    //if (m_keys[KEY_FAST]) cameraSpeed = 10.0f;
    //if (m_keys[KEY_SLOW]) cameraSpeed = 1.0f;

    if (!cameraMoveLocalDir.isZero())
    {
        cameraMoveLocalDir.normalize();
        cameraMoveLocalDir *= cameraSpeed;
    }

    Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
    m_rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

    Vector3 deltaPosition(0, 0, 0);
    deltaPosition += cameraDirForward * cameraMoveLocalDir.x * dt;
    deltaPosition += cameraDirRight * cameraMoveLocalDir.y * dt;
    deltaPosition += cameraDirUp * cameraMoveLocalDir.z * dt;

    m_rotation.pitch = std::clamp<float>(m_rotation.pitch + m_mouseDeltaY, -90.0f, 90.0f);
    m_rotation.yaw += m_mouseDeltaX;
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;

    m_position += deltaPosition;
}

//--

rendering::FilterFlags ISceneTest::st_FrameFilterFlags = rendering::FilterFlags::DefaultEditor();
rendering::FrameRenderMode ISceneTest::st_FrameMode = rendering::FrameRenderMode::Default;
Angles ISceneTest::st_GlobalLightingRotation(70.0f, 40.0f, 0.0f);

ISceneTest::GlobalLightingParams ISceneTest::st_GlobalLightingAdjust;

rendering::FrameParams_ShadowCascades ISceneTest::st_CascadeAdjust;
bool ISceneTest::st_CascadeAdjustEnabled = false;

rendering::FrameParams_ExposureAdaptation ISceneTest::st_GlobalExposureAdaptationAdjust;
bool ISceneTest::st_GlobalExposureAdaptationAdjustEnabled = false;

rendering::FrameParams_ToneMapping ISceneTest::st_GlobalTonemappingAdjust;
bool ISceneTest::st_GlobalTonemappingAdjustEnabled = false;

rendering::FrameParams_ColorGrading ISceneTest::st_GlobalColorGradingAdjust;
bool ISceneTest::st_GlobalColorGradingAdjustEnabled = false;

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneTest);
RTTI_END_TYPE();

ISceneTest::ISceneTest()
    : m_failed(false)
{
}

bool ISceneTest::processInitialization()
{
    m_camera = RefNew<FlyCamera>();

    const auto angles = Angles(30.0f, -15.0f, 0.0f);
    m_camera->place(Vector3(0, 0, 0.5f) - angles.forward() * 3.0f, angles);

    initialize();

    return !m_failed;
}

void ISceneTest::configure()
{
    configureLocalAdjustments();

    if (m_world)
        m_world->renderDebugGui();
}

void ISceneTest::initialize()
{
    // scenes usually create world here
}

void ISceneTest::render(rendering::FrameParams& info)
{
    info.filters = st_FrameFilterFlags;
    info.mode = st_FrameMode;

    info.cascades.numCascades = 3;
    //info.cascades.baseRange = 10.0f;

    if (st_CascadeAdjustEnabled)
        info.cascades = st_CascadeAdjust;

    info.globalLighting.globalLightDirection = st_GlobalLightingRotation.forward();
    if (info.globalLighting.globalLightDirection.z < 0.0f)
        info.globalLighting.globalLightDirection.z = -info.globalLighting.globalLightDirection.z;

    st_GlobalLightingAdjust.pack(info.globalLighting);

    if (st_GlobalExposureAdaptationAdjustEnabled)
        info.exposureAdaptation = st_GlobalExposureAdaptationAdjust;

    if (st_GlobalTonemappingAdjustEnabled)
        info.toneMapping = st_GlobalTonemappingAdjust;

    if (st_GlobalColorGradingAdjustEnabled)
        info.colorGrading = st_GlobalColorGradingAdjust;

    if (m_camera)
        m_camera->compute(info);

    if (m_world)
        m_world->render(info);
}

void ISceneTest::update(float dt)
{
    if (m_camera)
        m_camera->update(dt);

    if (m_world)
        m_world->update(dt);
}

bool ISceneTest::processInput(const input::BaseEvent& evt)
{
    if (m_camera)
        return m_camera->processRawInput(evt);
    return false;
}

void ISceneTest::reportError(StringView msg)
{
    TRACE_ERROR("SceneTest initialization error: {}", msg);
    m_failed = true;
}

MeshRef ISceneTest::loadMesh(StringView assetFile)
{
    auto meshPtr = LoadResource<Mesh>(assetFile);
    if (!meshPtr)
    {
        reportError(TempString("Failed to load mesh '{}'", meshPtr));
        return nullptr;
    }

    return meshPtr;
}

//--

ConfigProperty<bool> cvDebugFilterPanel("DebugPage.Rendering.Filters", "IsVisible", false);
ConfigProperty<bool> cvDebugGlobalLightAdjust("DebugPage.SceneDebug.GlobalLight", "IsVisible", false);
ConfigProperty<bool> cvDebugGlobalCascadesAdjust("DebugPage.SceneDebug.GlobalCascades", "IsVisible", false);
ConfigProperty<bool> cvDebugGlobalExposureAdjust("DebugPage.SceneDebug.Exposure", "IsVisible", false);
ConfigProperty<bool> cvDebugGlobalToneMappingAdjust("DebugPage.SceneDebug.ToneMapping", "IsVisible", false);
ConfigProperty<bool> cvDebugGlobalColorGradingAdjust("DebugPage.SceneDebug.ColorGrading", "IsVisible", false);

template <typename T >
static void DrawEnumOptions(T& mode)
{
    if (const auto* enumType = static_cast<const EnumType*>(GetTypeObject<T>().ptr()))
    {
        for (const auto& option : enumType->options())
        {
            int64_t value = 0;
            if (enumType->findValue(option, value))
            {
                const bool selected = (mode == (T)value);
                if (ImGui::Selectable(TempString("{}", option), selected))
                    mode = (T)value;
                if (selected)
                    ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
    }
}

static void DrawFilterNodes(const rendering::FilterBitInfo* bit, rendering::FilterFlags& flags)
{
    if (bit->children.empty())
    {
        bool flag = flags.test(bit->bit);
        if (ImGui::Checkbox(TempString("{}", bit->name), &flag))
            flags ^= bit->bit;
    }
    else
    {
        if (ImGui::TreeNode(bit, bit->name.c_str()))
        {
            for (const auto* child : bit->children)
                DrawFilterNodes(child, flags);
            ImGui::TreePop();
        }
    }
}

void ISceneTest::configureLocalAdjustments()
{
    if (cvDebugFilterPanel.get() && ImGui::Begin("Filters", &cvDebugFilterPanel.get()))
    {
        if (ImGui::BeginCombo("Mode", TempString("{}", st_FrameMode)))
        {
            DrawEnumOptions(st_FrameMode);
            ImGui::EndCombo();
        }


        const auto root = rendering::GetFilterTree();
        DrawFilterNodes(root, st_FrameFilterFlags);
        ImGui::End();
    }

    if (cvDebugGlobalLightAdjust.get() && ImGui::Begin("GlobalLight", &cvDebugGlobalLightAdjust.get()))
    {
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("Pitch", &st_GlobalLightingRotation.pitch, 0.0f, 90.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("Yaw", &st_GlobalLightingRotation.yaw, -360.0f, 360.0f);

        ImGui::Separator();

        ImGui::Checkbox("Enable light override", &st_GlobalLightingAdjust.enableLightOverride);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::ColorEdit3("Light Color", st_GlobalLightingAdjust.globalLightColor, ImGuiColorEditFlags_Float);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("Light Brightness (log2)", &st_GlobalLightingAdjust.globalLightBrightness, 0.0f, 15.0f);

        ImGui::Separator();

        ImGui::Checkbox("Enable ambient override", &st_GlobalLightingAdjust.enableAmbientOverride);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::ColorEdit3("Ambient Horizon", st_GlobalLightingAdjust.globalAmbientHorizonColor, ImGuiColorEditFlags_Float);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::ColorEdit3("Ambient Zenith", st_GlobalLightingAdjust.globalAmbientZenithColor, ImGuiColorEditFlags_Float);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("Ambient Brightness (0.1*log2)", &st_GlobalLightingAdjust.globalAmbientBrightness, 0.0f, 15.0f);

        ImGui::End();
    }

    if (cvDebugGlobalCascadesAdjust.get() && ImGui::Begin("GlobalCascades", &cvDebugGlobalCascadesAdjust.get()))
    {
        ImGui::Checkbox("Enable", &st_CascadeAdjustEnabled);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::Text("Global settings");
        ImGui::DragInt("NumCascades", &st_CascadeAdjust.numCascades, 0.01f, 0, 4);
        ImGui::SliderFloat("BaseRange", &st_CascadeAdjust.baseRange, 0.5f, 20.0f);
        ImGui::SliderFloat("BaseEdgeFade", &st_CascadeAdjust.baseEdgeFade, 0.0f, 0.5f);
        ImGui::SliderFloat("BaseFilterSize", &st_CascadeAdjust.baseFilterSize, 0.0f, 64.0f);
        ImGui::Text("Depth bias");
        ImGui::DragFloat("DepthBias", &st_CascadeAdjust.baseDepthBiasConstant, 1.0f, 0.0f, 1000.0f, "%.6f");
        ImGui::DragFloat("DepthSlope", &st_CascadeAdjust.baseDepthBiasSlope, 0.1f, -10.0f, 10.0f, "%.6f");
        ImGui::Text("Cascade related adjustments");
        ImGui::DragFloat("DepthBiasTexelSizeMul", &st_CascadeAdjust.depthBiasSlopeTexelSizeMul, 0.01f, 0.0f, 2.0f, "%.2f");
        ImGui::DragFloat("FilterSizeTexelSizeMul", &st_CascadeAdjust.filterSizeTexelSizeMul, 0.01f, 0.0f, 2.0f, "%.2f");
        ImGui::Text("Per cascade settings");
        ImGui::SliderFloat("RangeMul1", &st_CascadeAdjust.rangeMul1, 1.5f, 10.0f);
        ImGui::SliderFloat("RangeMul2", &st_CascadeAdjust.rangeMul2, 1.5f, 10.0f);
        ImGui::SliderFloat("RangeMul3", &st_CascadeAdjust.rangeMul3, 1.5f, 10.0f);
        ImGui::End();
    }

    if (cvDebugGlobalExposureAdjust.get() && ImGui::Begin("Exposure", &cvDebugGlobalExposureAdjust.get()))
    {
        ImGui::Checkbox("Enable", &st_GlobalExposureAdaptationAdjustEnabled);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("KeyValue", &st_GlobalExposureAdaptationAdjust.keyValue, 0.0f, 1.0f);
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("ExposureCompensationEV", &st_GlobalExposureAdaptationAdjust.exposureCompensationEV, -5.0f, 5.0f, "%.3f EV");
        ImGui::Separator();
        ImGui::Text("Adaptation");
        ImGui::SliderFloat("MinLumEV", &st_GlobalExposureAdaptationAdjust.minLuminanceEV, -5.0f, 5.0f, "%.3f EV");
        ImGui::SliderFloat("MaxLumEV", &st_GlobalExposureAdaptationAdjust.maxLuminanceEV, -5.0f, 5.0f, "%.3f EV");
        ImGui::SliderFloat("Speed", &st_GlobalExposureAdaptationAdjust.adaptationSpeed, 0.0f, 10.0f, "%.3f", 2.0f);
        ImGui::End();
    }

    if (cvDebugGlobalToneMappingAdjust.get() && ImGui::Begin("ToneMapping", &cvDebugGlobalToneMappingAdjust.get()))
    {
        ImGui::Checkbox("Enable", &st_GlobalTonemappingAdjustEnabled);
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::BeginCombo("Mode", TempString("{}", st_GlobalTonemappingAdjust.type)))
        {
            DrawEnumOptions(st_GlobalTonemappingAdjust.type);
            ImGui::EndCombo();
        }
        ImGui::End();
    }
}
        
//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneTestEmptyWorld);
        
RTTI_END_TYPE();

ISceneTestEmptyWorld::ISceneTestEmptyWorld()
{
    m_initialCameraPosition = Vector3(-2, 0, 1);
    m_initialCameraRotation = Angles(20.0f, 0.0f, 0.0f);
}

void ISceneTestEmptyWorld::recreateWorld()
{
    m_world.reset();
    m_world = RefNew<World>();

    createWorldContent();
}

void ISceneTestEmptyWorld::createWorldContent()
{
    // TODO
}

//--

void ISceneTestEmptyWorld::initialize()
{
    TBaseClass::initialize();
    recreateWorld();
}

//--

END_BOOMER_NAMESPACE_EX(test)
