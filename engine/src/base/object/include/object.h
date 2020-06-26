/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

#include "base/containers/include/stringID.h"

#include "rttiDataHolder.h"
#include "rttiClassRef.h"

// Helper macros for old stuff
#define NO_DEFAULT_CONSTRUCTOR( _class )    \
    protected: _class() {};

// Event function
#define OBJECT_EVENT_FUNC base::StringID eventID, const base::IObject* object, base::StringView<char> eventPath, const base::rtti::DataHolder& eventData

namespace base
{
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

        // Get parent object, does not have to be defined
        INLINE IObject* parent() const { return m_parent; }

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

        // Get default template for this object, usually this is the default class object
        // This is used for differential serialization and can be overridden in case the object's defaults are not coming from class
        virtual const void* defaultObject() const;

        //---

        // Called before object is saved, last time to update stuff
        // NOTE: modified properties should be mutable
        virtual void onPreSave() const;

        // Called after object and all child objects were loaded and before the object is returned from job
        virtual void onPostLoad();

        // Load object from binary stream
        virtual bool onReadBinary(stream::IBinaryReader& reader);

        // Save object to binary stream
        virtual bool onWriteBinary(stream::IBinaryWriter& writer) const;

        // Load object from text stream
        virtual bool onReadText(stream::ITextReader& reader);

        // Save object to binary stream
        virtual bool onWriteText(stream::ITextWriter& writer) const;

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
        virtual bool describeDataView(StringView<char> viewPath, rtti::DataViewInfo& outInfo) const;

        /// Read data from memory
        virtual bool readDataView(const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* targetData, Type targetType) const;

        /// Write data to memory
        virtual bool writeDataView(const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* sourceData, Type sourceType);

        //---

        // Notification about property change in progress, can be denied if the value is not ok
        // Object should return true to accept the change or false to reject it
        virtual bool onPropertyChanging(StringView<char> path, const void* newData, Type newDataType) const;

        // Notification about property change that occurred
        virtual void onPropertyChanged(StringView<char> path);

        // Filter visibility of given property in the data view of this object
        // Can be used to conditionally mask/unmask properties based on some part of object configuration
        // NOTE: the property that is used for masking must call postEvent("OnFullObjectRefresh") on the object to be able to refresh the UI properly.
        // NOTE: this function is called only when the property is initially requested (when user expands node in the property grid)
        virtual bool onPropertyFilter(StringView<char> propertyName) const;

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
        void postEvent(StringID eventID, StringView<char> eventPath = "", rtti::DataHolder eventData = nullptr, bool alwaysExecuteLater = false);

        //--

        // allocate unique object ID
        static ObjectID AllocUniqueObjectID();

        //--

        // clone this object
        ObjectPtr clone(const IObject* newParent = nullptr, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedObjectClass = nullptr) const;

        // save this object tree to a buffer
        Buffer toBuffer() const;

        //--

        // load object from buffer in memory
        static ObjectPtr FromBuffer(const void* data, uint32_t size, res::IResourceLoader* loader = nullptr, SpecificClassType<IObject> mutatedClass = nullptr);

        //--

        // create data view for this object
        virtual DataViewPtr createDataView() const;

        // create a data proxy for this object (usually just wraps the view)
        virtual DataProxyPtr createDataProxy() const;

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
        IObject* m_parent = nullptr;

        // unique (runtime) ID for this object
        ObjectID m_id;
    };

} // base

#include "object.inl"

