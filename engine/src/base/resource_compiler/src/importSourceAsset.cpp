/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importSourceAsset.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace res
    {

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

            const ISourceAssetLoader* findLoader(StringView<char> importPath) const
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
                    if (auto metadata = cls->findMetadata<base::res::ResourceSourceFormatMetadata>())
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

        SourceAssetPtr ISourceAssetLoader::LoadFromMemory(StringView<char> importPath, StringView<char> contextPath, Buffer data)
        {
            if (const auto* loader = SoruceAssetLoaderRegistry::GetInstance().findLoader(importPath))
                return loader->loadFromMemory(importPath, contextPath, data);

            return nullptr;
        }

        //--

    } // res
} // base
