/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "debugGeometry.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

enum class FrameFilterBit : uint16_t
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

struct ENGINE_RENDERING_API FrameFilterFlags
{
    FrameFilterFlags();
    FrameFilterFlags(const FrameFilterFlags& other);
    FrameFilterFlags(FrameFilterFlags&& other);
    FrameFilterFlags& operator=(const FrameFilterFlags& other);
    FrameFilterFlags& operator=(FrameFilterFlags&& other);

    static const FrameFilterFlags& DefaultGame();
    static const FrameFilterFlags& DefaultEditor();

    INLINE FrameFilterFlags& operator+=(FrameFilterBit bit)
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        m_bits[wordIndex] |= 1ULL << bitIndex;
        return *this;
    }

    INLINE FrameFilterFlags& operator|=(FrameFilterBit bit)
    {
        return *this += bit;
    }

    INLINE FrameFilterFlags& operator-=(FrameFilterBit bit)
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        m_bits[wordIndex] &= ~(1ULL << bitIndex);
        return *this;
    }

    INLINE FrameFilterFlags& operator^=(FrameFilterBit bit)
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        m_bits[wordIndex] ^= (1ULL << bitIndex);
        return *this;
    }

    INLINE bool test(FrameFilterBit bit) const
    {
        auto wordIndex = (int)bit / 64;
        auto bitIndex = (int)bit % 64;
        return 0 != (m_bits[wordIndex] & (1ULL << bitIndex));
    }

    INLINE bool operator&(FrameFilterBit bit) const
    {
        return test(bit);
    }

    INLINE void toggle(FrameFilterBit bit, bool value)
    {
        if (value)
            *this += bit;
        else
            *this -= bit;
    }

private:
    static const auto NUM_WORDS = ((int)FrameFilterBit::MAX + 63) / 64;
    uint64_t m_bits[NUM_WORDS];
};

///---

struct FrameFilterBitInfo : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

public:
    StringID name;
    Array<const FrameFilterBitInfo*> children;
    FrameFilterBit bit; // may not be set
};

// get extra info about a filter bit
extern ENGINE_RENDERING_API const FrameFilterBitInfo* GetFilterTree();

//---

END_BOOMER_NAMESPACE_EX(rendering)
