/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::font)

/// rendering parameters for the font
class BASE_FONT_API FontRenderingParams
{
public: 
    // text color to apply while rendering
    Color m_textColor;

    // placement in the buffer
    Vector2 m_placement;

    FontRenderingParams();
};
        
END_BOOMER_NAMESPACE(base::font)
