/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

/// basic singleton interface
/// NOTE: we try to cleanup all singletons before closing the app
class BASE_SYSTEM_API ISingleton : public base::NoCopy
{
public:
    /// called to cleanup the singleton before app shutdown
    virtual void deinit();

    ///---

    /// deinitialize all singletons
    static void DeinitializeAll();

protected:
    ISingleton();
    virtual ~ISingleton();
};

#define DECLARE_SINGLETON(T) \
    public: static T& GetInstance() { static T* instance = new T(); return *instance; } \
    private: virtual ~T() {}

END_BOOMER_NAMESPACE(base)
