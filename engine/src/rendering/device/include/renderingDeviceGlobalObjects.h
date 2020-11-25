/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#pragma once

#include "base/app/include/localService.h"
#include "base/app/include/commandline.h"

namespace rendering
{
    //---

	/// common default textures/buffers
	class RENDERING_DEVICE_API DeviceGlobalObjects : public base::NoCopy
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);

	public:
		DeviceGlobalObjects(IDevice* dev);
		~DeviceGlobalObjects();

		//--
		// default resources

		ImageViewPtr TextureWhite; // 1,1,1
		ImageViewPtr TextureBlack; // 0,0,0
		ImageViewPtr TextureGray; // 0.5, 0.5, 0.5f linear
		ImageViewPtr TextureNormZ; // 0.5, 0.5, 1, linear

		ImageViewPtr TextureArrayBlack; // 4 slices
		ImageViewPtr TextureArrayWhite; // 4 slices
		ImageViewPtr TextureArrayGray; // 4 slices

		ImageViewPtr TextureCubeBlack;
		ImageViewPtr TextureCubeWhite;
		ImageViewPtr TextureCubeGray;
		ImageViewPtr TextureCubeColor; // face colors symbolize the directions

		ImageViewPtr TextureCubeArrayBlack; 

		ImageViewPtr Texture3DBlack; // 8x8x8, R16F single channel

		//--
		// default samplers

		SamplerObjectPtr SamplerClampPoint;
		SamplerObjectPtr SamplerClampBiLinear;
		SamplerObjectPtr SamplerClampTriLinear;
		SamplerObjectPtr SamplerClampAniso;
		SamplerObjectPtr SamplerWrapPoint;
		SamplerObjectPtr SamplerWrapBiLinear;
		SamplerObjectPtr SamplerWrapTriLinear;
		SamplerObjectPtr SamplerWrapAniso;

		SamplerObjectPtr SamplerPointDepthLE;
		SamplerObjectPtr SamplerPointDepthGE;
		SamplerObjectPtr SamplerBiLinearDepthLE;
		SamplerObjectPtr SamplerBiLinearDepthGE;

		//--
		// useful textures (some may be loaded from files)

		ImageViewPtr NoiseUniform; // single channel noise texture, uniform distribution, [0,1] 256x256, R32F
		ImageViewPtr NoiseGausian; // single channel noise texture, Gaussian distribution, [0,1] mean at 0.5, 256x256, R32F
		ImageViewPtr NoiseRGBA; // four channel noise texture, 256x256, RGBA8
		ImageViewPtr NoiseRotation; // computed sin/cos of random angle, 256x256, RG16F
		ImageViewPtr NoiseDirection; // random 3D direction (on sphere surface), 128x128, RGBA16F
		ImageViewPtr NoiseHemisphere; // random 3D direction (on Z+ hemi sphere), 128x128, RGBA16F

	private:
		ImageViewPtr createDefaultTexture(const char* tag, uint32_t width, uint32_t height, ImageFormat format, ISourceDataProvider* data);
		ImageViewPtr createDefaultTextureArray(const char* tag, uint32_t width, uint32_t height, uint32_t slices, ImageFormat format, ISourceDataProvider* data);
		ImageViewPtr createDefaultTextureCube(const char* tag, uint32_t size, ImageFormat format, ISourceDataProvider* data);
		ImageViewPtr createDefaultTextureCubeArray(const char* tag, uint32_t size, uint32_t slices, ImageFormat format, ISourceDataProvider* data);
		ImageViewPtr createDefaultTexture3D(const char* tag, uint32_t width, uint32_t height, uint32_t depth, ImageFormat format, ISourceDataProvider* data);

		void createTextures();
		void createNoiseTextures();
		void createSamples();

		IDevice* m_device = nullptr;
	};

	//--

	// get global objects (NOTE: valid only if device service was started)
	extern RENDERING_DEVICE_API const DeviceGlobalObjects& Globals();

	//--

} // rendering


