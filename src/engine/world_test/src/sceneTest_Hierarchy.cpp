/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "sceneTest.h"
#include "sceneTestUtil.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

/// simple transform hierarchy of components
class SceneTest_Hierarchy : public ISceneTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_Hierarchy, ISceneTest);

public:
    virtual void configure() override
    {
        TBaseClass::configure();

        if (ImGui::Begin("Scene"))
        {
            bool changed = false;

            ImGui::Text("Scene configuration");
            changed |= ImGui::SliderInt("Tower count", &st_RootCount, 0, 500);
            changed |= ImGui::SliderInt("Tower height", &st_TowerHeight, 1, 10);
            changed |= ImGui::SliderFloat("Separation", &st_RootDistance, 0.1f, 10.0, "%.2f", 2.0f);
            changed |= ImGui::Combo("Mesh Type", &st_MeshType, "Box\0Cylinder\0Sphere\0Random");
            changed |= ImGui::Checkbox("Use build hierarchy", &st_UseBuildHierarchy);
            changed |= ImGui::Checkbox("Use transform hierarchy", &st_UseTransformHierarchy);
            changed |= ImGui::Combo("Rotation type", &st_RotationType, "None\0External\0Script");

            if (st_TowerHeight * st_RootCount > 300)
            {
                if (ImGui::Button("Recreate"))
                    recreateWorld();
            }
            else if (changed)
            {
                recreateWorld();
            }

            ImGui::End();
        }
    }

    virtual void update(float dt) override
    {
        if (st_RotationType == 1)
        {
            m_totalTime += dt;

            for (auto& dyn : m_dynamicEntities)
            {
                if (!dyn.ent)
                    dyn.ent = world()->findEntityByStaticPath(dyn.path);

                if (auto ent = dyn.ent.lock())
                {
                    const auto yaw = m_totalTime * dyn.speed;

                    auto transform = ent->transform();
                    transform.R = Angles(0, yaw, 0).toQuat();
                    ent->requestTransformChange(transform);
                }
            }
        }

        TBaseClass::update(dt);
    }

    static int SelectMesh(MTRandState& rng, int meshCount)
    {
        if (st_MeshType < meshCount)
            return st_MeshType;

        return rng.range(meshCount);
    }

    virtual CompiledWorldDataPtr createStaticContent() override
    {
        SceneBuilder b;

        ResourceID meshes[3];
        meshes[0] = b.mapResource("/engine/meshes/cube.xmeta");
        meshes[1] = b.mapResource("/engine/meshes/cylinder.xmeta");
        meshes[2] = b.mapResource("/engine/meshes/sphere.xmeta");

        MTRandState rng;

        m_dynamicEntities.reset();

        for (uint32_t n = 0; n < st_RootCount; ++n)
        {
            const auto center = (UlamSpiral(n).toVector() * st_RootDistance).xyz(0.0f);
            b.ensureGroundUnder(center);

            RawEntityPtr prevEntity;
            for (uint32_t k = 0; k < st_TowerHeight; ++k)
            {
                auto meshIndex = SelectMesh(rng, ARRAY_COUNT(meshes));

                if (k == 0)
                {
                    auto& ent = m_dynamicEntities.emplaceBack();
                    ent.path = TempString("/root{}", n);
                    ent.speed = 1.0 + 20.0f * rng.unit();

                    b.pushTransform();
                    b.deltaTranslate(center);
                    prevEntity = b.buildMeshNode(meshes[meshIndex], ent.path.subString(1));
                    b.popTransform();
                }
                else
                {
                    b.toggleTransformParent(true);
                    b.pushParent(prevEntity);
                    b.deltaTranslate(0, 0, 1);
                    prevEntity = b.buildMeshNode(meshes[meshIndex]);
                    b.popParent();
                    b.toggleTransformParent(false);

                }
            }
        }

        return b.extractWorld();
    }

protected:
    struct Dynamic
    {
        StringBuf path;
        EntityWeakPtr ent;
        float speed = 1.0f;
    };

    Array<Dynamic> m_dynamicEntities;

    float m_totalTime = 0.0f;
    static int st_RootCount;
    static float st_RootDistance;
    static int st_TowerHeight;
    static int st_MeshType;
    static int st_RotationType;
    static bool st_UseBuildHierarchy;
    static bool st_UseTransformHierarchy;
};

int SceneTest_Hierarchy::st_RootCount = 1;
int SceneTest_Hierarchy::st_TowerHeight = 2;
int SceneTest_Hierarchy::st_MeshType = 0;
float SceneTest_Hierarchy::st_RootDistance = 2.0f;
bool SceneTest_Hierarchy::st_UseBuildHierarchy = true;
bool SceneTest_Hierarchy::st_UseTransformHierarchy = true;
int SceneTest_Hierarchy::st_RotationType = 0;

RTTI_BEGIN_TYPE_CLASS(SceneTest_Hierarchy);
    RTTI_METADATA(SceneTestOrderMetadata).order(50);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
