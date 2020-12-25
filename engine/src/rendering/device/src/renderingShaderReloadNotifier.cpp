/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "renderingShaderReloadNotifier.h"

namespace rendering
{
    //--

    class ShaderReloadNotifierRegistry : public base::ISingleton
    {
        DECLARE_SINGLETON(ShaderReloadNotifierRegistry);

    public:
        ShaderReloadNotifierRegistry()
        {}

        void registerEntry(ShaderReloadNotifier* entry)
        {
            auto lock = CreateLock(m_lock);
            m_notifier.pushBack(entry);
        }

        void unregisterEntry(ShaderReloadNotifier* entry)
        {
            auto lock = CreateLock(m_lock);
            auto index = m_notifier.find(entry);
            if (index != INDEX_NONE)
                m_notifier[index] = nullptr;
        }

        void notifyAll()
        {
            auto lock = CreateLock(m_lock);

            for (auto* ptr : m_notifier)
                if (ptr)
                    ptr->notify();

            m_notifier.removeAll(nullptr);
        }

    private:
        base::Mutex m_lock;
        base::Array<ShaderReloadNotifier*> m_notifier;
    };

    //--

    ShaderReloadNotifier::ShaderReloadNotifier()
    {
        ShaderReloadNotifierRegistry::GetInstance().registerEntry(this);
    }

    ShaderReloadNotifier::~ShaderReloadNotifier()
    {
        ShaderReloadNotifierRegistry::GetInstance().unregisterEntry(this);
    }

    void ShaderReloadNotifier::notify()
    {
        if (m_func)
            m_func();
    }

    ShaderReloadNotifier& ShaderReloadNotifier::operator=(const TFunction& func)
    {
        m_func = func;
        return *this;
    }

    void ShaderReloadNotifier::NotifyAll()
    {
        ShaderReloadNotifierRegistry::GetInstance().notifyAll();
    }

    //--

} // rendering
