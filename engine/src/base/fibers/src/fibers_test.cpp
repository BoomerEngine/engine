/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"
#include "fiberSystem.h"

#include "base/test/include/gtest/gtest.h"
#include "base/containers/include/hashSet.h"

#include <stdarg.h>

DECLARE_TEST_FILE(Fibers);

#if 0

using namespace base;

class FibersFixture : public ::testing::Test
{
public:
    FibersFixture()
        : m_outputIndex(0)
        , m_echo(true)//TRUE == IsDebuggerPresent())
    {}

    void ExpectOutput(const char* txt)
    {
        output.push_back(txt);
    }

    void Output(const char* txt)
    {
        if (m_echo)
            fprintf(stderr, "[%d]: '%s'\n", m_outputIndex, txt);

        if (m_outputIndex >= (int)output.size())
        {
            FAIL() << "Unexpected output generated by test: '" << txt << "'";
        }
        else
        {
            auto expectedTxt  = output[m_outputIndex];
            if (0 != strcmp(expectedTxt, txt))
            {
                FAIL() << "Mismatched output generated by test: '" << txt << "', expected '" << expectedTxt << "'";
            }

            m_outputIndex += 1;
        }
    }

    virtual void SetUp() override final
    {
        m_outputIndex = 0;
        output.clear();
    }

    virtual void TearDown() override final
    {
        if (m_outputIndex != (int)output.size())
        {
            auto expectedOutput  = output[m_outputIndex];
            FAIL() << "Expected output never generated by test: '" << expectedOutput << "'";
        }
    }

public:
    std::vector<const char*>    output;
    int                         m_outputIndex;
    bool                        m_echo;
};

TEST_F(FibersFixture, HelloWorld)
{
    ExpectOutput("HelloWorld!");

    fibers::Job job;
    job.targetFunction = [this](uint64_t) { Output("HelloWorld!"); };

    auto counter = Fibers::GetInstance().scheduleFiber(job);
    Fibers::GetInstance().waitForCounterAndRelease(counter);
}

TEST_F(FibersFixture, MultipleJobInvocations4)
{
	base::SpinLock interationsLock;
    base::HashSet<uint64_t> interations;

    fibers::Job job;
    job.targetFunction = [this, &interations, &interationsLock](uint64_t i)
    {
		interationsLock.acquire();
		interations.insert(i);
		interationsLock.release();
    };

    auto counter = Fibers::GetInstance().scheduleAuto(job, 4);
    Fibers::GetInstance().waitForCounterAndRelease(counter);

	EXPECT_EQ(4, interations.size());
}

TEST_F(FibersFixture, MultipleJobInvocations1024)
{
	base::SpinLock interationsLock;
	base::HashSet<uint64_t> interations;

	fibers::Job job;
	job.targetFunction = [this, &interations, &interationsLock](uint64_t i)
	{
		interationsLock.acquire();
		interations.insert(i);
		interationsLock.release();
	};

	auto counter = Fibers::GetInstance().scheduleAuto(job, 1024);
	Fibers::GetInstance().waitForCounterAndRelease(counter);

	EXPECT_EQ(1024, interations.size());
}

#endif