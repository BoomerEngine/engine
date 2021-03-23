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

/// a simple one-layer deep prefab instantiation
class SceneTest_Prefabs : public ISceneTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_Prefabs, ISceneTest);

public:
    int m_numPrefabs = 1;
    int m_sceneType = 0;
    float m_placementDistance = 3.0f;

    virtual void configure() override
    {
        TBaseClass::configure();

        if (ImGui::Begin("Scene"))
        {
            bool changed = false;

            ImGui::Text("Scene configuration");
            ImGui::SliderInt("Num prefabs", &m_numPrefabs, 0, 500);
            ImGui::SliderFloat("Prefab distance", &m_placementDistance, 0.1f, 20.0f);
            ImGui::Combo("Scene type", &m_sceneType, "Simple prefab\0Nested prefab\0Nested with override\0Nested deep\0Nested deep with inner override");
                    
            if (ImGui::Button("Recreate"))
                recreateWorld();

            ImGui::End();
        }
    }

    virtual void update(float dt) override
    {
        TBaseClass::update(dt);
    }

    PrefabPtr buildPrefab()
    {
        SceneBuilder b;

        const auto box = b.mapResource("/engine/meshes/cube.xmeta");
        b.deltaTranslate(0, 0, 0.0f);
        b.buildMeshNode(box, "box0");

        b.deltaTranslate(0, 0, 1.0f);
        b.deltaRotate(0, 5, 0);
        b.buildMeshNode(box, "box1");

        b.deltaTranslate(0, 0, 1.0f);
        b.deltaRotate(0, 5, 0);
        b.buildMeshNode(box, "box2");

        return b.extractPrefab();
    }

    PrefabPtr buildPrefabStack(bool overrideTest)
    {
        SceneBuilder b;

        const auto innerPrefab = buildPrefab();

        {
            b.pushTransform();
            b.deltaTranslate(0, 0, 0.0f);
            auto node = b.buildPrefabNode(innerPrefab);
            b.popTransform();

            if (overrideTest)
            {
                b.pushParent(node);

                const auto cyl = b.mapResource("/engine/meshes/cylinder.xmeta");
                b.buildMeshNode(cyl, "box1");

                b.popParent();
            }
        }

        {
            b.pushTransform();
            b.deltaTranslate(0, 0, 3.0f);
            b.deltaRotate(0, 15.0f, 0.0f);
            auto node = b.buildPrefabNode(innerPrefab);
            b.popTransform();

            if (overrideTest)
            {
                b.pushParent(node);

                const auto shp = b.mapResource("/engine/meshes/sphere.xmeta");
                b.buildMeshNode(shp, "box2");

                b.popParent();
            }
        }

        return b.extractPrefab();
    }

    PrefabPtr buildDeepPrefabStack(bool overrideTest)
    {
        SceneBuilder b;

        const auto innerPrefab = buildPrefabStack(overrideTest);

        {
            b.pushTransform();
            b.deltaTranslate(0, 0, 0.0f);
            auto node = b.buildPrefabNode(innerPrefab);
            b.popTransform();
        }

        {
            b.pushTransform();
            b.deltaTranslate(0, 0, 6.0f);
            auto node = b.buildPrefabNode(innerPrefab);
            b.popTransform();
        }

        return b.extractPrefab();
    }

    virtual CompiledWorldDataPtr createStaticContent() override
    {
        SceneBuilder b;

        PrefabPtr prefab;
        switch (m_sceneType)
        {
        case 0: prefab = buildPrefab(); break;
        case 1: prefab = buildPrefabStack(false); break;
        case 2: prefab = buildPrefabStack(true); break;
        case 3: prefab = buildDeepPrefabStack(false); break;
        case 4: prefab = buildDeepPrefabStack(true); break;
        }

        for (uint32_t i=0; i<m_numPrefabs; ++i)
        {
            const auto pos = UlamSpiral(i).toVector().xyz() * m_placementDistance + Vector3(0, 0, 0.5f);
            b.ensureGroundUnder(pos);

            {
                b.pushTransform();
                b.deltaTranslate(pos);
                b.buildPrefabNode(prefab);
                b.popTransform();
            }
        }

        return b.extractWorld();
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_Prefabs);
    RTTI_METADATA(SceneTestOrderMetadata).order(20);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
