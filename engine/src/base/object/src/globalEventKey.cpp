/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: event #]
***/

#include "build.h"
#include "globalEventKey.h"

namespace base
{
    //---

    std::atomic<uint64_t> GGlobalEventKeyCounter = 1;

    //---

    void GlobalEventKey::print(IFormatStream& f) const
    {
        if (m_key)
        {
            f.appendf("event key {}", m_key);

#ifdef GLOBAL_EVENTS_DEBUG_INFO
            if (m_debugInfo)
                f.appendf(" ({})", m_debugInfo);
#endif
        }
        else
        {
            f.append("empty");
        }
    }

    //--

    GlobalEventKey MakeUniqueEventKey(StringView<char> debugInfo)
    {
        GlobalEventKey ret;
        ret.m_key = GGlobalEventKeyCounter++;
#ifdef GLOBAL_EVENTS_DEBUG_INFO
        ret.m_debugInfo = StringBuf(debugInfo);
#endif
        return ret;
    }

    class SharedEventKeyRepository : public ISingleton
    {
        DECLARE_SINGLETON(SharedEventKeyRepository);

    public:
        GlobalEventKey mapPathToKey(StringView<char> path)
        {
            auto lock = CreateLock(m_keysMapLock);

            GlobalEventKey ret;
            if (!m_keysMap.find(path, ret))
            {
                auto str = StringBuf(path);
                ret = MakeUniqueEventKey(str);
                m_keysMap[str] = ret;
            }

            return ret;
        }

    private:
        SpinLock m_keysMapLock;
        HashMap<StringBuf, GlobalEventKey> m_keysMap;

        virtual void deinit() override
        {
            m_keysMap.clear();
        }
    };

    GlobalEventKey MakeSharedEventKey(StringView<char> path)
    {
        return SharedEventKeyRepository::GetInstance().mapPathToKey(path);
    }

    //--

} // base

