/*
 * src/Testing/test_MathVector.cpp
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
#include <array>

/* Local inclusions. */
#include "Libs/Math/Vector.hpp"

using namespace Libs::Math;

using MathTypeList = testing::Types< int, float, double >;

template< typename >
struct MathVector
	: testing::Test
{

};

TYPED_TEST_SUITE(MathVector, MathTypeList);

TYPED_TEST(MathVector, Vector2Default)
{
	const std::array< TypeParam, 2 > dst{0, 0};

	auto test = Vector< 2, TypeParam >{};

	for ( size_t i = 0; i < dst.size(); ++i )
	{
		ASSERT_EQ(test[i], dst[i]);
	}

	test.reset();

	for ( size_t i = 0; i < dst.size(); ++i )
	{
		ASSERT_EQ(test[i], dst[i]);
	}
}

TYPED_TEST(MathVector, Vector3Default)
{
	const std::array< TypeParam, 3 > dst{0, 0, 0};

	auto test = Vector< 3, TypeParam >{};

	for ( size_t i = 0; i < dst.size(); ++i )
	{
		ASSERT_EQ(test[i], dst[i]);
	}

	test.reset();

	for ( size_t i = 0; i < dst.size(); ++i )
	{
		ASSERT_EQ(test[i], dst[i]);
	}
}

TYPED_TEST(MathVector, Vector4Default)
{
	const std::array< TypeParam, 4 > dst{0, 0, 0, 0};

	auto test = Vector< 4, TypeParam >{};

	for ( size_t i = 0; i < dst.size(); ++i )
	{
		ASSERT_EQ(test[i], dst[i]);
	}

	test.reset();

	for ( size_t i = 0; i < dst.size(); ++i )
	{
		ASSERT_EQ(test[i], dst[i]);
	}
}
