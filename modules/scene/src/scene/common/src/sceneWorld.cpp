/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "sceneWorld.h"
#include "sceneEntity.h"
#include "sceneLayer.h"

#include "base/resources/include/resourceFactory.h"
#include "base/resources/include/resourceSerializationMetadata.h"
#include "base/depot/include/depotStructure.h"
#include "base/resources/include/resourceUncached.h"

namespace scene
{
    ///----

    // factory class for the scene world
    class SceneWorldFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneWorldFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<World>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(SceneWorldFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<World>();
    RTTI_END_TYPE();

    ///----

    RTTI_BEGIN_TYPE_STRUCT(WorldSectorDesc);
        RTTI_PROPERTY(m_alwaysLoaded);
        RTTI_PROPERTY(m_name);
        RTTI_PROPERTY(m_streamingBox);
    RTTI_END_TYPE();

    ///----

    RTTI_BEGIN_TYPE_CLASS(WorldSector);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4sector");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("World Sector");
        RTTI_PROPERTY(m_entities);
    RTTI_END_TYPE();

    WorldSector::WorldSector()
    {}

    WorldSector::~WorldSector()
    {}

    ///----

	RTTI_BEGIN_TYPE_CLASS(World);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4world");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("World");
        RTTI_PROPERTY(m_parameters);
        RTTI_OLD_NAME("scene::world::World");
    RTTI_END_TYPE();

    //--

    World::World()
    {
        if (!base::rtti::IClassType::IsDefaultObjectCreation())
        {
            m_parameters = base::CreateSharedPtr<WorldParameterContainer>();
            m_parameters->parent(sharedFromThis());
        }
    }

    void World::collectLayerPaths(base::depot::DepotStructure& loader, const base::StringBuf& directory, base::Array<base::res::ResourcePath>& outLayerPaths) const
    {
        static auto layerExt = base::res::IResource::GetResourceExtensionForClass(Layer::GetStaticClass());

        {
            base::Array<base::depot::DepotStructure::FileInfo> allFiles;
            loader.enumFilesAtPath(directory, allFiles);

            for (auto& info : allFiles)
            {
                if (info.name.view().endsWith(layerExt))
                {
                    base::StringBuf layerPath = base::TempString("{}{}", directory, info.name);
                    outLayerPaths.pushBack(base::res::ResourcePath(layerPath));
                }
            }
        }

        {
            base::Array<base::depot::DepotStructure::DirectoryInfo> allDirs;
            loader.enumDirectoriesAtPath(directory, allDirs);

            for (auto& info : allDirs)
            {
                base::StringBuf childDirPath = base::TempString("{}{}/", directory, info.name);
                collectLayerPaths(loader, childDirPath, outLayerPaths);
            }
        }
    }

    void World::collectLayerPaths(base::depot::DepotStructure& loader, base::Array<base::res::ResourcePath>& outLayerPaths) const
    {
        if (path().empty())
            return;

        auto worldDirectory = path().path().beforeLast("/");
        base::StringBuf layerDirectory = base::TempString("{}/layers/", worldDirectory);

        collectLayerPaths(loader, layerDirectory, outLayerPaths);
    }

    ///----

    RTTI_BEGIN_TYPE_CLASS(CompiledWorld);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4compiled");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Compiled World");
        RTTI_PROPERTY(m_sectors);
    RTTI_END_TYPE();

    CompiledWorld::CompiledWorld()
    {
    }

    void CompiledWorld::content(const base::RefPtr<WorldParameterContainer>& params, const TSectorInfos& sectors)
    {
        if (params)
        {
            if (auto paramClone = base::rtti_cast<WorldParameterContainer>(params))
                m_parameters = paramClone;
        }

        m_sectors = sectors;

        for (auto& sector : m_sectors)
            if (sector.m_unsavedSectorData)
                sector.m_unsavedSectorData->parent(sharedFromThis());
    }

    bool CompiledWorld::save(base::depot::DepotStructure& loader, const base::StringBuf& worldPath)
    {
        base::res::ResourceMountPoint mountPoint;
        if (!loader.queryFileMountPoint(worldPath, mountPoint))
        {
            TRACE_ERROR("Unable to determine mount point for path '{}'", worldPath);
            return false;
        }

        // serialize new content to a buffer
        auto buffer = base::res::SaveUncachedToBuffer(sharedFromThisType<CompiledWorld>(), mountPoint);
        if (!buffer)
        {
            TRACE_WARNING("Failed to serialize content of file '{}'", worldPath);
            return false;
        }
        else if (loader.storeFileContent(worldPath, buffer))
        { 
            TRACE_WARNING("Failed to save content of file '{}'", worldPath);
            return false;
        }

        // get world directory
        auto cookedDirectory = worldPath.stringBeforeLastNoCase("/");

        // sectors
        for (auto& sector : m_sectors)
        {
            if (sector.m_unsavedSectorData)
            {
                auto sectorFileExtension = base::res::IResource::GetResourceExtensionForClass(WorldSector::GetStaticClass());
                base::StringBuf sectorPath = base::TempString("{}/{}.{}", cookedDirectory, sector.m_name, sectorFileExtension);

                auto buffer = base::res::SaveUncachedToBuffer(sector.m_unsavedSectorData, mountPoint);
                if (!buffer)
                {
                    TRACE_WARNING("Failed to serialize content of file '{}'", sectorPath);
                    return false;
                }
                else if (loader.storeFileContent(sectorPath, buffer))
                {
                    TRACE_WARNING("Failed to save content of file '{}'", worldPath);
                    return false;
                }

                sector.m_unsavedSectorData.reset();
            }
        }

        // all saved
        return true;
    }

    ///----
    
} // scene
