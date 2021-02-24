/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\debug #]
* [#platform: windows #]
***/

#include "build.h"
#include "systemInfo.h"

#include <windows.h>
#include <Lmcons.h>

BEGIN_BOOMER_NAMESPACE(base)

class UserNameString
{
public:
    UserNameString()
    {
        DWORD size = UNLEN;
        memset(m_text, 0, sizeof(m_text));
        GetUserNameA(m_text, &size);
        m_text[size] = 0;
            
        for (uint32_t i=0; i<size; ++i)
            if (m_text[i] && m_text[i] <= ' ')
                m_text[i] = '_';
    }

    INLINE const char* c_str() const
    {
        return m_text;
    }

private:
    char m_text[UNLEN+1];
};

#undef GetUserName
const char* GetUserName()
{
    static UserNameString theData;
    return theData.c_str();
}

//--

class HostNameString
{
public:
    HostNameString()
    {
        DWORD size = 256;
        memset(m_text, 0, sizeof(m_text));
        GetComputerNameExA(ComputerNameDnsHostname, m_text, &size);
        m_text[size] = 0;

        for (uint32_t i=0; i<size; ++i)
            if (m_text[i] && m_text[i] <= ' ')
                m_text[i] = '_';
    }

    INLINE const char* c_str() const
    {
        return m_text;
    }

private:
    char m_text[256+1];
};

const char* GetHostName()
{
    static HostNameString theData;
    return theData.c_str();
}

//--

const char* GetSystemName()
{
    return "Windows";
}

const char* GetEnv(const char* key)
{
    auto value = getenv(key);
    return value ? value : "";
}

bool GetRegistryKey(const char* path, const char* key, char* outBuffer, uint32_t& outBufferSize)
{
    HKEY hKey;
    LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        auto nError = RegQueryValueExA(hKey, key, 0, NULL, (LPBYTE)outBuffer, (DWORD*)&outBufferSize);
        return ERROR_SUCCESS == nError;
    }

    return false;
}

END_BOOMER_NAMESPACE(base)
