/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization #]
***/

#pragma once

#include "base/containers/include/array.h"
#include "rttiClassType.h"

namespace base
{
    namespace stream
    {
        class MemoryWriter;

        // Resource saving context
        class BASE_OBJECT_API SavingContext
        {
        public:
            typedef Array< const IObject* > TObjectsToMap;

            // Roots of the object trees to save
            TObjectsToMap m_initialExports;

            // Save the editor only properties
            bool m_saveEditorOnlyProperties;

            // Base path for all paths being saved
            // All paths that are based on that path are saved as relative paths
            // e.g. saving path to "engine/textures/lena.png" with the "engine/" reference base path will actually save "textures/lena.png"
            StringBuf m_baseReferncePath;

            // Context name for reporting any loading errors, usually a full absolute path of the file being saved
            StringBuf m_contextName;

        public:
            template< typename T >
            SavingContext(const T* singleObject)
                : m_saveEditorOnlyProperties(false)
            {
                if (singleObject)
                {
                    auto object = base::rtti_cast<IObject>(singleObject);
                    DEBUG_CHECK_EX(object, "Object passed is not an IObject");

                    m_initialExports.pushBack(singleObject);
                }
            }

            template< typename T >
            SavingContext(const RefPtr<T>& singleObject)
                : m_saveEditorOnlyProperties(false)
            {
                if (singleObject)
                {
                    auto object = base::rtti_cast<IObject>(singleObject);
                    DEBUG_CHECK_EX(object, "Object passed is not an IObject");

                    m_initialExports.pushBack(singleObject);
                }
            }

            template< typename T >
            SavingContext(const Array< RefPtr<T> >& objects)
                    : m_saveEditorOnlyProperties(false)
            {
                m_initialExports.reserve(objects.size());

                for (auto& it : objects)
                {
                    if (it)
                    {
                        auto object = rtti_cast<IObject>(it);
                        DEBUG_CHECK_EX(object != nullptr, "Object passed is not an IObject");

                        m_initialExports.pushBack(object);
                    }
                }
            }

            template< typename T >
            SavingContext(const Array< T* >& objects)
                    : m_saveEditorOnlyProperties(false)
            {
                m_initialExports.reserve(objects.size());

                for (auto& it : objects)
                {
                    if (it)
                    {
                        auto object = rtti_cast<IObject>(it);
                        DEBUG_CHECK_EX(object != nullptr, "Object passed is not an IObject");

                        m_initialExports.pushBack(object);
                    }
                }
            }

        };

        class ISaver;
        typedef RefPtr<ISaver> SaverPtr;

        // Object saver interface
        class BASE_OBJECT_API ISaver : public IReferencable
        {
        public:
            ISaver();
            virtual ~ISaver();

            // Save objects, returns true on success
            // Content is saved at the current file pointer, 
            // After saving file pointer is scrolled to the END of the saved data
            virtual bool saveObjects(IBinaryWriter& file, const SavingContext& context) = 0;
        };

    } // stream
} // base