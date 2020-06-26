/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: text #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"

namespace base
{
    namespace res
    {
        namespace text
        {
            // a simple parameter file format
            /// # comment
            /// key value attrib=value attrib=value
            ///   key value attrib=value attrib=value

            //---

            class ParameterFileParser;
            class ParameterTokenIterator;
            class ParameterNodeIterator;

            /// a token in a node
            /// NOTE: do not cache beyond the lifetime of ParameterFile
            struct ParameterToken : public base::NoCopy
            {
                template< typename T >
                INLINE T read(const T& defaultValue = T()) const
                {
                    T ret = defaultValue;
                    StringView<char>(value).match(ret);
                    return ret;
                }

                StringView<char> value;
                const ParameterToken* next = nullptr;

                friend class ParameterFileParser;
                friend class ParameterTokenIterator;
                friend class ParameterNode;
            };

            /// token iterator
            class ParameterTokenIterator
            {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = const ParameterToken;
                using difference_type = ptrdiff_t;
                using pointer = const ParameterToken*;
                using reference = const ParameterToken&;

                INLINE ParameterTokenIterator()
                {}

                INLINE ParameterTokenIterator(const ParameterToken* ptr_)
                    : ptr(ptr_)
                {}

                INLINE ParameterTokenIterator(const ParameterTokenIterator& other)
                    : ptr(other.ptr)
                {}

                //! Valid ?
                INLINE operator bool() const
                {
                    return ptr != nullptr;
                }

                //! Move to next elem
                INLINE ParameterTokenIterator& operator++()
                {
                    advance();
                    return *this;
                }

                //! Move to next elem
                INLINE ParameterTokenIterator operator++(int)
                {
                    ParameterTokenIterator copy(*this);
                    advance();
                    return copy;
                }

                //! Get reference to item
                INLINE const ParameterToken& operator*() const
                {
                    return *ptr;
                }

                //! Get pointer to item
                INLINE const ParameterToken* operator->() const
                {
                    return ptr;
                }

                //! Compare relative position with other iterator
                INLINE bool operator==(const ParameterTokenIterator& other) const
                {
                    return ptr == other.ptr;
                }

                //! Compare relative position with other iterator
                INLINE bool operator!=(const ParameterTokenIterator& other) const
                {
                    return ptr != other.ptr;
                }

                //! Compare relative position with other iterator
                INLINE bool operator<(const ParameterTokenIterator& other) const
                {
                    return ptr < other.ptr;
                }

                //! Move forward
                INLINE ParameterTokenIterator operator+(const difference_type diff) const
                {
                    ParameterTokenIterator copy(*this);
                    while (diff > 0 && copy)
                        ++copy;
                    return copy;
                }

            private:
                const ParameterToken* ptr = nullptr;

                INLINE void advance()
                {
                    if (ptr) ptr = ptr->next;
                }
            };

            //---

            class ParameterNode;

            class ParameterNodeIterator
            {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = const ParameterNode;
                using difference_type = ptrdiff_t;
                using pointer = const ParameterNode*;
                using reference = const ParameterNode&;

                INLINE ParameterNodeIterator()
                {}

                INLINE ParameterNodeIterator(const ParameterNode* ptr_)
                    : ptr(ptr_)
                {}

                INLINE ParameterNodeIterator(const ParameterNodeIterator& other)
                    : ptr(other.ptr)
                {}

                //! Valid ?
                INLINE operator bool() const
                {
                    return ptr != nullptr;
                }

                //! Move to next elem
                INLINE ParameterNodeIterator& operator++()
                {
                    advance();
                    return *this;
                }

                //! Move to next elem
                INLINE ParameterNodeIterator operator++(int)
                {
                    ParameterNodeIterator copy(*this);
                    advance();
                    return copy;
                }

                //! Get the node
                INLINE const ParameterNode* get() const
                {
                    return ptr;
                }

                //! Get reference to item
                INLINE const ParameterNode& operator*() const
                {
                    return *ptr;
                }

                //! Get pointer to item
                INLINE const ParameterNode* operator->() const
                {
                    return ptr;
                }

                //! Compare relative position with other iterator
                INLINE bool operator==(const ParameterNodeIterator& other) const
                {
                    return ptr == other.ptr;
                }

                //! Compare relative position with other iterator
                INLINE bool operator!=(const ParameterNodeIterator& other) const
                {
                    return ptr != other.ptr;
                }

                //! Compare relative position with other iterator
                INLINE bool operator<(const ParameterNodeIterator& other) const
                {
                    return ptr < other.ptr;
                }

                //! Move forward
                INLINE ParameterNodeIterator operator+(const ptrdiff_t diff) const
                {
                    ParameterNodeIterator copy(*this);
                    while (diff > 0 && copy)
                        ++copy;
                    return copy;
                }

            private:
                const ParameterNode* ptr = nullptr;

                INLINE void advance();
            };

            //---

            /// a node of parameter file
            /// NOTE: do not cache beyond the lifetime of ParameterFile
            class ParameterNode : public base::NoCopy
            {
            public:
                INLINE ParameterNode()
                    : m_children(nullptr)
                    , m_next(nullptr)
                    , m_tokens(nullptr)
                    , m_numTokens(0)
                    , m_numChildren(0)
                    , m_line(0)
                {}

                INLINE ParameterTokenIterator tokens() const
                {
                    return ParameterTokenIterator(m_tokens);
                }

                INLINE ParameterNodeIterator children() const
                {
                    return ParameterNodeIterator(m_children);
                }

                INLINE StringView<char> keyValue(const char* key) const
                {
                    for (auto cur  = m_children; cur; cur = cur->m_next)
                        if (cur->m_numTokens == 2 && cur->m_tokens->value == key)
                                return cur->m_tokens->next->value;

                    return nullptr;
                }

                template< typename T >
                INLINE T keyValueRead(const char* key, const T& defaultValue = T()) const
                {
                    T ret = defaultValue;

                    auto cur = keyValue(key);
                    if (cur)
                        cur.match(ret);

                    return ret;
                }

                INLINE const ParameterToken* lastToken() const
                {
                    const ParameterToken* lastToken = m_tokens;
                    while (lastToken && lastToken->next)
                        lastToken = lastToken->next;
                    return lastToken;
                }

                INLINE const ParameterToken* token(uint32_t index = 0) const
                {
                    const ParameterToken* ret = m_tokens;
                    uint32_t curIndex = 0;
                    while (ret != nullptr && curIndex < index)
                    {
                        ret = ret->next;
                        curIndex += 1;
                    }
                    return ret;
                }

                INLINE StringView<char> tokenValue(uint32_t index = 0) const
                {
                    auto cur  = token(index);
                    return cur ? cur->value : nullptr;
                }

                INLINE StringView<char> tokenValueStr(uint32_t index = 0) const
                {
                    auto cur  = token(index);
                    return cur ? StringView<char>(cur->value) : StringView<char>();
                }

                template< typename T >
                INLINE T tokenRead(uint32_t index, const T& defaultValue = T()) const
                {
                    T ret = defaultValue;

                    auto cur  = token(index);
                    if (cur)
                        base::StringView<char>(cur->value).match(ret);

                    return ret;
                }

                INLINE uint32_t size() const
                {
                    return m_numTokens;
                }

                INLINE uint32_t childrenCount() const
                {
                    return m_numChildren;
                }

                INLINE StringView<char> fileName() const
                {
                    return m_fileName;
                }

                INLINE uint32_t line() const
                {
                    return m_line;
                }

                void reportError(const char* txt) const;

            private:
                const ParameterNode* m_children;
                const ParameterNode* m_next;
                const ParameterToken* m_tokens;
                uint32_t m_numTokens;
                uint32_t m_numChildren;
                StringView<char> m_fileName;
                uint32_t m_line;

                friend class ParameterFileParser;
                friend class ParameterNodeIterator;
            };

            //---

            INLINE void ParameterNodeIterator::advance()
            {
                if (ptr) ptr = ptr->m_next;
            }

            //---

            /// a simple parameter file
            class ParameterFile : public base::NoCopy
            {
            public:
                ParameterFile(mem::LinearAllocator& mem);
                ~ParameterFile();

                /// get file allocator
                INLINE mem::LinearAllocator& mem() { return m_mem; }

                /// get the top level nodes of the file
                ParameterNodeIterator nodes() const;

            private:
                mem::LinearAllocator& m_mem;
                const ParameterNode* m_nodes;

                friend class ParameterFileParser;
            };

            //---

            class IParameterFileParserErrorReporter : public base::NoCopy
            {
            public:
                virtual ~IParameterFileParserErrorReporter();
                virtual void reportError(StringView<char> context, uint32_t line, StringView<char> txt) = 0;

                ///

                // get a default error reporter
                static IParameterFileParserErrorReporter& GetDefault();
            };

            /// parse file content, parameter buffer is not modified
            /// NOTE: the buffer MUST be kept alive as long as the results are in use
            extern bool ParseParameters(StringView<char> data, ParameterFile& outFile, StringView<char> context = nullptr, IParameterFileParserErrorReporter* errorReporter = nullptr);

            //---

        } // text
    } // res
} // base