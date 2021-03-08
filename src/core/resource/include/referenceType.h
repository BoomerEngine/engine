/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\reference #]
***/

#pragma once

#include "core/object/include/rttiMetadata.h"
#include "core/object/include/rttiResourceReferenceType.h"

BEGIN_BOOMER_NAMESPACE()

class IResource;

//--

/// Type for a resource reference
class CORE_RESOURCE_API ResourceRefType : public IResourceReferenceType
{
public:
    ResourceRefType(SpecificClassType<IResource> classType);
    virtual ~ResourceRefType();

    /// get the class we are pointing to
    INLINE SpecificClassType<IResource> resourceClass() const { return m_resourceClass; }

    /// read the data 
    void readResourceRef(const void* data, BaseReference& outResRef) const;

    /// write the data 
    void writeResourceRef(void* data, const BaseReference& resRef) const;

    //----

    // IResourceReferenceType
    virtual ClassType referenceResourceClass() const override final;
    /*virtual void referenceReadResource(const void* data, RefPtr<IResource>& outRef) const override final;
    virtual void referenceWriteResource(void* data, IResource* resource) const override final;*/
    virtual bool referencePatchResource(void* data, IResource* currentResource, IResource* newResources) const override final;

    //----

    virtual void construct(void* object) const override final;
    virtual void destruct(void* object) const override final;

    virtual bool compare(const void* data1, const void* data2) const override final;
    virtual void copy(void* dest, const void* src) const override final;

    virtual void writeBinary(TypeSerializationContext& typeContext, SerializationWriter& file, const void* data, const void* defaultData) const override final;
    virtual void readBinary(TypeSerializationContext& typeContext, SerializationReader& file, void* data) const override final;

    virtual void writeXML(TypeSerializationContext& typeContext, xml::Node& node, const void* data, const void* defaultData) const override final;
    virtual void readXML(TypeSerializationContext& typeContext, const xml::Node& node, void* data) const override final;

    //----

    static const char* TypePrefix;
    static Type ParseType(StringParser& typeNameString, TypeSystem& typeSystem);

private:
    SpecificClassType<IResource>  m_resourceClass;
};

//---

extern CORE_RESOURCE_API StringID FormatRefTypeName(StringID className);

//---

END_BOOMER_NAMESPACE()
