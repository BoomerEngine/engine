/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#include "build.h"
#include "renderingGraphicsPassLayout.h"

namespace rendering
{

	//---
	
	RTTI_BEGIN_TYPE_ENUM(LoadOp);
		RTTI_ENUM_OPTION(Keep);
		RTTI_ENUM_OPTION(Clear);
		RTTI_ENUM_OPTION(DontCare);
	RTTI_END_TYPE()

	RTTI_BEGIN_TYPE_ENUM(StoreOp);
		RTTI_ENUM_OPTION(Store);
		RTTI_ENUM_OPTION(DontCare);
	RTTI_END_TYPE()

	//--

	void GraphicsPassAttachment::print(base::IFormatStream& f) const
	{
		f.appendf("{} load={} store={}", format, loadOp, storeOp);
	}


	//--

	GraphicsPassLayoutSetup::GraphicsPassLayoutSetup()
	{
		reset();
	}

	void GraphicsPassLayoutSetup::reset()
	{
		samples = 1;
		depth = GraphicsPassAttachment();
		for (uint32_t i=0; i<MAX_TARGETS; ++i)
			color[i] = GraphicsPassAttachment();
	}
	
	uint64_t GraphicsPassLayoutSetup::key() const
	{
		base::CRC64 crc;
		crc.append(this, sizeof(GraphicsPassLayoutSetup));
		return crc;
	}
	
	void GraphicsPassLayoutSetup::print(base::IFormatStream& f) const
	{
		if (depth)
		{
			f.appendf("DEPTH: {}", depth);
			if (samples)
				f.appendf(" MSAA x{}", samples);
			f << "\n";
		}

		for (uint32_t i=0; i<MAX_TARGETS; ++i)
		{
			if (!color[i])
				break;

			f.appendf("COLOR[{}]: {}", i, color[i]);
			if (samples)
				f.appendf(" MSAA x{}", samples);
			f << "\n";
		}
	}

	bool GraphicsPassLayoutSetup::operator==(const GraphicsPassLayoutSetup& other) const
	{
		if (depth != other.depth || samples != other.samples)
			return false;

		for (uint32_t i = 0; i < MAX_TARGETS; ++i)
		{
			if (color[i] != other.color[i])
				return false;
			if (!color[i])
				break;
		}

		return true;
	}

	//--

} // rendering