/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image\atlas #]
***/

#pragma once

namespace base
{
    namespace image
    {
        //--

        /// entry in the atlas
        struct BASE_IMAGE_API DynamicAtlasEntry
        {
            Vector2 uvStart;
            Vector2 uvEnd;
            Rect placement;

            INLINE DynamicAtlasEntry()
                : uvStart(0, 0)
                , uvEnd(0, 0)
                , placement(0, 0, 0, 0)
            {}
        };

        //--

        /// dynamic atlas of small images conglomerated into a big image
        class BASE_IMAGE_API DynamicAtlas : public base::NoCopy
        {
        public:
            DynamicAtlas(uint32_t width, uint32_t height, uint8_t numChannels=1, uint32_t pixelSpacing=1, PixelFormat format = PixelFormat::Uint8_Norm);
            ~DynamicAtlas();

            ///--

            INLINE uint32_t width() const { return m_width; }
            INLINE uint32_t height() const { return m_height; }

            INLINE const ImagePtr& image() const { return m_image; }

            ///--

            /// reset the atlas, removes all content, resets the allocator and the dirty area tracker
            void reset();

            /// place an image inside the atlas, returns false if the atlas is full
            bool placeImage(const ImageView& sourceImage, DynamicAtlasEntry& outEntry);

        private:
            ImagePtr m_image; // image area
            UniquePtr<RectAllocator> m_spaceAllocator; // allocator for the sub-space in the image

            uint32_t m_width;
            uint32_t m_height;
            uint32_t m_pixelSpacing;

            float m_invWidth;
            float m_invHeight;
        };

        //--

    } // image
} // base
