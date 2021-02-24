/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "physics_data_glue.inl"

BEGIN_BOOMER_NAMESPACE(boomer)

class IPhysicsCollider;
typedef base::RefPtr<IPhysicsCollider> PhysicsColliderPtr;

class PhysicsCompiledShapeData;
typedef base::RefPtr<PhysicsCompiledShapeData> PhysicsCompiledShapeDataPtr;

END_BOOMER_NAMESPACE(boomer)