/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\descriptor #]
***/

#pragma once

namespace rendering
{
	///--

	namespace command
	{
		struct OpUploadConstants;
	}

    ///--

    // entry in the recording side descriptor table
    // NOTE: structure MUST be memcpy'able
    struct RENDERING_DEVICE_API DescriptorEntry
    {
        ObjectID id; // view object
        DeviceObjectViewType type = DeviceObjectViewType::Invalid; // type of view object
        uint32_t offset = 0; // internal offset
        uint32_t size = 0;

		union
		{
			const void* sourceDataPtr = nullptr;
			const command::OpUploadConstants* uploadedDataPtr;
		} inlinedConstants;

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
		union
		{
			const IDeviceObjectView* viewPtr = nullptr; // not ref counted
			const IDeviceObject* objectPtr;
		};
#endif

        INLINE DescriptorEntry() = default;
        INLINE DescriptorEntry(DescriptorEntry& other) = default;
        INLINE DescriptorEntry& operator=(DescriptorEntry& other) = default;

        INLINE operator bool() const { return type != DeviceObjectViewType::Invalid; }
        INLINE bool empty() const { return type == DeviceObjectViewType::Invalid; }

        //--

        // bind an object view to this descriptor
        void view(const IDeviceObjectView* ptr, uint32_t offset=0);

        // bind an object view to this descriptor
        INLINE DescriptorEntry& operator=(const IDeviceObjectView* ptr)
        {
            view(ptr);
            return *this;
        }

		//--

		// bind sampler
		void sampler(const SamplerObject* ptr);

		// bind an object view to this descriptor
		INLINE DescriptorEntry& operator=(const SamplerObject* ptr)
		{
			sampler(ptr);
			return *this;
		}

        //--

        // bind constant data to descriptor entry (will be saved inside command buffer once descriptor is bound)
        void constants(const void* data, uint32_t size);

        template< typename T >
        INLINE void constants(const T& data)
        {
            static_assert(!std::is_pointer<T>::value, "Do not use pointers here");
            constants(&data, sizeof(T));
        }

        //--

        // debug print
        void print(base::IFormatStream& f) const;

        //--
    };

    ///--

} // rendering
