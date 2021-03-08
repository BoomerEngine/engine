/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

struct FileLoadingContext;
struct FileLoadingResult;
struct FileLoadingDependency;

/// custom resource loader
class CORE_RESOURCE_API ICustomFileLoader : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICustomFileLoader);

public:
    virtual ~ICustomFileLoader();

    /// load content from a file
    virtual bool loadContent(IAsyncFileHandle* file, const FileLoadingContext& context, FileLoadingResult& outResult) const = 0;

    /// collect file dependencies only (ie. other files we will need)
    virtual bool loadDependencies(IAsyncFileHandle* file, const FileLoadingContext& context, Array<FileLoadingDependency>& outDependencies) const = 0;
};

//--

/// metadata for custom file loader indicating what kind of extension it supports
class CORE_RESOURCE_API CustomFileLoaderSupportedExtensionMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(CustomFileLoaderSupportedExtensionMetadata, IMetadata);

public:
    CustomFileLoaderSupportedExtensionMetadata();

    INLINE CustomFileLoaderSupportedExtensionMetadata& extension(StringView ext)
    {
        if (ext)
            m_extensions.pushBackUnique(ext);
        return *this;
    }

    INLINE const Array<StringView>& extensions() const { return m_extensions; }

private:
    Array<StringView> m_extensions;
};

//--

// enumerate all file extensions supported by custom loaders
extern CORE_RESOURCE_API void EnumerateCustomFileExtensions(Array<StringView>& outExtensions);

// find custom loader class that supports given extension
extern CORE_RESOURCE_API SpecificClassType<ICustomFileLoader> FindCustomLoaderForFileExtension(StringView ext);

//--

END_BOOMER_NAMESPACE()
