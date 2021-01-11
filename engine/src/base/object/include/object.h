/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

#include "globalEventKey.h"
#include "rttiDataHolder.h"
#include "rttiClassRef.h"
#include "rttiProperty.h"

#include "base/containers/include/stringID.h"


// Helper macros for old stuff
#define NO_DEFAULT_CONSTRUCTOR( _class )    \
    protected: _class() {};

// Event function
#define OBJECT_EVENT_FUNC base::StringID eventID, const base::IObject* object, base::StringView eventPath, const base::rtti::DataHolder& eventData

namespace base
{
    //---

    /// helper interface to list object's template properties
    class BASE_OBJECT_API ITemplatePropertyBuilder : public NoCopy
    {
    public:
        virtual ~ITemplatePropertyBuilder();

        virtual void prop(StringID category, StringID name, Type type, const void* defaultValue, const rtti::PropertyEditorData& editorData) = 0;

        template< typename T >
        inline void prop(StringID category, StringID name, const T& defaultValue = T(), const rtti::PropertyEditorData& editorData = PropertyEditorData())
        {
            prop(category, name, reflection::GetTypeObject<T>(), &defaultValue, editorData);
        }
    };

    /// helper interface to provide values for object template properties
    class BASE_OBJECT_API ITemplatePropertyValueContainer : public NoCopy
    {
    public:
        virtual ~ITemplatePropertyValueContainer();

        virtual ClassType compileClass() const = 0;

        virtual bool compileValue(StringID name, Type expectedType, void* ptr) const = 0;

        template< typename T >
        inline bool compileValue(StringID name, T& ptr) const
        {
            return compileValue(name, reflection::GetTypeObject<T>(), &ptr);
        }

        template< typename T >
        inline T compileValueOrDefault(StringID name, T defaultValue = T()) const
        {
            T ret;
            if (compileValue(name, reflection::GetTypeObject<T>(), &ptr))
                return ret;
            return defaultValue;
        }
    };

    //---

    // object ID Type
    typedef uint32_t ObjectID;

    /// Base class of manageable objects
    /// NOTE: objects are NOT copyable, period
    class BASE_OBJECT_API IObject : public IReferencable
    {
    public:
        IObject();
        virtual ~IObject();

        //---

        // Get unique object ID, this number will never repeat (pointer might though...)
        INLINE ObjectID id() const { return m_id; }

        // Get event ID we can use to listen for events on this object
        INLINE GlobalEventKey eventKey() const { return m_eventKey; }

        // Get parent object, does not have to be defined
        INLINE IObject* parent() const { return m_parent.unsafe(); }

        // Get parent object, does not have to be defined
        INLINE ObjectPtr parentRef() const { return m_parent.lock(); }

        // Check if object is of given class
        template< class T > INLINE bool is() const;

        // Cast this object to a handle of different type, ie. resource.as<Texture>()
        template< class T > INLINE RefPtr<T> as() const;

        // Cast this object to a handle of different type, will assert if conversion fails
        template< class T > INLINE RefPtr<T> asChecked() const;

        // Check if object is of given class
        bool is(ClassType objectClass) const;

        // Check if object is of given class
        template< typename T >
        INLINE bool is(SpecificClassType<T> objectClass) const;

        //---

        // Get static object class, overloaded in child classes
        static SpecificClassType<IObject> GetStaticClass();

        // Get object class, dynamic
        virtual ClassType cls() const;

        // Get object class, dynamic
        virtual ClassType nativeClass() const = 0;

        // Print object description, usually just class name but for named objects or objects with captions we can print their name as well
        virtual void print(IFormatStream& f) const override;

        //---

        // Called after object and all child objects were loaded and before the object is returned from job
        virtual void onPostLoad();

        // Load object from binary stream
        virtual void onReadBinary(stream::OpcodeReader& reader);

        // Save object to binary stream
        virtual void onWriteBinary(stream::OpcodeWriter& writer) const;

        // We are reading object from text/binary stream and previously serialized property is no longer in the object
        virtual bool onPropertyMissing(StringID propertyName, Type originalType, const void* originalData);

        // We are reading object from text/binary stream and previously serialized property has different type than now AND we could not handle the conversion automatically
        virtual bool onPropertyTypeChanged(StringID propertyName, Type originalDataType, const void* originalData, Type currentType, void* currentData);

        //---

        // Mark this object as modified, this propagates up the object hierarchy eventually (hopefully) reaching the parent resource
        // This is the main functionality used to track editor side changes to objects
        virtual void markModified();

        //---

        /// Get metadata for view - describe what we will find here: flags, list of members, size of array, etc
        virtual DataViewResult describeDataView(StringView viewPath, rtti::DataViewInfo& outInfo) const;

        /// Read data from memory
        virtual DataViewResult readDataView(StringView viewPath, void* targetData, Type targetType) const;

        /// Write data to memory
        virtual DataViewResult writeDataView(StringView viewPath, const void* sourceData, Type sourceType);

        //---

        /// Write object into XML node (note: node must exist before, the "class" attribute is not written, it's responsibility of the parent)
        virtual void writeXML(xml::Node& node) const;

        /// Load object from XML node, object must be created beforehand
        virtual void readXML(const xml::Node& node);

        //---

        // Notification about property change in progress, can be denied if the value is not ok
        // Object should return true to accept the change or false to reject it
        virtual bool onPropertyChanging(StringView path, const void* newData, Type newDataType) const;

        // Notification about property change that occurred
        virtual void onPropertyChanged(StringView path);

        // Filter visibility of given property in the data view of this object
        // Can be used to conditionally mask/unmask properties based on some part of object configuration
        // NOTE: the property that is used for masking must call postEvent("OnFullObjectRefresh") on the object to be able to refresh the UI properly.
        // NOTE: this function is called only when the property is initially requested (when user expands node in the property grid)
        virtual bool onPropertyFilter(StringID propertyName) const;

        //---

        // Check if property should be saved during serialization, usually properties that are the same as in base are filtered out
        virtual bool onPropertyShouldSave(const rtti::Property* prop) const;

        // Notifies that property value was found in file - we can skip loading it for some reason
        virtual bool onPropertyShouldLoad(const rtti::Property* prop);

        //---

        // Patch resource pointers - called during reloading, can be used to use custom implementation, should return true if anything was patched
        // This function is called GLOBALLY ON ALL OBJECTSs when a in-editor resource reload occurs, by default it automatically patches up properties referencing that object
        // NOTE: this function should return true if it did some patching and it requires some post-reload work (ie. after all other objects were patched)
        // NOTE: the automatic patching will call "onPropertyChanged" on each changed property
        virtual bool onResourceReloading(res::IResource* currentResource, res::IResource* newResource);

        // Called after all objects were patched after resource reload
        virtual void onResourceReloadFinished(res::IResource* currentResource, res::IResource* newResource);

        //---

        // set parent object for this object
        // parent objects are used to determine what hierarchy of objects should be saved together
        void parent(const IObject* parentObject);

        // check if this object has a given parent
        bool hasParent(const IObject* parentObject) const;

        // find parent of specific class
        IObject* findParent(ClassType parentObjectClass) const;

        // find parent of specific class
        template< typename T >
        INLINE T* findParent() const { return static_cast<T*>(findParent(T::GetStaticClass())); }

        //---

        // initialize class (since we have no automatic reflection in this project)
        static void RegisterType(rtti::TypeSystem& typeSystem);

        //---

        // run some code on main thread in context of this object (assuming it was not deleted before)
        template< typename T >
        void runSync(const std::function<void(T& obj)>& func)
        {
            auto weakSelf = RefWeakPtr<T>(base::rtti_cast<T>(this));
            RunSync("ObjectSyncJob") << [weakSelf, func](FIBER_FUNC)
            {
                if (auto obj = weakSelf.lock())
                    func(*obj);
            };
        }

        //---

        // post event from this object to all interested object's observers
        // NOTE: this is intended for editor or non-time critical functionality as most of the events will be routed through main thread
        // NOTE: if calling function is running on main thread we will execute callbacks immediately, if it's not the even will be posted to be executed later on main thread
        void postEvent(StringID eventID, const void* data = nullptr, Type dataType = Type());

        // post event from this object to all interested object's observers
        template< typename T >
        INLINE void postEvent(StringID eventID, const T & data)
        {
            postEvent(eventID, &data, reflection::GetTypeObject<T>());
        }

        //--

        // allocate unique object ID
        static ObjectID AllocUniqueObjectID();

        // get object by ID
        static ObjectPtr FindUniqueObjectById(ObjectID id);

        //--

        // clone this object
        ObjectPtr clone(const IObject* newParent = nullptr, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedObjectClass = nullptr) const;

        // save this object tree to a buffer
        Buffer toBuffer() const;

        //--

        // load object from buffer in memory
        static ObjectPtr FromBuffer(const void* data, uint32_t size, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

        //--

        // create data view for editing this object, especially in data inspector + data view is usually needed for undo/redo
        virtual DataViewPtr createDataView(bool forceReadOnly=false) const;

        //--

        // list all template properties for this object
        // NOTE: slow function, results should be cached
        virtual void queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const;

        // initialize from template properties
        virtual bool initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties);

        //--

        // register clone function
        static void RegisterCloneFunction(const std::function<ObjectPtr(const IObject*, const IObject*, res::IResourceLoader * loader, SpecificClassType<IObject>)>& func);
        static void RegisterSerializeFunction(const std::function<Buffer(const IObject*)>& func);
        static void RegisterDeserializeFunction(const std::function<ObjectPtr(const void* data, uint32_t size, res::IResourceLoader* loader, SpecificClassType<IObject> mutatedClass)>& func);

    protected:
        INLINE IObject(const IObject&) {};
        INLINE IObject& operator=(const IObject&) { return *this;  };

        // objects form a hierarchy, mostly for the purpose of saving in resources, this is our pointer to parent object
        // NOTE: it's considered dick move to delete parent object without deleting the children
        ObjectWeakPtr m_parent;

        // unique (runtime) ID for this object
        ObjectID m_id;

        // event key (TODO: merge somehow with ObjectID ?)
        GlobalEventKey m_eventKey;
    };

} // base

#include "object.inl"

