/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "renderingFrameFilters.h"

namespace rendering
{
    namespace scene
    {
        //--

        FilterFlags::FilterFlags()
        {
            //*this = DefaultGame();
            memset(m_bits, 0, sizeof(m_bits));
        }

        FilterFlags::FilterFlags(const FilterFlags& other) = default;
        FilterFlags::FilterFlags(FilterFlags&& other) = default;
        FilterFlags& FilterFlags::operator=(const FilterFlags& other) = default;
        FilterFlags& FilterFlags::operator=(FilterFlags&& other) = default;

        //--

        static FilterFlags MakeDefaultGameFilter()
        {
            FilterFlags ret;

            ret |= FilterBit::Meshes;
            ret |= FilterBit::CascadeShadows;

            ret |= FilterBit::PassClear;
            ret |= FilterBit::PassDepthPrepass;
            ret |= FilterBit::PassShadowDepth;
            ret |= FilterBit::PassForward;

            ret |= FilterBit::FragOpaqueNonMovable;
            ret |= FilterBit::FragOpaqueSolid;
            ret |= FilterBit::FragOpaqueMasked;
            ret |= FilterBit::FragTransparent;

            ret |= FilterBit::Lighting;
            ret |= FilterBit::Lighting_Global;
            ret |= FilterBit::Lighting_IBL;

            ret |= FilterBit::PostProcessing;
            ret |= FilterBit::PostProcesses_ToneMap;

            return ret;
        }

        const FilterFlags& FilterFlags::DefaultGame()
        {
            static FilterFlags theResult = MakeDefaultGameFilter();
            return theResult;
        }

        static FilterFlags MakeDefaultEditorFilter()
        {
            FilterFlags ret = MakeDefaultGameFilter();

            ret |= FilterBit::PassDepthSelection;
            ret |= FilterBit::PassSelectionFragments;
            ret |= FilterBit::PassOverlay;

            ret |= FilterBit::DebugGeometry;
            ret |= FilterBit::DebugGeometryLines;
            ret |= FilterBit::DebugGeometrySolid;
            ret |= FilterBit::DebugGeometryScreen;
            ret |= FilterBit::DebugGeometryTransparent;
            ret |= FilterBit::DebugGeometryOverlay;

            ret |= FilterBit::ViewportCameraInfo;
            ret |= FilterBit::ViewportCameraAxes; // small camera orientation axes in the BL
            ret |= FilterBit::ViewportWorldGrid; // world grid at Z=0
            ret |= FilterBit::ViewportWorldAxes; // bit, one meter axes at [0,0,0]

            ret |= FilterBit::PostProcesses_SelectionHighlight;
            ret |= FilterBit::PostProcesses_SelectionOutline;

            return ret;
        }

        const FilterFlags& FilterFlags::DefaultEditor()
        {
            static FilterFlags theResult = MakeDefaultEditorFilter();
            return theResult;
        }

        //--

        class FilterBitRegistry : public base::ISingleton
        {
            DECLARE_SINGLETON(FilterBitRegistry);

        public:
            FilterBitRegistry()
            {
                m_root = create(nullptr, "Filters");

                {
                    auto* parent = create(m_root, "Main");
                    create(parent, "Meshes", FilterBit::Meshes);
                    create(parent, "CascadeShadows", FilterBit::CascadeShadows);
                }

                {
                    auto* parent = create(m_root, "Pass");
                    create(parent, "Clear", FilterBit::PassClear);
                    create(parent, "DepthPrepass", FilterBit::PassDepthPrepass);
                    create(parent, "DepthSelection", FilterBit::PassDepthSelection);
                    create(parent, "SelectionFragments", FilterBit::PassSelectionFragments);
                    create(parent, "ShadowDepth", FilterBit::PassShadowDepth);
                    create(parent, "Forward", FilterBit::PassForward);
                    create(parent, "Overlay", FilterBit::PassOverlay);
                }

                {
                    auto* parent = create(m_root, "Fragments");
                    create(parent, "OpaqueNonMovable", FilterBit::FragOpaqueNonMovable);
                    create(parent, "OpaqueSolid", FilterBit::FragOpaqueSolid);
                    create(parent, "OpaqueMasked", FilterBit::FragOpaqueMasked);
                    create(parent, "Transparent", FilterBit::FragTransparent);
                }

                {
                    auto* parent = create(m_root, "DebugGeometry", FilterBit::DebugGeometry);
                    create(parent, "Solid", FilterBit::DebugGeometrySolid);
                    create(parent, "Lines", FilterBit::DebugGeometryLines);
                    create(parent, "Transparent", FilterBit::DebugGeometryTransparent);
                    create(parent, "Screen", FilterBit::DebugGeometryScreen);
                    create(parent, "Overlay", FilterBit::DebugGeometryOverlay);
                }

                {
                    auto* parent = create(m_root, "DebugViewport");
                    create(parent, "CameraInfo", FilterBit::ViewportCameraInfo);
                    create(parent, "CameraAxes", FilterBit::ViewportCameraAxes);
                    create(parent, "SafeArea", FilterBit::ViewportSafeArea);
                    create(parent, "WorldGrid", FilterBit::ViewportWorldGrid);
                    create(parent, "WorldAxes", FilterBit::ViewportWorldAxes);
                }

                {
                    auto* parent = create(m_root, "Material");
                    create(parent, "DisableLighting", FilterBit::Material_DisableLighting);
                    create(parent, "DisableColorMap", FilterBit::Material_DisableColorMap);
                    create(parent, "DisableObjectColor", FilterBit::Material_DisableObjectColor);
                    create(parent, "DisableVertexColor", FilterBit::Material_DisableVertexColor);
                    create(parent, "DisableTextures", FilterBit::Material_DisableTextures);
                    create(parent, "DisableNormals", FilterBit::Material_DisableNormals);
                    create(parent, "DisableMasking", FilterBit::Material_DisableMasking);
                    create(parent, "DisableVertexMotion", FilterBit::Material_DisableVertexMotion);
                }

                {
                    auto* parent = create(m_root, "POSTFX", FilterBit::PostProcessing);
                    create(parent, "Tonemap", FilterBit::PostProcesses_ToneMap);
                    create(parent, "SelectionOutline", FilterBit::PostProcesses_SelectionOutline);
                    create(parent, "SelectionHighlight", FilterBit::PostProcesses_SelectionHighlight);
                }
            }

            base::Array<FilterBitInfo*> m_infos;
            FilterBitInfo* m_root = nullptr;

            FilterBitInfo* create(FilterBitInfo* parent, base::StringView<char> name, FilterBit bit = FilterBit::MAX)
            {
                auto* ret = MemNew(FilterBitInfo).ptr;
                ret->name = base::StringID(name);
                ret->bit = bit;

                if (parent)
                    parent->children.pushBack(ret);

                return ret;            
            }

            virtual void deinit()
            {
                m_infos.clearPtr();
                m_root = nullptr;
            }

        };

        // get extra info about a filter bit
        const FilterBitInfo* GetFilterTree()
        {
            return FilterBitRegistry::GetInstance().m_root;
        }

        //

    } // rendering
} // scene
