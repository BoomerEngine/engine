/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: styles #]
***/

#pragma once
#include "base/containers/include/sortedArray.h"

namespace ui
{
    namespace style
    {
#if 0
        /// internal style parameter index
        typedef uint16_t ParamIndex;
        /// generalized parameter table, contains values for parameters
        /// parameters are addressable by the ParamID
        class BASE_UI_API ParamTable
        {
        public:
            ParamTable(uint64_t compoundStyleHash, const StyleLibraryPtr& library);
            ParamTable(const ParamTable& other);
            ParamTable(ParamTable&& other);

            // copy
            ParamTable& operator=(ParamTable&& other);
            ParamTable& operator=(const ParamTable& other);

            /// set parameter value
            void set(const ParamID& param, const Value* valuePtr);

            /// get value for parameter, returns default parameter value if not specified
            const Value& get(const ParamID& param, base::ClassType paramClass) const;

            /// get value for parameter, returns null if not was not found
            const Value* ptr(const ParamID& param, base::ClassType paramClass) const;

            // get parameter value directly, returns the default value if the value was not found
            template< typename T >
            INLINE const T& typed(ParamID param) const
            {
                return static_cast<const T&>(get(param, T::GetStaticClass()));
            }

            // get parameter value directly, returns the default value if the value was not found
            template< typename T >
            INLINE const T& typed(const style::ParamDeclaration<T>& paramDeclaration) const
            {
                return static_cast<const T&>(get(paramDeclaration.paramId(), T::GetStaticClass()));
            }

            // get parameter value directly, returns null if not was not found
            template< typename T >
            INLINE const T* typedPtr(ParamID param) const
            {
                return static_cast<const T*>(ptr(param, T::GetStaticClass()));
            }

            // get parameter value directly, returns null if not was not found
            template< typename T >
            INLINE const T* typedPtr(const style::ParamDeclaration<T>& paramDeclaration) const
            {
                return static_cast<const T*>(ptr(paramDeclaration.paramId(), T::GetStaticClass()));
            }

            // get hash of the style used to build this param table
            // if the hash is the same we can reuse the table
            INLINE uint64_t compoundHash() const { return m_compoundStyleHash; }

            // get the library this styles were compiled from
            INLINE StyleLibraryPtr library() const { return m_library.lock(); }

            //--

            // get empty parameter table
            static ParamTablePtr NullTable();

        private:
            typedef base::HashMap<ParamIndex, const Value*> TParams;
            TParams m_params;

            uint64_t m_compoundStyleHash;

            StyleLibraryWeakPtr m_library;
        };
#endif
        
    } // style
} // ui
