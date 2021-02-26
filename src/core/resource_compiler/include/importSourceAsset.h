/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(res)

//--
        
/// semi-cachable source asset that is used in resource importing
/// NOTE: the source assets MAY be cached (if allowed) to speed up reimport but the usual scenario is that they are kept in the memory for some time to prevent the need of reparsing the file
class CORE_RESOURCE_COMPILER_API ISourceAsset : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ISourceAsset, IObject);

public:
    ISourceAsset();
    virtual ~ISourceAsset();

    ///---

    /// is the asset file-cachable ?
    virtual bool shouldCacheInFile() const { return false; }

    /// is the asset memory-cachable ?
    virtual bool shouldCacheInMemory() const { return true; }

    /// calculate estimated memory size of the asset
    virtual uint64_t calcMemoryUsage() const = 0;

    ///---
};

//--

/// loader for source assets
class CORE_RESOURCE_COMPILER_API ISourceAssetLoader : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISourceAssetLoader);

public:
    ISourceAssetLoader();
    virtual ~ISourceAssetLoader();

    /// load source asset from memory buffer
    virtual SourceAssetPtr loadFromMemory(StringView importPath, StringView contextPath, Buffer data) const = 0;

    ///---

    /// load source asset from buffer
    static SourceAssetPtr LoadFromMemory(StringView importPath, StringView contextPath, Buffer data);

    ///---
};

//--

END_BOOMER_NAMESPACE_EX(res)
