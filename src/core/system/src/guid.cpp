/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "guid.h"

#ifdef PLATFORM_WINDOWS
#include <combaseapi.h>
#elif defined(PLATFORM_POSIX)
#endif

BEGIN_BOOMER_NAMESPACE()

//--

void GUID::print(IFormatStream& f) const
{
    const auto* bytes = (const uint8_t*)data();

    // print in cannonical form: {123e4567 - e89b - 12d3 - a456 - 426652340000}

    f.append("{");
    f.appendHexNumber(bytes + 0, 4);
    f.append("-");
    f.appendHexNumber(bytes + 4, 2);
    f.append("-");
    f.appendHexNumber(bytes + 6, 2);
    f.append("-");
    f.appendHexNumber(bytes + 8, 2);
    f.append("-");
    f.appendHexNumber(bytes + 10, 6);
    f.append("}");
}

//--

uint32_t GUID::CalcHash(const GUID& guid)
{
    uint32_t ret = guid.m_words[0];
    ret = (ret * 17) + guid.m_words[1];
    ret = (ret * 17) + guid.m_words[2];
    ret = (ret * 17) + guid.m_words[3];
    return ret;
}

//--

GUID GUID::Create()
{
    GUID ret;

#ifdef PLATFORM_WINDOWS
    ::GUID data;
    ::CoCreateGuid(&data);
    ret.m_words[0] = ((const uint32_t*)&data)[0];
    ret.m_words[1] = ((const uint32_t*)&data)[1];
    ret.m_words[2] = ((const uint32_t*)&data)[2];
    ret.m_words[3] = ((const uint32_t*)&data)[3];
#else
    ret.m_words[0] = (rand() << 16) ^ rand();
    ret.m_words[1] = (rand() << 16) ^ rand();
    ret.m_words[2] = (rand() << 16) ^ rand();
    ret.m_words[3] = (rand() << 16) ^ rand();
#endif

    return ret;
}

//--

END_BOOMER_NAMESPACE()
