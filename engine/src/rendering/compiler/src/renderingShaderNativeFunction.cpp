/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#include "build.h"
#include "renderingShaderNativeFunction.h"

namespace rendering
{
    namespace compiler
    {
        ///---


        RTTI_BEGIN_TYPE_CLASS(FunctionNameMetadata);
        RTTI_END_TYPE();

        FunctionNameMetadata::FunctionNameMetadata()
            : m_name(nullptr)
        {}

        ///---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(INativeFunction);
        RTTI_END_TYPE();

        INativeFunction::INativeFunction()
        {}

        INativeFunction::~INativeFunction()
        {}

        const INativeFunction* INativeFunction::mutateFunction(TypeLibrary& typeLibrary, uint32_t numArgs, DataType* argTypes, const base::parser::Location& loc, base::parser::IErrorReporter& err) const
        {
            // function is not mutated
            return this;
        }

        //--

        class FunctionCache : public base::ISingleton
        {
            DECLARE_SINGLETON(FunctionCache);

        public:
            FunctionCache()
            {
                buildCache();
            }

            const INativeFunction* findFunctionByName(const char* name)
            {
                base::StringID funcName(name);

                INativeFunction* ret = nullptr;
                m_functions.find(funcName, ret);
                return ret;
            }

        private:
            typedef base::HashMap<base::StringID, INativeFunction*> TFunctions;
            TFunctions m_functions;

            virtual void deinit() override
            {
                m_functions.clearPtr();
            }

            void buildCache()
            {
                base::Array< base::ClassType > functionClasses;
                base::rtti::TypeSystem::GetInstance().enumClasses(INativeFunction::GetStaticClass(), functionClasses);

                for (auto classPtr  : functionClasses)
                {
                    auto functionNameData = classPtr->findMetadata<FunctionNameMetadata>();
                    if (functionNameData)
                    {
                        base::StringID functionName(functionNameData->name());
                        if (m_functions.contains(functionName))
                        {
                            FATAL_ERROR(base::TempString("Function '{}' already registered", functionName));
                        }
                        else
                        {
                            auto functionObject = classPtr->createPointer<INativeFunction>();
                            m_functions.set(functionName, functionObject);
                        }
                    }
                }
            }
        };

        const INativeFunction* INativeFunction::FindFunctionByName(const char* name)
        {
            return FunctionCache::GetInstance().findFunctionByName(name);
        }


    } // shader
} // rendering