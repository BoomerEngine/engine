/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\fragments #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {
        ///--

        struct FrameFragmentRenderStats;
        class FrameRenderer;

        ///--

        /// a render list element - usually a reference to a fragment in the fragment handler
        struct Fragment
        {
            FragmentHandlerType type = FragmentHandlerType::None; // index to the data handler that should handle rendering of this fragment
            uint8_t flags = 0; // generic flag
            uint16_t renderDistance = 0; // set only for transparent fragments
        };

        //---

        /// Helper class to iterate over fragments
        template< typename T >
        class FragmentIterator : public base::NoCopy
        {
        public:
            INLINE FragmentIterator(const Fragment* const* fragmentList, uint32_t numFragments)
                : m_fragments(fragmentList)
                , m_size(numFragments)
                , m_index(0)
            {}

            INLINE operator bool() const
            {
                return m_index < m_size;
            }

            INLINE uint32_t size() const
            {
                return m_size;
            }

            INLINE uint32_t pos() const
            {
                return m_index;
            }

            INLINE const T* advance()
            {
                if (++m_index < m_size)
                    return (const T*)m_fragments[m_index];
                return nullptr;
            }

            INLINE const T* operator->() const
            {
                if (m_index < m_size)
                    return (const T*)(m_fragments[m_index]);
                else
                    return nullptr;
            }

            INLINE void operator++()
            {
                if (m_index < m_size)
                    m_index += 1;
            }

        private:
            const Fragment* const* m_fragments;
            const uint32_t m_size;
            uint32_t m_index;
        };

        ///--

        /// rendering context for fragments
        struct FragmentRenderContext
        {
            MaterialPass pass = MaterialPass::Forward;
            const FilterFlags* filterFlags = nullptr;
            uint32_t msaaCount = 1;
            CompareOp depthCompare = CompareOp::LessEqual;
            bool allowsCustomRenderStates = true;
        };

        ///--

        /// a handler of fragments
        class RENDERING_SCENE_API IFragmentHandler : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IFragmentHandler);

        public:
            IFragmentHandler(FragmentHandlerType type);
            virtual ~IFragmentHandler();

            //--

            INLINE const FragmentHandlerType type() const { return m_type; }

            //--

            // initialize fragment handle for given scene
            virtual void handleInit(Scene* scene);

            // scene is being locked for rendering, lock/swap/fetch rendering data
            virtual void handleSceneLock();

            // scene was unlocked after rendering
            virtual void handleSceneUnlock();

            // prepare all data needed for rendering fragments in this frame, called once at the start of frame rendering
            // NOTE: this is the last moment we can modify the data - all other collect/rendering calls should be read only as they will run on jobs
            virtual void handlePrepare(command::CommandWriter& cmd);

            //--

            // render fragments from render list
            virtual void handleRender(command::CommandWriter& cmd,  const FrameView& view, const FragmentRenderContext& context, const Fragment* const* fragments, uint32_t numFragments, FrameFragmentRenderStats& outStats) const = 0;

            //--

        private:
            FragmentHandlerType m_type;
        };

        //---

        /// get the static handler ID
        template< typename T >
        static uint8_t GetHandlerIndex()
        {
            static const uint8_t id = (uint8_t)T::GetStaticClass()->userIndex();
            return id;
        }

        ///--
        
    } // scene
} // rendering

