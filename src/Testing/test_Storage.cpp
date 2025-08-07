/*
* src/Testing/test_Storage.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include <gtest/gtest.h>

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Libs/Storage.hpp"

namespace EmEn::Libs
{
	struct LifetimeTracker
	{
	    static int constructor_calls;
	    static int copy_constructor_calls;
	    static int move_constructor_calls;
	    static int destructor_calls;

	    explicit
		LifetimeTracker (int v = 0)
			: value(v)
	    {
		    constructor_calls++;
	    }

	    LifetimeTracker (const LifetimeTracker & other)
			: value(other.value)
	    {
		    copy_constructor_calls++;
	    }

	    LifetimeTracker (LifetimeTracker && other) noexcept
			: value(other.value)
	    {
		    move_constructor_calls++;
	    }

		~LifetimeTracker ()
	    {
	    	destructor_calls++;
	    }

	    LifetimeTracker & operator= (const LifetimeTracker &) = default;

	    LifetimeTracker & operator= (LifetimeTracker &&) = default;

		static
		void
		reset ()
		{
			constructor_calls = 0;
			copy_constructor_calls = 0;
			move_constructor_calls = 0;
			destructor_calls = 0;
		}

		int value;
	};

	int LifetimeTracker::constructor_calls = 0;
	int LifetimeTracker::copy_constructor_calls = 0;
	int LifetimeTracker::move_constructor_calls = 0;
	int LifetimeTracker::destructor_calls = 0;

	class StorageTest : public ::testing::Test
	{
		protected:

		    void
			SetUp () override
			{
		        LifetimeTracker::reset();
		    }
	};

	TEST_F (StorageTest, DefaultConstructor)
	{
	    Storage< int, 10 > s;
	    ASSERT_TRUE(s.empty());
	    ASSERT_EQ(s.size(), 0);
	    ASSERT_EQ(s.capacity(), 10);
	    ASSERT_EQ(s.max_size(), 10);
	}

	TEST_F (StorageTest, SizeConstructor)
	{
	    Storage< int, 10 > s(5);
	    ASSERT_FALSE(s.empty());
	    ASSERT_EQ(s.size(), 5);

	    for (size_t i = 0; i < 5; ++i)
	    {
	        ASSERT_EQ(s[i], 0);
	    }
	}

	TEST_F (StorageTest, PushBackAndAccess)
	{
	    Storage< int, 5 > s;
	    s.push_back(10);
	    s.push_back(20);

	    ASSERT_EQ(s.size(), 2);
	    ASSERT_EQ(s[0], 10);
	    ASSERT_EQ(s.at(1), 20);
	    ASSERT_EQ(s.front(), 10);
	    ASSERT_EQ(s.back(), 20);
	}

	TEST_F (StorageTest, PopBack)
	{
	    Storage< int, 5 > s;
	    s.push_back(10);
	    s.push_back(20);

	    s.pop_back();
	    ASSERT_EQ(s.size(), 1);
	    ASSERT_EQ(s.back(), 10);

	    s.pop_back();
	    ASSERT_TRUE(s.empty());

	    s.pop_back();
	    ASSERT_TRUE(s.empty());
	}

	TEST_F (StorageTest, Clear)
	{
	    Storage< int, 5 > s;
	    s.push_back(1);
	    s.push_back(2);
	    s.clear();

	    ASSERT_TRUE(s.empty());
	    ASSERT_EQ(s.size(), 0);
	}

	TEST_F (StorageTest, EmplaceBack)
	{
	    Storage< std::string, 5 > s;
	    s.emplace_back("hello");
	    s.emplace_back(5, 'c'); // "ccccc"

	    ASSERT_EQ(s.size(), 2);
	    ASSERT_EQ(s[0], "hello");
	    ASSERT_EQ(s[1], "ccccc");
	}

	TEST_F (StorageTest, CapacityLimit)
	{
	    Storage< int, 3 > s;
	    s.push_back(1);
	    s.push_back(2);
	    s.push_back(3);

	    ASSERT_EQ(s.size(), 3);

#if defined(__cpp_exceptions)
	    ASSERT_THROW(s.push_back(4), std::length_error);
#else
	    ASSERT_DEATH(s.push_back(4), "");
#endif
	}

	TEST_F (StorageTest, AtBoundsCheck)
	{
	    Storage< int, 5 > s;
	    s.push_back(10);

	    ASSERT_EQ(s.at(0), 10);

#if defined(__cpp_exceptions)
	    ASSERT_THROW(s.at(1), std::out_of_range);
#else
	    ASSERT_DEATH(s.at(1), "");
#endif
	}

	TEST_F (StorageTest, CopyConstructor)
	{
	    Storage< int, 10 > s1;
	    s1.push_back(10);
	    s1.push_back(20);

	    Storage<int, 10> s2(s1);

	    ASSERT_EQ(s1.size(), 2);
	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], 10);

	    s2[0] = 99;
	    ASSERT_EQ(s1[0], 10);
	}

	TEST_F (StorageTest, CopyAssignment)
	{
	    Storage< int, 10 > s1;
	    s1.push_back(10);
	    s1.push_back(20);

	    Storage< int, 10 > s2;
	    s2.push_back(99);

	    s2 = s1;

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], 10);
	}

	TEST_F (StorageTest, MoveConstructor)
	{
	    Storage< std::string, 10 > s1;
	    s1.push_back("hello");
	    s1.push_back("world");

	    Storage< std::string, 10 > s2(std::move(s1));

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], "hello");
	    ASSERT_TRUE(s1.empty());
	}

	TEST_F (StorageTest, MoveAssignment)
	{
	    Storage< std::string, 10 > s1;
	    s1.push_back("hello");
	    s1.push_back("world");

	    Storage< std::string, 10 > s2;
	    s2 = std::move(s1);

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], "hello");
	    ASSERT_TRUE(s1.empty());
	}

	TEST_F (StorageTest, Swap)
	{
	    Storage< int, 10 > s1;
	    s1.push_back(1);
	    s1.push_back(2);

	    Storage< int, 10 > s2;
	    s2.push_back(99);

	    s1.swap(s2);

	    ASSERT_EQ(s1.size(), 1);
	    ASSERT_EQ(s1[0], 99);
	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], 1);
	}

	TEST_F (StorageTest, IteratorsAndSTLAlgos)
	{
	    Storage< int, 10 > s;
	    s.push_back(10);
	    s.push_back(20);
	    s.push_back(30);

	    int sum = 0;

	    for ( const auto & val : s )
	    {
	        sum += val;
	    }
	    ASSERT_EQ(sum, 60);

	    auto it = std::find(s.begin(), s.end(), 20);
	    ASSERT_NE(it, s.end());
	    ASSERT_EQ(*it, 20);
	}

	TEST_F (StorageTest, LifetimeTracker_PushAndPop)
	{
	    {
	        Storage<LifetimeTracker, 5> s;
	        s.emplace_back(1);
	        s.emplace_back(2);

	        ASSERT_EQ(s.size(), 2);
	        ASSERT_EQ(LifetimeTracker::constructor_calls, 2);
	        ASSERT_EQ(LifetimeTracker::destructor_calls, 0);

	        s.pop_back();
	        ASSERT_EQ(s.size(), 1);
	        ASSERT_EQ(LifetimeTracker::destructor_calls, 1);
	    }

	    ASSERT_EQ(LifetimeTracker::destructor_calls, 2);
	}

	TEST_F (StorageTest, LifetimeTracker_Copy)
	{
	    Storage< LifetimeTracker, 5 > s1;
	    s1.emplace_back(1);
	    s1.emplace_back(2);
	    LifetimeTracker::reset();

	    Storage< LifetimeTracker, 5 > s2(s1);
	    ASSERT_EQ(LifetimeTracker::copy_constructor_calls, 2);
	    ASSERT_EQ(LifetimeTracker::constructor_calls, 0);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 0);
	}

	TEST_F (StorageTest, LifetimeTracker_Move)
	{
	    Storage< LifetimeTracker, 5 > s1;
	    s1.emplace_back(1);
	    s1.emplace_back(2);
	    LifetimeTracker::reset();

	    Storage< LifetimeTracker, 5 > s2(std::move(s1));
	    ASSERT_EQ(LifetimeTracker::move_constructor_calls, 2);
	    ASSERT_EQ(LifetimeTracker::constructor_calls, 0);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 0);
	}

	TEST_F (StorageTest, LifetimeTracker_Resize)
	{
	    Storage< LifetimeTracker, 10 > s;
	    s.resize(5);
	    ASSERT_EQ(LifetimeTracker::constructor_calls, 5);
	    ASSERT_EQ(s.size(), 5);

	    s.resize(2);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 3);
	    ASSERT_EQ(s.size(), 2);
	}
}