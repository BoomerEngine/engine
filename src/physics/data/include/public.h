/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "physics_data_glue.inl"

namespace physics
{
    namespace data
    {
        class ICollider;
        typedef base::RefPtr<ICollider> ColliderPtr;

        class CompiledShapeData;
        typedef base::RefPtr<CompiledShapeData> CompiledShapeDataPtr;

    } // data
} // physics

