/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#pragma once

#include "core/test/include/gtest/gtest.h"

namespace tests
{

	struct SimpleTracker
	{
		static uint32_t NumDefaultConstructed;
		static uint32_t NumValueConstructed;
		static uint32_t NumCopyConstructed;
		static uint32_t NumMoveConstructed;
		static uint32_t NumCopyAssigned;
		static uint32_t NumMoveAssigned;
		static uint32_t NumDestructed;

		static void ResetStats();
		static void ResetPopulation();

		static uint32_t AllocUnique();
		static void ReleaseUnique(uint32_t id); // errors if we release something we already did

		static void CheckPopulation(uint32_t expectedLiveObjectCount);

		uint32_t m_id;
		uint32_t m_value;

		SimpleTracker();
		SimpleTracker(uint32_t payload);
		SimpleTracker(const SimpleTracker& other);
		SimpleTracker(SimpleTracker&& other);
		SimpleTracker& operator=(const SimpleTracker& other);
		SimpleTracker& operator=(SimpleTracker&& other);
		~SimpleTracker();
	};

	class SimpleTrackerTest : public ::testing::Test
	{
	public:
		virtual void SetUp() override;
		virtual void TearDown() override;
	};

} // tests
