/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: material #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// a generalized material
class ENGINE_MATERIAL_API IMaterial : public res::IResource
{
    RTTI_DECLARE_VIRTUAL_CLASS(IMaterial, res::IResource);

public:
    IMaterial();
    virtual ~IMaterial();

    ///---

	// template proxy for this material - represents rendering techniques needed to render this material
	// NOTE: the proxy pointer itself may be updated during the lifetime of the material - especially if it reloads or base template changes
	virtual MaterialDataProxyPtr dataProxy() const = 0;

    // data proxy for this material - contains parameters required to render this material
    // NOTE: the proxy pointer itself may be updated during the lifetime of the material - especially if it reloads or base template changes
    virtual MaterialTemplateProxyPtr templateProxy() const = 0;

    ///---

    //! get the material template this material is based on
    /// NOTE: for templates it returns the same object :)
    /// NOTE: may be null for VERY broken materials
    virtual const MaterialTemplate* resolveTemplate() const = 0;

    //! check if parameter value differs from the base value (whatever that base seems to be)
    //! NOTE: for material templates this always returns false as, by definition, all template values are non-overridable
    virtual bool checkParameterOverride(StringID name) const = 0;

    //! reset parameter to default value
    virtual bool resetParameter(StringID name) = 0;

    //! write value of material parameter
    //! NOTE: data buffer must match the type of the param
    virtual bool writeParameter(StringID name, const void* data, Type type, bool refresh = true) = 0;

    //! read current value of parameter
    virtual bool readParameter(StringID name, void* data, Type type) const = 0;

    //! read base value of parameter, NOTE: the value is read only if the type matches our own definition of the same parameter
    //! NOTE: for material templates this reads the the same value as readParameter
    virtual bool readBaseParameter(StringID name, void* data, Type type) const = 0;

    //--

    // raw interface - find (recursively) the pointer to parameter data holder, IF IT EXISTS
    // NOTE: this checks in the base material as well
    virtual const void* findParameterDataInternal(StringID name, Type& dataType) const = 0;

    // raw interface - find (recursively) the pointer to parameter data holder, IF IT EXISTS
    virtual const void* findBaseParameterDataInternal(StringID name, Type& dataType) const = 0;

    ///---

    template< typename T >
    INLINE bool writeParameterTyped(StringID name, const T& data, bool refresh = true)
    {
        static_assert(!std::is_pointer<T>::value, "Pointer type is unexpected here");
        static_assert(!std::is_same<T, Variant>::value, "Variant should not be used here, use the real value or use the writeParameter");
        return writeParameter(name, &data, reflection::GetTypeObject<T>(), refresh);
    }

    template< typename T >
    INLINE bool readParameterTyped(StringID name, T& data) const
    {
        static_assert(!std::is_pointer<T>::value, "Pointer type is unexpected here");
        static_assert(!std::is_same<T, Variant>::value, "Variant should not be used here, use the real value or use the readParameter");
        return readParameter(name, &data, reflection::GetTypeObject<T>());
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
    RefWeakPtr<IMaterial> m_trackedMaterial;

    // materials that depend on this material
    // NOTE: editor only, not saved
    Array<RefWeakPtr<IMaterial>> m_runtimeDependencies;
    Mutex m_runtimeDependenciesLock;

    //--

    // notify all dependencies that the data in this material changed
    virtual void notifyDataChanged();

    // notify all dependencies that the base material changed and some stuff will have to be rebuilt
    virtual void notifyBaseMaterialChanged();
};

///---

END_BOOMER_NAMESPACE()
