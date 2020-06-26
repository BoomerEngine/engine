/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\debug #]
* [#platform: posix #]
***/

#include "build.h"
#include "systemInfo.h"
#include <unistd.h>
#include <sys/utsname.h>

namespace base
{

    class UserNameString
    {
    public:
        UserNameString()
        {
            if (0 != getlogin_r(m_text, ARRAY_COUNT(m_text)))
            {
                strcpy_s(m_text, "unknown");
            }
        }

        INLINE const char* c_str() const
        {
            return m_text;
        }

    private:
        char m_text[64];
    };

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
            if (0 != gethostname(m_text, ARRAY_COUNT(m_text)))
            {
                strcpy_s(m_text, "unknown");
            }
        }

        INLINE const char* c_str() const
        {
            return m_text;
        }

    private:
        char m_text[64];
    };

    const char* GetHostName()
    {
        static HostNameString theData;
        return theData.c_str();
    }

    //--

    class SystemNameString
    {
    public:
        SystemNameString()
        {
            utsname name;
            if (0 != uname(&name))
            {
                strcpy_s(m_text, "unknown");
            }
            else
            {
                sprintf(m_text, "%s %s %s", name.sysname, name.version, name.version);
            }
        }

        INLINE const char* c_str() const
        {
            return m_text;
        }

    private:
        char m_text[256];
    };

    const char* GetSystemName()
    {
        static SystemNameString theData;
        return theData.c_str();
    }

    const char* GetEnv(const char* key)
    {
        auto value = getenv(key);
        return value ? value : "";
    }

    bool GetRegistryKey(const char* path, const char* key, char* outBuffer, uint32_t& outBufferSize)
    {
        return false;
    }

} // base
