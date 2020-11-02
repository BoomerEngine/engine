/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/containers/include/hashSet.h"

namespace game
{

    /// node placement helper, all placement of this type are relative
    class GAME_WORLD_API NodeTemplatePlacement
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplatePlacement);

    public:
        base::Vector3 T;
        base::Angles R;
        base::Vector3 S;

        //--

        NodeTemplatePlacement();
        NodeTemplatePlacement(const base::Vector3& pos, const base::Angles& rot = base::Angles(0,0,0), const base::Vector3& scale = base::Vector3(1,1,1));
        NodeTemplatePlacement(float tx, float ty, float tz, float pitch = 0.0f, float yaw = 0.0f, float roll = 0.0f, float sx = 1.0f, float sy = 1.0f, float sz = 1.0f);
        NodeTemplatePlacement(const base::AbsoluteTransform& absoluteTransform);

        //---

        /// check if transform is identity
        bool identity() const;

        //--

        // compute the absolute transform
        base::AbsoluteTransform toAbsoluteTransform() const;

        // compute relative transform
        base::Transform toRelativeTransform(const base::AbsoluteTransform& relativeTo) const;

        // compute transform
        base::Transform toTransform() const;

        //--

        bool operator==(const NodeTemplatePlacement& other) const;
        bool operator!=(const NodeTemplatePlacement& other) const;

        //--

        // print to debug stream
        void print(base::IFormatStream& f) const;

        //--

        // identity placement (at origin)
        static const NodeTemplatePlacement& IDENTITY();
    };

} // game