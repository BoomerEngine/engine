/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

/// this "Module declarations" are used by the generated code to glue everything together in case of static libs and DLLs

#if defined(BUILD_AS_LIBS)  || !defined(BUILD_DLL) 

#define MODULE_DLL_INIT(_name)

#else

#define MODULE_DLL_INIT(_name)                                                              \
    void* GCurrentModuleHandle = nullptr;                                                   \
    unsigned char __stdcall DllMain(void* moduleInstance, unsigned long nReason, void*)     \
    {                                                                                       \
        GCurrentModuleHandle = moduleInstance;                                              \
        if (nReason == 1) InitModule_##_name();                                             \
        return 1;                                                                           \
    }

#endif

#ifdef BUILD_WITH_REFLECTION

#define DECLARE_MODULE_IMPL(_projectName) \
    extern void UserInitModule_##_projectName(boomer::modules::ModuleInitialization& init); \
    extern void InitializeReflection_##_projectName(); \
    extern void InitializeTests_##_projectName(); \
    extern void* GCurrentModuleHandle; \
    void InitModule_##_projectName() \
    { \
        TRACE_INFO("Loading module {} (with reflection)", #_projectName ); \
        boomer::modules::ModuleInitialization init([]() { InitializeReflection_##_projectName(); InitializeTests_##_projectName(); }); \
        boomer::modules::RegisterModule(#_projectName, __DATE__, __TIME__, _MSC_FULL_VER, GCurrentModuleHandle); \
        UserInitModule_##_projectName(init); \
    } \
    MODULE_DLL_INIT(_projectName); \
    void UserInitModule_##_projectName(boomer::modules::ModuleInitialization& init)

#else

#define DECLARE_MODULE_IMPL(_projectName) \
    extern void UserInitModule_##_projectName(boomer::modules::ModuleInitialization& init); \
    extern void* GCurrentModuleHandle; \
    void InitModule_##_projectName() \
    { \
        TRACE_INFO("Loading module {}", #_projectName ); \
        boomer::modules::ModuleInitialization init([]() { }); \
        boomer::modules::RegisterModule(#_projectName, __DATE__, __TIME__, _MSC_FULL_VER, GCurrentModuleHandle); \
        UserInitModule_##_projectName(init); \
    } \
    MODULE_DLL_INIT(_projectName); \
    void UserInitModule_##_projectName(boomer::modules::ModuleInitialization& init)

#endif

#define DECLARE_MODULE(_projectName) \
    DECLARE_MODULE_IMPL(_projectName)

BEGIN_BOOMER_NAMESPACE_EX(modules)

//--

struct ModuleInfo
{
    const char*     m_name;
    const char*     m_dateCompiled;
    const char*     m_timeCompiled;
    uint64_t              m_compilerVersion;
    void*               m_moduleHandle;

    INLINE ModuleInfo()
        : m_name(nullptr)
        , m_dateCompiled(nullptr)
        , m_timeCompiled(nullptr)
        , m_compilerVersion(0)
        , m_moduleHandle(nullptr)
    {}
};

struct ModuleInitialization
{
public:
    typedef std::function<void()> TInitFunction;

    INLINE ModuleInitialization( const TInitFunction& func)
        : m_func(func)
    {}

    INLINE ~ModuleInitialization()
    {
        run();
    }

    INLINE void run()
    {
        if (m_func)
        {
            m_func();
            m_func = TInitFunction();
        }
    }

private:
    TInitFunction m_func;
};

/// register information about a module being initialized
extern CORE_SYSTEM_API void RegisterModule(const char* name, const char* dateCompiled, const char* timeCompiled, uint64_t compilerVer, void* moduleHandle);

/// get list of initialized modules
extern CORE_SYSTEM_API void GetRegisteredModules(uint32_t maxEntries, ModuleInfo* outTable, uint32_t& outNumEntries);

/// load a dynamic module if its not yet loaded
extern CORE_SYSTEM_API void LoadDynamicModule(const char* name);

/// check if we have module loaded
extern CORE_SYSTEM_API bool HasModuleLoaded(const char* name);

//--

END_BOOMER_NAMESPACE_EX(modules)
