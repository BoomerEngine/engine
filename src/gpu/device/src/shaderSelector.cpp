/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "shaderSelector.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

ShaderSelector::ShaderSelector()
{
}

ShaderSelector::ShaderSelector(const ShaderSelector& other)
{
	memcpy(&entries, &other.entries, sizeof(entries));
}

ShaderSelector::ShaderSelector(ShaderSelector&& other)
{
	memcpy(&entries, &other.entries, sizeof(entries));
	memzero(&other.entries, sizeof(entries));
}

void ShaderSelector::clear()
{
	memzero(&entries, sizeof(entries));
}

bool ShaderSelector::empty() const
{
	return entries[0].key.empty();
}

ShaderSelector& ShaderSelector::operator=(const ShaderSelector& other)
{
	if (this != &other)
		memcpy(&entries, &other.entries, sizeof(entries));

	return *this;
}

ShaderSelector& ShaderSelector::operator=(ShaderSelector&& other)
{
	if (this != &other)
	{
		memcpy(&entries, &other.entries, sizeof(entries));
		memzero(&other.entries, sizeof(entries));
	}

	return *this;
}

uint64_t ShaderSelector::key() const
{
	if (empty())
		return 0;

	CRC64 crc;
	hash(crc);
	return crc;
}

bool ShaderSelector::operator==(const ShaderSelector& other) const
{
	const auto* ptrA = entries;
	const auto* ptrB = other.entries;
	for (uint32_t i = 0; i < MAX_SELECTORS; ++i, ++ptrA, ++ptrB)
	{
		if (ptrA->key != ptrB->key)
			return false;
		if (ptrA->value != ptrB->value)
			return false;

		if (!ptrA->key && !ptrB->key)
			break;
	}

	return true;
}

bool ShaderSelector::operator!=(const ShaderSelector& other) const
{
	return !operator==(other);
}

//--

bool ShaderSelector::has(StringID key) const
{
	if (!key)
		return false;

	const auto* ptr = entries;
	const auto* ptrEnd = entries + MAX_SELECTORS;
	while (ptr < ptrEnd && ptr->key)
	{
		if (ptr->key == key)
			return true;
		++ptr;
	}

	return false;
}

int ShaderSelector::get(StringID key) const
{
	if (!key)
		return 0;

	const auto* ptr = entries;
	const auto* ptrEnd = entries + MAX_SELECTORS;
	while (ptr < ptrEnd && ptr->key)
	{
		if (ptr->key == key)
			return ptr->value;
		++ptr;
	}

	return 0;
}

bool ShaderSelector::setNoSort(StringID key, int value)
{
	auto* ptr = entries;
	const auto* ptrEnd = entries + MAX_SELECTORS;
	while (ptr < ptrEnd && ptr->key)
	{
		if (ptr->key == key)
		{
			ptr->value = value;
			return false;  // no sorting needed
		}

		++ptr;
	}

	DEBUG_CHECK_RETURN_EX_V(ptr < ptrEnd, "To many selectors", false);

	ptr->key = key;
	ptr->value = value;
	return true;
}

void ShaderSelector::set(StringID key, int value)
{
	DEBUG_CHECK_RETURN_EX(key, "Cannot assign any value to empty key");

	if (value == 0)
	{
		if (setNoSort(key, value))
			sort();
	}
	else
	{
		remove(key);
	}
}

bool ShaderSelector::remove(StringID key)
{
	DEBUG_CHECK_RETURN_EX_V(key, "Cannot remove empty key", false);

	auto* ptr = entries;
	const auto* ptrEnd = entries + MAX_SELECTORS;
	while (ptr < ptrEnd)
	{
		if (!ptr->key)
			break;

		if (ptr->key == key)
		{
			// shift items
			auto* ptrBack = ptr; // where to write
			++ptr;
			while (ptr < ptrEnd)
				*ptrBack++ = *ptr++;

			// clear the last one
			ptrBack->key = StringID();
			ptrBack->value = 0;
			return true;
		}

		++ptr;
	}

	return false;
}

void ShaderSelector::apply(const ShaderSelector& selector)
{
	bool sortingNeeded = false;
	
	const auto* ptr = selector.entries;
	const auto* ptrEnd = selector.entries + MAX_SELECTORS;
	while (ptr < ptrEnd && ptr->key)
	{
		sortingNeeded |= setNoSort(ptr->key, ptr->value);
		++ptr;			
	}

	if (sortingNeeded)
		sort();
}

void ShaderSelector::sort()
{
	std::sort(entries, entries + MAX_SELECTORS, [](const Entry& a, const Entry& b) -> bool
		{
			if (a.key && b.key)
				return a.key < b.key;
			else
				return (int)a.key.empty() < (int)b.key.empty();
		});
}

//--

void ShaderSelector::hash(CRC64& crc) const
{
    const auto* ptr = entries;
    const auto* ptrEnd = entries + MAX_SELECTORS;
    while (ptr < ptrEnd && ptr->key)
    {
        crc << ptr->key.index();
        crc << ptr->value;
        ++ptr;
    }
}

void ShaderSelector::print(IFormatStream& f) const
{
	bool hasValues = false;

	const auto* ptr = entries;
	const auto* ptrEnd = entries + MAX_SELECTORS;
	while (ptr < ptrEnd && ptr->key) 
	{
		if (hasValues) f << ";";
		hasValues = true;

		f.appendf("{}={}", ptr->key, ptr->value);
		++ptr;
	}
}

void ShaderSelector::defines(HashMap<StringID, StringBuf>& outDefines) const
{
    const auto* ptr = entries;
    const auto* ptrEnd = entries + MAX_SELECTORS;
	while (ptr < ptrEnd && ptr->key)
	{
		outDefines[ptr->key] = TempString("{}", ptr->value);
		++ptr;
	}
}

bool ShaderSelector::Parse(StringView text, ShaderSelector& outSelectors)
{
	StringParser parser(text);

	ShaderSelector local;
	bool hasKeys = false;

	while (!parser.parseWhitespaces())
	{
		if (hasKeys && !parser.parseKeyword(";"))
			return false;

		StringView key;
		if (!parser.parseIdentifier(key))
			return false;

		if (!parser.parseKeyword("="))
			return false;

		int value = 0;
		if (!parser.parseInt32(value))
			return false;

		if (value)
			local.setNoSort(StringID(key), value);
		hasKeys = true;
	}

	outSelectors.apply(local); // output is modified only if we are successful
	return true;
}

//--

uint32_t ShaderSelector::CalcHash(const ShaderSelector& selectors)
{
	CRC32 crc;

	const auto* ptr = selectors.entries;
	const auto* ptrEnd = selectors.entries + MAX_SELECTORS;
    while (ptr < ptrEnd && ptr->key)
	{
		crc << ptr->key.index();
		crc << ptr->value;
		++ptr;
	}

	return crc;
}

//--

END_BOOMER_NAMESPACE_EX(gpu)
