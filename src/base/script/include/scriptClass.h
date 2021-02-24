/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

#include "base/object/include/rttiClassType.h"
#include "base/object/include/rttiProperty.h"
#include "base/containers/include/inplaceArray.h"

BEGIN_BOOMER_NAMESPACE(base::script)

//----

// scripted property
class BASE_SCRIPT_API ScriptedProperty : public rtti::Property
{
public:
    ScriptedProperty(Type parent, const rtti::PropertySetup& setup);
    virtual ~ScriptedProperty();

    // bind offset to scripted property
    void bindScriptedPropertyOffset(uint32_t offset);
};

//----

// scripted property
class BASE_SCRIPT_API ScriptedClassProperty : public ScriptedProperty
{
public:
    ScriptedClassProperty(Type parent, const rtti::PropertySetup& setup);
    virtual ~ScriptedClassProperty();

    /// get property data given the pointer to the object
    //virtual const void* offsetPtr(const void* data) const override final;
    //virtual void* offsetPtr(void* data) const override final;
};

//----

// scripted class type that derives from native type
class BASE_SCRIPT_API ScriptedClass : public rtti::IClassType
{
public:
    ScriptedClass(StringID name, ClassType nativeClass);

    /// rtti::IClassType
    virtual const void* createDefaultObject() const  override final;
    virtual const void* defaultObject() const override final;
    virtual void destroyDefaultObject() const override final;
    virtual bool isAbstract() const override final;
    virtual void construct(void *object) const override final;
    virtual void destruct(void *object) const override final;
    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    /// destroy scripted only data
    void destroyScriptedObject(void* object) const;

    /// update size of the script data in class, returns true if size changed
    bool updateScriptedDataSize();

    /// mark as abstract
    void markAsAbstract();

    /// clear class for script reloading
    void clearForReloading();

    /// bind internal functions
    void bindFunctions();

private:
    ClassType m_nativeClass;
    uint32_t m_scriptedDataSize;
    uint32_t m_scriptedDatAlignment;
    bool m_scriptedAbstract;

    mutable void* m_defaultObject;

    const rtti::Function* m_functionCtor;
    const rtti::Function* m_functionDtor;
};

//----

// scripted struct
class BASE_SCRIPT_API ScriptedStruct : public rtti::IClassType
{
public:
    ScriptedStruct(StringID name);

    /// rtti::IClassType
    virtual const void* createDefaultObject() const  override final;
    virtual const void* defaultObject() const override final;
    virtual void destroyDefaultObject() const override final;
    virtual bool isAbstract() const override final;
    virtual void construct(void *object) const override final;
    virtual void destruct(void *object) const override final;
    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    /// try to compute size and alignment, returns true if size changes
    bool updateDataSize();

    /// clear class for script reloading
    void clearForReloading();

    /// bind internal functions
    void bindFunctions();

public:
    mutable void* m_defaultObject;

    const rtti::Function* m_functionCtor;
    const rtti::Function* m_functionDtor;

    struct PropEntry
    {
        uint32_t offset = 0;
        Type type = nullptr;

        INLINE PropEntry(const rtti::Property* prop)
            : offset(prop->offset()), type(prop->type())
        {}
    };

    InplaceArray<PropEntry, 32> m_compareCopyList;
};

//--

END_BOOMER_NAMESPACE(base::script)
