/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\object #]
***/

#pragma once

#include "renderingImageFormat.h"
#include "renderingResources.h"
#include "renderingObject.h"

namespace rendering
{

    ///---

    /// view description "key" for map lookups
    struct RENDERING_DEVICE_API ImageViewKey
    {
        uint16_t firstSlice = 0;
        uint16_t numSlices = 0;
        ImageViewType viewType = ImageViewType::View2D;
        ImageFormat format = ImageFormat::UNKNOWN;
        uint8_t firstMip = 0;
        uint8_t numMips = 0;

        INLINE ImageViewKey() {};
        INLINE ImageViewKey(const ImageViewKey& other) = default;
        INLINE ImageViewKey& operator=(const ImageViewKey& other) = default;

        INLINE operator bool() const { return format != ImageFormat::UNKNOWN; }
        INLINE uint64_t key() const { return *(const uint64_t*)this; }

        INLINE bool cubemap() const { return (viewType == ImageViewType::ViewCube) || (viewType == ImageViewType::ViewCubeArray); }
        INLINE bool array() const { return (viewType == ImageViewType::View1DArray) || (viewType == ImageViewType::View2DArray) || (viewType == ImageViewType::ViewCubeArray); }

        INLINE ImageViewKey(ImageFormat format_, ImageViewType viewType_, uint16_t firstArraySlice_, uint16_t numArraySlices_, uint8_t firstMip_, uint8_t numMips_)
            : viewType(viewType_)
            , format(format_)
            , firstSlice(firstArraySlice_)
            , numSlices(numArraySlices_)
            , firstMip(firstMip_)
            , numMips(numMips_)
        {
            validate();        
        }

        INLINE bool operator==(const ImageViewKey& other) const { return key() == other.key(); }
        INLINE bool operator!=(const ImageViewKey & other) const { return key() != other.key(); }

        void print(base::IFormatStream& f) const;
        void validate();

        INLINE static uint32_t CalcHash(const ImageViewKey& key) { return base::CRC32() << key.key(); }
    };

    static_assert(sizeof(ImageViewKey) == 8, "There are places that take assumptions of layout of this structure");

	//--

    // sampled texture (SRV) view of image, access to all mips/slices
	class RENDERING_DEVICE_API ImageSampledView : public IDeviceObjectView
	{
		RTTI_DECLARE_VIRTUAL_CLASS(ImageSampledView, IDeviceObjectView);

	public:
		struct Setup
		{
			uint8_t firstMip = 0;
			uint8_t numMips = 0;
			uint32_t firstSlice = 0;
			uint32_t numSlices = 0;
		};

		ImageSampledView(ObjectID viewId, ImageObject* img, IDeviceObjectHandler* impl, const Setup& setup);
		virtual ~ImageSampledView();

		// get the original image, guaranteed to be alive
		INLINE ImageObject* image() const { return (ImageObject*)object(); }

		// get the first mipmap (in the source image) we are viewing
		INLINE uint8_t firstMip() const { return m_firstMip; }

		// get number of mipmaps in the view
		INLINE uint8_t mips() const { return m_numMips; }

		// get the first slice (in the source image) we are viewing
		INLINE uint32_t firstSlice() const { return m_firstSlice; }

		// get number of slices in the view
		INLINE uint32_t slices() const { return m_numSlices; }

	private:
		uint8_t m_firstMip = 0;
		uint8_t m_numMips = 0;
		uint32_t m_firstSlice = 0;
		uint32_t m_numSlices = 0;
	};

    //--

    // read only view of a single mip/slice in the image
    class RENDERING_DEVICE_API ImageReadOnlyView : public IDeviceObjectView
    {
		RTTI_DECLARE_VIRTUAL_CLASS(ImageReadOnlyView, IDeviceObjectView);

    public:
        struct Setup
        {
            uint8_t mip = 0;
            uint32_t slice = 0;
        };

		ImageReadOnlyView(ObjectID viewId, ImageObject* img, IDeviceObjectHandler* impl, const Setup& setup);
        virtual ~ImageReadOnlyView();

        // get the original image, guaranteed to be alive
        INLINE ImageObject* image() const { return (ImageObject*)object(); }

		// index of the mip we are writing view this view
		INLINE uint8_t mip() const { return m_mip; }

		// index of the slice we are writing view this view
		INLINE uint32_t slice() const { return m_slice; }

    private:
		uint8_t m_mip = 0;
		uint32_t m_slice = 0;
    };

    //--

    // writable texture (UAV) view of image
    class RENDERING_DEVICE_API ImageWritableView : public IDeviceObjectView
    {
		RTTI_DECLARE_VIRTUAL_CLASS(ImageWritableView, IDeviceObjectView);

    public:
        struct Setup
        {
            uint8_t mip = 0;
            uint32_t slice = 0;
        };

        ImageWritableView(ObjectID viewId, ImageObject* img, IDeviceObjectHandler* impl, const Setup& setup);
        virtual ~ImageWritableView();

        // get the original image, guaranteed to be alive
        INLINE ImageObject* image() const { return (ImageObject*)object(); }

        // index of the mip we are writing view this view
        INLINE uint8_t mip() const { return m_mip; }

        // index of the slice we are writing view this view
        INLINE uint32_t slice() const { return m_slice; }

    private:
        uint8_t m_mip = 0;
        uint32_t m_slice = 0;
    };

    //--

    // special view of the image (or output) that can be used in the passes as rendering surface
    // NOTE: the owning object of this can be different and is not always image
    class RENDERING_DEVICE_API RenderTargetView : public IDeviceObjectView
    {
		RTTI_DECLARE_VIRTUAL_CLASS(RenderTargetView, IDeviceObjectView);

    public:
        struct Setup
        {
            ImageFormat format;
            uint8_t mip = 0; // in original image
            uint8_t samples = 0; // number of MSAA samples
            uint32_t firstSlice = 0; // in original image
            uint32_t numSlices = 0; // in original image
            uint32_t width = 0;
            uint32_t height = 0;
            bool flipped = false;
            bool swapchain = false;
            bool depth = false;
            bool srgb = false;
            bool arrayed = false;
            bool msaa = false;
        };

        RenderTargetView(ObjectID viewId, IDeviceObject* owner, IDeviceObjectHandler* impl, const Setup& setup);
        virtual ~RenderTargetView();

        //--

        // width of the TOTAL rendering surface
        INLINE uint32_t width() const { return m_width; }

        // height of the TOTAL rendering surface
        INLINE uint32_t height() const { return m_height; }

        // format of the target surface
        INLINE ImageFormat format() const { return m_format; }

        // number of MSAA samples
        INLINE uint8_t samples() const { return m_samples; }

        //--

        // index of the mip we are rendering view this view
        INLINE uint8_t mip() const { return m_mip; }

        // index of the slice we are rendering view this view
        INLINE uint32_t slices() const { return m_numSlices; }

        // index of the slice we are rendering view this view
        INLINE uint32_t firstSlice() const { return m_firstSlice; }

        //--

        // is this render target with slices?
        INLINE bool arrayed() const { return m_array; }

        // is the surface coming from swapchain ?
        INLINE bool swapchain() const { return m_swapchain; }

        // is this surface vertically flipped on display (OpenGL style)
        INLINE bool flipped() const { return m_flipped; }

        // is this surface SRGB
        INLINE bool srgb() const { return m_srgb; }

        // is this surface a depth buffer
        INLINE bool depth() const { return m_depth; }

    private:
        uint32_t m_width = 0;
        uint32_t m_height = 0;

        ImageFormat m_format = ImageFormat::UNKNOWN;
        uint8_t m_mip = 0;
        uint8_t m_samples = 0;

        uint32_t m_firstSlice = 0;
        uint32_t m_numSlices = 0;

        bool m_swapchain : 1;
        bool m_depth : 1;
        bool m_srgb : 1;
        bool m_array : 1;
        bool m_flipped : 1;
    };

    //---

    // image object wrapper
    class RENDERING_DEVICE_API ImageObject : public IDeviceObject
    {
		RTTI_DECLARE_VIRTUAL_CLASS(ImageObject, IDeviceObject);

    public:
        struct Setup
        {
            ImageViewKey key; // image type/dimensions/format
            ImageViewFlags flags = ImageViewFlags(); // view flags
            uint8_t numSamples = 0; // num samples in the texture, 0 - NO MSAA, 1 - MSAA with 1 sample :(
            uint32_t width = 0; // width of the image in the view
            uint32_t height = 0; // height of the image in the view
            uint32_t depth = 0; // depth of the image in the view
			ResourceLayout initialLayout = ResourceLayout::INVALID;
        };

        ImageObject(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup);
        virtual ~ImageObject();

        //--

        // image type/dimensions/format
        INLINE const ImageViewKey& key() const { return m_key; }

        // image format
        INLINE ImageFormat format() const { return m_key.format; }

        // view dimensions
        INLINE ImageViewType type() const { return m_key.viewType; }

        // general flags
        INLINE ImageViewFlags flags() const { return m_flags; }

        // number of samples (only MSAA images)
        INLINE uint8_t samples() const { return m_numSamples; }

        // width of the image
        INLINE uint32_t width() const { return m_width; }

        // height of the image
        INLINE uint32_t height() const { return m_height; }

        // depth of the image (3D textures only)
        INLINE uint32_t depth() const { return m_depth; }

        // number of slices in the image (only ARRAYS)
        INLINE uint16_t slices() const { return m_key.numSlices; }

        // number of mipmaps in the image
        INLINE uint8_t mips() const { return m_key.numMips; }

		// initial resource layout
		INLINE ResourceLayout initialLayout() const { return m_initialLayout; }

        //--

        // is image preinitialized with static content (usually true for asset based textures)
        INLINE bool preinitialized() const { return m_flags.test(ImageViewFlag::Preinitialized); }

        // can the image be dynamically updated from CPU
        INLINE bool dynamic() const { return m_flags.test(ImageViewFlag::Dynamic); }

        // can the image be used in shaders (usually true) but not true for swap chain images (we can't read them)
        INLINE bool shaderReadable() const { return m_flags.test(ImageViewFlag::ShaderReadable); }

        // can we copy to/from the image
        INLINE bool copyCapable() const { return m_flags.test(ImageViewFlag::CopyCapable); }

        // can the image be written to in shaders ?
        INLINE bool uavCapable() const { return m_flags.test(ImageViewFlag::UAVCapable); }

        // can the image be used as render target (color)
        INLINE bool renderTarget() const { return m_flags.test(ImageViewFlag::RenderTarget); }

        // can the image be used as depth/stencil buffer
        INLINE bool renderTargetDepth() const { return m_flags.test(ImageViewFlag::Depth); }

        // is the image multisamples
        INLINE bool multisampled() const { return m_flags.test(ImageViewFlag::Multisampled); }

        // is the image compressed format (BC crap)
        INLINE bool compressed() const { return m_flags.test(ImageViewFlag::Compressed); }

        // is this a swapchain surface
        INLINE bool swapchain() const { return m_flags.test(ImageViewFlag::SwapChain); }

        // is the format SRGB
        INLINE bool srgb() const { return m_flags.test(ImageViewFlag::SRGB); }

        // is the image vertically flipped (usually true for swapchains on OpenGL)
        INLINE bool flippedY() const { return m_flags.test(ImageViewFlag::FlippedY); }

		// resource allows for sub-resources to be transition individually
		INLINE bool subResourceLayouts() const { return m_flags.test(ImageViewFlag::SubResourceLayouts); }

        ///--

        /// calculate estimated memory size used by this image
        virtual uint32_t calcMemorySize() const override;

        ///--

		/// create read-only image view usable as samplable texture
        virtual ImageSampledViewPtr createSampledView(uint32_t firstMip = 0, uint32_t firstSlice = 0) = 0;

		/// create read-only image view usable as samplable texture, more advanced function version
		virtual ImageSampledViewPtr createSampledViewEx(uint32_t firstMip, uint32_t firstSlice, uint32_t numMips=INDEX_MAX, uint32_t numSlices=INDEX_MAX) = 0;

		/// create read-only view of a single slice (usable as image in shaders, not usable as sampled texture)
		virtual ImageReadOnlyViewPtr createReadOnlyView(uint32_t mip = 0, uint32_t slice = 0) = 0;

        /// create writable UAV view of particular slice
        virtual ImageWritableViewPtr createWritableView(uint32_t mip = 0, uint32_t slice = 0) = 0;

        /// create render target view
        virtual RenderTargetViewPtr createRenderTargetView(uint32_t mip = 0, uint32_t firstSlice = 0, uint32_t numSlices = 1) = 0;

    protected:
        bool validateSampledView(uint32_t firstMip, uint32_t numMips, uint32_t firstSlice, uint32_t numSlices, ImageSampledView::Setup& outSetup) const;
		bool validateReadOnlyView(uint32_t mip, uint32_t slice, ImageReadOnlyView::Setup& outSetup) const;
        bool validateWritableView(uint32_t mip, uint32_t slice, ImageWritableView::Setup& outSetup) const;
        bool validateRenderTargetView(uint32_t mip, uint32_t firstSlice, uint32_t numSlices, RenderTargetView::Setup& outSetup) const;

    private:
        ImageViewKey m_key;
        ImageViewFlags m_flags = ImageViewFlags();
		ResourceLayout m_initialLayout = ResourceLayout::INVALID;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_depth = 0;
        uint8_t m_numSamples = 0;
    };

    //--

} // rendering
