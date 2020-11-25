/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\image #]
***/

#include "build.h"
#include "glUtils.h"
#include "glImage.h"
#include "glDevice.h"
#include "glDeviceThreadCopy.h"

#include "base/image/include/imageView.h"
#include "glSampler.h"
#include "glBuffer.h"

namespace rendering
{
    namespace gl4
    {
        

#define VALIDATION_CHECK(x, txt) \
        DEBUG_CHECK_EX(x, txt); \
        if (!(x)) return false;

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

        static bool ValidateCreationInfo(const ImageCreationInfo& setup)
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
                VALIDATION_CHECK((setup.numSlices % 6) == 0, "Cubemap array must have multiple of 6 slices");
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

        ///---

        Image::Image(Device* drv, const ImageCreationInfo& setup)
            : Object(drv, ObjectType::Image)
            , m_setup(setup)
        {
            DEBUG_CHECK(ValidateCreationInfo(setup));

			m_glType = TranslateTextureType(m_setup.view, m_setup.multisampled());
			m_glFormat = TranslateImageFormat(m_setup.format);

			if (m_setup.initialLayout == ResourceLayout::INVALID)
				m_setup.initialLayout = m_setup.computeDefaultLayout();

            if (setup.allowRenderTarget)
                m_poolTag = POOL_API_RENDER_TARGETS;
            else
                m_poolTag = POOL_API_STATIC_TEXTURES;
        }

        Image::~Image()
        {
            if (m_glImage)
            {
                GL_PROTECT(glDeleteTextures(1, &m_glImage));
                m_glImage = 0;

                // update stats
                auto memoryUsage = m_setup.calcMemoryUsage();
                base::mem::PoolStats::GetInstance().notifyFree(m_poolTag, memoryUsage);
            }
        }

        static uint32_t CalcMipSize(uint32_t x, uint32_t mipIndex)
        {
            return std::max<uint32_t>(1, x >> mipIndex);
        }

        void Image::finalizeCreation()
        {
            DEBUG_CHECK_RETURN(m_glImage == 0);
            PC_SCOPE_LVL1(ImageUpload);
			
            // decompose texture format
            GLenum imageBaseFormat, imageBaseType;
            bool imageFormatCompressed = false;
            DecomposeTextureFormat(m_glFormat, imageBaseFormat, imageBaseType, imageFormatCompressed);

            // create texture object
            ASSERT(m_glImage == 0);
            GL_PROTECT(glCreateTextures(m_glType, 1, &m_glImage));
            TRACE_SPAM("GL: Created image {} from {}", m_glImage, m_setup);

            // label the object
            if (!m_setup.label.empty())
            {
                GL_PROTECT(glObjectLabel(GL_TEXTURE, m_glImage, m_setup.label.length(), m_setup.label.c_str()));
            }

            // setup texture storage
            switch (m_glType)
            {
                case GL_TEXTURE_1D:
                    ASSERT(m_setup.numSlices == 1);
                    GL_PROTECT(glTextureStorage1D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width));
                    break;

                case GL_TEXTURE_1D_ARRAY:
                    GL_PROTECT(glTextureStorage2D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width, m_setup.numSlices));
                    break;

                case GL_TEXTURE_2D:
                    ASSERT(m_setup.numSlices == 1);
                    GL_PROTECT(glTextureStorage2D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width, m_setup.height));
                    break;

                case GL_TEXTURE_2D_MULTISAMPLE:
                    ASSERT(m_setup.numSlices == 1);
                    ASSERT(m_setup.numMips == 1);
                    ASSERT(m_setup.numSamples >= 2);
                    GL_PROTECT(glTextureStorage2DMultisample(m_glImage, m_setup.numSamples, m_glFormat, m_setup.width, m_setup.height, GL_TRUE));
                    break;

                case GL_TEXTURE_2D_ARRAY:
                    GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width, m_setup.height, m_setup.numSlices));
                    break;

                case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                    ASSERT(m_setup.numMips == 1);
                    ASSERT(m_setup.numSamples >= 2);
                    GL_PROTECT(glTextureStorage3DMultisample(m_glImage, m_setup.numSamples, m_glFormat, m_setup.width, m_setup.height, m_setup.numSlices, GL_TRUE));
                    break;

                case GL_TEXTURE_3D:
                    ASSERT(m_setup.numSlices == 1);
                    GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width, m_setup.height, m_setup.depth));
                    break;

                case GL_TEXTURE_CUBE_MAP:
                    GL_PROTECT(glTextureStorage2D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width, m_setup.height));
                    //GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.m_numMips, imageFormat, m_setup.width, m_setup.height, 6));
                    break;

                case GL_TEXTURE_CUBE_MAP_ARRAY:
                    GL_PROTECT(glTextureStorage3D(m_glImage, m_setup.numMips, m_glFormat, m_setup.width, m_setup.height, m_setup.numSlices));
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

            // inform about allocation
            auto memoryUsage = m_setup.calcMemoryUsage();
            base::mem::PoolStats::GetInstance().notifyAllocation(m_poolTag, memoryUsage);
        }

		ResolvedImageView Image::resolveMainView()
		{
			if (m_glImage == 0)
			{
				finalizeCreation();
				DEBUG_CHECK_RETURN_V(m_glImage != 0, ResolvedImageView());
			}

			ResolvedImageView ret;
			ret.glImage = m_glImage;
			ret.glInternalFormat = m_glFormat;
			ret.glImageView = m_glImage;
			ret.glImageViewType = m_glType; // GL_TEXTURE_2D, etc
			ret.glSampler = 0; // not assigned to main views
			ret.firstSlice = 0;
			ret.numSlices = m_setup.numSlices;
			ret.firstMip = 0;
			ret.numMips = m_setup.numMips;
			return ret;
		}
        
		void Image::copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range)
		{
			PC_SCOPE_LVL1(ImageCopyFromBuffer);

			if (m_glImage == 0)
			{
				finalizeCreation();
				DEBUG_CHECK_RETURN(m_glImage != 0);
			}

			const auto& im = range.image;

			const auto rowLength = m_setup.calcRowLength(im.firstMip);
			const auto mipHeight = m_setup.calcMipHeight(im.firstMip);

			const auto pixelSize = GetImageFormatInfo(im.format).bitsPerPixel / 8;
			//ASSERT_EX(view.offset % pixelSize == 0, "Unaligned staging data");

			GL_PROTECT(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
			GL_PROTECT(glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength));
			GL_PROTECT(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, mipHeight));
			GL_PROTECT(glPixelStorei(GL_UNPACK_SKIP_PIXELS, view.offset / pixelSize));
			GL_PROTECT(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, view.glBuffer));

			// TODO: support for sub-part ?

			// get texture type
			switch (m_glType)
			{
			case GL_TEXTURE_1D:
				GL_PROTECT(glCopyTextureSubImage1D(m_glImage, im.firstMip, im.offsetX, 0, 0, im.sizeX));
				break;

			case GL_TEXTURE_2D:
				GL_PROTECT(glCopyTextureSubImage2D(m_glImage, im.firstMip, im.offsetX, im.offsetY, 0, 0, im.sizeX, im.sizeY));
				break;

			case GL_TEXTURE_2D_ARRAY:
				GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.firstMip, im.offsetX, im.offsetY, im.firstSlice, 0, 0, im.sizeX, im.sizeY));
				break;

			case GL_TEXTURE_3D:
				GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.firstMip, im.offsetX, im.offsetY, im.offsetZ, 0, 0, im.sizeX, im.sizeY));// TODO:!! , im.sizeZ));
				break;

			case GL_TEXTURE_CUBE_MAP:
				GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.firstMip, im.offsetX, im.offsetY, im.firstSlice, 0, 0, im.sizeX, im.sizeY));
				break;

			case GL_TEXTURE_CUBE_MAP_ARRAY:
				GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.firstMip, im.offsetX, im.offsetY, im.firstSlice, 0, 0, im.sizeX, im.sizeY));
				break;

			default:
				FATAL_ERROR("Invalid texture type");
			}

			GL_PROTECT(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
		}

        ImageView* Image::createView_ClientAPI(const ImageViewKey& key, Sampler* sampler) const
        {
            DEBUG_CHECK_RETURN_V(key.firstMip < m_setup.numMips, nullptr);
            DEBUG_CHECK_RETURN_V(key.numMips > 0, nullptr);
            DEBUG_CHECK_RETURN_V(key.firstMip + key.numMips <= m_setup.numMips, nullptr);
            DEBUG_CHECK_RETURN_V(key.firstSlice < m_setup.numSlices, nullptr);
            DEBUG_CHECK_RETURN_V(key.numSlices > 0, nullptr);
            DEBUG_CHECK_RETURN_V(key.firstSlice + key.numSlices <= m_setup.numSlices, nullptr);

            return new ImageView(device(), const_cast<Image*>(this), key, sampler);
        }

        //--

        ImageView::ImageView(Device* drv, Image* img, ImageViewKey key, Sampler* sampler)
            : Object(drv, ObjectType::ImageView)
			, m_sampler(sampler)
			, m_image(img)
            , m_key(key)
        {
            m_resolved.glImageView = 0;
            m_resolved.glImage = m_image->m_glImage;
            m_resolved.glImageViewType = TranslateTextureType(key.viewType, m_image->m_setup.multisampled());
			m_resolved.glInternalFormat = m_image->m_glFormat;
			m_resolved.glSampler = 0;
			m_resolved.firstSlice = key.firstSlice;
            m_resolved.firstMip = key.firstMip;
            m_resolved.numSlices = key.numSlices;
            m_resolved.numMips = key.numMips;
        }

        ImageView::~ImageView()
        {
            if (m_resolved.glImageView && m_resolved.glImageView != m_resolved.glImage)
                GL_PROTECT(glDeleteTextures(1, &m_resolved.glImageView));

			memzero(&m_resolved, sizeof(m_resolved));
        }

        static ResolvedImageView TheEmptyView;

        const ResolvedImageView& ImageView::resolveView()
        {
            if (m_resolved.glImageView == 0)
            {
                finalizeCreation();
                DEBUG_CHECK_RETURN_V(m_resolved.glImageView != 0, TheEmptyView);
            }

            return m_resolved;
        }

        void ImageView::finalizeCreation()
        {
            DEBUG_CHECK_RETURN(m_resolved.glImageView == 0);

            // validate params
            DEBUG_CHECK_RETURN(m_resolved.numSlices >= 1);
            DEBUG_CHECK_RETURN(m_resolved.firstSlice + m_resolved.numSlices <= m_image->m_setup.numSlices);
            DEBUG_CHECK_RETURN(m_resolved.numMips >= 1);
            DEBUG_CHECK_RETURN(m_resolved.firstMip + m_resolved.numMips <= m_image->m_setup.numMips);

            // TODO: check format compatibility

            // ensure image is initialized
            if (m_image->m_glImage == 0)
                m_image->finalizeCreation();

			// ensure sampler is resolved as well
			if (m_resolved.glSampler == 0 && m_sampler)
			{
				m_resolved.glSampler = m_sampler->deviceObject();
				DEBUG_CHECK_RETURN(m_resolved.glSampler != 0);
			}

            // full view does not require a specialized view object
            if (m_resolved.firstMip == 0 && m_resolved.numMips == m_image->m_setup.numMips
                && m_resolved.firstSlice == 0 && m_resolved.numSlices == m_image->m_setup.numSlices
                && m_key.viewType == m_image->m_setup.view)
            {
				m_resolved.glImage = m_image->m_glImage;
                m_resolved.glImageView = m_image->m_glImage; // use texture directly
                return;
            }

            // create new texture
            GL_PROTECT(glGenTextures(1, &m_resolved.glImageView));
            GL_PROTECT(glTextureView(m_resolved.glImageView, m_resolved.glImageViewType, m_image->m_glImage, m_image->m_glFormat, m_resolved.firstMip, m_resolved.numMips, m_resolved.firstSlice, m_resolved.numSlices));
            TRACE_SPAM("GL: Created image view {} of {} with {}", m_resolved.glImageView, m_image->m_glImage, m_key);
			m_resolved.glImage = m_image->m_glImage;
        }

        //--

    } // gl4
} // rendering
