/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "renderingSceneProxy.h"
#include "renderingSelectable.h"

namespace rendering
{
    namespace scene
    {
        //--

        // base class for all proxy desc
        struct RENDERING_SCENE_API ProxyBaseDesc : public base::NoCopy
        {
        public:
            INLINE ProxyBaseDesc(ProxyType proxyType)
                : m_proxyType(proxyType)
            {}

            INLINE ProxyType proxyType() const { return m_proxyType; }

            template< typename T>
            INLINE const T& cast() const
            {
                DEBUG_CHECK_EX(proxyType() == T::PROXY_TYPE, "Invalid proxy type to cast to");
                return *static_cast<const T*>(this);
            }

            //---

            // placement of the proxy
            base::Matrix localToScene;

            // manual override of the auto hide distance, this distance will be respected regardless of any performance settings
            float autoHideDistanceOverride = 0.0f;

            // initial visibility state (usually set but we can create hidden proxies)
            bool visible = true;

            // initial selection state
            bool selected = false;  

        private:
            ProxyType m_proxyType;
        };

        //--

        // a common description for all mesh based proxies
        struct RENDERING_SCENE_API ProxyMeshDesc : public ProxyBaseDesc
        {
            static const auto PROXY_TYPE = ProxyType::Mesh;

            //--

            // mesh to render
            const Mesh* mesh = nullptr;

            // force single material to be used for whole mesh
            MaterialDataProxyPtr forceMaterial = nullptr;

            // force single LOD distance on the whole mesh
            char forcedLodLevel = 0;

            // should we cast shadows when rendering shadows
            bool castsShadows = true; 

            // should we receive shadows when rendered in lit modes
            bool receiveShadows = true; 

            //--

            // material override
            base::HashMap<base::StringID, const IMaterial*> materialOverrides; // TODO: remove the hashmap?

            //--

            ProxyMeshDesc();
        };

        ///--
        
    } // scene
} // rendering

