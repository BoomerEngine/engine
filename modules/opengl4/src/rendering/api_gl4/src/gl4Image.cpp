/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"

#include "gl4Thread.h"
#include "gl4Image.h"
#include "gl4Utils.h"
#include "gl4ObjectCache.h"

#include "rendering/api_common/include/apiCopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			///---

			Image::Image(Thread* drv, const ImageCreationInfo& setup)
				: IBaseImage(drv, setup)
			{
				m_glViewType = TranslateTextureType(setup.view, setup.multisampled());
				m_glFormat = TranslateImageFormat(setup.format);
			}

			Image::~Image()
			{
				if (m_glImage)
				{
					if (setup().allowRenderTarget)
						owner()->objectCache()->notifyImageDeleted(m_glImage);

					GL_PROTECT(glDeleteTextures(1, &m_glImage));
					m_glImage = 0;
				}
			}

			ResolvedImageView Image::resolve()
			{
				ensureCreated();

				ResolvedImageView ret;
				ret.glImage = m_glImage;
				ret.glImageView = m_glImage;
				ret.glInternalFormat = m_glFormat;
				ret.glViewType = m_glViewType;
				ret.firstMip = 0;
				ret.firstSlice = 0;
				ret.numMips = setup().numMips;
				ret.numSlices = setup().numSamples;
				return ret;
			}

			void Image::ensureCreated()
			{
				PC_SCOPE_LVL1(CreateImage);

				// already created
				if (m_glImage != 0)
					return;

				// decompose texture format
				GLenum imageBaseFormat, imageBaseType;
				bool imageFormatCompressed = false;
				DecomposeTextureFormat(m_glFormat, imageBaseFormat, imageBaseType, imageFormatCompressed);

				// create texture object
				ASSERT(m_glImage == 0);
				GL_PROTECT(glCreateTextures(m_glViewType, 1, &m_glImage));
				TRACE_SPAM("GL: Created image {} from {}", m_glImage, setup());

				// label the object
				if (!setup().label.empty())
				{
					GL_PROTECT(glObjectLabel(GL_TEXTURE, m_glImage, setup().label.length(), setup().label.c_str()));
				}

				// setup texture storage
				switch (m_glViewType)
				{
				case GL_TEXTURE_1D:
					ASSERT(setup().numSlices == 1);
					GL_PROTECT(glTextureStorage1D(m_glImage, setup().numMips, m_glFormat, setup().width));
					break;

				case GL_TEXTURE_1D_ARRAY:
					GL_PROTECT(glTextureStorage2D(m_glImage, setup().numMips, m_glFormat, setup().width, setup().numSlices));
					break;

				case GL_TEXTURE_2D:
					ASSERT(setup().numSlices == 1);
					GL_PROTECT(glTextureStorage2D(m_glImage, setup().numMips, m_glFormat, setup().width, setup().height));
					break;

				case GL_TEXTURE_2D_MULTISAMPLE:
					ASSERT(setup().numSlices == 1);
					ASSERT(setup().numMips == 1);
					ASSERT(setup().numSamples >= 2);
					GL_PROTECT(glTextureStorage2DMultisample(m_glImage, setup().numSamples, m_glFormat, setup().width, setup().height, GL_TRUE));
					break;

				case GL_TEXTURE_2D_ARRAY:
					GL_PROTECT(glTextureStorage3D(m_glImage, setup().numMips, m_glFormat, setup().width, setup().height, setup().numSlices));
					break;

				case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
					ASSERT(setup().numMips == 1);
					ASSERT(setup().numSamples >= 2);
					GL_PROTECT(glTextureStorage3DMultisample(m_glImage, setup().numSamples, m_glFormat, setup().width, setup().height, setup().numSlices, GL_TRUE));
					break;

				case GL_TEXTURE_3D:
					ASSERT(setup().numSlices == 1);
					GL_PROTECT(glTextureStorage3D(m_glImage, setup().numMips, m_glFormat, setup().width, setup().height, setup().depth));
					break;

				case GL_TEXTURE_CUBE_MAP:
					GL_PROTECT(glTextureStorage2D(m_glImage, setup().numMips, m_glFormat, setup().width, setup().height));
					//GL_PROTECT(glTextureStorage3D(m_glImage, setup().m_numMips, imageFormat, setup().width, setup().height, 6));
					break;

				case GL_TEXTURE_CUBE_MAP_ARRAY:
					GL_PROTECT(glTextureStorage3D(m_glImage, setup().numMips, m_glFormat, setup().width, setup().height, setup().numSlices));
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
				GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_MAX_LEVEL, setup().numMips - 1));

				// setup some more state :)
				if (!setup().multisampled())
				{
					GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
					GL_PROTECT(glTextureParameteri(m_glImage, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
				}
			}

			void Image::copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range)
			{
				PC_SCOPE_LVL1(ImageCopyFromBuffer);

				ensureCreated();

				const auto& im = range.image;

				const auto rowLength = setup().calcRowLength(im.firstMip);
				const auto mipHeight = setup().calcMipHeight(im.firstMip);

				const auto pixelSize = GetImageFormatInfo(setup().format).bitsPerPixel / 8;
				//ASSERT_EX(view.offset % pixelSize == 0, "Unaligned staging data");

				GL_PROTECT(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
				GL_PROTECT(glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength));
				GL_PROTECT(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, mipHeight));
				GL_PROTECT(glPixelStorei(GL_UNPACK_SKIP_PIXELS, view.offset / pixelSize));
				GL_PROTECT(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, view.glBuffer));

				// TODO: support for sub-part ?

				// get texture type
				switch (m_glViewType)
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

			//--

			IBaseImageView* Image::createView_ClientApi(const IBaseImageView::Setup& setup, IBaseSampler* sampler)
			{
				auto* localSampler = static_cast<Sampler*>(sampler);
				return new ImageAnyView(owner(), this, localSampler, setup);
			}

			IBaseImageView* Image::createWritableView_ClientApi(const IBaseImageView::Setup& setup)
			{
				return new ImageAnyView(owner(), this, nullptr, setup);
			}

			IBaseImageView* Image::createRenderTargetView_ClientApi(const IBaseImageView::Setup& setup)
			{
				return new ImageAnyView(owner(), this, nullptr, setup);
			}

			//--

			void Image::applyCopyAtoms(const base::Array<ResourceCopyAtom>& atoms, Frame* frame, const StagingArea& area)
			{
				PC_SCOPE_LVL1(ImageCopyFromBuffer);

				ensureCreated();

				GL_PROTECT(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, area.apiPointer.gl));
				
				for (const auto& atom : atoms)
				{
					const auto& im = atom.copyElement.image;

					const auto rowLength = setup().calcRowLength(im.mip);
					const auto mipHeight = setup().calcMipHeight(im.mip);

					const auto pixelSize = GetImageFormatInfo(setup().format).bitsPerPixel / 8;
					//ASSERT_EX(view.offset % pixelSize == 0, "Unaligned staging data");

					GL_PROTECT(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
					GL_PROTECT(glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength));
					GL_PROTECT(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, mipHeight));
					GL_PROTECT(glPixelStorei(GL_UNPACK_SKIP_PIXELS, atom.stagingAreaOffset / pixelSize));

					// TODO: support for sub-part ?

					// get texture type
					switch (m_glViewType)
					{
					case GL_TEXTURE_1D:
						GL_PROTECT(glCopyTextureSubImage1D(m_glImage, im.mip, 0, 0, 0, setup().width));
						break;

					case GL_TEXTURE_2D:
						GL_PROTECT(glCopyTextureSubImage2D(m_glImage, im.mip, 0, 0, 0, 0, setup().width, setup().height));
						break;

					case GL_TEXTURE_2D_ARRAY:
						GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.mip, 0, 0, 0, 0, 0, setup().width, setup().height));
						break;

					case GL_TEXTURE_3D:
						GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.mip, 0, 0, im.slice, 0, 0, setup().width, setup().height));// TODO:!! , im.sizeZ));
						break;

					case GL_TEXTURE_CUBE_MAP:
						GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.mip, 0, 0, im.slice, 0, 0, setup().width, setup().height));
						break;

					case GL_TEXTURE_CUBE_MAP_ARRAY:
						GL_PROTECT(glCopyTextureSubImage3D(m_glImage, im.mip, 0, 0, im.slice, 0, 0, setup().width, setup().height));
						break;

					default:
						FATAL_ERROR("Invalid texture type");
					}
				}

				GL_PROTECT(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
			}

			//--

			ImageAnyView::ImageAnyView(Thread* owner, Image* img, Sampler* sampler, const Setup& setup)
				: IBaseImageView(owner, setup.writable ? ObjectType::ImageWritableView : ObjectType::ImageView, img, sampler, setup)
			{}

			ImageAnyView::~ImageAnyView()
			{
				if (m_glViewObject && m_glViewObject != image()->object())
				{
					GL_PROTECT(glDeleteTextures(1, &m_glViewObject));
					m_glViewObject = 0;
				}
			}

			ResolvedImageView ImageAnyView::resolve()
			{
				ensureCreated();

				ResolvedImageView ret;
				ret.glImage = image()->object();
				ret.glImageView = m_glViewObject;
				ret.glInternalFormat = image()->format();
				ret.glViewType = image()->viewType();
				ret.firstMip = setup().firstMip;
				ret.firstSlice = setup().firstSlice;
				ret.numMips = setup().numMips;
				ret.numSlices = setup().numSlices;
				return ret;
			}

			static bool IsMainView(const ImageAnyView::Setup& setup, const ImageCreationInfo& image)
			{
				if (setup.firstMip == 0 && setup.firstSlice == 0 &&
					setup.numMips == image.numMips && setup.numSlices && image.numSlices)
					return true;
				
				return false;
			}

			void ImageAnyView::ensureCreated()
			{
				// already created ?
				if (m_glViewObject != 0)
					return;

				// created based on parent image
				if (auto glImage = image()->object())
				{
					if (IsMainView(setup(), image()->setup()))
					{
						m_glViewObject = glImage;
					}
					else
					{
						GL_PROTECT(glGenTextures(1, &m_glViewObject));
						GL_PROTECT(glTextureView(m_glViewObject, image()->viewType(), glImage, image()->format(), setup().firstMip, setup().numMips, setup().firstSlice, setup().numSlices));
						//TRACE_SPAM("GL: Created image view {} of {} with {}", m_glViewObject, glImage, setup());
					}
				}
			}

			//--

		} // gl4
    } // api
} // rendering
