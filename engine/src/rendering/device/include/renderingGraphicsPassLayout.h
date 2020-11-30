/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#pragma once

#include "renderingImageFormat.h"

namespace rendering
{
	
	//--

	// how is stuff loaded into the RT
	enum class LoadOp : uint8_t
	{
		Keep, // keep data that exists in the memory
		Clear, // clear the attachment when binding
		DontCare, // we don't care (fastest, can produce artifacts if all pixels are not written)
	};

	// how is stuff stored to memory after pass ends
	enum class StoreOp : uint8_t
	{
		Store, // store the data into the memory
		DontCare, // we don't care (allows to use Discard)
	};

	//--

#pragma pack(push)
#pragma pack(1)

	struct RENDERING_DEVICE_API GraphicsPassAttachment
	{
		ImageFormat format;
		LoadOp loadOp = LoadOp::Keep;
		StoreOp storeOp = StoreOp::Store;

		void print(base::IFormatStream& f) const;

		INLINE operator bool() const { return format != ImageFormat::UNKNOWN; }
		INLINE bool operator==(const GraphicsPassAttachment& other) const { return format == other.format; }
		INLINE bool operator!=(const GraphicsPassAttachment& other) const { return !operator==(other); }
	};

	//--

	/// pass attachment state
	struct RENDERING_DEVICE_API GraphicsPassLayoutSetup
	{
		static const uint32_t MAX_TARGETS = 8;

		uint8_t samples = 1;
		GraphicsPassAttachment depth;
		GraphicsPassAttachment color[MAX_TARGETS];

		//--

		GraphicsPassLayoutSetup();

		//--

		uint64_t key() const;

		void reset();

		void print(base::IFormatStream& f) const;

		bool operator==(const GraphicsPassLayoutSetup& other) const;
		INLINE bool operator!=(const GraphicsPassLayoutSetup& other) const { return !operator==(other); }
	};
#pragma pack(pop)

	static_assert(sizeof(GraphicsPassLayoutSetup) == 28, "Check for gaps");

	//--

} // rendering
