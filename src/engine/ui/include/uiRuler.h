/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"
#include "engine/canvas/include/canvasGeometry.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// horizontal ruler
class ENGINE_UI_API HorizontalRuler : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(HorizontalRuler, IElement);

public:
    HorizontalRuler();

    void region(float minVal, float maxVal);
    void activeRegion(float minVal, float maxVal);

    inline float regionMin() const { return m_viewRegionMin; }
    inline float regionMax() const { return m_viewRegionMax; }
    inline float activeRegionMin() const { return m_viewActiveRegionMin; }
    inline float activeRegionMax() const { return m_viewActiveRegionMax; }

private:
    float m_viewRegionMin = 0.0;
    float m_viewRegionMax = 0.0;
    float m_viewActiveRegionMin = 0.0;
    float m_viewActiveRegionMax = 0.0;

	mutable canvas::Geometry m_geometry;

	void generateGeometry(float width, float height) const;

    virtual void renderBackground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;
};

//--

/// vertical ruler
class ENGINE_UI_API VerticalRuler : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(VerticalRuler, IElement);

public:
    VerticalRuler();

    void region(float minVal, float maxVal);
    void activeRegion(float minVal, float maxVal);

    inline float regionMin() const { return m_viewRegionMin; }
    inline float regionMax() const { return m_viewRegionMax; }
    inline float activeRegionMin() const { return m_viewActiveRegionMin; }
    inline float activeRegionMax() const { return m_viewActiveRegionMax; }

private:
    float m_viewRegionMin = 0.0;
    float m_viewRegionMax = 0.0;
    float m_viewActiveRegionMin = 0.0;
    float m_viewActiveRegionMax = 0.0;

	mutable canvas::Geometry m_geometry;

	void generateGeometry(float width, float height) const;

    virtual void renderBackground(DataStash& stash, const ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity) override;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
