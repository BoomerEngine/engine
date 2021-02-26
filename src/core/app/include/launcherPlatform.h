/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(app)

class IApplication;
class CommandLine;

END_BOOMER_NAMESPACE_EX(app);

BEGIN_BOOMER_NAMESPACE_EX(platform)

// platform helper object - deals with initialization, ticking and everything else
class CORE_APP_API Platform
{
public:
    //--

    /// start application, returns true if application was initialized or false if it failed
    /// if application is start than the update can be started
    bool platformStart(const app::CommandLine& cmdline, app::IApplication* localApplication);

    /// perform a single step of application update
    /// will return false if application or anybody else requested exit via requestExit function
    bool platformUpdate();

    /// called between platform updates, can be used to throttle the execution
    void platformIdle();

    /// shutdown application and cleanup
    /// should be called to avoid nasty implicit call from automatic destructor
    void platformCleanup();

    //--

    /// request application exit, this will cause update() to return false on next call
    void requestExit(const char* reason);

    //--

    /// get the application we are running
    /// NOTE: can be used to access the "App Singleton":  (MyApp*)platform::GetLaunchPlatform().application()
    /// NOTE: this MAY be null when running in a "service only" mode
    INLINE app::IApplication* application() const { return m_application; }

    ///--

    /// do we have debugger attached ?
    static bool HasDebuggerAttached();
            
protected:
    virtual ~Platform(); // NEVER CALLED!
    Platform();

    virtual bool handleStart(const app::CommandLine& cmdline, app::IApplication* localApplication) = 0;
    virtual void handleUpdate() = 0;
    virtual void handleCleanup() = 0;

    std::atomic<uint32_t> m_exitRequestsed;
    app::IApplication* m_application;
};

// get the platform launcher object, there's only one per-platform so there's no point in dynamically instancing it
extern CORE_APP_API Platform& GetLaunchPlatform();

END_BOOMER_NAMESPACE_EX(platform);
