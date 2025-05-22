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

/* Local inclusions. */
#include "Libs/Math/Space2D/Point.hpp"
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/Line.hpp"
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"

using namespace EmEn::Libs::Math::Space2D;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathSpace2D
	: testing::Test
{

};

TYPED_TEST_SUITE(MathSpace2D, MathTypeList);

TYPED_TEST(MathSpace2D, Point)
{
	const Point< TypeParam > pointA{2, 4};
	const Point< TypeParam > pointB{4, 8};

	const auto midPoint = Point< TypeParam >::midPoint(pointA, pointB);

	ASSERT_EQ(midPoint.x(), 1);
	ASSERT_EQ(midPoint.y(), 2);
}

TYPED_TEST(MathSpace2D, IntersectionSegmentTriangle)
{
	const Point< TypeParam > pointA{2, 4};
	const Point< TypeParam > pointB{4, 8};

	const auto midPoint = Point< TypeParam >::midPoint(pointA, pointB);

	ASSERT_EQ(midPoint.x(), 1);
	ASSERT_EQ(midPoint.y(), 2);
}

TYPED_TEST(MathSpace2D, IntersectionSegmentCircle)
{
	const Point< TypeParam > pointA{2, 4};
	const Point< TypeParam > pointB{4, 8};

	const auto midPoint = Point< TypeParam >::midPoint(pointA, pointB);

	ASSERT_EQ(midPoint.x(), 1);
	ASSERT_EQ(midPoint.y(), 2);
}
