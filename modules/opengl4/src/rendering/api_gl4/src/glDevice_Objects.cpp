/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "glDevice.h"
#include "glBuffer.h"
#include "glImage.h"
#include "glSampler.h"
#include "glDeviceThread.h"
#include "glShaders.h"
#include "glObjectCache.h"
#include "glObjectRegistry.h"

#include "rendering/device/include/renderingBuffer.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
    namespace gl4
    {
        ///---

        /*void Device::createPredefinedImages()
        {
            //--

            createPredefinedImageFromColor(ID_BlackTexture, base::Color(0, 0, 0, 0), ImageFormat::RGBA8_UNORM, "DefaultBlack");
            createPredefinedImageFromColor(ID_WhiteTexture, base::Color(255, 255, 255, 255), ImageFormat::RGBA8_UNORM, "DefaultWhite");
            createPredefinedImageFromColor(ID_GrayLinearTexture, base::Color(127, 127, 127, 255), ImageFormat::RGBA8_UNORM, "DefaultLinearGray");
            createPredefinedImageFromColor(ID_GraySRGBTexture, base::Color(170, 170, 170, 255), ImageFormat::RGBA8_UNORM, "DefaultGammaGray");
            createPredefinedImageFromColor(ID_NormZTexture, base::Color(127, 127, 255, 255), ImageFormat::RGBA8_UNORM, "DefaultNormal");

            //--

            createPredefinedRenderTarget(ID_DefaultDepthRT, ImageFormat::D24S8, 1, "DefaultDepthRT");
            createPredefinedRenderTarget(ID_DefaultColorRT, ImageFormat::RGBA16F, 1, "DefaultColorRT");
            createPredefinedRenderTarget(ID_DefaultDepthArrayRT, ImageFormat::D24S8, 4, "DefaultDepthArrayRT");

            //--
        }

        void Device::createPredefinedSamplers()
        {
            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                createPredefinedSampler(ID_SamplerClampPoint, info, "ClampPoint");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapPoint, info, "WrapPoint");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                createPredefinedSampler(ID_SamplerClampBiLinear, info, "ClampBilinear");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapBiLinear, info, "WrapBilinear");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Linear;
                createPredefinedSampler(ID_SamplerClampTriLinear, info, "ClampTrilinear");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapTriLinear, info, "WrapTrilinear");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Linear;
                info.maxAnisotropy = 16; // TODO: limit by driver settings
                createPredefinedSampler(ID_SamplerClampAniso, info, "ClampAniso");
                info.addresModeU = AddressMode::Wrap;
                info.addresModeV = AddressMode::Wrap;
                info.addresModeW = AddressMode::Wrap;
                createPredefinedSampler(ID_SamplerWrapAniso, info, "WrapAniso");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::LessEqual;
                createPredefinedSampler(ID_SamplerPointDepthLE, info, "PointDepthLT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Nearest;
                info.minFilter = FilterMode::Nearest;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::GreaterEqual;
                createPredefinedSampler(ID_SamplerPointDepthGE, info, "PointDepthGT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::LessEqual;
                createPredefinedSampler(ID_SamplerBiLinearDepthLE, info, "BilinearDepthLT");
            }

            {
                SamplerState info;
                info.magFilter = FilterMode::Linear;
                info.minFilter = FilterMode::Linear;
                info.mipmapMode = MipmapFilterMode::Nearest;
                info.compareEnabled = true;
                info.compareOp = CompareOp::GreaterEqual;
                createPredefinedSampler(ID_SamplerBiLinearDepthGE, info, "BilinearDepthGT");
            }
        }*/
        
        //----

        BufferObjectPtr Device::createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished)
        {
            // NOTE: no lock since the actual buffer shit is created on first actual binding
            
            uint8_t numExclusiveModes = 0;
            if (info.allowCostantReads) numExclusiveModes += 1;
            if (info.allowVertex) numExclusiveModes += 1;
            if (info.allowIndirect) numExclusiveModes += 1;
            if (info.allowIndex) numExclusiveModes += 1;
            //if (info.allowShaderReads) numExclusiveModes += 1;
			DEBUG_CHECK_RETURN_EX_V(numExclusiveModes <= 1, "Only one of the exclusive primary usage modes must be set: vertex, index, uniform, shader or indirect", nullptr);

            // constant buffer can't be read as a format buffer
            DEBUG_CHECK_RETURN_EX_V(!info.allowCostantReads || !info.allowShaderReads, "Buffer can't be used as uniform and shader buffer at the same time (it might by UAV buffer though)", nullptr);

			// align to min size
			DEBUG_CHECK_RETURN_EX_V(!info.allowCostantReads || ((info.size & 15) == 0), "Constant buffer size must be aligned to 16 bytes", nullptr);

            // create the buffer in the static pool
            auto* buffer = new Buffer(this, info);

            // describe buffer
            BufferObject::Setup setup;
            setup.flags = info.computeFlags();
            setup.layout = info.initialLayout;
            setup.size = info.size;
            setup.stride = info.stride;

			if (info.initialLayout == ResourceLayout::INVALID)
				setup.layout = info.computeDefaultLayout();
			else
				setup.layout = info.initialLayout;

            if (info.stride)
                setup.flags |= BufferViewFlag::Structured;

			if (sourceData)
			{
				ResourceCopyRange range;
				range.buffer.offset = 0;
				range.buffer.size = info.size;
				if (!m_thread->scheduleAsyncCopy_ClientApi(buffer, range, sourceData, initializationFinished))
				{
					TRACE_ERROR("GL: Error setting up initial data, buffer creation from '{}' will fail", info);

					delete buffer; // legal since no GPU objects were created yet
					return nullptr;
				}
			}

            return base::RefNew<DeviceBufferObject>(buffer->handle(), m_objectRegistry->proxy(), setup);			
        }

        ///---

        class DeviceImageObject : public ImageObject
        {
        public:
            DeviceImageObject(ObjectID id, ObjectRegistryProxy* impl, const Setup& setup)
                : ImageObject(id, impl, setup)
				, m_proxy(impl)
            {}

            virtual ImageViewPtr createView(SamplerObject* sampler, uint8_t firstMip, uint8_t numMips) override
            {
                rendering::ImageView::Setup setup;
                DEBUG_CHECK_RETURN_V(validateCreateView(sampler, firstMip, 0, numMips, slices(), setup), nullptr);

				ImageViewPtr ret;
				if (auto proxy = m_proxy.lock())
				{
					auto* sampler = proxy->resolveStatic<Sampler>(setup.sampler->id());

					proxy->run<Image>(id(), [this, setup, sampler, proxy, &ret](Image* ptr)
						{
							ImageViewKey key;
							key.viewType = type();
							key.format = format();
							key.firstMip = setup.firstMip;
							key.numMips = setup.numMips;
							key.firstSlice = setup.firstSlice;
							key.numSlices = setup.numSlices;

							if (auto view = ptr->createView_ClientAPI(key, sampler)) // rendering level
								ret = base::RefNew<rendering::ImageView>(view->handle(), this, proxy, setup); // API level
						});
				}

                return ret;
            }

            virtual ImageViewPtr createArrayView(SamplerObject* sampler, uint8_t firstMip, uint32_t firstSlice, uint8_t numMips, uint32_t numSlices) override
            {
                rendering::ImageView::Setup setup;
                DEBUG_CHECK_RETURN_V(validateCreateView(sampler, firstMip, firstSlice, numMips, numSlices, setup), nullptr);

                return nullptr;
            }

            virtual ImageWritableViewPtr createWritableView(uint8_t mip /*= 0*/, uint32_t slice /*= 0*/) override
            {
                rendering::ImageWritableView::Setup setup;
                DEBUG_CHECK_RETURN_V(validateCreateWritableView(mip, slice, setup), nullptr);

                return nullptr;
            }

            virtual RenderTargetViewPtr createRenderTargetView(uint8_t mip, uint32_t firstSlice, uint32_t numSlices) override
            {
                rendering::RenderTargetView::Setup setup;
                DEBUG_CHECK_RETURN_V(validateCreateRenderTargetView(mip, firstSlice, numSlices, setup), nullptr);

				RenderTargetViewPtr ret;
				if (auto proxy = m_proxy.lock())
				{
					proxy->run<Image>(id(), [this, setup, proxy, &ret](Image* ptr)
						{
							ImageViewKey key;
							key.viewType = type();
							key.format = format();
							key.firstMip = setup.mip;
							key.numMips = 1;
							key.firstSlice = setup.firstSlice;
							key.numSlices = setup.numSlices;

							if (auto view = ptr->createView_ClientAPI(key, nullptr)) // rendering level
								ret = base::RefNew<rendering::RenderTargetView>(view->handle(), this, proxy, setup); // API level
						});
				}

				return ret;
            }

		private:
			base::RefWeakPtr<ObjectRegistryProxy> m_proxy;
        };

        ImageObjectPtr Device::createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished)
        {
            // create the image in the static pool
            auto image = new Image(this, info);
            if (!image)
                return nullptr;

            // setup flags
            auto flags = info.computeViewFlags();
            if (sourceData)
                flags |= ImageViewFlag::Preinitialized;

            // configure
            ImageObject::Setup setup;
            setup.key = ImageViewKey(info.format, info.view, 0, info.numSlices, 0, info.numMips);
            setup.width = info.width;
            setup.height = info.height;
            setup.depth = info.depth;
            setup.flags = info.computeViewFlags();
            setup.numSamples = info.numSamples;

			// upload initial data
			if (sourceData)
			{
				ResourceCopyRange range;
				memzero(&range, sizeof(range));
				range.image.format = info.format;
				range.image.numMips = info.numMips;
				range.image.numSlices = info.numSlices;
				range.image.sizeX = info.width;
				range.image.sizeY = info.height;
				range.image.sizeZ = info.depth;

				if (!m_thread->scheduleAsyncCopy_ClientApi(image, range, sourceData, initializationFinished))
				{
					TRACE_ERROR("GL: Error setting up initial data, image creation from '{}' will fail", info);

					delete image; // legal since no GPU objects were created yet
					return nullptr;
				}
			}

            return base::RefNew<DeviceImageObject>(image->handle(), m_objectRegistry->proxy(), setup);
        }

        SamplerObjectPtr Device::createSampler(const SamplerState& info)
        {
            auto sampler = new Sampler(this, info);
            return base::RefNew<SamplerObject>(sampler->handle(), m_objectRegistry->proxy());
        }

        ShaderObjectPtr Device::createShaders(const ShaderLibraryData* shaders)
        {
            DEBUG_CHECK_RETURN_V(shaders, nullptr);

            auto object = new Shaders(this, shaders);
            return base::RefNew<ShaderObject>(object->handle(), AddRef(shaders), m_objectRegistry->proxy());
        }

        //--

		void Device::asyncCopy(const IDeviceObject* object, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter initializationFinished /*= base::fibers::WaitCounter()*/)
		{
			DEBUG_CHECK_RETURN(object != nullptr);
			DEBUG_CHECK_RETURN(sourceData != nullptr);

			bool validObject = false;
			m_objectRegistry->runWithObject(object->id(), [this, range, sourceData, initializationFinished, &validObject](Object* obj)
				{
					validObject = m_thread->scheduleAsyncCopy_ClientApi(obj, range, sourceData, initializationFinished);
				});

			if (!validObject)
			{
				TRACE_ERROR("GL: Failed to start async copy operation to a buffer");
				if (!initializationFinished.empty())
					Fibers::GetInstance().signalCounter(initializationFinished);
			}
		}
		
		//--

    } // gl4
} // rendering
