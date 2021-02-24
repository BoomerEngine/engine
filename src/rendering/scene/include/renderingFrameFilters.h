/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "renderingFrameDebugGeometry.h"
#include "renderingFrameCamera.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

///---

enum class FilterBit : uint16_t
{
    Meshes,
    CascadeShadows, // render cascade shadows

    PassClear, // render the clear pass (clear RTs)
    PassDepthPrepass, // render the depth prepass
    PassShadowDepth, // render the depth prepass
    PassForward, // render the forward pass
    PassDepthSelection, // render depth pass with selected proxies
    PassSelectionFragments, // render selection fragments
    PassOverlay, // render the overlay pass

    FragOpaqueNonMovable, // render the static non movable fragments
    FragOpaqueSolid, // render solid fragments
    FragOpaqueMasked, // render masked fragments
    FragTransparent, // render transparent fragments

    DebugGeometry, // render debug geometry
    DebugGeometrySolid,
    DebugGeometryOverlay,
    DebugGeometryLines,
    DebugGeometryTransparent,
    DebugGeometryScreen,

    ViewportCameraInfo, // camera placement in the BR
    ViewportCameraAxes, // small camera orientation axes in the BL
    ViewportSafeArea, // safe area boundaries
    ViewportWorldGrid, // world grid at Z=0
    ViewportWorldAxes, // bit, one meter axes at [0,0,0]

    Material_DisableLighting, // bypass all lighting calculations on material
    Material_DisableColorMap, // disable object color (resets it to white)
    Material_DisableObjectColor, // disable object color (resets it to white)
    Material_DisableVertexColor, // disable vertex color (resets it to white)
    Material_DisableTextures, // disable ALL texture sampling
    Material_DisableNormals, // disable ALL normal modifications (normalmapping)
    Material_DisableMasking, // disable any discarding/masking
    Material_DisableVertexMotion, // allow vertex motion
            
    Lighting, // all of it
    Lighting_Global, // toggle global directional lighting
    Lighting_IBL, // toggle global IBL

    PostProcessing, // all of it
    PostProcesses_SelectionOutline, // render the outline around the selection
    PostProcesses_SelectionHighlight, // render the highlight on the selection
    PostProcesses_ToneMap, 

    MAX,
};

struct RENDERING_SCENE_API FilterFlags
{
    FilterFlags();
    FilterFlags(const FilterFlags& other);
    FilterFlags(FilterFlags&& other);
    FilterFlags& operator=(const FilterFlags& other);
    FilterFlags& operator=(FilterFlags&& other);

    static const FilterFlags& DefaultGame();
    static const FilterFlags& DefaultEditor();

    INLINE FilterFlags& operator+=(FilterBit bit)
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        m_bits[wordIndex] |= 1ULL << bitIndex;
        return *this;
    }

    INLINE FilterFlags& operator|=(FilterBit bit)
    {
        return *this += bit;
    }

    INLINE FilterFlags& operator-=(FilterBit bit)
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        m_bits[wordIndex] &= ~(1ULL << bitIndex);
        return *this;
    }

    INLINE FilterFlags& operator^=(FilterBit bit)
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        m_bits[wordIndex] ^= (1ULL << bitIndex);
        return *this;
    }

    INLINE bool test(FilterBit bit) const
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        return 0 != (m_bits[wordIndex] & (1ULL << bitIndex));
    }

    INLINE bool operator&(FilterBit bit) const
    {
        return test(bit);
    }

    INLINE void toggle(FilterBit bit, bool value)
    {
        if (value)
            *this += bit;
        else
            *this -= bit;
    }

private:
    static const auto NUM_WORDS = ((int)FilterBit::MAX + 63) / 64;
    uint64_t m_bits[NUM_WORDS];
};

///---

struct FilterBitInfo : public base::NoCopy
{
    RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

public:
    base::StringID name;
    base::Array<const FilterBitInfo*> children;
    FilterBit bit; // may not be set
};

// get extra info about a filter bit
extern RENDERING_SCENE_API const FilterBitInfo* GetFilterTree();

//---

END_BOOMER_NAMESPACE(rendering::scene)