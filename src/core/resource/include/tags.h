/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "core/object/include/rttiMetadata.h"

BEGIN_BOOMER_NAMESPACE()

//-----

/// version of the resource class internal serialization, if this does not match the resource we recook the resource
class CORE_RESOURCE_API ResourceDataVersionMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceDataVersionMetadata, IMetadata);

public:
    ResourceDataVersionMetadata();

    INLINE uint32_t version() const
    {
        return m_version;
    }

    INLINE void version(uint32_t version)
    {
        m_version = version;
    }

private:
    uint32_t m_version;
};

//------

// Resource description (class metadata)
class CORE_RESOURCE_API ResourceDescriptionMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceDescriptionMetadata, IMetadata);

public:
    ResourceDescriptionMetadata();

    INLINE ResourceDescriptionMetadata& description(const char* desc)
    {
        m_desc = desc;
        return *this;
    }

    INLINE const char* description() const { return m_desc; }

private:
    const char* m_desc;
};

//--

/// metadata for raw resource to specify the target (imported) class
class CORE_RESOURCE_API ResourceImportedClassMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceImportedClassMetadata, IMetadata);

public:
    ResourceImportedClassMetadata();

    template< typename T >
    INLINE ResourceImportedClassMetadata& addClass()
    {
        static_assert(std::is_base_of< IResource, T >::value, "Only resource classes can be specified here");
        m_classes.pushBackUnique(T::GetStaticClass());
        return *this;
    }

    INLINE const Array<SpecificClassType<IResource>>& classList() const
    {
        return m_classes;
    }

private:
    Array<SpecificClassType<IResource>> m_classes;
};

//---

/// metadata for input format supported for cooking (obj, jpg, etc)
class CORE_RESOURCE_API ResourceSourceFormatMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceSourceFormatMetadata, IMetadata);

public:
    ResourceSourceFormatMetadata();

    INLINE ResourceSourceFormatMetadata& addSourceExtension(const char* ext)
    {
        if (ext && *ext)
            m_extensions.pushBackUnique(ext);
        return *this;
    }

    INLINE ResourceSourceFormatMetadata& addSourceExtensions(const char* extList)
    {
        InplaceArray<StringView, 50> list;
        StringView(extList).slice(";,", false, list);

        for (const auto& ext : list)
            m_extensions.pushBackUnique(ext);
        return *this;
    }

    INLINE const Array<StringView>& extensions() const
    {
        return m_extensions;
    }

private:
    Array<StringView> m_extensions;
    Array<ClassType> m_classes;
};

//---

/// version of the cooker class, if this does not match the resource we recook the resource
class CORE_RESOURCE_API ResourceCookerVersionMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceCookerVersionMetadata, IMetadata);

public:
    ResourceCookerVersionMetadata();

    INLINE uint32_t version() const
    {
        return m_version;
    }

    INLINE void version(uint32_t version)
    {
        m_version = version;
    }

private:
    uint32_t m_version;
};

//---

END_BOOMER_NAMESPACE()
