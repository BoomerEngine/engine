/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resource #]
***/

#include "build.h"
#include "loader.h"
#include "loadingService.h"

#include "core/resource/include/loader.h"
#include "core/app/include/configService.h"
#include "core/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(LoadingService);
RTTI_END_TYPE();

LoadingService::LoadingService()
{}

app::ServiceInitializationResult LoadingService::onInitializeService(const app::CommandLine& cmdLine)
{
    m_resourceLoader = RefNew<ResourceLoader>();
    return app::ServiceInitializationResult::Finished;
}

void LoadingService::onShutdownService()
{
    m_resourceLoader.reset();
}

void LoadingService::onSyncUpdate()
{
    releaseRetainedFiles();
    m_resourceLoader->processReloadEvents();
}

void LoadingService::releaseRetainedFiles()
{
    m_retainedFilesLock.acquire();
    auto files = std::move(m_retainedFiles);
    m_retainedFilesLock.release();
}

void LoadingService::addRetainedFile(IResource* file)
{
    if (file)
    {
        auto lock = CreateLock(m_retainedFilesLock);
        m_retainedFiles.pushBack(AddRef(file));
    }

}

//--

ResourcePtr LoadingService::loadResource(const ResourceID& id, ClassType expectedClassType)
{
    // TODO!
    return nullptr;
}

//--

ResourcePtr LoadingService::loadResource(StringView path, ClassType expectedClassType)
{
    if (path.empty())
        return nullptr;

    return m_resourceLoader->loadResource(path, expectedClassType);
}

//--

ResourceLoader* GlobalLoader()
{
    return GetService<LoadingService>()->loader();
}

ResourcePtr LoadResource(StringView path, ClassType expectedClass)
{
    return GetService<LoadingService>()->loadResource(path, expectedClass);
}

ResourcePtr LoadResource(const ResourceID& id, ClassType expectedClass)
{
    return GetService<LoadingService>()->loadResource(id, expectedClass);
}

//--

END_BOOMER_NAMESPACE()
