/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#include "build.h"
#include "renderingDeviceApi.h"
#include "renderingDeviceGlobalObjects.h"
#include "renderingResources.h"
#include "renderingImage.h"
#include "renderingSamplerState.h"

namespace rendering
{
	//--

	// initializer that fills texture with single color
	class SourceProviderSingleColor : public ISourceDataProvider
	{
	public:
		SourceProviderSingleColor(ImageFormat format, const void* color)
		{
			m_valueSize = GetImageFormatInfo(format).bitsPerPixel / 8;
			DEBUG_CHECK_RETURN(m_valueSize != 0);
			DEBUG_CHECK_RETURN(m_valueSize <= MAX_SIZE);

			memcpy(m_valueData, color, m_valueSize);
		}

		virtual base::StringView debugLabel() const
		{
			return "SourceProviderSingleColor";
		}

		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / m_valueSize;
			DEBUG_CHECK(info.dataSize % m_valueSize == 0);

			auto* ptr = (uint8_t*)destPointer;
			auto* endPtr = ptr + (numPixels * m_valueSize);
			while (ptr < endPtr)
			{
				memcpy(ptr, m_valueData, m_valueSize);
				ptr += m_valueSize;
			}
		}

	private:
		static const uint32_t MAX_SIZE = 16; // 4*4 for RGBA32F

		uint8_t m_valueData[MAX_SIZE];
		uint32_t m_valueSize = 0;
	};

	//--

	// fills each size with specified cubemap direction color
	class SourceProviderCubemapDirectionColor : public ISourceDataProvider
	{
	public:
		SourceProviderCubemapDirectionColor()
		{
		}

		virtual base::StringView debugLabel() const
		{
			return "SourceProviderCubemapDirectionColor";
		}

		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / sizeof(base::Color);
			DEBUG_CHECK(info.dataSize % sizeof(base::Color) == 0);

			const auto writeValue = COLORS[info.image.slice % 6];

			auto* ptr = (base::Color*)destPointer;
			auto* endPtr = ptr + numPixels;

			while (ptr < endPtr)
				*ptr++ = writeValue;
		}

	private:
		static const uint8_t N = 0;
		static const uint8_t Z = 127;
		static const uint8_t P = 255;

		static inline const base::Color COLORS[6] = {
			base::Color(N,Z,Z), // -X
			base::Color(P,Z,Z), // +X
			base::Color(Z,N,Z), // -Y
			base::Color(Z,P,Z), // +Y
			base::Color(Z,Z,N), // -Z
			base::Color(Z,Z,P), // +Z
		};
	};

	//--

	DeviceGlobalObjects::DeviceGlobalObjects(IDevice* dev)
		: m_device(dev)
	{
		createSamples();
		createTextures();
		createNoiseTextures();
	}

	DeviceGlobalObjects::~DeviceGlobalObjects()
	{}

    //--

	ImageViewPtr DeviceGlobalObjects::createDefaultTexture(const char* tag, uint32_t width, uint32_t height, ImageFormat format, ISourceDataProvider* data)
	{
		ImageCreationInfo info;
		info.view = ImageViewType::View2D;
		info.allowCopies = true;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.initialLayout = ResourceLayout::ShaderResource;
		info.width = width;
		info.height = height;
		info.format = format;
		info.label = tag;
		info.numMips = 1; // do not generate mips

		auto image = m_device->createImage(info, data);
		DEBUG_CHECK_RETURN_V(image, nullptr);

		return image->createView(SamplerClampPoint);
	}

	ImageViewPtr DeviceGlobalObjects::createDefaultTextureArray(const char* tag, uint32_t width, uint32_t height, uint32_t slices, ImageFormat format, ISourceDataProvider* data)
	{
		ImageCreationInfo info;
		info.view = ImageViewType::View2DArray;
		info.allowCopies = true;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.initialLayout = ResourceLayout::ShaderResource;
		info.width = width;
		info.height = height;
		info.numSlices = slices;
		info.format = format;
		info.label = tag;
		info.numMips = 1; // do not generate mips

		auto image = m_device->createImage(info, data);
		DEBUG_CHECK_RETURN_V(image, nullptr);

		return image->createView(SamplerClampPoint);
	}

	ImageViewPtr DeviceGlobalObjects::createDefaultTextureCube(const char* tag, uint32_t size, ImageFormat format, ISourceDataProvider* data)
	{
		ImageCreationInfo info;
		info.view = ImageViewType::ViewCube;
		info.allowCopies = true;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.initialLayout = ResourceLayout::ShaderResource;
		info.numSlices = 6;
		info.width = size;
		info.height = size;
		info.format = format;
		info.label = tag;
		info.numMips = 1; // do not generate mips

		auto image = m_device->createImage(info, data);
		DEBUG_CHECK_RETURN_V(image, nullptr);

		return image->createView(SamplerClampPoint);
	}

	ImageViewPtr DeviceGlobalObjects::createDefaultTextureCubeArray(const char* tag, uint32_t size, uint32_t slices, ImageFormat format, ISourceDataProvider* data)
	{
		ImageCreationInfo info;
		info.view = ImageViewType::ViewCubeArray;
		info.allowCopies = true;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.initialLayout = ResourceLayout::ShaderResource;
		info.width = size;
		info.height = size;
		info.numSlices = slices * 6;
		info.format = format;
		info.label = tag;
		info.numMips = 1; // do not generate mips

		auto image = m_device->createImage(info, data);
		DEBUG_CHECK_RETURN_V(image, nullptr);

		return image->createView(SamplerClampPoint);
	}

	ImageViewPtr DeviceGlobalObjects::createDefaultTexture3D(const char* tag, uint32_t width, uint32_t height, uint32_t depth, ImageFormat format, ISourceDataProvider* data)
	{
		ImageCreationInfo info;
		info.view = ImageViewType::View3D;
		info.allowCopies = true;
		info.allowShaderReads = true;
		info.allowUAV = true;
		info.initialLayout = ResourceLayout::ShaderResource;
		info.width = width;
		info.height = height;
		info.depth = depth;
		info.format = format;
		info.label = tag;
		info.numMips = 1; // do not generate mips

		auto image = m_device->createImage(info, data);
		DEBUG_CHECK_RETURN_V(image, nullptr);

		return image->createView(SamplerClampPoint);
	}

	//--

	void DeviceGlobalObjects::createTextures()
	{
		{
			const auto data = base::RefNew<SourceProviderSingleColor>(ImageFormat::RGBA8_UNORM, &base::Color::WHITE);
			TextureWhite = createDefaultTexture("DefaultWhite", 16, 16, ImageFormat::RGBA8_UNORM, data);
			TextureArrayWhite = createDefaultTextureArray("DefaultArrayWhite", 16, 16, 4, ImageFormat::RGBA8_UNORM, data);
			TextureCubeWhite = createDefaultTextureCube("DefaultCubeyWhite", 16, ImageFormat::RGBA8_UNORM, data);
		}

		{
			const auto data = base::RefNew<SourceProviderSingleColor>(ImageFormat::RGBA8_UNORM, &base::Color::BLACK);
			TextureBlack = createDefaultTexture("DefaultBlack", 16, 16, ImageFormat::RGBA8_UNORM, data);
			TextureArrayBlack = createDefaultTextureArray("DefaultArrayBlack", 16, 16, 4, ImageFormat::RGBA8_UNORM, data);
			TextureCubeBlack = createDefaultTextureCube("DefaultCubeBlack", 16, ImageFormat::RGBA8_UNORM, data);
			TextureCubeArrayBlack = createDefaultTextureCubeArray("DefaultCubeArrayBlack", 16, 4, ImageFormat::RGBA8_UNORM, data);
			Texture3DBlack = createDefaultTexture3D("Default3DBlack", 8, 8, 8, ImageFormat::RGBA8_UNORM, data);
		}

		{
			const auto data = base::RefNew<SourceProviderSingleColor>(ImageFormat::RGBA8_UNORM, &base::Color::GRAY);
			TextureGray = createDefaultTexture("DefaultGray", 16, 16, ImageFormat::RGBA8_UNORM, data);
			TextureArrayGray = createDefaultTextureArray("DefaultArrayGray", 16, 16, 4, ImageFormat::RGBA8_UNORM, data);
			TextureCubeGray = createDefaultTextureCube("DefaultCubeGray", 16, ImageFormat::RGBA8_UNORM, data);
		}

		{
			const auto color = base::Color(127, 127, 255, 255);
			const auto data = base::RefNew<SourceProviderSingleColor>(ImageFormat::RGBA8_UNORM, &color);
			TextureNormZ = createDefaultTexture("DefaultNormZ", 16, 16, ImageFormat::RGBA8_UNORM, data);
		}

		{
			const auto data = base::RefNew<SourceProviderCubemapDirectionColor>();
			TextureCubeColor = createDefaultTextureCube("DefaultDirectionCube", 16, ImageFormat::RGBA8_UNORM, data);
		}
	}

	//--

	// 0-1 noise
	class SourceProviderFloatNoise : public ISourceDataProvider
	{
	public:
		SourceProviderFloatNoise(float min, float max, bool gaussian)
			: m_min(min)
			, m_max(max)
			, m_gaussian(gaussian)
		{
		}

		virtual base::StringView debugLabel() const
		{
			return "SourceProviderFloatNoise";
		}

		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / sizeof(float);
			DEBUG_CHECK(info.dataSize % sizeof(float) == 0);

			auto* ptr = (float*)destPointer;
			auto* endPtr = ptr + numPixels;

			base::MTRandState rnd;

			/*if (m_gaussian)
			{ }*/

			// TODO:

			while (ptr < endPtr)
				*ptr++ = rnd.range(m_min, m_max);
		}

	private:
		float m_min = 0.0f;
		float m_max = 1.0f;
		bool m_gaussian = false;
	};

	//--

	// RGBA noise
	class SourceProviderRGBANoise : public ISourceDataProvider
	{
	public:
		virtual base::StringView debugLabel() const
		{
			return "SourceProviderRGBANoise";
		}

		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / sizeof(uint32_t);
			DEBUG_CHECK(info.dataSize % sizeof(uint32_t) == 0);

			auto* ptr = (uint32_t*)destPointer;
			auto* endPtr = ptr + numPixels;

			base::MTRandState rnd;

			while (ptr < endPtr)
				*ptr++ = rnd.next();
		}
	};

	//--

	// Sin/Cos noise
	class SourceProviderRotationNoise : public ISourceDataProvider
	{
	public:
		virtual base::StringView debugLabel() const
		{
			return "SourceProviderRotationNoise";
		}

		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / 4;
			DEBUG_CHECK(info.dataSize % 4 == 0);

			auto* ptr = (uint16_t*)destPointer;
			auto* endPtr = ptr + numPixels;

			base::MTRandState rnd;

			while (ptr < endPtr)
			{
				auto angle = rnd.unit() * TWOPI;
				*ptr++ = base::Float16Helper::Compress(cos(angle));
				*ptr++ = base::Float16Helper::Compress(sin(angle));
			}
		}
	};

	//--

	// Sphere noise
	class SourceProviderHemiSpherePoints : public ISourceDataProvider
	{
	public:
		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / 8;
			DEBUG_CHECK(info.dataSize % 8 == 0);

			auto* ptr = (uint16_t*)destPointer;
			auto* endPtr = ptr + numPixels;

			base::MTRandState rnd;

			while (ptr < endPtr)
			{
				auto point = base::RandHemiSphereSurfacePoint(rnd.unit2());
				*ptr++ = base::Float16Helper::Compress(point.x);
				*ptr++ = base::Float16Helper::Compress(point.y);
				*ptr++ = base::Float16Helper::Compress(point.z);
				*ptr++ = rnd.unit();				
			}
		}
	};

	//--

	// Sphere noise
	class SourceProviderSpherePoints : public ISourceDataProvider
	{
	public:
		virtual CAN_YIELD void writeSourceData(void* destPointer, void* token, const ResourceCopyElement& info) const override final
		{
			const auto numPixels = info.dataSize / 8;
			DEBUG_CHECK(info.dataSize % 8 == 0);

			auto* ptr = (uint16_t*)destPointer;
			auto* endPtr = ptr + numPixels;

			base::MTRandState rnd;

			while (ptr < endPtr)
			{
				auto point = base::RandSphereSurfacePoint(rnd.unit2(), base::Vector3::ZERO(), 1.0f);
				*ptr++ = base::Float16Helper::Compress(point.x);
				*ptr++ = base::Float16Helper::Compress(point.y);
				*ptr++ = base::Float16Helper::Compress(point.z);
				*ptr++ = rnd.unit();
			}
		}
	};

	
	void DeviceGlobalObjects::createNoiseTextures()
	{
		{
			auto data = base::RefNew<SourceProviderFloatNoise>(0.0f, 1.0f, false);
			NoiseUniform = createDefaultTexture("NoiseUniform", 256, 256, ImageFormat::R32F, data);
		}

		{
			auto data = base::RefNew<SourceProviderFloatNoise>(0.0f, 1.0f, true);
			NoiseGausian = createDefaultTexture("NoiseGausian", 256, 256, ImageFormat::R32F, data);
		}

		{
			auto data = base::RefNew<SourceProviderRGBANoise>();
			NoiseRGBA = createDefaultTexture("NoiseRGBA", 256, 256, ImageFormat::RGBA8_UNORM, data);
		}

		{
			auto data = base::RefNew<SourceProviderRotationNoise>();
			NoiseRotation = createDefaultTexture("NoiseRotation", 256, 256, ImageFormat::RG16F, data);
			
		}

		{
			auto data = base::RefNew<SourceProviderSpherePoints>();
			NoiseDirection = createDefaultTexture("NoiseDirection", 256, 256, ImageFormat::RGBA16F, data);
		}

		{
			auto data = base::RefNew<SourceProviderHemiSpherePoints>();
			NoiseHemisphere = createDefaultTexture("NoiseHemisphere", 256, 256, ImageFormat::RGBA16F, data);
		}
	}

	void DeviceGlobalObjects::createSamples()
	{
		{
			SamplerState ss;
			ss.magFilter = FilterMode::Nearest;
			ss.minFilter = FilterMode::Nearest;
			ss.mipmapMode = MipmapFilterMode::Nearest;

			ss.addresModeU = AddressMode::Clamp;
			ss.addresModeV = AddressMode::Clamp;
			ss.addresModeW = AddressMode::Clamp;
			ss.label = "SamplerClampPoint";
			SamplerClampPoint = m_device->createSampler(ss);

			ss.addresModeU = AddressMode::Wrap;
			ss.addresModeV = AddressMode::Wrap;
			ss.addresModeW = AddressMode::Wrap;
			ss.label = "SamplerWrapPoint";
			SamplerWrapPoint = m_device->createSampler(ss);
		}

		{
			SamplerState ss;
			ss.magFilter = FilterMode::Linear;
			ss.minFilter = FilterMode::Linear;
			ss.mipmapMode = MipmapFilterMode::Nearest;

			ss.addresModeU = AddressMode::Clamp;
			ss.addresModeV = AddressMode::Clamp;
			ss.addresModeW = AddressMode::Clamp;
			ss.label = "SamplerClampBiLinear";
			SamplerClampBiLinear = m_device->createSampler(ss);

			ss.addresModeU = AddressMode::Wrap;
			ss.addresModeV = AddressMode::Wrap;
			ss.addresModeW = AddressMode::Wrap;
			ss.label = "SamplerWrapBiLinear";
			SamplerWrapBiLinear = m_device->createSampler(ss);
		}

		{
			SamplerState ss;
			ss.magFilter = FilterMode::Linear;
			ss.minFilter = FilterMode::Linear;
			ss.mipmapMode = MipmapFilterMode::Linear;

			ss.addresModeU = AddressMode::Clamp;
			ss.addresModeV = AddressMode::Clamp;
			ss.addresModeW = AddressMode::Clamp;
			ss.label = "SamplerClampTriLinear";
			SamplerClampTriLinear = m_device->createSampler(ss);

			ss.addresModeU = AddressMode::Wrap;
			ss.addresModeV = AddressMode::Wrap;
			ss.addresModeW = AddressMode::Wrap;
			ss.label = "SamplerWrapTriLinear";
			SamplerWrapTriLinear = m_device->createSampler(ss);
		}

		{
			SamplerState ss;
			ss.magFilter = FilterMode::Linear;
			ss.minFilter = FilterMode::Linear;
			ss.mipmapMode = MipmapFilterMode::Linear;
			ss.maxAnisotropy = 16;

			ss.addresModeU = AddressMode::Clamp;
			ss.addresModeV = AddressMode::Clamp;
			ss.addresModeW = AddressMode::Clamp;
			ss.label = "SamplerClampAniso";
			SamplerClampAniso = m_device->createSampler(ss);

			ss.addresModeU = AddressMode::Wrap;
			ss.addresModeV = AddressMode::Wrap;
			ss.addresModeW = AddressMode::Wrap;
			ss.label = "SamplerWrapAniso";
			SamplerWrapAniso = m_device->createSampler(ss);
		}

		{
			SamplerState ss;
			ss.magFilter = FilterMode::Nearest;
			ss.minFilter = FilterMode::Nearest;
			ss.mipmapMode = MipmapFilterMode::Nearest;
			ss.addresModeU = AddressMode::Clamp;
			ss.addresModeV = AddressMode::Clamp;
			ss.addresModeW = AddressMode::Clamp;
			ss.compareEnabled = true;

			ss.compareOp = CompareOp::LessEqual;
			ss.label = "SamplerPointDepthLE";
			SamplerPointDepthLE = m_device->createSampler(ss);

			ss.compareOp = CompareOp::GreaterEqual;
			ss.label = "SamplerPointDepthGE";
			SamplerPointDepthGE = m_device->createSampler(ss);
		}

		{
			SamplerState ss;
			ss.magFilter = FilterMode::Linear;
			ss.minFilter = FilterMode::Linear;
			ss.mipmapMode = MipmapFilterMode::Nearest;
			ss.addresModeU = AddressMode::Clamp;
			ss.addresModeV = AddressMode::Clamp;
			ss.addresModeW = AddressMode::Clamp;
			ss.compareEnabled = true;

			ss.compareOp = CompareOp::LessEqual;
			ss.label = "SamplerBiLinearDepthLE";
			SamplerBiLinearDepthLE = m_device->createSampler(ss);

			ss.compareOp = CompareOp::GreaterEqual;
			ss.label = "SamplerBiLinearDepthGE";
			SamplerBiLinearDepthGE = m_device->createSampler(ss);
		}
	}

	//--

} // rendering

