/*
 * src/Testing/test_LineFormula.cpp
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

/* Local inclusions. */
#include "Libs/Math/LineFormula.hpp"

using namespace EmEn::Libs::Math;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct Math
	: testing::Test
{
};
TYPED_TEST_SUITE(Math, MathTypeList);

TYPED_TEST(Math, LineFormula)
{
	LineFormula< TypeParam > algorithm{{{static_cast< TypeParam >(0.25), static_cast< TypeParam >(0.33)},
										{static_cast< TypeParam >(0.69), static_cast< TypeParam >(0.95)},
										{static_cast< TypeParam >(1.324), static_cast< TypeParam >(1.964)},
										{static_cast< TypeParam >(1.99), static_cast< TypeParam >(2.01)},
										{static_cast< TypeParam >(3.2), static_cast< TypeParam >(3.151)},
										{static_cast< TypeParam >(3.95), static_cast< TypeParam >(3.9555)},
										{static_cast< TypeParam >(4.225), static_cast< TypeParam >(4.1015)}}};

	ASSERT_TRUE(algorithm.compute());

	ASSERT_NEAR(algorithm.getSlope(), static_cast< TypeParam >(0.904971), static_cast< TypeParam >(0.001));
	ASSERT_NEAR(algorithm.getYIntersect(), static_cast< TypeParam >(0.331172), static_cast< TypeParam >(0.001));
	ASSERT_NEAR(algorithm.getCoefficientDetermination(), static_cast< TypeParam >(0.978823), static_cast< TypeParam >(0.001));
	ASSERT_NEAR(algorithm.getRobustness(), static_cast< TypeParam >(2.7326), static_cast< TypeParam >(0.01));
}
