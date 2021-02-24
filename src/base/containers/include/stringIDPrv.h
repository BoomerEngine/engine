/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringBuf.h"

BEGIN_BOOMER_NAMESPACE(base)

namespace prv
{

	///--

	class StringIDDataStorage
	{
	public:
		StringIDDataStorage();
		~StringIDDataStorage();

		StringIDIndex place(StringView buf);

	private:
		uint32_t m_stringTableSize = 0;
		uint32_t m_stringTableWriteOffset = 0;

		void resize();
	};

	///--

	class StringIDMap
	{
	public:
		StringIDMap();
		~StringIDMap();

		StringIDIndex find(uint32_t stringHash, const char* stringData, uint32_t stringLength, uint32_t& outFreeBucket) const;

		void insertAfterFailedFind(uint32_t stringHash, StringIDIndex stringIndex, uint32_t freeBucket);

	private:
		uint32_t m_numBuckets = 0;
		uint32_t m_numEntries = 0;
		uint32_t m_maxEntries = 0; // some % of the nubmer of buckets, roughly 70%

		uint32_t* m_buckets = nullptr;

		void resize();

		void insertAfterResize(uint32_t stringHash, StringIDIndex stringIndex);
	};

	///--

} // prv

END_BOOMER_NAMESPACE(base)

