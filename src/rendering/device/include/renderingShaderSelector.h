/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(rendering)

//----

/// helper class that allows to select permutations in shaders
/// NOTE: the very special thing about selectors is that value of 0 for a key is the same as "no entry", ie. the key=0 entries are not stored and are implicit
struct RENDERING_DEVICE_API ShaderSelector
{
	static const uint32_t MAX_SELECTORS = 16; // if we have more we are fucked any way

public:
	ShaderSelector();
	ShaderSelector(const ShaderSelector& other);
	ShaderSelector(ShaderSelector&& other);
	ShaderSelector& operator=(const ShaderSelector& other);
	ShaderSelector& operator=(ShaderSelector&& other);

	//--

	// get unique enough (TM) key for this selector - stable across runs - computed from sorted list of names and values
	// NOTE: empty selector has a special key of "0"
	uint64_t key() const;

	// comparisons, compares sorted lists
	bool operator==(const ShaderSelector& other) const;
	bool operator!=(const ShaderSelector& other) const;

	//--

	// clear all key/value pairs
	void clear();

	// check if it is empty selector (no values defined)
	bool empty() const;

	//--

	// check if we have value for given key
	// NOTE: zero is not a defined value
	bool has(base::StringID key) const;

	// get value of selector for given key, if key is not defined zero is returned
	// NOTE: the value of 0 is implicit for any key that was not set
	int get(base::StringID key) const;

	// set value for a key, setting a value of 0 removes it
	void set(base::StringID key, int value);

	// remove key (set it value to zero)
	bool remove(base::StringID key);

	//--

	// apply values from other selector on top of this selector
	// NOTE: the major optimization is that the internal list is only resorted once
	void apply(const ShaderSelector& selector);

	//--

	// compute CRC
	void hash(base::CRC64& crc) const;

	// print, only non zero values are printed
	// NOTE: the string is generated in a typical "define list" format:
	// e.g. "MSAA=1;NUM_SAMPLES=4"
	void print(base::IFormatStream& f) const;

	// export to table of defines
	void defines(base::HashMap<base::StringID, base::StringBuf>& outDefines) const;

	// parse from string
	static bool Parse(base::StringView text, ShaderSelector& outSelectors);

	//--

	// calculate hash, for hash map
	static uint32_t CalcHash(const ShaderSelector& selectors);

	//--

private:
	struct Entry
	{
		base::StringID key;
		int value = 0;
	};

	Entry entries[MAX_SELECTORS];

	bool setNoSort(base::StringID key, int value);
	void sort();
};

//----

END_BOOMER_NAMESPACE(rendering)
