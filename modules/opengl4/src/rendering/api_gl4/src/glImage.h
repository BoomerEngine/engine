/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\image #]
***/

#pragma once

#include "glObject.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
    namespace gl4
    {
        ///---

        class ImageView;

        /// resolved image view information
        struct ResolvedImageView
        {
            GLuint glImage = 0; // actual image
            GLuint glImageView = 0; // view
            GLenum glImageViewType = 0; // GL_TEXTURE_2D, etc
			GLenum glInternalFormat = 0;
			GLenum glSampler = 0;
            uint16_t firstSlice = 0;
            uint16_t numSlices = 0;
            uint16_t firstMip = 0;
            uint16_t numMips = 0;

            INLINE ResolvedImageView() {}

            INLINE bool empty() const { return (glImage == 0) || (glImageView == 0); }
            INLINE operator bool() const { return !empty(); }

            //--

            static GLenum GetTargetForViewType(ImageViewType viewType)
            {
                switch (viewType)
                {
                    case ImageViewType::View1D: return GL_TEXTURE_1D;
                    case ImageViewType::View1DArray: return GL_TEXTURE_1D_ARRAY;
                    case ImageViewType::View2D: return GL_TEXTURE_2D;
                    case ImageViewType::View2DArray: return GL_TEXTURE_2D_ARRAY;
                    case ImageViewType::View3D: return GL_TEXTURE_3D;
                    case ImageViewType::ViewCube: return GL_TEXTURE_CUBE_MAP;
                    case ImageViewType::ViewCubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
                }

                FATAL_ERROR("Invalid view type");
                return 0;
            }
        };

        ///---

        /// wrapper for image
        class Image : public Object
        {
        public:
            Image(Device* drv, const ImageCreationInfo& setup);
            virtual ~Image();

            static const auto STATIC_TYPE = ObjectType::Image;

            //--

            // get the format of the image
            INLINE GLuint format() const { return m_glFormat; }

            // get the format of the image (GL_TEXTURE_X)
            INLINE GLuint type() const { return m_glType; }

            // get size of used memory
            INLINE uint32_t dataSize() const { return 0; }

            /// get the image creation setup
            INLINE const ImageCreationInfo& creationSetup() const { return m_setup; }

            // get the label
            INLINE const base::StringBuf& label() const { return m_setup.label; }

            //---

			// resolve main view of the image
			ResolvedImageView resolveMainView();

			// copy new content from staging area
			void copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range);

            //---

            // create an image view, works for all kind of images
            ImageView* createView_ClientAPI(const ImageViewKey& key, Sampler* sampler) const;

            //---

        private:
            GLuint m_glImage = 0;
            GLuint m_glFormat = 0;
            GLuint m_glType = 0;

            ImageCreationInfo m_setup;

            PoolTag m_poolTag = POOL_API_STATIC_TEXTURES;

            //--

            void finalizeCreation();

            friend class ImageView;
        };

        //--

        /// wrapper for typical image view
        class ImageView : public Object
        {
        public:
            ImageView(Device* drv, Image* img, ImageViewKey key, Sampler* sampler);
            virtual ~ImageView();

            static const auto STATIC_TYPE = ObjectType::ImageView;

			// get the format of the image
			INLINE GLuint format() const { return m_image->format(); }

			// get the format of the image (GL_TEXTURE_X)
			INLINE GLuint type() const { return m_image->type(); }

			/// get the image creation setup
			INLINE const ImageCreationInfo& imageSetup() const { return m_image->creationSetup(); }

            //--

            const ResolvedImageView& resolveView();

            //--

        private:
            Image* m_image = nullptr;
			Sampler* m_sampler = nullptr;

			ImageViewKey m_key;
            ResolvedImageView m_resolved;

            void finalizeCreation();
        };

        //--

    } // gl4
} // rendering