/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#pragma once

namespace base
{
    namespace font
    {
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

        /// render glyph buffer into an image
        /// assumes the image has RGBA channels, alpha will contain text transparency
        /// allows the text color to be pre multiplied by color values
        /// NOTE: this is a mostly debug/offline functionality and it's not optimized
        

    } // font
} // base
