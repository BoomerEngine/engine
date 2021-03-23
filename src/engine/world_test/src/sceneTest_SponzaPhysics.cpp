/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "sceneTest.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

/// a simple box on plane test, can be upscaled to more shapes
class SceneTest_SponzaPhysics : public ISceneTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_SponzaPhysics, ISceneTest);

public:
    virtual void configure() override
    {
        TBaseClass::configure();
    }

    virtual void update(float dt) override
    {
        TBaseClass::update(dt);
    }
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_SponzaPhysics);
    RTTI_METADATA(SceneTestOrderMetadata).order(200);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(test)
