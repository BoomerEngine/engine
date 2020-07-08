/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "rendering/driver/include/renderingParametersView.h"

namespace rendering
{

    ///---

    /// a generalized material
    class RENDERING_MATERIAL_API IMaterial : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IMaterial, base::res::IResource);

    public:
        IMaterial();
        virtual ~IMaterial();

        ///---

        // data proxy for this material - contains everything required to render this material
        // NOTE: the proxy pointer itself may be updated during the lifetime of the material - especially if it reloads or base template changes
        virtual MaterialDataProxyPtr dataProxy() const = 0;

        ///---

        //! get the material template this material is based on
        /// NOTE: for templates it returns the same object :)
        /// NOTE: may be null for VERY broken materials
        virtual const MaterialTemplate* resolveTemplate() const = 0;

        //! reset parameter to default value
        virtual bool resetParameterRaw(base::StringID name) = 0;

        //! write value of material parameter
        //! NOTE: data buffer must match the type of the param
        virtual bool writeParameterRaw(base::StringID name, const void* data, base::Type type, bool refresh = true) = 0;

        //! read current value of parameter
        virtual bool readParameterRaw(base::StringID name, void* data, base::Type type, bool defaultValueOnly = false) const = 0;

        ///---

        template< typename T >
        INLINE bool writeParameter(base::StringID name, const T& data, bool refresh = true)
        {
            return writeParameterRaw(name, &data, base::reflection::GetTypeObject<T>(), refresh);
        }

        template< typename T >
        INLINE bool readParameter(base::StringID name, T& data) const
        {
            return readParameterRaw(name, &data, base::reflection::GetTypeObject<T>());
        }

        //----

    protected:
        // register other material as dependent on this material
        // NOTE: object should unregister itself via unregisterDependentMaterial when no longer needs to observer given material
        void registerDependentMaterial(IMaterial* material);

        // unregister dependent material
        void unregisterDependentMaterial(IMaterial* material);

        // change tracked material for current material
        void changeTrackedMaterial(IMaterial* material);

        // material we are following
        base::RefWeakPtr<IMaterial> m_trackedMaterial;

        // materials that depend on this material
        // NOTE: editor only, not saved
        base::Array<base::RefWeakPtr<IMaterial>> m_runtimeDependencies;
        base::Mutex m_runtimeDependenciesLock;

        //--

        // notify all dependencies that the data in this material changed
        virtual void notifyDataChanged();

        // notify all dependencies that the base material changed and some stuff will have to be rebuilt
        virtual void notifyBaseMaterialChanged();
    };

    ///---

} // rendering