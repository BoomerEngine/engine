/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects\image #]
***/

#include "build.h"
#include "glUtils.h"
#include "glImage.h"
#include "glDriver.h"

#include "base/image/include/imageView.h"

namespace rendering
{
    namespace gl4
    {
        
        ///---

        Image::Image(Driver* drv, const ImageCreationInfo& setup, const SourceData* initData, PoolTag poolID)
            : Object(drv, ObjectType::Image)
            , m_poolID(poolID)
            , m_setup(setup)
            , m_glImage(0)
            , m_glFormat(0)
            , m_glType(0)
        {
            auto memoryUsage = m_setup.calcMemoryUsage();
            base::mem::PoolStats::GetInstance().notifyAllocation(poolID, memoryUsage);

            if (initData)
            {
                uint32_t numSourceSlices = setup.numSlices * setup.numMips;
                m_initData.resize(numSourceSlices);

                for (uint32_t i = 0; i < numSourceSlices; ++i)
                    m_initData[i] = initData[i];
            }
        }

        Image::Image(Driver* drv, const ImageCreationInfo& setup, GLuint id, PoolTag poolID)
            : Object(drv, ObjectType::Image)
            , m_poolID(poolID)
            , m_setup(setup)
            , m_glImage(id)
            , m_glFormat(TranslateImageFormat(setup.format))
            , m_glType(TranslateTextureType(setup.view, setup.multisampled()))
        {
            auto memoryUsage = m_setup.calcMemoryUsage();
            base::mem::PoolStats::GetInstance().notifyAllocation(poolID, memoryUsage);
        }

        Image::~Image()
        {
            // release views
            for (auto view : m_imageViewMap.values())
                GL_PROTECT(glDeleteTextures(1, &view));
            m_imageViewMap.clear();

            // release the buffer object
            GL_PROTECT(glDeleteTextures(1, &m_glImage));
            m_glImage = 0;

            // update stats
            auto memoryUsage = m_setup.calcMemoryUsage();
            base::mem::PoolStats::GetInstance().notifyFree(m_poolID, memoryUsage);
        }

        bool Image::CheckClassType(ObjectType type)
        {
            return type == ObjectType::Image;
        }

        static uint32_t CalcMipSize(uint32_t x, uint32_t mipIndex)
        {
            return std::max<uint32_t>(1, x >> mipIndex);
        }

        void Image::finalizeCreation()
        {
            PC_SCOPE_LVL1(ImageUpload);

            // get texture format and type
            auto imageType = TranslateTextureType(m_setup.view, m_setup.multisampled());
            auto imageFormat = TranslateImageFormat(m_setup.format);

            // decompose texture format
            GLenum imageBaseFormat, imageBaseType;
            bool imageFormatCompressed = false;
            DecomposeTextureFormat(imageFormat, imageBaseFormat, imageBaseType, imageFormatCompressed);

            // create texture object
            ASSERT(m_glImage == 0);
            GL_PROTECT(glCreateTextures(imageType, 1, &m_glImage));

            // label the object
            if (!m_setup.label.empty())
            {
                GL_PROTECT(glObjectLabel(GL_TEXTURE, m_glImage, m_setup.label.length(), m_setup.label.c_str()));
            }

            // setup texture storage
            switch (imageType)
            {
                case GL_TEXTURE_1D:
                    ASSERT(m_setup.numSlices == 1);
                    GL_PROTECT(glTextureStorage1D(m_glImage, m_setup.numMips, imageFormat, m_setup.width));
                    break;

                case GL_TEXTURE_1D_ARRAY:
                    GL_PROTECT(glTextureStorage2D(m_glImage, m_setup.numMips, imageFormat, m_setup.width, m_setup.numSlices));
                    break;

                case GL_TEXTURE_2D:
                    ASSERT(m_setup.numSlices == 1);
                    GL_PROTECT(glTextureStorage2D(m_glImage, m_setup.numMips, imageFormat, m_setup.width, m_setup.height));
                    break;

                case GL_TEXTURE_2D_MULTISAMPLE:
                    ASSERT(m_setup.numSlices == 1);
                    ASSERT(m_setup.numMips == 1);
                    ASSERT(m_setup.numSamples >= 2);
                    GL_PROTECT(glTextureStorage2DMultisample(m_glImage, m_setup.numSamples, imageFormat, m_setup.width, m_setup.height, GL_TRUE));
                    break;

                case GL_TEXTURE_2D_ARRAY:
                    GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.numMips, imageFormat, m_setup.width, m_setup.height, m_setup.numSlices));
                    break;

                case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                    ASSERT(m_setup.numMips == 1);
                    ASSERT(m_setup.numSamples >= 2);
                    GL_PROTECT(glTextureStorage3DMultisample(m_glImage, m_setup.numSamples, imageFormat, m_setup.width, m_setup.height, m_setup.numSlices, GL_TRUE));
                    break;

                case GL_TEXTURE_3D:
                    ASSERT(m_setup.numSlices == 1);
                    GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.numMips, imageFormat, m_setup.width, m_setup.height, m_setup.depth));
                    break;

                case GL_TEXTURE_CUBE_MAP:
                    GL_PROTECT(glTextureStorage2D(m_glImage, m_setup.numMips, imageFormat, m_setup.width, m_setup.height));
                    //GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.m_numMips, imageFormat, m_setup.width, m_setup.height, 6));
                    break;

                case GL_TEXTURE_CUBE_MAP_ARRAY:
                    GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.numMips, imageFormat, m_setup.width, m_setup.height, m_setup.numSlices));
                    break;

                default:
                    FATAL_ERROR("Invalid texture type");
            }

            // setup state
            GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_SWIZZLE_R, GL_RED));
            GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_SWIZZLE_G, GL_GREEN));
            GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_SWIZZLE_B, GL_BLUE));
            GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_SWIZZLE_A, GL_ALPHA));
            GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_BASE_LEVEL, 0));
            GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_MAX_LEVEL, m_setup.numMips-1));

            // setup some more state :)
            if (!m_setup.multisampled())
            {
                GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            }

            // upload initial data
            if (!m_initData.empty())
            {
               // auto imageBytesPerPixel = GetTextureFormatBytesPerPixel(imageFormat);

                for (uint8_t sliceIndex = 0; sliceIndex<m_setup.numSlices; ++sliceIndex)
                {
                    for (uint8_t mipIndex = 0; mipIndex<m_setup.numMips; ++mipIndex)
                    {
                        auto srcSliceIndex = (sliceIndex * m_setup.numMips) + mipIndex;
                        auto& srcSlice = m_initData[srcSliceIndex];

                        auto mipWidth = CalcMipSize(m_setup.width, mipIndex);
                        auto mipHeight = CalcMipSize(m_setup.height, mipIndex);
                        auto mipDepth = CalcMipSize(m_setup.depth, mipIndex);

                        //ASSERT_EX(srcSlice.m_rowPitch % imageBytesPerPixel == 0, "Invalid source data row pitch");
                        //GL_PROTECT(glPixelStorei(GL_UNPACK_ROW_LENGTH, srcSlice.m_rowPitch / imageBytesPerPixel));
                        //ASSERT_EX(srcSlice.m_slicePitch % imageBytesPerPixel == 0, "Invalid source data slice pitch");
                        //GL_PROTECT(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, srcSlice.m_slicePitch / imageBytesPerPixel));
                        GL_PROTECT(glPixelStorei(GL_UNPACK_ROW_LENGTH, mipWidth));
                        GL_PROTECT(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, mipHeight));
                        GL_PROTECT(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

                        auto sourceData  = base::OffsetPtr(srcSlice.data.data(), srcSlice.offset);

                        switch (imageType)
                        {
                            case GL_TEXTURE_1D:
                                ASSERT(!imageFormatCompressed);
                                GL_PROTECT(glTextureSubImage1D(m_glImage, mipIndex, 0, mipWidth, imageBaseFormat, imageBaseType, sourceData));
                                break;

                            case GL_TEXTURE_2D:
                                if (imageFormatCompressed)
                                {
                                    unsigned int blockSize = (imageFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || imageFormat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT || imageFormat == GL_COMPRESSED_RED_RGTC1) ? 8 : 16;
                                    unsigned int size = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
                                    ASSERT(size == srcSlice.size);
                                    GL_PROTECT(glCompressedTextureSubImage2D(m_glImage, mipIndex, 0, 0, mipWidth, mipHeight, imageFormat, srcSlice.size, sourceData));
                                }
                                else
                                {
                                    GL_PROTECT(glTextureSubImage2D(m_glImage, mipIndex, 0, 0, mipWidth, mipHeight, imageBaseFormat, imageBaseType, sourceData));
                                }
                                break;

                            case GL_TEXTURE_2D_ARRAY:
                                if (imageFormatCompressed)
                                {
                                    unsigned int blockSize = (imageFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || imageFormat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT || imageFormat == GL_COMPRESSED_RED_RGTC1) ? 8 : 16;
                                    unsigned int size = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
                                    ASSERT(size == srcSlice.size);
                                    GL_PROTECT(glCompressedTextureSubImage3D(m_glImage, mipIndex, 0, 0, sliceIndex, mipWidth, mipHeight, 1, imageFormat, srcSlice.size, sourceData));
                                }
                                else
                                {
                                    GL_PROTECT(glTextureSubImage3D(m_glImage, mipIndex, 0, 0, sliceIndex, mipWidth, mipHeight, 1, imageBaseFormat, imageBaseType, sourceData));
                                }
                                break;
                                //ASSERT(!imageFormatCompressed);
                                //GL_PROTECT(glTextureSubImage3D(m_glImage, mipIndex, 0, 0, sliceIndex, mipWidth, mipHeight, 1, imageBaseFormat, imageBaseType, sourceData));
                                //break;

                            case GL_TEXTURE_3D:
                            ASSERT(!imageFormatCompressed);
                                GL_PROTECT(glTextureSubImage3D(m_glImage, mipIndex, 0, 0, 0, mipWidth, mipHeight, mipDepth, imageBaseFormat, imageBaseType, sourceData));
                                break;

                            case GL_TEXTURE_CUBE_MAP:
                                if (imageFormatCompressed)
                                {
                                    unsigned int blockSize = (imageFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || imageFormat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) ? 8 : 16;
                                    unsigned int size = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
                                    ASSERT(size == srcSlice.size);
                                    GL_PROTECT(glCompressedTextureSubImage3D(m_glImage, mipIndex, 0, 0, sliceIndex, mipWidth, mipHeight, 1, imageFormat, srcSlice.size, sourceData));
                                }
                                else
                                {
                                    GL_PROTECT(glTextureSubImage3D(m_glImage, mipIndex, 0, 0, sliceIndex, mipWidth, mipHeight, 1, imageBaseFormat, imageBaseType, sourceData));
                                }
                                break;

                            case GL_TEXTURE_CUBE_MAP_ARRAY:
                                ASSERT(!imageFormatCompressed);
                                GL_PROTECT(glTextureSubImage3D(m_glImage, mipIndex, 0, 0, sliceIndex, mipWidth, mipHeight, 1, imageBaseFormat, imageBaseType, sourceData));
                                break;

                            default:
                                FATAL_ERROR("Invalid texture type");
                        }
                    }
                }
            }

            // release the initialization data
            m_initData.clear();

            // remember final format
            m_glFormat = imageFormat;
        }

        static uint32_t CalcMaxMipCount(const ImageCreationInfo& setup)
        {
            auto w = setup.width;
            auto h = setup.height;
            auto d = setup.depth;

            uint32_t mipCount = 1;
            while (w > 1 || h > 1 || d > 1)
            {
                w = std::max<uint32_t>(1, w / 2);
                h = std::max<uint32_t>(1, h / 2);
                d = std::max<uint32_t>(1, d / 2);
                mipCount += 1;
            }

            return mipCount;
        }

#define VALIDATION_CHECK(x, txt) \
        DEBUG_CHECK_EX(x, txt); \
        if (!(x)) return false;

        static bool ValidateCreationInfo(ImageCreationInfo& setup)
        {
            VALIDATION_CHECK(setup.width >= 1 && setup.height >= 1 && setup.depth >= 1, "Invalid image size");
            VALIDATION_CHECK(setup.format != ImageFormat::UNKNOWN, "Unknown image format");

            uint32_t maxMipCount = CalcMaxMipCount(setup);
            VALIDATION_CHECK(setup.numMips != 0, "Invalid mip count");
            VALIDATION_CHECK(setup.numMips <= maxMipCount, "To many mipmaps for image of this size");

            auto array = (setup.view == ImageViewType::View1DArray) || (setup.view == ImageViewType::View2DArray) || (setup.view == ImageViewType::ViewCubeArray) || (setup.view == ImageViewType::ViewCube);
            VALIDATION_CHECK(setup.numSlices >= 1, "Invalid slice count");
            VALIDATION_CHECK(array || setup.numSlices == 1, "Only array images may have slices");
            VALIDATION_CHECK(!array || setup.numSlices >= 1, "Array images should have slices");

            if (setup.view == ImageViewType::View1D || setup.view == ImageViewType::View1DArray)
            {
                VALIDATION_CHECK(setup.height == 1, "1D texture should have no height");
                VALIDATION_CHECK(setup.depth == 1, "1D texture should have no depth");
            }
            else if (setup.view == ImageViewType::View2D || setup.view == ImageViewType::View2DArray)
            {
                VALIDATION_CHECK(setup.depth == 1, "2D texture should have no depth");
            }
            if (setup.view == ImageViewType::ViewCube)
            {
                VALIDATION_CHECK(setup.numSlices == 6, "Cubemap must have 6 slices");
                VALIDATION_CHECK(setup.width == setup.height, "Cubemap must be square");
                VALIDATION_CHECK(setup.depth == 1, "Cubemap must can't have depth");
            }
            else if (setup.view == ImageViewType::ViewCubeArray)
            {
                VALIDATION_CHECK(setup.numSlices >= 6, "Cubemap must have 6 slices");
                VALIDATION_CHECK((setup.numSlices % 6) == 6, "Cubemap array must have multiple of 6 slices");
                VALIDATION_CHECK(setup.width == setup.height, "Cubemap must be square");
                VALIDATION_CHECK(setup.depth == 1, "Cubemap must can't have depth");
            }

            if (setup.multisampled())
            {
                VALIDATION_CHECK(setup.allowRenderTarget, "Multisampling is only allowed for render targets");
            }

            // TODO: more validation :)

            return true;
        }

        Image* Image::CreateImage(Driver* drv, const ImageCreationInfo& originalSetup, const SourceData* sourceData)
        {
            ImageCreationInfo setup = originalSetup;

            // make sure we can create this texture
            if (!ValidateCreationInfo(setup))
                return nullptr;

            // determine ID of the pool
            auto poolID = POOL_API_STATIC_TEXTURES;
            if (setup.allowRenderTarget)
                poolID = POOL_API_RENDER_TARGETS;

            // create the image wrapper
            return MemNew(Image, drv, setup, sourceData, poolID);
        }

        Image* Image::CreateImage(Driver* drv, const ImageCreationInfo& setup, GLuint id, PoolTag poolID)
        {
            return MemNew(Image, drv, setup, id, poolID);
        }

        void Image::ensureInitialized()
        {
            if (m_glImage == 0)
                finalizeCreation();
        }

        ResolvedImageView Image::resolveView(ImageViewKey key)
        {
            ensureInitialized();
        
            // validate params
            DEBUG_CHECK_EX(key.numSlices >= 1, "Invalid slice count in view key");
            DEBUG_CHECK_EX(key.firstSlice + key.numSlices <= m_setup.numSlices, "To many requested slices in view key");
            DEBUG_CHECK_EX(key.numMips >= 1, "Invalid mip count in view key");
            DEBUG_CHECK_EX(key.firstMip + key.numMips <= m_setup.numMips, "To many mipmaps requested in view key");

            // TODO: check format compatibility

            // get texture type
            auto glImageViewType = TranslateTextureType(key.viewType, m_setup.multisampled());

            // full view does not require a specialized view object
            if (key.firstMip == 0 && key.firstSlice == 0 && key.numMips == m_setup.numMips && key.numSlices == m_setup.numSlices && key.viewType == m_setup.view)
                return ResolvedImageView(m_glImage, m_glImage, glImageViewType, 0, key.numSlices, 0, key.numMips);

            // use existing view
            GLuint glImageView = 0;
            if (m_imageViewMap.find(key, glImageView))
                return ResolvedImageView(m_glImage, glImageView, glImageViewType, 0, key.numSlices, 0, key.numMips);

            // create new texture
            GL_PROTECT(glGenTextures(1, &glImageView));
            GL_PROTECT(glTextureView(glImageView, glImageViewType, m_glImage, m_glFormat, key.firstMip, key.numMips, key.firstSlice, key.numSlices));
            m_imageViewMap[key] = glImageView;

            return ResolvedImageView(m_glImage, glImageView, glImageViewType, 0, key.numSlices, 0, key.numMips);
        }

        void Image::updateContent(const rendering::ImageView& view, const base::image::ImageView& data, uint32_t x, uint32_t y, uint32_t z)
        {
            PC_SCOPE_LVL1(ImageUpdate);

            // make sure view exists
            if (m_glImage == 0)
                finalizeCreation();

            // get update target
            auto imageMip = view.firstMip();
            auto imageSlice = view.firstArraySlice();

            // validate params
            ASSERT(imageSlice < m_setup.numSlices);
            ASSERT(imageMip < m_setup.numMips);
            ASSERT_EX(!m_setup.multisampled(), "Cannot upload multisampled image");

            // get texture format and type
            auto imageType = TranslateTextureType(m_setup.view, m_setup.multisampled());
            auto imageFormat = TranslateImageFormat(m_setup.format);
            auto imageBytesPerPixel = GetTextureFormatBytesPerPixel(imageFormat);

            // decompose texture format
            GLenum targetImageBaseFormat, targetImageBaseType;
            bool targetImageCompressed;
            DecomposeTextureFormat(imageFormat, targetImageBaseFormat, targetImageBaseType, targetImageCompressed);

            // we cannot update compressed images
            if (targetImageCompressed)
            {
                TRACE_ERROR("Cannot update compressed image");
                return;
            }

            // decompose texture format
            GLenum sourceImageBaseFormat, sourceImageBaseType;
            DecomposeTextureFormat(data.format(), data.channels(), sourceImageBaseFormat, sourceImageBaseType);

            // setup format specification
            auto mipWidth = std::max(1, m_setup.width >> imageMip);
            auto mipHeight = std::max(1, m_setup.height >> imageMip);
            auto mipDepth = std::max(1, m_setup.depth >> imageMip);
            GL_PROTECT(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
            GL_PROTECT(glPixelStorei(GL_UNPACK_ROW_LENGTH, data.rowPitch() / data.pixelPitch()));
            GL_PROTECT(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, data.height()));

            // get texture type
            switch (imageType)
            {
                case GL_TEXTURE_1D:
                    GL_PROTECT(glTextureSubImage1D(m_glImage, imageMip, x, data.width(), sourceImageBaseFormat, sourceImageBaseType, data.data()));
                    break;

                case GL_TEXTURE_2D:
                    GL_PROTECT(glTextureSubImage2D(m_glImage, imageMip, x, y, data.width(), data.height(), sourceImageBaseFormat, sourceImageBaseType, data.data()));
                    break;

                case GL_TEXTURE_2D_ARRAY:
                    GL_PROTECT(glTextureSubImage3D(m_glImage, imageMip, x, y, imageSlice, data.width(), data.height(), 1, sourceImageBaseFormat, sourceImageBaseType, data.data()));
                    break;

                case GL_TEXTURE_3D:
                    GL_PROTECT(glTextureSubImage3D(m_glImage, imageMip, x, y, z, data.width(), data.height(), data.depth(), sourceImageBaseFormat, sourceImageBaseType, data.data()));
                    break;

                case GL_TEXTURE_CUBE_MAP:
                    GL_PROTECT(glTextureSubImage3D(m_glImage, imageMip, x, y, imageSlice, data.width(), data.height(), 1, sourceImageBaseFormat, sourceImageBaseType, data.data()));
                    break;

                case GL_TEXTURE_CUBE_MAP_ARRAY:
                    GL_PROTECT(glTextureSubImage3D(m_glImage, imageMip, x, y, imageSlice, data.width(), data.height(), 1, sourceImageBaseFormat, sourceImageBaseType, data.data()));
                    break;

                default:
                    FATAL_ERROR("Invalid texture type");
            }
        }

    } // gl4
} // driver
