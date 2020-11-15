/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/object/include/objectTemplate.h"

namespace base
{
    namespace world
    {

        //--

        /// Streaming model for the node
        enum class EntityStreamingModel : uint8_t
        {
            // Auto mode
            Auto,

            // Stream with parent node, this allows nodes to be grouped together
            // If there's no streamable parent the node is streamed on it's own using the streaming distane
            // The streaming distance of the parent node is calculated based on the maximum streaming distance of the child nodes
            StreamWithParent,

            // Node will be streamed separately based on it's own position and streaming distance
            // NOTE: this node will not be accessible as target of bindings (since we cannot guarantee it's loaded)
            HierarchicalGrid,

            // Node will be always loaded when the world is loaded
            // NOTE: this node will always be accessible for bindings
            AlwaysLoaded,

            // Node will be placed in a separate sector when baking
            // Good only for heavy data like navmesh
            SeparateSector,

            // Node will be discarded during world baking
            // Good for temporary data
            Discard,
        };

        //--

        /// template of an entity
        class BASE_WORLD_API EntityTemplate : public IObjectTemplate
        {
            RTTI_DECLARE_VIRTUAL_CLASS(EntityTemplate, IObjectTemplate);

        public:
            EntityTemplate();

            //--

            // is this template enabled ? disabled template will not be applied (as if it was deleted)
            // NOTE: this property is not overridden but directly controlled
            INLINE bool enabled() const { return m_enabled; }

            // placement
            // NOTE: this property is not overridden but stacks (ie. transforms are stacking together)
            INLINE const Transform& placement() const { return m_placement; }

            // streaming model, overridable
            INLINE EntityStreamingModel streamingModel() const { return m_streamingModel; }

            // streaming distance override, overridable 
            INLINE float streamingDistanceOverride() const { return m_streamingDistanceOverride; }

            //--
            
            // toggle the enable flag
            void enable(bool flag, bool callEvent = true);

            // set placement
            void placement(const Transform& placement, bool callEvent = true);

            //--

            // change the streaming model
            void streamingModel(EntityStreamingModel model, bool makeOverride = true, bool callEvent = true);

            // change the streaming override distance
            void streamingDistanceOverride(float distance, bool makeOverride = true, bool callEvent = true);

            //--

            // create the entity from data
            virtual EntityPtr createEntity() const;

        protected:
            bool m_enabled = true;
            Transform m_placement;

            EntityStreamingModel m_streamingModel = EntityStreamingModel::Auto;
            float m_streamingDistanceOverride = 0.0f;
        };

        //--

    } // world
} // base