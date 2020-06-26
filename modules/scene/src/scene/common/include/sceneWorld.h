/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "base/resources/include/resource.h"
#include "scene/common/include/sceneWorldParameters.h"

namespace scene
{
    //--

    /// Compiled world sector data
    class SCENE_COMMON_API WorldSector : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldSector, base::res::IResource);

    public:
        WorldSector();
        virtual ~WorldSector();

        //--

        base::Array<base::RefPtr<Entity>> m_entities;
    };

    //--

    /// Information about world sector
    struct SCENE_COMMON_API WorldSectorDesc
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(WorldSectorDesc);

    public:
        // is this sector always loaded 
        bool m_alwaysLoaded;

        // a reference to packed sector data
        base::StringBuf m_name;

        // a boundary in world that defines where sector date should be loaded
        // if a content observer is inside this volume the sector data will be loaded
        // NOTE: always loaded sectors have a streaming box as big as the whole world
        base::Box m_streamingBox;

        // unsaved sector data
        base::RefPtr<WorldSector> m_unsavedSectorData;
    };

    //--

    /// World - group of placed stuff that constitutes the gameplay world
    class SCENE_COMMON_API World : public base::res::ITextResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(World, base::res::ITextResource);

    public:
        World();
        
        /// get the global world parameters
        INLINE const base::RefPtr<WorldParameterContainer>& parameters() const { return m_parameters; }

        //--

        // collect all layer paths in this world
        void collectLayerPaths(base::depot::DepotStructure& loader, base::Array<base::res::ResourcePath>& outLayerPaths) const;

    protected:
        base::RefPtr<WorldParameterContainer> m_parameters;

        void collectLayerPaths(base::depot::DepotStructure& loader, const base::StringBuf& directore, base::Array<base::res::ResourcePath>& outLayerPaths) const;
    };

    //--

    /// Compile world
    class SCENE_COMMON_API CompiledWorld : public World
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CompiledWorld, World);

    public:
        CompiledWorld();

        /// get informations about streaming sectors in the world
        typedef base::Array<WorldSectorDesc> TSectorInfos;
        INLINE const TSectorInfos& sectors() const { return m_sectors; }

        //--

        // set world content
        void content(const base::RefPtr<WorldParameterContainer>& params, const TSectorInfos& sectors);

        // save content of compiled world
        bool save(base::depot::DepotStructure& loader, const base::StringBuf& worldPath);

    private:
        TSectorInfos m_sectors;
    };

    //--

} // scene