/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#pragma once

namespace rendering
{

    ///---

    /// notifier that is called whenever shaders should be reloaded
    class RENDERING_DEVICE_API ShaderReloadNotifier : public base::NoCopy
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

} // rendering
