/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//----

DECLARE_GLOBAL_EVENT(EVENT_MONO_SCRIPTS_RELOADING) // scripts started to reload
DECLARE_GLOBAL_EVENT(EVENT_MONO_SCRIPTS_RELOADED) // scripts have been reloaded

//--

// Mono service
class CORE_MONO_API MonoService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(MonoService, IService);

public:
    MonoService();
    virtual ~MonoService();

    // true if we have loaded scripts
    virtual bool loaded() const = 0;

    // request script reloading
    virtual void requestReload() = 0;

    //--

    // find classes that implement a given class tag and derive from given base class
    virtual void enumClasses(StringID classTag, ClassType engineBaseClass, Array<StringID>& outScriptClassNames) const = 0;

    //--

    // create a suitable native object with bounded scripted object of provided class
    // NOTE: which native class is created is determined by the script
    virtual ObjectPtr createScriptedObject(StringID className) const = 0;

    // resolve (or create) a internal script object ID for given engine object
    // NOTE: this never fails (but we can end up with only an "IEngineObject" wrapper)
    virtual void* resolveScriptedObject(const IObject* obj) const = 0;

    //--
};

//----

END_BOOMER_NAMESPACE()
