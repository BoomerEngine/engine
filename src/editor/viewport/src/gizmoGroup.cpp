/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: gizmos #]
***/

#include "build.h"
#include "gizmo.h"
#include "gizmoGroup.h"
#include "gizmoReferenceSpace.h"
#include "viewportCameraSetup.h"

#include "engine/ui/include/uiInputAction.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

IGizmoHost::~IGizmoHost()
{}

//--

GizmoGroup::GizmoGroup(const IGizmoHost* host, Array<GizmoPtr>&& gizmos)
    : m_host(host)
    , m_gizmos(std::move(gizmos))
    , m_hoverClientPos(0,0)
    , m_hoverCursor(CursorType::Hand)
{
    alignGizmos();
}

void GizmoGroup::alignGizmos()
{
    if (m_host->gizmoHost_hasSelection())
    {
        const auto& space = m_host->gizmoHost_referenceSpace();

        const auto viewportSize = m_host->gizmoHost_viewportSize();
        const auto& viewportCamera = m_host->gizmoHost_camera();
        const ui::ViewportCameraSetup viewport(viewportCamera, viewportSize.x, viewportSize.y);

        const auto scaleFactor = viewport.calculateViewportScaleFactor(space.absoluteTransform().T);

        for (auto& gizmo : m_gizmos)
            gizmo->transfom(space, scaleFactor);
    }
}

GizmoRenderMode GizmoGroup::determineRenderMode(const IGizmo* gizmo) const
{
    if (m_activeGizmo)
    {
        if (m_activeGizmo == gizmo)
            return GizmoRenderMode::Active; // we are the one
        else
            return GizmoRenderMode::OtherActive; // you are not the one, Neo
    }
    else if (m_hoverGizmo == gizmo)
    {
        return GizmoRenderMode::Hover;
    }

    return GizmoRenderMode::Idle;
}

void GizmoGroup::render(rendering::FrameParams& frame)
{
    // transform points
    alignGizmos();

    // always make sure we get rid of dangling pointers
    if (m_activeInput.expired())
        m_activeInput.reset();

    // draw all non active gizmos
    for (auto& gizmo : m_gizmos)
    {
        if (m_activeGizmo != gizmo)
        {
            auto renderMode = determineRenderMode(gizmo);
            gizmo->render(frame, renderMode);
        }
    }

    // draw active gizmo(s)
    for (auto& gizmo : m_gizmos)
    {
        if (m_activeGizmo == gizmo)
        {
            auto renderMode = determineRenderMode(gizmo);
            gizmo->render(frame, renderMode);
        }
    }

    // render the gizmo into
    if (auto inputAction = m_activeInput.lock())
        inputAction->onRender3D(frame);
}

bool GizmoGroup::updateHover(const Point& clientPoint)
{
    alignGizmos();

    m_activeGizmo = nullptr;
    m_activeInput = nullptr;

    m_hoverCursor = CursorType::Hand;
    m_hoverClientPos = clientPoint;
    m_hoverGizmo = findGizmoUnderCursor(m_hoverClientPos, m_hoverCursor);

    return m_hoverGizmo;
}

bool GizmoGroup::updateCursor(CursorType& outCursor)
{
    if (m_hoverGizmo)
    {
        outCursor = m_hoverCursor;
        return true;
    }

    return false;
}

ui::InputActionPtr GizmoGroup::activate()
{
    if (m_hoverGizmo)
    {
        if (auto actionContext = m_host->gizmoHost_startAction())
        {
            if (auto inputAction = m_hoverGizmo->activate(m_hoverClientPos, actionContext))
            {
                m_activeInput = inputAction;
                m_activeGizmo = m_hoverGizmo;
                return inputAction;
            }
        }
    }

    return nullptr;
}

IGizmo* GizmoGroup::findGizmoUnderCursor(const Point& point, CursorType& outCursor) const
{
    const auto viewportSize = m_host->gizmoHost_viewportSize();
    const auto& viewportCamera = m_host->gizmoHost_camera();
    const ui::ViewportCameraSetup viewport(viewportCamera, viewportSize.x, viewportSize.y);

    IGizmo* bestGizmo = nullptr;
    float minDistance = VERY_LARGE_FLOAT;
    for (auto& gizmo : m_gizmos)
    {
        float gizmoMinDistance = VERY_LARGE_FLOAT;
        if (gizmo->hitTest(point, gizmoMinDistance, viewport, outCursor))
        {
            if (gizmoMinDistance < minDistance)
            {
                minDistance = gizmoMinDistance;
                bestGizmo = gizmo;
            }
        }
    }

    return bestGizmo;
}

//--

END_BOOMER_NAMESPACE_EX(ed)
