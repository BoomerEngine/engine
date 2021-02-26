/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(gpu)

///---

/// notifier that is called whenever shaders should be reloaded
class GPU_DEVICE_API ShaderReloadNotifier : public NoCopy
{
public:
    ShaderReloadNotifier();
    ~ShaderReloadNotifier();

    //--

    typedef std::function<void(void)> TFunction;
    ShaderReloadNotifier& operator=(const TFunction& func);

    //--

    // call the callback on this notifier
    void notify();

    //--

    // call all notifications (slow)
    static void NotifyAll();

    //--

private:
    TFunction m_func;
};

//---

END_BOOMER_NAMESPACE_EX(gpu)
