/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"
#include "scenePreviewPanel.h"
#include "sceneContentNodes.h"

#include "sceneEditMode.h"
#include "sceneEditMode_Default.h"
#include "sceneEditMode_Default_UI.h"
#include "sceneEditMode_Default_Transform.h"

#include "base/object/include/actionHistory.h"

namespace ed
{

    //--

    SceneEditModeDefaultTransformAction::SceneEditModeDefaultTransformAction(SceneEditMode_Default* mode, const ScenePreviewPanel* panel, const Array<SceneContentDataNodePtr>& nodes, const SceneGridSettings& grid, const SceneGizmoSettings& gizmo)
        : m_mode(mode)
        , m_panel(panel)
        , m_grid(grid)
        , m_gizmo(gizmo)
    {
        m_referenceSpace = m_mode->calculateGizmoReferenceSpace();

        m_nodes.reserve(nodes.size());
        for (const auto& node : nodes)
        {
            auto& entry = m_nodes.emplaceBack();
            entry.node = node;
            entry.parent = AddRef(rtti_cast<SceneContentDataNode>(node->parent()));
            entry.initialLocalTransform = node->calcLocalToParent();
            entry.initialWorldTransform = node->localToWorldTransform();
            entry.lastTransform = entry.initialWorldTransform;
        }
    }

    SceneEditModeDefaultTransformAction::~SceneEditModeDefaultTransformAction()
    {}

    const IGizmoHost* SceneEditModeDefaultTransformAction::host() const
    {
        return m_panel;
    }

    const GizmoReferenceSpace& SceneEditModeDefaultTransformAction::capturedReferenceSpace() const
    {
        return m_referenceSpace;
    }

    void SceneEditModeDefaultTransformAction::revert()
    {
        for (const auto& entry : m_nodes)
            entry.node->changePlacement(entry.initialWorldTransform);
    }

    void SceneEditModeDefaultTransformAction::applyDeltaTransform(const base::Transform& deltaTransform)
    {
        for (auto& entry : m_nodes)
            entry.lastTransform = m_referenceSpace.transform(entry.initialWorldTransform, deltaTransform);
    }

    void SceneEditModeDefaultTransformAction::preview(const base::Transform& deltaTransform)
    {
        applyDeltaTransform(deltaTransform);

        for (const auto& entry : m_nodes)
        {
            entry.node->changePlacement(entry.lastTransform);

            if (m_gizmo.target == SceneGizmoTarget::SelectionOnly)
                if (auto* entity = rtti_cast<SceneContentEntityNode>(entry.node.get()))
                    entity->invalidateData();
        }

        m_mode->handleTransformsChanged();
    }

    void SceneEditModeDefaultTransformAction::apply(const base::Transform& deltaTransform)
    {
        applyDeltaTransform(deltaTransform);

        Array<ActionMoveSceneNodeData> undoData;
        undoData.reserve(m_nodes.size());

        for (const auto& entry : m_nodes)
        {
            auto& data = undoData.emplaceBack();
            data.node = entry.node;
            data.oldTransform = entry.initialWorldTransform;
            data.newTransform = entry.lastTransform;
        }

        if (!undoData.empty())
        {
            auto fullRefresh = (m_gizmo.target == SceneGizmoTarget::SelectionOnly);
            auto action = CreateSceneNodeTransformAction(std::move(undoData), m_mode, fullRefresh);
            m_mode->actionHistory()->execute(action);
        }
    }

    static double Snap(double value, float grid, bool filter)
    {
        if (filter)
        {
            return base::Snap(value, (double)grid);
        }
        else
        {
            return value;
        }
    }

    static float Snap(float value, float grid, bool filter)
    {
        if (filter)
        {
            return base::Snap(value, grid);
        }
        else
        {
            return value;
        }
    }

    static float IsNearZero(float value)
    {
        const auto eps = FLT_EPSILON;
        return value >= -eps && value <= eps;
    }

    bool SceneEditModeDefaultTransformAction::filterTranslation(const base::Vector3& deltaTranslationInSpace, base::Transform& outTransform) const
    {
        if (deltaTranslationInSpace.isNearZero())
        {
            outTransform = Transform();
            return true;
        }

        if (m_referenceSpace.space() == GizmoSpace::World)
        {
            auto fullWorldPos = m_referenceSpace.absoluteTransform().position() + deltaTranslationInSpace;

            double x, y, z;
            fullWorldPos.expand(x, y, z);

            auto snappedX = Snap(x, m_grid.positionGridSize, !IsNearZero(deltaTranslationInSpace.x) && m_gizmo.enableX && m_grid.positionGridEnabled);
            auto snappedY = Snap(y, m_grid.positionGridSize, !IsNearZero(deltaTranslationInSpace.y) && m_gizmo.enableY && m_grid.positionGridEnabled);
            auto snappedZ = Snap(z, m_grid.positionGridSize, !IsNearZero(deltaTranslationInSpace.z) && m_gizmo.enableZ && m_grid.positionGridEnabled);
            fullWorldPos = base::AbsolutePosition(snappedX, snappedY, snappedZ);

            // compute final delta
            auto finalDelta = fullWorldPos - m_referenceSpace.absoluteTransform().position();
            outTransform = base::Transform(finalDelta);
        }
        else
        {
            //TRACE_INFO("Delta: {}, {}, {}", deltaTranslationInSpace.x, deltaTranslationInSpace.y, deltaTranslationInSpace.z)
            auto snappedX = Snap(deltaTranslationInSpace.x, m_grid.positionGridSize, !IsNearZero(deltaTranslationInSpace.x) && m_gizmo.enableX && m_grid.positionGridEnabled);
            auto snappedY = Snap(deltaTranslationInSpace.y, m_grid.positionGridSize, !IsNearZero(deltaTranslationInSpace.y) && m_gizmo.enableY && m_grid.positionGridEnabled);
            auto snappedZ = Snap(deltaTranslationInSpace.z, m_grid.positionGridSize, !IsNearZero(deltaTranslationInSpace.z) && m_gizmo.enableZ && m_grid.positionGridEnabled);
            //TRACE_INFO("Snapped: {}, {}, {}", snappedX, snappedY, snappedZ);

            //auto move = referenceSpace.axis(0) * snappedX;
            //move += referenceSpace.axis(1) * snappedY;
            //move += referenceSpace.axis(2) * snappedZ;
            //TRACE_INFO("Move: {}, {}, {}", move.x, move.y, move.z);
            auto move = base::Vector3(snappedX, snappedY, snappedZ);

            outTransform = base::Transform(move);
        }

        return true;
    }

    bool SceneEditModeDefaultTransformAction::filterRotation(const base::Angles& rotationAnglesInSpace, base::Transform& outTransform) const
    {
        auto snappedX = Snap(rotationAnglesInSpace.roll, m_grid.rotationGridSize, m_gizmo.enableX && m_grid.rotationGridEnabled);
        auto snappedY = Snap(rotationAnglesInSpace.pitch, m_grid.rotationGridSize, m_gizmo.enableY && m_grid.rotationGridEnabled);
        auto snappedZ = Snap(rotationAnglesInSpace.yaw, m_grid.rotationGridSize, m_gizmo.enableZ && m_grid.rotationGridEnabled);

        auto finalRot = base::Angles(snappedY, snappedZ, snappedX);
        outTransform = base::Transform(Vector3::ZERO(), finalRot.toQuat());
        return true;
    }

    //--

    SceneEditModeDefaultTransformDragger::SceneEditModeDefaultTransformDragger(SceneEditMode_Default* mode, const Array<SceneContentDataNodePtr>& nodes, const HashSet<SceneContentDataNode*>* filterSet, const SceneGridSettings& grid, GizmoSpace space, SceneNodeTransformValueFieldType field, double displacementPerStep)
        : m_mode(mode)
        , m_grid(grid)
        , m_space(space)
        , m_field(field)
        , m_displacementPerStep(displacementPerStep)
    {
        m_nodes.reserve(nodes.size());

        HashMap<SceneContentNode*, int> parentMap;

        for (const auto& node : nodes)
        {
            int parentIndex = -1;

            const auto partOfDragSet = !filterSet || filterSet->contains(node);
            if (!partOfDragSet)
            {
                parentMap.find(node->parent(), parentIndex);
                DEBUG_CHECK(parentIndex != -1);
                if (parentIndex == -1)
                    continue;
            }

            auto& entry = m_nodes.emplaceBack();
            entry.initialLocalTransformQuat = node->calcLocalToParent();
            entry.initialLocalTransform = entry.initialLocalTransformQuat.toEulerTransform();
            entry.initialWorldTransform = node->localToWorldTransform();
            entry.calculatedTransform = entry.initialWorldTransform;
            entry.node = node;
            entry.parentDragNode = parentIndex;

            parentMap[entry.node] = m_nodes.lastValidIndex();
        }
    }

    void SceneEditModeDefaultTransformDragger::cancel()
    {
        for (const auto& entry : m_nodes)
            entry.node->changePlacement(entry.initialWorldTransform);
    }

    void SceneEditModeDefaultTransformDragger::step(int stepDelta)
    {
        if (stepDelta)
        {
            m_accumulatedSteps += stepDelta;
            apply(m_accumulatedSteps * m_displacementPerStep);
        }
    }

    void SceneEditModeDefaultTransformDragger::apply(double finalValue)
    {
        if (finalValue != m_lastDisplacement)
        {
            m_lastDisplacement = finalValue;
            computeTransforms();

            for (const auto& entry : m_nodes)
                entry.node->changePlacement(entry.calculatedTransform);
        }
    }

    static double ApplyTransformDelta(double val, double delta, bool add)
    {
        return add ? val + delta : delta;
    }

    static double ApplyRotationDelta(double val, double delta, bool add)
    {
        val = add ? (val+delta) : delta;
        val -= std::trunc(val / 360.0) * 360.0;
        return val;
    }

    static double ApplyScaleDelta(double val, double delta, bool add)
    {
        if (add)
        {
            const float factor = std::exp(delta);
            return std::clamp<double>(val * factor, 0.0001, 10000.0);
        }
        else
        {
            return std::clamp<double>(delta, 0.0001, 10000.0);
        }
    }

    void ApplyLocalTransformField(EulerTransform& data, SceneNodeTransformValueFieldType field, double value, bool delta)
    {
        switch (field)
        {
        case SceneNodeTransformValueFieldType::TranslationX:
            data.T.x = ApplyTransformDelta(data.T.x, value, delta);
            break;

        case SceneNodeTransformValueFieldType::TranslationY:
            data.T.y = ApplyTransformDelta(data.T.y, value, delta);
            break;

        case SceneNodeTransformValueFieldType::TranslationZ:
            data.T.z = ApplyTransformDelta(data.T.x, value, delta);
            break;

        case SceneNodeTransformValueFieldType::RotationX:
            data.R.roll = ApplyRotationDelta(data.R.roll, value, delta);
            break;

        case SceneNodeTransformValueFieldType::RotationY:
            data.R.pitch = ApplyRotationDelta(data.R.pitch, value, delta);
            break;

        case SceneNodeTransformValueFieldType::RotationZ:
            data.R.yaw = ApplyRotationDelta(data.R.yaw, value, delta);
            break;

        case SceneNodeTransformValueFieldType::ScaleX:
            data.S.x = ApplyScaleDelta(data.S.x, value, delta);
            break;

        case SceneNodeTransformValueFieldType::ScaleY:
            data.S.y = ApplyScaleDelta(data.S.y, value, delta);
            break;

        case SceneNodeTransformValueFieldType::ScaleZ:
            data.S.z = ApplyScaleDelta(data.S.z, value, delta);
            break;
        }
    }

    void ResetLocalTransformField(EulerTransform& data, SceneNodeTransformValueFieldType field)
    {
        switch (field)
        {
        case SceneNodeTransformValueFieldType::TranslationX:
        case SceneNodeTransformValueFieldType::TranslationY:
        case SceneNodeTransformValueFieldType::TranslationZ:
            data.T = EulerTransform::Translation::ZERO();
            break;

        case SceneNodeTransformValueFieldType::RotationX:
        case SceneNodeTransformValueFieldType::RotationY:
        case SceneNodeTransformValueFieldType::RotationZ:
            data.R = EulerTransform::Rotation::ZERO();
            break;

        case SceneNodeTransformValueFieldType::ScaleX:
        case SceneNodeTransformValueFieldType::ScaleY:
        case SceneNodeTransformValueFieldType::ScaleZ:
            data.S = EulerTransform::Scale::ONE();
            break;
        }
    }

    static Vector3 TranslationDeltaVector(SceneNodeTransformValueFieldType field, double value)
    {
        switch (field)
        {
        case SceneNodeTransformValueFieldType::TranslationX:
            return Vector3(value, 0, 0);

        case SceneNodeTransformValueFieldType::TranslationY:
            return Vector3(0, value, 0);

        case SceneNodeTransformValueFieldType::TranslationZ:
            return Vector3(0, 0, value);

        }

        return Vector3::ZERO();
    }

    static void ApplyAbsolutePositionChange(double& x, double& y, double& z, SceneNodeTransformValueFieldType field, double value, bool delta)
    {
        switch (field)
        {
        case SceneNodeTransformValueFieldType::TranslationX:
            x = delta ? (x+value) : value;
            break;

        case SceneNodeTransformValueFieldType::TranslationY:
            y = delta ? (y + value) : value;
            break;

        case SceneNodeTransformValueFieldType::TranslationZ:
            z = delta ? (z + value) : value;
            break;
        }
    }

    static Quat RotationDeltaVector(SceneNodeTransformValueFieldType field, double value)
    {
        switch (field)
        {
        case SceneNodeTransformValueFieldType::RotationX:
            return Quat(Vector3::EX(), value);

        case SceneNodeTransformValueFieldType::RotationY:
            return Quat(Vector3::EY(), value);

        case SceneNodeTransformValueFieldType::RotationZ:
            return Quat(Vector3::EZ(), value);
        }

        return Quat::IDENTITY();
    }

    void ApplyWorldTransformField(AbsoluteTransform& data, SceneNodeTransformValueFieldType field, double value, bool delta)
    {
        switch (field)
        {
            case SceneNodeTransformValueFieldType::TranslationX:
            case SceneNodeTransformValueFieldType::TranslationY:
            case SceneNodeTransformValueFieldType::TranslationZ:
                {
                    double x, y, z;
                    data.position().expand(x, y, z);
                    ApplyAbsolutePositionChange(x, y, z, field, value, delta);
                    data.position(x, y, z);
                }
                break;

            case SceneNodeTransformValueFieldType::RotationX:
            case SceneNodeTransformValueFieldType::RotationY:
            case SceneNodeTransformValueFieldType::RotationZ:
                if (delta)
                {
                    data.rotation(data.rotation() * RotationDeltaVector(field, value));
                }
                else
                {
                    auto rotation = data.rotation().toRotator();

                    switch (field)
                    {
                    case SceneNodeTransformValueFieldType::RotationX:
                        rotation.roll = AngleNormalize(value);
                        break;
                    case SceneNodeTransformValueFieldType::RotationY:
                        rotation.pitch = AngleNormalize(value);
                        break;
                    case SceneNodeTransformValueFieldType::RotationZ:
                        rotation.yaw = AngleNormalize(value);
                        break;
                    }

                    data.rotation(rotation);
                }

                break;

            case SceneNodeTransformValueFieldType::ScaleX:
            {
                auto val = data.scale();
                val.x = ApplyScaleDelta(val.x, value, delta);
                data.scale(val);
                break;
            }

            case SceneNodeTransformValueFieldType::ScaleY:
            {
                auto val = data.scale();
                val.y = ApplyScaleDelta(val.y, value, delta);
                data.scale(val);
                break;
            }

            case SceneNodeTransformValueFieldType::ScaleZ:
            {
                auto val = data.scale();
                val.z = ApplyScaleDelta(val.z, value, delta);
                data.scale(val);
                break;
            }
        }
    }

    void ResetWorldTransformField(AbsoluteTransform& data, SceneNodeTransformValueFieldType field)
    {
        switch (field)
        {
        case SceneNodeTransformValueFieldType::TranslationX:
        case SceneNodeTransformValueFieldType::TranslationY:
        case SceneNodeTransformValueFieldType::TranslationZ:
            data.position(0, 0, 0);
            break;

        case SceneNodeTransformValueFieldType::RotationX:
        case SceneNodeTransformValueFieldType::RotationY:
        case SceneNodeTransformValueFieldType::RotationZ:
            data.rotation(Quat::IDENTITY());
            break;

        case SceneNodeTransformValueFieldType::ScaleX:
        case SceneNodeTransformValueFieldType::ScaleY:
        case SceneNodeTransformValueFieldType::ScaleZ:
            data.scale(1.0f);
            break;
        }
    }

    //--

    void SceneEditModeDefaultTransformDragger::computeTransforms()
    {
        for (auto& entry : m_nodes)
        {
            if (entry.parentDragNode == -1)
            {
                if (m_space == GizmoSpace::Local)
                {
                    auto localTransform = entry.initialLocalTransform;
                    ApplyLocalTransformField(localTransform, m_field, m_lastDisplacement, true);
                    entry.calculatedTransform = entry.node->parentToWorldTransform() * localTransform.toTransform();
                }
                else if (m_space == GizmoSpace::World)
                {
                    auto worldTransform = entry.initialWorldTransform;
                    ApplyWorldTransformField(worldTransform, m_field, m_lastDisplacement, true);
                    entry.calculatedTransform = worldTransform;
                }
            }
            else
            {
                const auto& parentTransform = m_nodes[entry.parentDragNode].calculatedTransform;
                entry.calculatedTransform = parentTransform * entry.initialLocalTransformQuat;
                entry.node->changePlacement(entry.calculatedTransform);
            }
        }
    }

    ActionPtr SceneEditModeDefaultTransformDragger::createAction() const
    {
        Array<ActionMoveSceneNodeData> undoData;
        undoData.reserve(m_nodes.size());

        for (const auto& node : m_nodes)
        {
            auto& data = undoData.emplaceBack();
            data.node = node.node;
            data.oldTransform = node.initialWorldTransform;
            data.newTransform = node.calculatedTransform;
        }

        return CreateSceneNodeTransformAction(std::move(undoData), m_mode, true);
    }

    //--

} // ed
    ;
