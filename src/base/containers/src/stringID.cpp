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

//#pragma optimize("", off)

namespace base
{

	//---

	struct StringIDGlobalState
	{
		prv::StringIDDataStorage storage;

		prv::StringIDMap globalMap;
		SpinLock globalMapLock;

		static StringIDGlobalState& GetInstance()
		{
			static StringIDGlobalState theGlobalState;
			return theGlobalState;
		}
	};

	static TYPE_TLS prv::StringIDMap* GStringIDLocalMap = nullptr;

	static StringID GEmptyStringID;

	std::atomic<const char*> StringID::st_StringTable;

    StringID StringID::EMPTY()
    {
        return GEmptyStringID;
    }

    StringID StringID::Find(StringView txt)
    {
		if (!txt)
			return StringID();

		const auto hash = CalcHash(txt);

		// allocate local map on first use
		if (!GStringIDLocalMap)
			GStringIDLocalMap = new prv::StringIDMap();

		// find in local map
		uint32_t freeLocalBucket = 0;
		auto index = GStringIDLocalMap->find(hash, txt.data(), txt.length(), freeLocalBucket);
		if (index)
			return StringID(index);

		// get global state
		auto& globalState = StringIDGlobalState::GetInstance();

		// find in global map
		{
			auto lock = CreateLock(globalState.globalMapLock);

			uint32_t freeGlobalBucket = 0;
			index = globalState.globalMap.find(hash, txt.data(), txt.length(), freeGlobalBucket);
		}

		// if found in global map store in local as well
		if (index)
			GStringIDLocalMap->insertAfterFailedFind(hash, index, freeLocalBucket);

		return StringID(index);
    }

	StringIDIndex StringID::Alloc(StringView txt)
	{
		if (!txt)
			return 0;

		const auto hash = CalcHash(txt);

		// allocate local map on first use
		if (!GStringIDLocalMap)
			GStringIDLocalMap = new prv::StringIDMap();

		// find in local map
		uint32_t freeLocalBucket = 0;
		auto index = GStringIDLocalMap->find(hash, txt.data(), txt.length(), freeLocalBucket);
		if (index)
			return index;

		// get global state
		auto& globalState = StringIDGlobalState::GetInstance();

		// find in global map
		{
			auto lock = CreateLock(globalState.globalMapLock);

			uint32_t freeGlobalBucket = 0;
			index = globalState.globalMap.find(hash, txt.data(), txt.length(), freeGlobalBucket);

			// allocate in global map
			if (index == 0)
			{
				// place the string in the storage
				index = globalState.storage.place(txt);

				// store in global map
				globalState.globalMap.insertAfterFailedFind(hash, index, freeGlobalBucket);
			}
		}

		// place in local map as well
		GStringIDLocalMap->insertAfterFailedFind(hash, index, freeLocalBucket);

		return index;
	}

} // base