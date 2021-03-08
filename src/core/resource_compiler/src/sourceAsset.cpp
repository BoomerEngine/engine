/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "sourceAsset.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISourceAsset);
RTTI_END_TYPE();

ISourceAsset::ISourceAsset()
{}

ISourceAsset::~ISourceAsset()
{}
        
//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISourceAssetLoader);
RTTI_END_TYPE();

ISourceAssetLoader::ISourceAssetLoader()
{}

ISourceAssetLoader::~ISourceAssetLoader()
{}

///---

class SoruceAssetLoaderRegistry : public ISingleton
{
    DECLARE_SINGLETON(SoruceAssetLoaderRegistry);

public:
    SoruceAssetLoaderRegistry()
    {
        initClassMap();
    }

    const ISourceAssetLoader* findLoader(StringView importPath) const
    {
        const auto ext = importPath.afterLast(".");

        RefPtr<ISourceAssetLoader> ret;
        m_loaderMap.find(ext, ret);
        return ret;
    }

private:
    HashMap<CaseInsensitiveKey, RefPtr<ISourceAssetLoader>> m_loaderMap;

    void initClassMap()
    {
        InplaceArray<SpecificClassType<ISourceAssetLoader>, 32> loaderClasses;
        RTTI::GetInstance().enumClasses(loaderClasses);

        for (auto cls : loaderClasses)
        {
            if (auto metadata = cls->findMetadata<ResourceSourceFormatMetadata>())
            {
                const auto loader = cls.create();

                for (const auto ext : metadata->extensions())
                {
                    if (m_loaderMap.contains(ext))
                    {
                        TRACE_ERROR("Source asset loader for '{}' is already registered with class '{}'. Adding '{}' not possible.", 
                            ext, m_loaderMap[ext]->cls()->name(), cls->name());
                    }
                    else
                    {
                        TRACE_INFO("Registered source asset loader for '{}' with class '{}'", ext, cls->name());
                        m_loaderMap[ext] = loader;
                    }
                }
            }
        }
    }

    virtual void deinit() override
    {
        m_loaderMap.clear();
    }
};

SourceAssetPtr ISourceAssetLoader::LoadFromMemory(StringView importPath, StringView contextPath, Buffer data)
{
    if (const auto* loader = SoruceAssetLoaderRegistry::GetInstance().findLoader(importPath))
        return loader->loadFromMemory(importPath, contextPath, data);

    return nullptr;
}

//--

END_BOOMER_NAMESPACE()
