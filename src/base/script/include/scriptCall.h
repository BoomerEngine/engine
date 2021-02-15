/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#pragma once

namespace base
{
    namespace script
    {

        //----

        class ScriptedObject;

        // base wrapper for callable function
        class BASE_SCRIPT_API ScriptedCallRaw
        {
        public:
            ScriptedCallRaw(void* context, ClassType classPtr, StringID name);
            ScriptedCallRaw(IObject* context, StringID name);

            INLINE ScriptedCallRaw() : m_context(nullptr), m_function(nullptr) {}
            INLINE ScriptedCallRaw(ScriptedCallRaw&& other) = default;
            INLINE ScriptedCallRaw(const ScriptedCallRaw& other) = default;
            INLINE ScriptedCallRaw& operator=(ScriptedCallRaw&& other) = default;
            INLINE ScriptedCallRaw& operator=(const ScriptedCallRaw& other) = default;

            // get context
            INLINE void* context() const { return m_context; }

            // get resolved function
            INLINE const rtti::Function* function() const { return m_function; }

            // valid call ?
            INLINE operator bool() const { return m_function != nullptr; }

            //--

            INLINE void callRaw(rtti::FunctionCallingParams& params) const
            {
                if (m_function)
                    m_function->run(nullptr, m_context, params);
            }

        protected:
            void* m_context;
            const rtti::Function* m_function;
        };

        //----

        // callable function, no return
        struct BASE_SCRIPT_API ScriptedCall : public ScriptedCallRaw
        {
        public:
            INLINE ScriptedCall()
                : ScriptedCallRaw()
            {}

            INLINE ScriptedCall(void* context, ClassType classPtr, StringID name)
                : ScriptedCallRaw(context, classPtr, name)
            {}

            INLINE ScriptedCall(IObject* context, StringID name)
                : ScriptedCallRaw(context, name)
            {}

            INLINE ScriptedCall(ScriptedCall&& other) = default;
            INLINE ScriptedCall(const ScriptedCall& other) = default;
            INLINE ScriptedCall& operator=(ScriptedCall&& other) = default;
            INLINE ScriptedCall& operator=(const ScriptedCall& other) = default;

            //--

            INLINE void call() const
            {
                rtti::FunctionCallingParams params;
                callRaw(params);
            }

            template< typename T0 >
            INLINE void call(const T0& p0) const
            {
                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                callRaw(params);
            }

            template< typename T0, typename T1  >
            INLINE void call(const T0& p0, const T1& p1) const
            {
                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_argumentsPtr[1] = (void*)&p1;
                callRaw(params);
            }

            template< typename T0, typename T1, typename T2 >
            INLINE void call(const T0& p0, const T1& p1, const T2& p2) const
            {
                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_argumentsPtr[1] = (void*)&p1;
                params.m_argumentsPtr[2] = (void*)&p2;
                callRaw(params);
            }

            template< typename T0, typename T1, typename T2, typename T3 >
            INLINE void call(const T0& p0, const T1& p1, const T2& p2, const T3& p3) const
            {
                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_argumentsPtr[1] = (void*)&p1;
                params.m_argumentsPtr[2] = (void*)&p2;
                params.m_argumentsPtr[3] = (void*)&p3;
                callRaw(params);
            }
        };

        //----

        // callable function, returning a type
        template< typename Ret >
        struct ScriptedCallRet : public ScriptedCallRaw
        {
        public:
            INLINE ScriptedCallRet()
                : ScriptedCallRaw()
            {}

            INLINE ScriptedCallRet(void* context, ClassType classPtr, StringID name)
                : ScriptedCallRaw(context, classPtr, name)
            {}

            INLINE ScriptedCallRet(IObject* context, StringID name)
                : ScriptedCallRaw(context, name)
            {}

            INLINE ScriptedCallRet(ScriptedCallRet<Ret>&& other) = default;
            INLINE ScriptedCallRet(const ScriptedCallRet<Ret>& other) = default;
            INLINE ScriptedCallRet& operator=(ScriptedCallRet<Ret>&& other) = default;
            INLINE ScriptedCallRet& operator=(const ScriptedCallRet<Ret>& other) = default;

            //--

            INLINE Ret call() const
            {
                auto ret = Ret();

                rtti::FunctionCallingParams params;
                params.m_returnPtr = &ret;
                callRaw(params);

                return ret;
            }

            template< typename T0 >
            INLINE Ret call(const T0& p0)
            {
                auto ret = Ret();

                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_returnPtr = &ret;
                callRaw(params);

                return ret;
            }

            template< typename T0, typename T1  >
            INLINE Ret call(const T0& p0, const T1& p1)
            {
                auto ret = Ret();

                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_argumentsPtr[1] = (void*)&p1;
                params.m_returnPtr = &ret;
                callRaw(params);

                return ret;
            }

            template< typename T0, typename T1, typename T2 >
            INLINE Ret call(const T0& p0, const T1& p1, const T2& p2)
            {
                auto ret = Ret();

                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_argumentsPtr[1] = (void*)&p1;
                params.m_argumentsPtr[2] = (void*)&p2;
                params.m_returnPtr = &ret;
                callRaw(params);

                return ret;
            }

            template< typename T0, typename T1, typename T2, typename T3 >
            INLINE Ret call(const T0& p0, const T1& p1, const T2& p2, const T3& p3)
            {
                auto ret = Ret();

                rtti::FunctionCallingParams params;
                params.m_argumentsPtr[0] = (void*)&p0;
                params.m_argumentsPtr[1] = (void*)&p1;
                params.m_argumentsPtr[2] = (void*)&p2;
                params.m_argumentsPtr[3] = (void*)&p3;
                params.m_returnPtr = &ret;
                callRaw(params);

                return ret;
            }
        };

        //----

    } // script
} // base