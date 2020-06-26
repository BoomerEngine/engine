/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"
#include "resourceLoaderFinal.h"
#include "resourceUncached.h"

#include "base/app/include/commandline.h"
#include "base/io/include/ioSystem.h"

namespace base
{
    namespace res
    {
        //---

        RTTI_BEGIN_TYPE_CLASS(ResourceLoaderFinal);
        RTTI_END_TYPE();

        //---

        ResourceLoaderFinal::~ResourceLoaderFinal()
        {}

        bool ResourceLoaderFinal::initialize(const app::CommandLine& cmdLine)
        {
            if (cmdLine.hasParam("cookedDir"))
            {
                m_looseFileDir = io::AbsolutePath::BuildAsDir(cmdLine.singleValueUTF16("cookedDir"));
                TRACE_INFO("Cooked files directory: '{}'", m_looseFileDir);
            }
            else
            {
                m_looseFileDir = IO::GetInstance().systemPath(io::PathCategory::ExecutableDir).addDir("cooked");

                // LOAD packages
                //TRACE_ERROR("No cooked packages found");
                //return false;
            }

            if (!buildLoadingExtensionMap())
                return false;

            return true;
        }

        void ResourceLoaderFinal::update()
        {
            // nothing here
        }

        bool ResourceLoaderFinal::buildLoadingExtensionMap()
        {
            InplaceArray<SpecificClassType<res::IResource>, 20> allResourceClasses;
            RTTI::GetInstance().enumClasses(allResourceClasses);

            for (const auto cls : allResourceClasses)
            {
                if (const auto loadingExtensionMetadata = cls->findMetadata<res::ResourceExtensionMetadata>())
                {
                    const auto loadingExtension = StringBuf(loadingExtensionMetadata->extension());
                    if (loadingExtension)
                    {
                        auto currentCls = cls;
                        while (currentCls)
                        {
                            auto& entry = m_loadingExtensionsMap[currentCls].emplaceBack();
                            entry.cls = cls;
                            entry.extension = loadingExtension;
                            TRACE_ERROR("Resource '{}' is loadable from '{}' as class '{}'", currentCls->name(), loadingExtension, cls->name());

                            currentCls = currentCls->baseClass().cast<res::IResource>();
                        }
                    }
                }
            }

            return true;
        }

        bool ResourceLoaderFinal::assembleCookedFilePath(const ResourceKey& key, io::AbsolutePath& outPath) const
        {
            const auto* entryTable = m_loadingExtensionsMap.find(key.cls());
            if (!entryTable || entryTable->empty())
                return false;

            const auto& ext = (*entryTable)[0].extension;
            
            outPath = m_looseFileDir.addFile(key.path().path()).addExtension(ext);
            return true;
        }

        ResourceHandle ResourceLoaderFinal::loadResourceOnce(const ResourceKey& key)
        {
            // try to load a loose file first
            if (!m_looseFileDir.empty())
            {
                io::AbsolutePath filePath;
                assembleCookedFilePath(key, filePath);

                if (auto ret = res::LoadUncached(filePath, key.cls(), this, nullptr))
                {
                    ret->bindToLoader(this, key, ResourceMountPoint(), false);
                    return ret;
                }
            }

            return nullptr;
        }

        //---

    } // res
} // base

