/*
* src/Testing/test_StaticVector.cpp
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
#include "Libs/StaticVector.hpp"

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

	class StaticVectorTest : public ::testing::Test
	{
		protected:

		    void
			SetUp () override
			{
		        LifetimeTracker::reset();
		    }
	};

	TEST_F (StaticVectorTest, DefaultConstructor)
	{
	    StaticVector< int, 10 > s;
	    ASSERT_TRUE(s.empty());
	    ASSERT_EQ(s.size(), 0);
	    ASSERT_EQ(s.capacity(), 10);
	    ASSERT_EQ(s.max_size(), 10);
	}

	TEST_F (StaticVectorTest, SizeConstructor)
	{
	    StaticVector< int, 10 > s(5);
	    ASSERT_FALSE(s.empty());
	    ASSERT_EQ(s.size(), 5);

	    for (size_t i = 0; i < 5; ++i)
	    {
	        ASSERT_EQ(s[i], 0);
	    }
	}

	TEST_F (StaticVectorTest, PushBackAndAccess)
	{
	    StaticVector< int, 5 > s;
	    s.push_back(10);
	    s.push_back(20);

	    ASSERT_EQ(s.size(), 2);
	    ASSERT_EQ(s[0], 10);
	    ASSERT_EQ(s.at(1), 20);
	    ASSERT_EQ(s.front(), 10);
	    ASSERT_EQ(s.back(), 20);
	}

	TEST_F (StaticVectorTest, PopBack)
	{
	    StaticVector< int, 5 > s;
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

	TEST_F (StaticVectorTest, Clear)
	{
	    StaticVector< int, 5 > s;
	    s.push_back(1);
	    s.push_back(2);
	    s.clear();

	    ASSERT_TRUE(s.empty());
	    ASSERT_EQ(s.size(), 0);
	}

	TEST_F (StaticVectorTest, EmplaceBack)
	{
	    StaticVector< std::string, 5 > s;
	    s.emplace_back("hello");
	    s.emplace_back(5, 'c'); // "ccccc"

	    ASSERT_EQ(s.size(), 2);
	    ASSERT_EQ(s[0], "hello");
	    ASSERT_EQ(s[1], "ccccc");
	}

	TEST_F (StaticVectorTest, CapacityLimit)
	{
	    StaticVector< int, 3 > s;
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

	TEST_F (StaticVectorTest, AtBoundsCheck)
	{
	    StaticVector< int, 5 > s;
	    s.push_back(10);

	    ASSERT_EQ(s.at(0), 10);

#if defined(__cpp_exceptions)
	    ASSERT_THROW(s.at(1), std::out_of_range);
#else
	    ASSERT_DEATH((void) s.at(1), "");
#endif
	}

	TEST_F (StaticVectorTest, CopyConstructor)
	{
	    StaticVector< int, 10 > s1;
	    s1.push_back(10);
	    s1.push_back(20);

	    StaticVector<int, 10> s2(s1);

	    ASSERT_EQ(s1.size(), 2);
	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], 10);

	    s2[0] = 99;
	    ASSERT_EQ(s1[0], 10);
	}

	TEST_F (StaticVectorTest, CopyAssignment)
	{
	    StaticVector< int, 10 > s1;
	    s1.push_back(10);
	    s1.push_back(20);

	    StaticVector< int, 10 > s2;
	    s2.push_back(99);

	    s2 = s1;

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], 10);
	}

	TEST_F (StaticVectorTest, MoveConstructor)
	{
	    StaticVector< std::string, 10 > s1;
	    s1.push_back("hello");
	    s1.push_back("world");

	    StaticVector< std::string, 10 > s2(std::move(s1));

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], "hello");
	    ASSERT_TRUE(s1.empty());
	}

	TEST_F (StaticVectorTest, MoveAssignment)
	{
	    StaticVector< std::string, 10 > s1;
	    s1.push_back("hello");
	    s1.push_back("world");

	    StaticVector< std::string, 10 > s2;
	    s2 = std::move(s1);

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], "hello");
	    ASSERT_TRUE(s1.empty());
	}

	TEST_F (StaticVectorTest, Swap)
	{
	    StaticVector< int, 10 > s1;
	    s1.push_back(1);
	    s1.push_back(2);

	    StaticVector< int, 10 > s2;
	    s2.push_back(99);

	    s1.swap(s2);

	    ASSERT_EQ(s1.size(), 1);
	    ASSERT_EQ(s1[0], 99);
	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], 1);
	}

	TEST_F (StaticVectorTest, IteratorsAndSTLAlgos)
	{
	    StaticVector< int, 10 > s;
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

	TEST_F (StaticVectorTest, LifetimeTracker_PushAndPop)
	{
	    {
	        StaticVector<LifetimeTracker, 5> s;
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

	TEST_F (StaticVectorTest, LifetimeTracker_Copy)
	{
	    StaticVector< LifetimeTracker, 5 > s1;
	    s1.emplace_back(1);
	    s1.emplace_back(2);
	    LifetimeTracker::reset();

	    StaticVector< LifetimeTracker, 5 > s2(s1);
	    ASSERT_EQ(LifetimeTracker::copy_constructor_calls, 2);
	    ASSERT_EQ(LifetimeTracker::constructor_calls, 0);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 0);
	}

	TEST_F (StaticVectorTest, LifetimeTracker_Move)
	{
	    StaticVector< LifetimeTracker, 5 > s1;
	    s1.emplace_back(1);
	    s1.emplace_back(2);
	    LifetimeTracker::reset();

	    StaticVector< LifetimeTracker, 5 > s2(std::move(s1));
	    ASSERT_EQ(LifetimeTracker::move_constructor_calls, 2);
	    ASSERT_EQ(LifetimeTracker::constructor_calls, 0);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 0);
	}

	TEST_F (StaticVectorTest, LifetimeTracker_Resize)
	{
	    StaticVector< LifetimeTracker, 10 > s;
	    s.resize(5);
	    ASSERT_EQ(LifetimeTracker::constructor_calls, 5);
	    ASSERT_EQ(s.size(), 5);

	    s.resize(2);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 3);
	    ASSERT_EQ(s.size(), 2);
	}

	TEST_F (StaticVectorTest, InitializerListConstructor)
	{
	    StaticVector< int, 10 > s{1, 2, 3, 4, 5};
	    ASSERT_EQ(s.size(), 5);
	    ASSERT_EQ(s[0], 1);
	    ASSERT_EQ(s[4], 5);
	}

	TEST_F (StaticVectorTest, ResizeWithValue)
	{
	    StaticVector< int, 10 > s;
	    s.resize(5, 42);
	    ASSERT_EQ(s.size(), 5);
	    for ( size_t i = 0; i < 5; ++i )
	    {
	        ASSERT_EQ(s[i], 42);
	    }
	}

	TEST_F (StaticVectorTest, ReverseIterators)
	{
	    StaticVector< int, 10 > s{1, 2, 3, 4, 5};

	    std::vector< int > reversed;
	    for ( auto it = s.rbegin(); it != s.rend(); ++it )
	    {
	        reversed.push_back(*it);
	    }

	    ASSERT_EQ(reversed.size(), 5);
	    ASSERT_EQ(reversed[0], 5);
	    ASSERT_EQ(reversed[4], 1);
	}

	TEST_F (StaticVectorTest, ComparisonOperators)
	{
	    StaticVector< int, 10 > s1{1, 2, 3};
	    StaticVector< int, 10 > s2{1, 2, 3};
	    StaticVector< int, 10 > s3{1, 2, 4};
	    StaticVector< int, 10 > s4{1, 2};

	    ASSERT_TRUE(s1 == s2);
	    ASSERT_FALSE(s1 == s3);
	    ASSERT_FALSE(s1 == s4);
	    ASSERT_TRUE(s1 != s3);
	    ASSERT_TRUE(s1 < s3);
	    ASSERT_TRUE(s4 < s1);
	}

	TEST_F (StaticVectorTest, SwapDifferentSizes)
	{
	    StaticVector< int, 10 > s1{1, 2, 3, 4, 5};
	    StaticVector< int, 10 > s2{99, 88};

	    s1.swap(s2);

	    ASSERT_EQ(s1.size(), 2);
	    ASSERT_EQ(s1[0], 99);
	    ASSERT_EQ(s1[1], 88);
	    ASSERT_EQ(s2.size(), 5);
	    ASSERT_EQ(s2[0], 1);
	    ASSERT_EQ(s2[4], 5);
	}

	TEST_F (StaticVectorTest, QuickSwap)
	{
	    StaticVector< int, 10 > s1{1, 2, 3};
	    StaticVector< int, 10 > s2{7, 8, 9, 10};

	    s1.quick_swap(s2);

	    ASSERT_EQ(s1.size(), 4);
	    ASSERT_EQ(s1[0], 7);
	    ASSERT_EQ(s1[3], 10);
	    ASSERT_EQ(s2.size(), 3);
	    ASSERT_EQ(s2[0], 1);
	    ASSERT_EQ(s2[2], 3);
	}

	TEST_F (StaticVectorTest, ConstCorrectness)
	{
	    StaticVector< int, 10 > s{1, 2, 3};
	    const StaticVector< int, 10 > & constRef = s;

	    ASSERT_EQ(constRef.size(), 3);
	    ASSERT_EQ(constRef[0], 1);
	    ASSERT_EQ(constRef.at(1), 2);
	    ASSERT_EQ(constRef.front(), 1);
	    ASSERT_EQ(constRef.back(), 3);
	    ASSERT_FALSE(constRef.empty());
	    ASSERT_EQ(constRef.capacity(), 10);

	    int sum = 0;
	    for ( auto it = constRef.cbegin(); it != constRef.cend(); ++it )
	    {
	        sum += *it;
	    }
	    ASSERT_EQ(sum, 6);
	}

	TEST_F (StaticVectorTest, DataPointer)
	{
	    StaticVector< int, 10 > s{1, 2, 3};
	    int * ptr = s.data();
	    ASSERT_NE(ptr, nullptr);
	    ASSERT_EQ(ptr[0], 1);
	    ASSERT_EQ(ptr[2], 3);

	    ptr[1] = 99;
	    ASSERT_EQ(s[1], 99);
	}

	TEST_F (StaticVectorTest, EmptyDataPointer)
	{
	    StaticVector< int, 10 > s;
	    ASSERT_NE(s.data(), nullptr);
	}

	TEST_F (StaticVectorTest, MoveAssignmentBehavior)
	{
	    StaticVector< std::string, 10 > s1{"hello", "world"};
	    StaticVector< std::string, 10 > s2{"foo", "bar", "baz"};

	    const auto s2_original_size = s2.size();
	    s2 = std::move(s1);

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0], "hello");
	    ASSERT_EQ(s2[1], "world");
	    ASSERT_EQ(s1.size(), s2_original_size);
	}

	TEST_F (StaticVectorTest, SelfAssignment)
	{
	    StaticVector< int, 10 > s{1, 2, 3};
	    s = s;
	    ASSERT_EQ(s.size(), 3);
	    ASSERT_EQ(s[0], 1);
	    ASSERT_EQ(s[1], 2);
	    ASSERT_EQ(s[2], 3);
	}

	TEST_F (StaticVectorTest, LifetimeTracker_CopyAssignment)
	{
	    StaticVector< LifetimeTracker, 5 > s1;
	    s1.emplace_back(1);
	    s1.emplace_back(2);

	    StaticVector< LifetimeTracker, 5 > s2;
	    s2.emplace_back(99);

	    LifetimeTracker::reset();
	    s2 = s1;

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0].value, 1);
	    ASSERT_EQ(LifetimeTracker::copy_constructor_calls, 2);
	}

	TEST_F (StaticVectorTest, LifetimeTracker_MoveAssignment)
	{
	    StaticVector< LifetimeTracker, 5 > s1;
	    s1.emplace_back(1);
	    s1.emplace_back(2);

	    StaticVector< LifetimeTracker, 5 > s2;
	    s2.emplace_back(99);

	    LifetimeTracker::reset();
	    s2 = std::move(s1);

	    ASSERT_EQ(s2.size(), 2);
	    ASSERT_EQ(s2[0].value, 1);
	}

	TEST_F (StaticVectorTest, Erase_SingleElement)
	{
	    StaticVector< int, 10 > vec{1, 2, 3, 4, 5};

	    auto it = vec.erase(vec.begin() + 2);

	    ASSERT_EQ(vec.size(), 4);
	    ASSERT_EQ(vec[0], 1);
	    ASSERT_EQ(vec[1], 2);
	    ASSERT_EQ(vec[2], 4);
	    ASSERT_EQ(vec[3], 5);
	    ASSERT_EQ(*it, 4);
	}

	TEST_F (StaticVectorTest, Erase_FirstElement)
	{
	    StaticVector< int, 10 > vec{1, 2, 3, 4, 5};

	    auto it = vec.erase(vec.begin());

	    ASSERT_EQ(vec.size(), 4);
	    ASSERT_EQ(vec[0], 2);
	    ASSERT_EQ(vec[1], 3);
	    ASSERT_EQ(vec[2], 4);
	    ASSERT_EQ(vec[3], 5);
	    ASSERT_EQ(*it, 2);
	}

	TEST_F (StaticVectorTest, Erase_LastElement)
	{
	    StaticVector< int, 10 > vec{1, 2, 3, 4, 5};

	    auto it = vec.erase(vec.end() - 1);

	    ASSERT_EQ(vec.size(), 4);
	    ASSERT_EQ(vec[0], 1);
	    ASSERT_EQ(vec[1], 2);
	    ASSERT_EQ(vec[2], 3);
	    ASSERT_EQ(vec[3], 4);
	    ASSERT_EQ(it, vec.end());
	}

	TEST_F (StaticVectorTest, Erase_Range)
	{
	    StaticVector< int, 10 > vec{1, 2, 3, 4, 5, 6, 7};

	    auto it = vec.erase(vec.begin() + 2, vec.begin() + 5);

	    ASSERT_EQ(vec.size(), 4);
	    ASSERT_EQ(vec[0], 1);
	    ASSERT_EQ(vec[1], 2);
	    ASSERT_EQ(vec[2], 6);
	    ASSERT_EQ(vec[3], 7);
	    ASSERT_EQ(*it, 6);
	}

	TEST_F (StaticVectorTest, Erase_EmptyRange)
	{
	    StaticVector< int, 10 > vec{1, 2, 3, 4, 5};

	    auto it = vec.erase(vec.begin() + 2, vec.begin() + 2);

	    ASSERT_EQ(vec.size(), 5);
	    ASSERT_EQ(vec[0], 1);
	    ASSERT_EQ(vec[1], 2);
	    ASSERT_EQ(vec[2], 3);
	    ASSERT_EQ(vec[3], 4);
	    ASSERT_EQ(vec[4], 5);
	    ASSERT_EQ(*it, 3);
	}

	TEST_F (StaticVectorTest, Erase_WithLifetimeTracker)
	{
	    StaticVector< LifetimeTracker, 10 > vec;
	    vec.emplace_back(1);
	    vec.emplace_back(2);
	    vec.emplace_back(3);
	    vec.emplace_back(4);
	    vec.emplace_back(5);

	    LifetimeTracker::reset();

	    vec.erase(vec.begin() + 2);

	    ASSERT_EQ(vec.size(), 4);
	    ASSERT_EQ(vec[0].value, 1);
	    ASSERT_EQ(vec[1].value, 2);
	    ASSERT_EQ(vec[2].value, 4);
	    ASSERT_EQ(vec[3].value, 5);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 1);
	}

	TEST_F (StaticVectorTest, Erase_RangeWithLifetimeTracker)
	{
	    StaticVector< LifetimeTracker, 10 > vec;
	    vec.emplace_back(1);
	    vec.emplace_back(2);
	    vec.emplace_back(3);
	    vec.emplace_back(4);
	    vec.emplace_back(5);

	    LifetimeTracker::reset();

	    vec.erase(vec.begin() + 1, vec.begin() + 4);

	    ASSERT_EQ(vec.size(), 2);
	    ASSERT_EQ(vec[0].value, 1);
	    ASSERT_EQ(vec[1].value, 5);
	    ASSERT_EQ(LifetimeTracker::destructor_calls, 3);
	}
}
