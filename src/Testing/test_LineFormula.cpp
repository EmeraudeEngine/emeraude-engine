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

using MathTypeList = testing::Types< int, float, double >;

template< typename >
struct Math
	: testing::Test
{

};
TYPED_TEST_SUITE(Math, MathTypeList);

TYPED_TEST(Math, LineFormula)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		LineFormula< TypeParam > algorithm{{
			{0.25, 0.33},
			{0.69, 0.95},
			{1.324, 1.964},
			{1.99, 2.01},
			{3.2, 3.151},
			{3.95, 3.9555},
			{4.225, 4.1015}
		}};

		ASSERT_EQ(algorithm.compute(), true);

		ASSERT_NEAR(algorithm.getSlope(), 0.984299, 0.001);

		ASSERT_NEAR(algorithm.getCoefficientDetermination(), 0.980484, 0.001);

		ASSERT_NEAR(algorithm.getYIntersect(), 0.0157008, 0.001);

		ASSERT_NEAR(algorithm.getRobustness(), 62.6912, 0.001);
	}
}
