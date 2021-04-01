/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "monoService.h"

BEGIN_BOOMER_NAMESPACE()

//--

// Mono scripting service implementation
class CORE_MONO_API MonoServiceImpl : public MonoService
{
    RTTI_DECLARE_VIRTUAL_CLASS(MonoServiceImpl, MonoService);

public:
    MonoServiceImpl();
    virtual ~MonoServiceImpl();

    //--

    virtual bool loaded() const override;
    virtual void requestReload() override;

    //--

    virtual void enumClasses(StringID classTag, ClassType engineBaseClass, Array<StringID>& outScriptClassNames) const override final;

    //--

    virtual ObjectPtr createScriptedObject(StringID className) const override final;
    virtual void* resolveScriptedObject(const IObject* obj) const override final;

    //--

    IObject* createEngineObjectInternalForScriptedClass(MonoClass* cls) const;
    void createEngineObjectInternalForScriptedObject(MonoObject* obj) const;

    MonoClass* findMonoClassForType(Type type) const;

    //-

private:
    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;

    //--

    MonoDomain* m_domain = nullptr;

    //--
    
    bool m_reloadRequested = false;

    Array<StringBuf> m_assembliesPath;

    //--

    struct ClassInfo
    {
        StringID monoClassName;
        MonoClass* monoClass;
        MonoClassField* monoObjectPtrFiled = nullptr; // not set for structs

        ClassType engineClass;
        Array<StringID> tags;
    };

    Array<ClassInfo*> m_classes;
    HashMap<StringID, const ClassInfo*> m_classMap;
    HashMap<MonoClass*, const ClassInfo*> m_classPrvMap;
    HashMap<StringID, Array<const ClassInfo*>> m_classTagMap;
    HashMap<ClassType, const ClassInfo*> m_classEngineMap;

    MonoClassField* m_engineObjectFieldPtr = nullptr;

    void processImageReflection(MonoImage* img);
    void processClassReflection(MonoClass* cls);
    void processMethodReflection(MonoClass* cls, MonoMethod* mth, ClassInfo* clsInfo);

    //--

    HashMap<Type, MonoClass*> m_reverseEngineToMonoTypeMap;

    //--

    void reload();

    void close();
    bool load();
};

//----

END_BOOMER_NAMESPACE()
