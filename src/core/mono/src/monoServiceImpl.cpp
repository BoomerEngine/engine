/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "monoService.h"
#include "monoServiceImpl.h"
#include "monoBindings.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{
    extern void InitializeMonoBindings();
    extern bool RunSelfTest();
} // prv

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MonoService);
RTTI_END_TYPE();

MonoService::MonoService()
{}

MonoService::~MonoService()
{}

//--

RTTI_BEGIN_TYPE_CLASS(MonoServiceImpl);
RTTI_END_TYPE();

MonoServiceImpl::MonoServiceImpl()
{}

MonoServiceImpl::~MonoServiceImpl()
{}

bool MonoServiceImpl::loaded() const
{
    return m_domain != nullptr;
}

void MonoServiceImpl::requestReload()
{
    if (!m_reloadRequested)
    {
        TRACE_INFO("Requested reload of Mono scripts");
        m_reloadRequested = true;
    }
}

static void GMonoAbortFunc()
{
    ASSERT(!"MonoAssertion");
}

bool MonoServiceImpl::onInitializeService(const CommandLine& cmdLine)
{
    const auto sharedDir = SystemPath(PathCategory::SharedDir);

    auto monoLibDir = StringBuf(TempString("{}mono/lib", sharedDir));
    auto monoEtcDir = StringBuf(TempString("{}mono/etc", sharedDir));
    monoLibDir.replaceChar(WRONG_SYSTEM_PATH_SEPARATOR, SYSTEM_PATH_SEPARATOR);
    monoEtcDir.replaceChar(WRONG_SYSTEM_PATH_SEPARATOR, SYSTEM_PATH_SEPARATOR);

    //mono_set_use_llvm(true);

    //--

    /*g_assertion_disable_global(&GMonoAbortFunc);
    void       g_assert_abort(void);
    void       g_log_default_handler(const gchar * log_domain, GLogLevelFlags log_level, const gchar * message, gpointer unused_data);
    GLogFunc   g_log_set_default_handler(GLogFunc log_func, gpointer user_data);
    GPrintFunc g_set_print_handler(GPrintFunc func);
    GPrintFunc g_set_printerr_handler(GPrintFunc func);*/

    //--

    TRACE_INFO("Mono lib directory: '{}'", monoLibDir);
    TRACE_INFO("Mono etc directory: '{}'", monoEtcDir);

    mono_set_dirs(monoLibDir.c_str(), monoEtcDir.c_str());

    {
        const auto& enginePath = SystemPath(PathCategory::EngineDir);
        m_assembliesPath.emplaceBack(TempString("{}data/scripts/Boomer.Engine.dll", enginePath));
        m_assembliesPath.back().replaceChar(WRONG_SYSTEM_PATH_SEPARATOR, SYSTEM_PATH_SEPARATOR);
    }

    // TODO: add project script files

    TRACE_INFO("Mono assemblies:");
    for (const auto& path : m_assembliesPath)
        TRACE_INFO("Assembly '{}'", path);

    load();

    //--

    return true;
}

void MonoServiceImpl::onShutdownService()
{
    close();
}

void MonoServiceImpl::onSyncUpdate()
{
    if (m_reloadRequested)
    {
        m_reloadRequested = false;
        reload();
    }
}

//---

void MonoServiceImpl::enumClasses(StringID classTag, ClassType engineBaseClass, Array<StringID>& outScriptClassNames) const
{
    const Array<const ClassInfo*>* classList = nullptr;

    if (!classTag)
        classList = (const Array<const ClassInfo*>*) &m_classes;
    else
        classList = m_classTagMap.find(classTag);

    if (classList)
    {
        for (const auto* info : *classList)
            if (!engineBaseClass || info->engineClass->is(engineBaseClass))
                outScriptClassNames.pushBack(info->monoClassName);
    }
}

MonoClass* MonoServiceImpl::findMonoClassForType(Type type) const
{
    if (!type)
        return nullptr;

    MonoClass* ret = nullptr;
    m_reverseEngineToMonoTypeMap.find(type, ret);
    DEBUG_CHECK_EX(ret, TempString("No Mono type mapped to engine type '{}'", type));
    return ret;
}

//--

void MonoServiceImpl::createEngineObjectInternalForScriptedObject(MonoObject* obj) const
{
    if (obj)
    {
        IObject* engineObjectPtr = nullptr;
        mono_field_get_value(obj, m_engineObjectFieldPtr, &engineObjectPtr);

        if (!engineObjectPtr)
        {
            auto* cls = mono_object_get_class(obj);
            engineObjectPtr = createEngineObjectInternalForScriptedClass(cls);

            mono_field_set_value(obj, m_engineObjectFieldPtr, &engineObjectPtr);
        }

        if (engineObjectPtr)
        {
            auto monoObjectGCHandle = mono_gchandle_new_weakref(obj, false);
            DEBUG_CHECK_EX(monoObjectGCHandle != 0, "Invalid GC handle");

            DEBUG_CHECK_EX(engineObjectPtr->m_monoScriptId == 0, "Engine object already linked");
            engineObjectPtr->m_monoScriptId = monoObjectGCHandle;
        }
    }
}

IObject* MonoServiceImpl::createEngineObjectInternalForScriptedClass(MonoClass* cls) const
{
    DEBUG_CHECK_RETURN_EX_V(cls, "Invalid class name", 0);

    const auto* classInfo = m_classPrvMap.findSafe(cls, nullptr);
    DEBUG_CHECK_RETURN_EX_V(classInfo, "Unknown class", 0);

    if (classInfo->engineClass)
    {
        if (auto ptr = classInfo->engineClass->create<IObject>())
        {
            ptr->addRef();
            return ptr.get();
        }
    }

    return 0;
}

ObjectPtr MonoServiceImpl::createScriptedObject(StringID className) const
{
    DEBUG_CHECK_RETURN_EX_V(className, "Invalid class name", nullptr);

    const auto* classInfo = m_classMap.findSafe(className, nullptr);
    DEBUG_CHECK_RETURN_EX_V(classInfo, "Unknown class", nullptr);

    //auto engineObject = classInfo->engineClass->create<IObject>();
    //DEBUG_CHECK_RETURN_EX_V(engineObject, "Failed to create engine object", nullptr);

    auto* monoObject = mono_object_new(m_domain, classInfo->monoClass);
    mono_runtime_object_init(monoObject);

    IObject* engineObjectPtr = nullptr;
    mono_field_get_value(monoObject, classInfo->monoObjectPtrFiled, &engineObjectPtr);

    return AddRef(engineObjectPtr); // retrieve created engine object
}

void* MonoServiceImpl::resolveScriptedObject(const IObject* engineObjectPtr) const
{
    if (!engineObjectPtr)
        return nullptr;

    if (auto id = engineObjectPtr->m_monoScriptId)
        return mono_gchandle_get_target(id);

    // create ad-hoc scripted object to wrap the engine object
    const ClassInfo* classInfo = nullptr;
    if (auto cls = engineObjectPtr->cls())
    {
        while (cls)
        {
            if (m_classEngineMap.find(cls, classInfo))
                break;
            cls = cls->baseClass();
        }

        ASSERT_EX(classInfo, "No class mapping, should be at least object");
    }

    // create the scripted object
    auto* monoObject = mono_object_new(m_domain, classInfo->monoClass);

    // bind the object pointer NOW
    mono_field_set_value(monoObject, classInfo->monoObjectPtrFiled, &engineObjectPtr);
    const_cast<IObject*>(engineObjectPtr)->addRef();

    // call the constructor
    mono_runtime_object_init(monoObject);

    // make sure no new object was generate
    {
        IObject* ptr = nullptr;
        mono_field_get_value(monoObject, classInfo->monoObjectPtrFiled, &ptr);
        ASSERT_EX(ptr == engineObjectPtr, "Object recreated");
    }

    // make sure object was bound
    ASSERT_EX(engineObjectPtr->m_monoScriptId != 0, "Engine object not linked propertly to scripted object");

    return monoObject;
}

//---

void MonoServiceImpl::close()
{
    TRACE_INFO("Closing Mono environment...");

    if (m_domain)
    {
        //mono_domain_finalize(m_domain, 500);
        mono_domain_free(m_domain, true);
        m_domain = nullptr;
    }

    m_classes.clearPtr();
    m_classMap.clear();
    m_classTagMap.clear();
}

bool MonoServiceImpl::load()
{
    ScopeTimer timer;

    // create env
    static std::atomic<int> GCounter = 1;
    auto* domain = mono_jit_init(TempString("MonoBoomerEnv{}", GCounter++));
    if (!domain)
    {
        TRACE_ERROR("Failed to create Mono domain");
        return false;
    }

    // reset type mapping
    m_reverseEngineToMonoTypeMap.clear();
    m_reverseEngineToMonoTypeMap[GetTypeObject<char>()] = mono_get_char_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<short>()] = mono_get_int16_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<int>()] = mono_get_int32_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<int64_t>()] = mono_get_int64_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<uint8_t>()] = mono_get_byte_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<uint16_t>()] = mono_get_uint16_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<uint32_t>()] = mono_get_uint32_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<uint64_t>()] = mono_get_uint64_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<float>()] = mono_get_single_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<double>()] = mono_get_double_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<bool>()] = mono_get_boolean_class();
    m_reverseEngineToMonoTypeMap[GetTypeObject<StringBuf>()] = mono_get_string_class();

    // load assemblies from paths
    bool allLoaded = true;
    MonoImage* rootImage = nullptr;
    for (const auto& path : m_assembliesPath)
    {
        TRACE_INFO("Loading Mono assembly '{}'...", path);

        // load content of assembly to memory (so we don't touch the file)
        Buffer data = LoadFileToBuffer(path);
        if (!data)
        {
            TRACE_ERROR("Failed to load file '{}' into memory", path);
            allLoaded = false;
            continue;
        }

        // TODO: optional description etc

        // load image
        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full((char*)data.data(), data.size(), true, &status, false);
        if (!image)
        {
            TRACE_ERROR("Failed to load image from file '{}', status: {}", path, (int)status);
            allLoaded = false;
            continue;
        }

        // create and bind assembly
        auto bindName = StringBuf(path.view().fileStem());
        MonoAssembly* assembly = mono_assembly_load_from(image, bindName.c_str(), &status);
        if (!assembly)
        {
            mono_image_close(image);
            TRACE_ERROR("Failed to load assembly from file '{}', status: {}", path, (int)status);
            allLoaded = false;
            continue;
        }

        // iterate classes
        processImageReflection(image);

        if (!rootImage)
            rootImage = image;
    }

    if (!allLoaded)
    {
        mono_domain_free(domain, true);

        TRACE_ERROR("There were errors loading assemblies");
        return false;
    }

    TRACE_INFO("Discovered {} scripted classes in {} loaded assemblies in {}", m_classes.size(), m_assembliesPath.size(), timer);

    m_domain = domain;

    prv::InitializeMonoBindings();

    if (!prv::RunSelfTest())
    {
        TRACE_INFO("Self test failed");
        close();
        return false;
    }

    return true;
}

void MonoServiceImpl::reload()
{
    // close current env
    if (m_domain)
    {
        TRACE_INFO("Closing current Mono domain...");

        DispatchGlobalEvent(eventKey(), EVENT_MONO_SCRIPTS_RELOADING);

        // TODO: store existing objects snapshot

        // close current env
        close();
    }

    // load new env
    if (!m_domain && load())
    {
        // TODO: restore existing objects snapshot

        DispatchGlobalEvent(eventKey(), EVENT_MONO_SCRIPTS_RELOADED);
    }
}

//--

void MonoServiceImpl::processImageReflection(MonoImage* img)
{
    auto* table = mono_image_get_table_info(img, MONO_TABLE_TYPEDEF);
    DEBUG_CHECK_RETURN(table);

    auto count = mono_table_info_get_rows(table);
    for (auto i = 0; i < count; ++i)
    {
        if (auto* klass = mono_class_get(img, MONO_TOKEN_TYPE_DEF | (1 + i)))
        {
            processClassReflection(klass);
        }
    }
}

void MonoServiceImpl::processClassReflection(MonoClass* cls)
{
    // no custom attributes, yay
    auto* attrsData = mono_custom_attrs_from_class(cls);
    if (!attrsData)
        return;

    auto* attrs = mono_custom_attrs_construct(attrsData);
    DEBUG_CHECK_RETURN(attrs);

    StringBuf engineClassName;
    StringBuf scriptClassName = TempString("{}.{}", mono_class_get_namespace(cls), mono_class_get_name(cls));

    Array<StringID> classTags;

    MonoClassField* pointerField = nullptr;

    bool exported = false;
    bool exportedAsStruct = false;
    const auto numAttrs = mono_array_length(attrs);
    for (uint32_t j = 0; j < numAttrs; ++j)
    {
        if (auto* attr = mono_array_get(attrs, MonoObject*, j))
        {
            auto* attrKlass = mono_object_get_class(attr);
            auto attrKlassName = StringView(mono_class_get_name(attrKlass));

            if (attrKlassName == "EngineClass")
            {
                MonoClassField* field = mono_class_get_field_from_name(attrKlass, "name");

                MonoString* str = nullptr;
                mono_field_get_value(attr, field, &str);

                if (str != nullptr)
                    engineClassName = StringBuf(mono_string_chars(str));

                pointerField = mono_class_get_field_from_name(cls, "__engineObjectPtr");
                exported = true;
            }

            else if (attrKlassName == "EngineStruct")
            {
                MonoClassField* field = mono_class_get_field_from_name(attrKlass, "name");

                MonoString* str = nullptr;
                mono_field_get_value(attr, field, &str);

                if (str != nullptr)
                    engineClassName = StringBuf(mono_string_chars(str));

                pointerField = nullptr;
                exportedAsStruct = true;
                exported = true;
            }

            else if (attrKlassName == "EngineTag")
            {
                MonoClassField* field = mono_class_get_field_from_name(attrKlass, "tag");

                MonoString* str = nullptr;
                mono_field_get_value(attr, field, &str);

                if (str != nullptr)
                {
                    InplaceArray<BaseStringView<wchar_t>, 10> parts;
                    BaseStringView<wchar_t>(mono_string_chars(str)).slice(";", false, parts);

                    for (auto part : parts)
                    {
                        part = part.trim();

                        if (!part.empty())
                            classTags.pushBackUnique(StringID(TempString("{}", part)));
                    }
                }
            }
        }
    }

    mono_custom_attrs_free(attrsData);

    if (!exported)
        return;

    if (engineClassName.empty())
        engineClassName = scriptClassName;

    auto engineClass = RTTI::GetInstance().findClass(StringID::Find(engineClassName.view()));
    if (!engineClass)
    {
        TRACE_ERROR("Scripted class '{}' can't be conntected to engine class '{}' because engine class does not exist",
            scriptClassName, engineClassName);
        return;
    }

    if (exportedAsStruct)
    {

    }
    else
    {
        if (!pointerField)
        {
            TRACE_ERROR("Scripted class '{}' can't be conntected to engine class '{}' because it's missing __engineObjectPtr field",
                scriptClassName, engineClassName);
        }

        if (!engineClass->is<IObject>())
        {
            TRACE_ERROR("Scripted class '{}' can't be conntected to engine class '{}' because engine class does not derive from IObject",
                scriptClassName, engineClassName);
            return;
        }

        if (engineClass->isAbstract())
        {
            TRACE_ERROR("Scripted class '{}' can't be conntected to engine class '{}' because engine class is abstract",
                scriptClassName, engineClassName);
            return;
        }
    }

    if (const auto* existingClass = m_classEngineMap.findSafe(engineClass, nullptr))
    {
        TRACE_ERROR("Scripted class '{}' can't be conntected to engine class '{}' because engine class is already connected with '{}'. This has to be 1-1 mapping.",
            scriptClassName, engineClassName, existingClass->monoClassName);
        return;
    }

    auto* info = new ClassInfo();
    info->engineClass = engineClass;
    info->monoClass = cls;
    info->monoClassName = StringID(scriptClassName.view());
    info->monoObjectPtrFiled = pointerField;
    info->tags = std::move(classTags);
    m_classes.pushBack(info);
    m_classMap[info->monoClassName] = info;
    m_classEngineMap[info->engineClass] = info;
    m_classPrvMap[cls] = info;

    if (cls && engineClass)
        m_reverseEngineToMonoTypeMap[engineClass] = cls;

    if (scriptClassName == "Boomer.Engine.IEngineObject")
        m_engineObjectFieldPtr = pointerField;

    for (const auto& tag : info->tags)
        m_classTagMap[tag].pushBack(info);

    TRACE_INFO("Connected scripted class '{}' with engine class '{}'", scriptClassName, engineClassName);

    if (info->monoClassName != "Boomer.Engine.IEngineObject"_id)
    {
        void* iter = NULL;
        while (MonoMethod* method = mono_class_get_methods(cls, &iter))
        {
            processMethodReflection(cls, method, info);
        }
    }
}

void MonoServiceImpl::processMethodReflection(MonoClass* cls, MonoMethod* mth, ClassInfo* clsInfo)
{
    auto name = mono_method_get_name(mth);

    uint32_t internalFlags = 0;
    mono_method_get_flags(mth, &internalFlags);

    if (!(internalFlags & MONO_METHOD_IMPL_ATTR_INTERNAL_CALL))
        return;

    const auto* monoFunction = clsInfo->engineClass->findMonoFunction(StringID(name));
    if (!monoFunction)
    {
        if (name[0] == '_')
            monoFunction = clsInfo->engineClass->findMonoFunction(StringID(name+1));

        if (!monoFunction)
        {
            TRACE_ERROR("InternalCall method '{}' not found in class '{}'", name, clsInfo->engineClass->name());
            return;
        }
    }
   
    TRACE_INFO("Found internal method '{}' in class '{}'", name, clsInfo->monoClassName);
    mono_add_internal_call(TempString("{}::{}", clsInfo->monoClassName, name).c_str(), monoFunction->monoFunctionWrapper());
}

END_BOOMER_NAMESPACE()

