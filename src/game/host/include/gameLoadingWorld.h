/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

namespace game
{
    //--

    /// A world that is being loaded
    class GAME_HOST_API LoadingWorld : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(LoadingWorld, base::IObject);

    public:
        LoadingWorld(base::res::ResourcePath path);
        virtual ~LoadingWorld();

        //--

        // path to scene being loaded
        INLINE const base::res::ResourcePath& path() const { return m_path; }

        // loading status
        INLINE bool loading() const { return m_loading; }

        //--

        // aquire loaded world
        WorldPtr acquireLoadedWorld();

        //--

    private:
        std::atomic<bool> m_loading = true;

        base::res::ResourcePath m_path;
        WorldPtr m_world;

        void processInternalLoading();
    };

    //--

} // game
