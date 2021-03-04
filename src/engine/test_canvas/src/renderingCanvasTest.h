/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/object/include/rttiMetadata.h"
#include "core/resource/include/resourceLoader.h"

#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/style.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"

#include "engine/font/include/font.h"
#include "engine/font/include/fontInputText.h"
#include "engine/font/include/fontGlyphCache.h"
#include "engine/font/include/fontGlyphBuffer.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//---

// order of test
class CanvasTestOrderMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasTestOrderMetadata, IMetadata);

public:
    INLINE CanvasTestOrderMetadata()
        : m_order(-1)
    {}

    CanvasTestOrderMetadata& order(int val)
    {
        m_order = val;
        return *this;
    }

    int m_order;
};

//---

/// a basic rendering test for the scene
class ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasTest);

public:
    ICanvasTest();

    bool processInitialization();

    virtual void initialize() {};
	virtual void shutdown() {};
    virtual void render(canvas::Canvas& canvas) {};
    virtual void update(float dt) {};
    virtual void processInput(const input::BaseEvent& evt) {};

    void reportError(StringView msg);

    image::ImagePtr loadImage(StringView imageName);
    FontPtr loadFont(StringView fontName);

protected:
    bool m_hasErrors;

    image::ImagePtr m_defaultImage;
};

///---

/// helper class that build as grid in the 2D space
class CanvasGridBuilder
{
public:
    CanvasGridBuilder(uint32_t sizeX, uint32_t sizeY, uint32_t margin, uint32_t width, uint32_t height)
        : m_x(0)
        , m_y(0)
        , m_maxX(sizeX)
        , m_maxY(sizeY)
        , m_margin(margin)
        , m_width(std::min(width, height))
        , m_height(std::min(width, height))
    {}

    Rect cell()
    {
        Rect r;
        r.min.x = ((m_x * m_width) / m_maxX) + m_margin;
        r.min.y = ((m_y * m_height) / m_maxY) + m_margin;
        r.max.x = (((m_x + 1) * m_width) / m_maxX) - m_margin;
        r.max.y = (((m_y + 1) * m_height) / m_maxY) - m_margin;

        m_x += 1;
        if (m_x == m_maxX)
        {
            m_x = 0;
            m_y += 1;
            if (m_y == m_maxY)
                m_y = 0;
        }

        return r;
    }

private:
    uint32_t m_x;
    uint32_t m_y;
    uint32_t m_maxX;
    uint32_t m_maxY;
    uint32_t m_margin;
    uint32_t m_width;
    uint32_t m_height;
};

///---

END_BOOMER_NAMESPACE_EX(test)
