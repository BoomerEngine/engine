/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "filters.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

FrameFilterFlags::FrameFilterFlags()
{
    //*this = DefaultGame();
    memset(m_bits, 0, sizeof(m_bits));
}

FrameFilterFlags::FrameFilterFlags(const FrameFilterFlags& other) = default;
FrameFilterFlags::FrameFilterFlags(FrameFilterFlags&& other) = default;
FrameFilterFlags& FrameFilterFlags::operator=(const FrameFilterFlags& other) = default;
FrameFilterFlags& FrameFilterFlags::operator=(FrameFilterFlags&& other) = default;

//--

static FrameFilterFlags MakeDefaultGameFilter()
{
    FrameFilterFlags ret;

    ret |= FrameFilterBit::Meshes;
    ret |= FrameFilterBit::CascadeShadows;

    ret |= FrameFilterBit::PassClear;
    ret |= FrameFilterBit::PassDepthPrepass;
    ret |= FrameFilterBit::PassShadowDepth;
    ret |= FrameFilterBit::PassForward;

    ret |= FrameFilterBit::FragOpaqueNonMovable;
    ret |= FrameFilterBit::FragOpaqueSolid;
    ret |= FrameFilterBit::FragOpaqueMasked;
    ret |= FrameFilterBit::FragTransparent;

    ret |= FrameFilterBit::Lighting;
    ret |= FrameFilterBit::Lighting_Global;
    ret |= FrameFilterBit::Lighting_IBL;

    ret |= FrameFilterBit::PostProcessing;
    ret |= FrameFilterBit::PostProcesses_ToneMap;

    return ret;
}

const FrameFilterFlags& FrameFilterFlags::DefaultGame()
{
    static FrameFilterFlags theResult = MakeDefaultGameFilter();
    return theResult;
}

static FrameFilterFlags MakeDefaultEditorFilter()
{
    FrameFilterFlags ret = MakeDefaultGameFilter();

    ret |= FrameFilterBit::PassDepthSelection;
    ret |= FrameFilterBit::PassSelectionFragments;
    ret |= FrameFilterBit::PassOverlay;

    ret |= FrameFilterBit::DebugGeometry;
    ret |= FrameFilterBit::DebugGeometryLines;
    ret |= FrameFilterBit::DebugGeometrySolid;
    ret |= FrameFilterBit::DebugGeometryScreen;
    ret |= FrameFilterBit::DebugGeometryTransparent;
    ret |= FrameFilterBit::DebugGeometryOverlay;

    ret |= FrameFilterBit::ViewportCameraInfo;
    ret |= FrameFilterBit::ViewportCameraAxes; // small camera orientation axes in the BL
    ret |= FrameFilterBit::ViewportWorldGrid; // world grid at Z=0
    ret |= FrameFilterBit::ViewportWorldAxes; // bit, one meter axes at [0,0,0]

    ret |= FrameFilterBit::PostProcesses_SelectionHighlight;
    ret |= FrameFilterBit::PostProcesses_SelectionOutline;

    return ret;
}

const FrameFilterFlags& FrameFilterFlags::DefaultEditor()
{
    static FrameFilterFlags theResult = MakeDefaultEditorFilter();
    return theResult;
}

//--

class FilterBitRegistry : public ISingleton
{
    DECLARE_SINGLETON(FilterBitRegistry);

public:
    FilterBitRegistry()
    {
        m_root = create(nullptr, "Filters");

        {
            auto* parent = create(m_root, "Main");
            create(parent, "Meshes", FrameFilterBit::Meshes);
            create(parent, "CascadeShadows", FrameFilterBit::CascadeShadows);
        }

        {
            auto* parent = create(m_root, "Pass");
            create(parent, "Clear", FrameFilterBit::PassClear);
            create(parent, "DepthPrepass", FrameFilterBit::PassDepthPrepass);
            create(parent, "DepthSelection", FrameFilterBit::PassDepthSelection);
            create(parent, "SelectionFragments", FrameFilterBit::PassSelectionFragments);
            create(parent, "ShadowDepth", FrameFilterBit::PassShadowDepth);
            create(parent, "Forward", FrameFilterBit::PassForward);
            create(parent, "Overlay", FrameFilterBit::PassOverlay);
        }

        {
            auto* parent = create(m_root, "Fragments");
            create(parent, "OpaqueNonMovable", FrameFilterBit::FragOpaqueNonMovable);
            create(parent, "OpaqueSolid", FrameFilterBit::FragOpaqueSolid);
            create(parent, "OpaqueMasked", FrameFilterBit::FragOpaqueMasked);
            create(parent, "Transparent", FrameFilterBit::FragTransparent);
        }

        {
            auto* parent = create(m_root, "DebugGeometry", FrameFilterBit::DebugGeometry);
            create(parent, "Solid", FrameFilterBit::DebugGeometrySolid);
            create(parent, "Lines", FrameFilterBit::DebugGeometryLines);
            create(parent, "Transparent", FrameFilterBit::DebugGeometryTransparent);
            create(parent, "Screen", FrameFilterBit::DebugGeometryScreen);
            create(parent, "Overlay", FrameFilterBit::DebugGeometryOverlay);
        }

        {
            auto* parent = create(m_root, "DebugViewport");
            create(parent, "CameraInfo", FrameFilterBit::ViewportCameraInfo);
            create(parent, "CameraAxes", FrameFilterBit::ViewportCameraAxes);
            create(parent, "SafeArea", FrameFilterBit::ViewportSafeArea);
            create(parent, "WorldGrid", FrameFilterBit::ViewportWorldGrid);
            create(parent, "WorldAxes", FrameFilterBit::ViewportWorldAxes);
        }

        {
            auto* parent = create(m_root, "Material");
            create(parent, "DisableLighting", FrameFilterBit::Material_DisableLighting);
            create(parent, "DisableColorMap", FrameFilterBit::Material_DisableColorMap);
            create(parent, "DisableObjectColor", FrameFilterBit::Material_DisableObjectColor);
            create(parent, "DisableVertexColor", FrameFilterBit::Material_DisableVertexColor);
            create(parent, "DisableTextures", FrameFilterBit::Material_DisableTextures);
            create(parent, "DisableNormals", FrameFilterBit::Material_DisableNormals);
            create(parent, "DisableMasking", FrameFilterBit::Material_DisableMasking);
            create(parent, "DisableVertexMotion", FrameFilterBit::Material_DisableVertexMotion);
        }

        {
            auto* parent = create(m_root, "POSTFX", FrameFilterBit::PostProcessing);
            create(parent, "Tonemap", FrameFilterBit::PostProcesses_ToneMap);
            create(parent, "SelectionOutline", FrameFilterBit::PostProcesses_SelectionOutline);
            create(parent, "SelectionHighlight", FrameFilterBit::PostProcesses_SelectionHighlight);
        }
    }

    Array<FrameFilterBitInfo*> m_infos;
    FrameFilterBitInfo* m_root = nullptr;

    FrameFilterBitInfo* create(FrameFilterBitInfo* parent, StringView name, FrameFilterBit bit = FrameFilterBit::MAX)
    {
        auto* ret = new FrameFilterBitInfo;
        ret->name = StringID(name);
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
const FrameFilterBitInfo* GetFilterTree()
{
    return FilterBitRegistry::GetInstance().m_root;
}

//

END_BOOMER_NAMESPACE_EX(rendering)
