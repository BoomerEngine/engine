/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "base/object/include/rttiResourceReferenceType.h"

BEGIN_BOOMER_NAMESPACE(base::res)

//--

/// Type for a resource reference
class BASE_RESOURCE_API ResourceAsyncRefType : public rtti::IResourceReferenceType
{
public:
    ResourceAsyncRefType(SpecificClassType<IResource> classType);
    virtual ~ResourceAsyncRefType();

    /// get the class we are pointing to
    INLINE SpecificClassType<IResource> resourceClass() const { return m_resourceClass; }

    /// read the data 
    void readResourceRef(const void* data, BaseAsyncReference& outResRef) const;

    /// write the data 
    void writeResourceRef(void* data, const BaseAsyncReference& resRef) const;

    //----

    virtual void construct(void* object) const override final;
    virtual void destruct(void* object) const override final;

    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    virtual void writeBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeWriter& file, const void* data, const void* defaultData) const override final;
    virtual void readBinary(rtti::TypeSerializationContext& typeContext, stream::OpcodeReader& file, void* data) const override final;

    virtual void writeXML(rtti::TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
    virtual void readXML(rtti::TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

    //----

    virtual ClassType referenceResourceClass() const override final;
    /*virtual void referenceReadResource(const void* data, RefPtr<res::IResource>& outRef) const override final;
    virtual void referenceWriteResource(void* data, res::IResource* resource) const override final;*/
    virtual bool referencePatchResource(void* data, res::IResource* currentResource, res::IResource* newResources) const override final;

    //----

    static const char* TypePrefix;
    static Type ParseType(StringParser& typeNameString, rtti::TypeSystem& typeSystem);

private:
    SpecificClassType<IResource> m_resourceClass;
};

//---

extern BASE_RESOURCE_API StringID FormatAsyncRefTypeName(StringID className);
extern BASE_RESOURCE_API const ResourceAsyncRefType* CreateAsyncRefType(SpecificClassType<IResource> resourceClass);

//---

END_BOOMER_NAMESPACE(base::res)
