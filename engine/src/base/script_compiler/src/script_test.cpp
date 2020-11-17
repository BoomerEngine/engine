/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: script #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(Scripts);

using namespace base;

namespace testing
{



#if 0


class ScriptFixture : public ::testing::Test
    {
    public:
        script::Environment m_env;

        AngelScript::asIScriptModule* m_module;

        virtual void SetUp()
        {
            m_module = m_env.engine()->GetModule("Testing", AngelScript::asGM_ALWAYS_CREATE);
            ASSERT_TRUE(m_module != nullptr);
        }

        virtual void TearDown()
        {
        }

        void compile(const char* code)
        {
            auto r = m_module->AddScriptSection("script", code, strlen(code));
            ASSERT_TRUE(r >= 0);

            r = m_module->Build();
            ASSERT_TRUE(r >= 0);
        }
    };

    static const bool GFunctionCalled = 0;

    void TestFunction()
    {
        GFunctionCalled = 1;
    }

    TEST_F(ScriptFixture, SimpleTest)
    {
        // register a global function
        auto r = m_env.engine()->RegisterGlobalFunction("void Test()", asFUNCTION(TestFunction), AngelScript::asCALL_CDECL);
        ASSERT_TRUE(r >= 0);

        // compile test code
        compile("void GlobalMethod() { Test(); }\n");

        // Find the function for the function we want to execute.
        auto func  = m_env.engine()->GetModule("Testing")->GetFunctionByName("GlobalMethod");
        ASSERT_TRUE(func);

        // create context
        auto ctx  = m_env.engine()->CreateContext();
        ASSERT_TRUE(ctx);

        // prepare
        r = ctx->Prepare(func);
        ASSERT_TRUE(r >= 0);

        // run
        r = ctx->Execute();
        ASSERT_EQ(AngelScript::asEXECUTION_FINISHED, r);
        ctx->Release();

        // make sure we were called
        ASSERT_EQ(1, GFunctionCalled);
    }

    static StringBuf GFunctionStringIn;
    static StringBuf GFunctionStringOut;

    void TestStringFunctionIn(const StringBuf& str)
    {
        GFunctionStringIn = str;
    }

    TEST_F(ScriptFixture, StringInTest)
    {
        // register a global function
        auto r = m_env.engine()->RegisterGlobalFunction("void Test(string& in)", asFUNCTION(TestStringFunctionIn), AngelScript::asCALL_CDECL);
        ASSERT_TRUE(r >= 0);

        // compile test code
        compile(
        "void GlobalMethod() { "
        "  string s = \"Hello\" + \" \";\n"
        "  s += \"World!\";\n"
        "  s += 42;\n"
        "  Test(s);\n"
        "}");

        // Find the function for the function we want to execute.
        auto func  = m_env.engine()->GetModule("Testing")->GetFunctionByName("GlobalMethod");
        ASSERT_TRUE(func);

        // create context
        auto ctx  = m_env.engine()->CreateContext();
        ASSERT_TRUE(ctx);

        // prepare
        r = ctx->Prepare(func);
        ASSERT_TRUE(r >= 0);

        // run
        r = ctx->Execute();
        ASSERT_EQ(AngelScript::asEXECUTION_FINISHED, r);
        ctx->Release();

        // make sure we were called
        ASSERT_EQ(std::string("Hello World!42"), std::string(GFunctionStringIn.c_str()));
    }

    static base::Vector3 GFunctionVectorIn;
    static base::Vector3 GFunctionVectorIn2;

    void TestVectorFunctionIn(const base::Vector3& str)
    {
        GFunctionVectorIn = str;
    }

    void TestVectorFunctionIn2(const base::Vector3& str)
    {
        GFunctionVectorIn2 = str;
    }

    TEST_F(ScriptFixture, VectorTest)
    {
        // register a global function
        auto r = m_env.engine()->RegisterGlobalFunction("void Test(const Vector3& in)", asFUNCTION(TestVectorFunctionIn), AngelScript::asCALL_CDECL); ASSERT_TRUE(r >= 0);
        m_env.engine()->RegisterGlobalFunction("void Test2(const Vector3& in)", asFUNCTION(TestVectorFunctionIn2), AngelScript::asCALL_CDECL); ASSERT_TRUE(r >= 0);

        // compile test code
        compile(
                "void GlobalMethod() { "
                "  Vector3 pos;"
                "  pos.x = -1;"
                "  pos.y = 2;"
                "  pos.z = -3;"
                "  Test(pos);"
                "  Test2(pos.abs());"
                "}");

        // Find the function for the function we want to execute.
        auto func  = m_env.engine()->GetModule("Testing")->GetFunctionByName("GlobalMethod");
        ASSERT_TRUE(func);

        // create context
        auto ctx  = m_env.engine()->CreateContext();
        ASSERT_TRUE(ctx);

        // prepare
        r = ctx->Prepare(func);
        ASSERT_TRUE(r >= 0);

        // run
        r = ctx->Execute();
        ASSERT_EQ(AngelScript::asEXECUTION_FINISHED, r);
        ctx->Release();

        // make sure we were called
        ASSERT_EQ(-1.0f, GFunctionVectorIn.x);
        ASSERT_EQ(2.0f, GFunctionVectorIn.y);
        ASSERT_EQ(-3.0f, GFunctionVectorIn.z);
        ASSERT_EQ(1.0f, GFunctionVectorIn2.x);
        ASSERT_EQ(2.0f, GFunctionVectorIn2.y);
        ASSERT_EQ(3.0f, GFunctionVectorIn2.z);
    }

    static base::RefPtr<NativeScriptObject> GFunctionObjectPtr;

    template< typename T >
    struct NakedPointer
    {
        ~NakedPointer()
        {
            reset();
        }

        RefPtr<T> ptr() const
        {
            if (ptr)
                return ptr->sharedFromThisType<T>();

            return nullptr;
        }

        void reset()
        {
            if (ptr)
            {
                ptr->removeManualStrongRef();
                ptr = nullptr;
            }
        }

        IObject* ptr;
    };

    void TestObjectPtrFunction(NativeScriptObject* ptr)
    {
        GFunctionObjectPtr = ptr ? ptr->sharedFromThisType<NativeScriptObject>() : nullptr;
        if (ptr) ptr->removeManualStrongRef();
    }

    TEST_F(ScriptFixture, NativeClassTest)
    {
        auto r = m_env.engine()->RegisterGlobalFunction("void Test(NativeScriptObject@ ptr)", asFUNCTION(TestObjectPtrFunction), AngelScript::asCALL_CDECL); ASSERT_TRUE(r >= 0);

        // compile test code
        compile(
                "void GlobalMethod(NativeScriptObject@ a) { "
                "  a.a = 21; "
                "  a.makeDouble();"
                "  a.hello();"
                "  a.str = \"Answer is: \" + a.str;"
                "  Test(a);"
                "}");

        // Find the function for the function we want to execute.
        auto func  = m_env.engine()->GetModule("Testing")->GetFunctionByName("GlobalMethod");
        ASSERT_TRUE(func);

        // create context
        auto ctx  = m_env.engine()->CreateContext();
        ASSERT_TRUE(ctx);

        // create object
        auto ptr = base::RefNew<NativeScriptObject>();

        // prepare
        r = ctx->Prepare(func);
        ctx->SetArgObject(0, ptr.get());
        ASSERT_TRUE(r >= 0);
        ptr.reset();

        // run
        r = ctx->Execute();
        if (r == AngelScript::asEXECUTION_EXCEPTION)
        {
            fprintf(stderr, "Execution exception: '%s'\n", ctx->GetExceptionString());
        }
        ASSERT_EQ(AngelScript::asEXECUTION_FINISHED, r);
        ctx->SetArgObject(0, nullptr);
        ctx->Release();

        // make sure we were called
        ASSERT_TRUE(GFunctionObjectPtr.get());
        ASSERT_EQ(std::string("Answer is: '42'"), std::string(GFunctionObjectPtr->str.c_str()));
        base::UntypedRefWeakPtr weakRef = GFunctionObjectPtr;
        GFunctionObjectPtr.reset();

        // object should be deleted by now
        ASSERT_TRUE(weakRef.expired());
    }

#endif
} // testing

