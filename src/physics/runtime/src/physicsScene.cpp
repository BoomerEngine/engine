/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "physicsService.h"
#include "physicsScene.h"

BEGIN_BOOMER_NAMESPACE(boomer)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(PhysicsScene);
RTTI_END_TYPE();

PhysicsScene::PhysicsScene(physx::PxScene* scene)
    : m_scene(scene)
{
}

PhysicsScene::~PhysicsScene()
{

}

void PhysicsScene::simulate(float dt)
{
}

//---

END_BOOMER_NAMESPACE(boomer)