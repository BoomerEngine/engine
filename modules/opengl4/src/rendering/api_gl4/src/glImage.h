/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects\image #]
***/

#pragma once

#include "glObject.h"

#include "rendering/driver/include/renderingImageView.h"
#include "base/containers/include/sortedArray.h"

namespace rendering
{
    namespace gl4
    {
        ///---

           /// resolved image view information
        struct ResolvedImageView
        {
            GLuint glImage = 0; // actual image
            GLuint glImageView = 0; // view
            GLenum glImageViewType = 0; // GL_TEXTURE_2D, etc
            uint16_t firstSlice = 0;
            uint16_t numSlices = 0;
            uint16_t firstMip = 0;
            uint16_t numMips = 0;

            INLINE ResolvedImageView() {}
            INLINE ResolvedImageView(GLuint glImage_, GLuint glImageView_, GLenum glImageViewType_ = GL_TEXTURE_2D, uint16_t firstSlice_ = 0, uint16_t numSlices_ = 1, uint16_t firstMip_ = 0, uint16_t numMips_ = 1)
                : glImage(glImage_)
                , glImageView(glImageView_)
                , glImageViewType(glImageViewType_)
                , firstSlice(firstSlice_)
                , numSlices(numSlices_)
                , firstMip(firstMip_)
                , numMips(numMips_)
            {}

            INLINE ResolvedImageView(GLuint glImage_, GLuint glImageView_, const ImageViewKey& key)
                : glImage(glImage)
                , glImageView(glImageView)
                , glImageViewType(GetTargetForViewType(key.viewType))
                , firstSlice(key.firstSlice)
                , numSlices(key.numSlices)
                , firstMip(key.firstMip)
                , numMips(key.numMips)
            {}

            INLINE bool empty() const { return (glImage == 0); }
            INLINE operator bool() const { return (glImage != 0); }


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
            Image(Driver* drv, const ImageCreationInfo& setup, const SourceData* initData, PoolTag poolID);
            Image(Driver* drv, const ImageCreationInfo& setup, GLuint id, PoolTag poolID);
            virtual ~Image();

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

            static bool CheckClassType(ObjectType type);

            //---

            // ensure all data for this object is created
            void ensureInitialized();

            // resolve a view of this image
            ResolvedImageView resolveView(ImageViewKey key);

            //---

            // update image content
            // NOTE: not synchronized
            void updateContent(const ImageView& view, const base::image::ImageView& data, uint32_t x, uint32_t y, uint32_t z);

            //---

            // create an image
            static Image* CreateImage(Driver* drv, const ImageCreationInfo& setup, const SourceData *sourceData);

            // create an image with already existing resource (recycled)
            static Image* CreateImage(Driver* drv, const ImageCreationInfo& setup, GLuint id, PoolTag poolID);

        private:
            GLuint m_glImage = 0;
            GLuint m_glFormat = 0;
            GLuint m_glType = 0;

            ImageCreationInfo m_setup;
            base::Array<SourceData> m_initData;
            base::HashMap<ImageViewKey, GLuint> m_imageViewMap;

            PoolTag m_poolID;

            //--

            void finalizeCreation();
        };

    } // gl4
} // driver