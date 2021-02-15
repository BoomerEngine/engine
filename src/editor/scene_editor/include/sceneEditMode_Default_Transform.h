/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#pragma once

#include "sceneEditMode.h"
#include "scenePreviewContainer.h"
#include "editor/gizmos/include/gizmo.h"

namespace ed
{
    //--

    /// transform action for group of nodes
    class EDITOR_SCENE_EDITOR_API SceneEditModeDefaultTransformAction : public IGizmoActionContext
    {
    public:
        SceneEditModeDefaultTransformAction(SceneEditMode_Default* mode, const ScenePreviewPanel* panel, const Array<SceneContentDataNodePtr>& nodes, const SceneGridSettings& grid, const SceneGizmoSettings& gizmo);
        virtual ~SceneEditModeDefaultTransformAction();

        virtual const IGizmoHost* host() const override final;
        virtual const GizmoReferenceSpace& capturedReferenceSpace() const override final;

        virtual void revert() override final;
        virtual void preview(const base::Transform& deltaTransform) override final;
        virtual void apply(const base::Transform& deltaTransform) override final;

        virtual bool filterTranslation(const base::Vector3& deltaTranslationInSpace, base::Transform& outTransform) const override final;
        virtual bool filterRotation(const base::Angles& rotationAnglesInSpace, base::Transform& outTransform) const override final;

    private:
        SceneEditMode_Default* m_mode = nullptr;
        const ScenePreviewPanel* m_panel = nullptr;
        
        struct CapturedNode
        {
            SceneContentDataNodePtr node;
            SceneContentDataNodePtr parent;
            EulerTransform initialLocalTransform;
            AbsoluteTransform initialWorldTransform;
            AbsoluteTransform initialParentWorldTransform;

            EulerTransform lastTransform;
        };

        GizmoReferenceSpace m_referenceSpace;
        
        Array<CapturedNode> m_nodes;
        SceneGridSettings m_grid;
        SceneGizmoSettings m_gizmo;

        void applyDeltaTransform(const base::Transform& deltaTransform);
    };

    //--

    /// helper for an action of changing one field via dragger
    class EDITOR_SCENE_EDITOR_API SceneEditModeDefaultTransformDragger : public IReferencable
    {
    public:
        SceneEditModeDefaultTransformDragger(SceneEditMode_Default* mode, const Array<SceneContentDataNodePtr>& nodes, const SceneGridSettings& grid, GizmoSpace space, SceneNodeTransformValueFieldType field, double displacementPerStep);

        void cancel();
        void step(int stepDelta);
        void apply(double finalValue);

        ActionPtr createAction() const;

    private:
        struct CapturedNode
        {
            SceneContentDataNodePtr node;
            EulerTransform initialLocalTransform;
            AbsoluteTransform initialWorldTransform;
            AbsoluteTransform initialParentWorldTransform;

            EulerTransform calculatedTransform;
        };

        SceneEditMode_Default* m_mode = nullptr;

        Array<CapturedNode> m_nodes;
        SceneGridSettings m_grid;

        int m_accumulatedSteps = 0;
        double m_displacementPerStep = 0.01;
        double m_lastDisplacement = 0.0;

        GizmoSpace m_space;
        SceneNodeTransformValueFieldType m_field;

        //--

        void computeTransforms();
    };

    //--

    extern EDITOR_SCENE_EDITOR_API void ApplyLocalTransformField(EulerTransform& data, SceneNodeTransformValueFieldType field, double value, bool delta);
    extern EDITOR_SCENE_EDITOR_API void ApplyWorldTransformField(AbsoluteTransform& data, SceneNodeTransformValueFieldType field, double value, bool delta);
    extern EDITOR_SCENE_EDITOR_API void ResetLocalTransformField(EulerTransform& data, SceneNodeTransformValueFieldType field);
    extern EDITOR_SCENE_EDITOR_API void ResetWorldTransformField(AbsoluteTransform& data, SceneNodeTransformValueFieldType field);

    //--

} // ed
