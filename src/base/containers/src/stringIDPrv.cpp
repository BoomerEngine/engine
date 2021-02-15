/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringView.h"
#include "stringID.h"
#include "stringIDPrv.h"
#include "base/memory/include/linearAllocator.h"

//#pragma optimize("", off)

namespace base
{
	namespace prv
	{
		///--

		StringIDDataStorage::StringIDDataStorage()
		{
			resize();

			// create the "NULL" element
			m_stringTableWriteOffset = 1;
			((char*)StringID::st_StringTable.load())[0] = 0;
		}

		StringIDDataStorage::~StringIDDataStorage()
		{
			auto* ptr = StringID::st_StringTable.exchange(nullptr);
			mem::FreeBlock((void*)ptr);
		}

		StringIDIndex StringIDDataStorage::place(StringView buf)
		{
			auto ret = m_stringTableWriteOffset;
			if (m_stringTableWriteOffset + buf.length() + 1 >= m_stringTableSize)
				resize();

			auto* currentData = (char*)StringID::st_StringTable.load();
			memcpy(currentData + ret, buf.data(), buf.length());
			currentData[ret + buf.length()] = 0; // zero terminate the inserted string
			m_stringTableWriteOffset += buf.length() + 1;

			return ret;
		}

		void StringIDDataStorage::resize()
		{
			static const uint32_t MIN_STORAGE_SIZE = 128 << 10; // 128 KB

			// resize roughly 2x
			const auto newSize = std::max<uint32_t>(MIN_STORAGE_SIZE, m_stringTableSize * 2);
			m_stringTableSize = newSize;

			// resize buffer - do not reuse previous one - copy data first and than swap the pointer
			auto* currentData = StringID::st_StringTable.load();
			auto* newTable = (char*)mem::AllocateBlock(POOL_STRING_ID, newSize, 1, "StringIDStrings");
			memcpy(newTable, currentData, m_stringTableWriteOffset);
			StringID::st_StringTable.exchange(newTable); // atomically reconnect to new storage
		}

		///--

		StringIDMap::StringIDMap()
		{
			resize();
		}

		StringIDMap::~StringIDMap()
		{
		}

		void StringIDMap::insertAfterFailedFind(uint32_t stringHash, StringIDIndex stringIndex, uint32_t freeBucket)
		{
			if (m_numEntries < m_maxEntries)
			{
				ASSERT(m_buckets[freeBucket] == 0);
				m_buckets[freeBucket] = stringIndex;
			}
			else
			{
				resize();
				insertAfterResize(stringHash, stringIndex);
			}
		}

		void StringIDMap::insertAfterResize(uint32_t stringHash, StringIDIndex stringIndex)
		{
			// find first free bucket starting at the initial one that depends on the string hash
			auto bucketIndex = stringHash & (m_numBuckets - 1);
			while (m_buckets[bucketIndex] != 0)
				++bucketIndex;

			// write the entry index there
			m_buckets[bucketIndex] = stringIndex;
		}
			   
		StringIDIndex StringIDMap::find(uint32_t stringHash, const char* stringData, uint32_t stringLength, uint32_t& outFreeBucket) const
		{
			const auto* stringTable = StringID::st_StringTable.load();

			// search the entry table starting from given entry
			auto bucketIndex = stringHash & (m_numBuckets - 1);
			while (m_buckets[bucketIndex] != 0)
			{
				const auto bucketString = stringTable + m_buckets[bucketIndex];
				if (0 == strncmp(stringData, bucketString, stringLength))
					return m_buckets[bucketIndex];

				++bucketIndex;
			}

			// not found
			outFreeBucket = bucketIndex;
			return 0;
		}

		void StringIDMap::resize()
		{
			static const uint32_t MIN_SIZE = 65536;

			// determine new size, roughly grow x2
			auto oldBucketCount = m_numBuckets;
			m_numBuckets = std::max<uint32_t>(MIN_SIZE, oldBucketCount * 2);
			m_maxEntries = m_numBuckets / 4 * 3; // 75% occupancy

			// allocate new bucket table
			const auto* oldBuckets = m_buckets;
			m_buckets = (uint32_t*)mem::AllocateBlock(POOL_STRING_ID, sizeof(uint32_t) * m_numBuckets, 4, "StringIDBuckets");
			memzero(m_buckets, sizeof(uint32_t) * m_numBuckets);

			// using old bucket table initialize the new one
			// NOTE: this is going to cost us roughly 80ms....
			const auto* stringTable = StringID::st_StringTable.load();
			for (uint32_t i = 0; i < oldBucketCount; ++i)
			{
				const auto stringIndex = oldBuckets[i];
				const auto* str = stringTable + stringIndex;

				const auto strHash = StringView::CalcHash(str); // it's important to use the same hash
				insertAfterResize(strHash, stringIndex); // there's guarantee of no duplicates
			}

			// now the original bucket table is not needed
			mem::FreeBlock((void*)oldBuckets);
		}

		///--

	} // prv
} // base
