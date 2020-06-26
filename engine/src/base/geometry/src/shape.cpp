/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shapes #]
***/

#include "build.h"
#include "shape.h"

namespace base
{
    namespace shape
    {

        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IShape);
        RTTI_END_TYPE();

        IShape::IShape()
        {}

        IShape::~IShape()
        {}

        uint32_t IShape::calcDataSize() const
        {
            return cls()->size();
        }

        //---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IConvexShape);
        RTTI_END_TYPE();

        IConvexShape::IConvexShape()
        {}

        IConvexShape::~IConvexShape()
        {}

        bool IConvexShape::isConvex() const
        {
            return true;
        }

        //----

        RTTI_BEGIN_TYPE_ENUM(ShapeRenderingQualityLevel);
            RTTI_ENUM_OPTION(Low);
            RTTI_ENUM_OPTION(Medium);
            RTTI_ENUM_OPTION(High);
        RTTI_END_TYPE();

        //---

        RTTI_BEGIN_TYPE_ENUM(ShapeRenderingMode);
            RTTI_ENUM_OPTION(Wire);
            RTTI_ENUM_OPTION(Solid);
        RTTI_END_TYPE();

        //---

        IShapeRenderer::~IShapeRenderer()
        {}

        //---

    } // shape
} // base