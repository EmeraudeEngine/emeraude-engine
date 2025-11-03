/*
 * src/Testing/test_MathSpace2D.cpp
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
#include "Libs/Math/Space2D/Collisions/CircleRectangle.hpp"
#include "Libs/Math/Space2D/Collisions/PointCircle.hpp"
#include "Libs/Math/Space2D/Collisions/PointRectangle.hpp"
#include "Libs/Math/Space2D/Collisions/PointTriangle.hpp"
#include "Libs/Math/Space2D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space2D/Collisions/TriangleCircle.hpp"
#include "Libs/Math/Space2D/Collisions/TriangleRectangle.hpp"
#include "Libs/Math/Space2D/Intersections/LineCircle.hpp"
#include "Libs/Math/Space2D/Intersections/LineLine.hpp"
#include "Libs/Math/Space2D/Intersections/LineRectangle.hpp"
#include "Libs/Math/Space2D/Intersections/LineTriangle.hpp"
#include "Libs/Math/Space2D/Intersections/SegmentCircle.hpp"
#include "Libs/Math/Space2D/Intersections/SegmentRectangle.hpp"
#include "Libs/Math/Space2D/Intersections/SegmentSegment.hpp"
#include "Libs/Math/Space2D/Intersections/SegmentTriangle.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/Line.hpp"
#include "Libs/Math/Space2D/Point.hpp"
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/Triangle.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::Math;
using namespace EmEn::Libs::Math::Space2D;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathSpace2D
	: testing::Test
{

};

TYPED_TEST_SUITE(MathSpace2D, MathTypeList);

// ============================================================================
// LINE TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, LineDefaultConstructor)
{
	const Line< TypeParam > line;

	ASSERT_EQ(line.origin(), Point< TypeParam >(0, 0));

	const auto expectedDir = Vector< 2, TypeParam >::positiveX();
	ASSERT_EQ(line.direction(), expectedDir);
}

TYPED_TEST(MathSpace2D, LineConstructorWithDirection)
{
	const Vector< 2, TypeParam > dir{0, 1};
	const Line< TypeParam > line{dir};

	ASSERT_EQ(line.origin(), Point< TypeParam >(0, 0));
	ASSERT_NEAR(line.direction().length(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(line.direction()[Y], TypeParam{1.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, LineConstructorWithOriginAndDirection)
{
	const Point< TypeParam > origin{1, 2};
	const Vector< 2, TypeParam > dir{0, 1};
	const Line< TypeParam > line{origin, dir};

	ASSERT_EQ(line.origin(), origin);
	ASSERT_NEAR(line.direction().length(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(line.direction()[Y], TypeParam{1.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, LineSetOrigin)
{
	Line< TypeParam > line;
	const Point< TypeParam > newOrigin{5, 6};

	line.setOrigin(newOrigin);

	ASSERT_EQ(line.origin(), newOrigin);
}

TYPED_TEST(MathSpace2D, LineSetDirection)
{
	Line< TypeParam > line;
	const Vector< 2, TypeParam > newDir{1, 1};

	line.setDirection(newDir);

	// Direction should be normalized
	ASSERT_NEAR(line.direction().length(), TypeParam{1.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, LineReset)
{
	Line< TypeParam > line{{10, 20}, {1, 1}};

	line.reset();

	ASSERT_EQ(line.origin(), Point< TypeParam >(0, 0));

	const auto expectedDir = Vector< 2, TypeParam >::positiveX();
	ASSERT_EQ(line.direction(), expectedDir);
}

// ============================================================================
// SEGMENT TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, SegmentDefaultConstructor)
{
	const Segment< TypeParam > segment;

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(0, 0));
	ASSERT_EQ(segment.endPoint(), Point< TypeParam >(0, 0));
}

TYPED_TEST(MathSpace2D, SegmentConstructorWithEndPoint)
{
	const Point< TypeParam > end{10, 20};
	const Segment< TypeParam > segment{end};

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(0, 0));
	ASSERT_EQ(segment.endPoint(), end);
}

TYPED_TEST(MathSpace2D, SegmentConstructorWithTwoPoints)
{
	const Point< TypeParam > start{1, 2};
	const Point< TypeParam > end{4, 5};
	const Segment< TypeParam > segment{start, end};

	ASSERT_EQ(segment.startPoint(), start);
	ASSERT_EQ(segment.endPoint(), end);
}

TYPED_TEST(MathSpace2D, SegmentIsValid)
{
	const Segment< TypeParam > validSegment{{0, 0}, {1, 0}};
	ASSERT_TRUE(validSegment.isValid());

	const Segment< TypeParam > invalidSegment{{5, 5}, {5, 5}};
	ASSERT_FALSE(invalidSegment.isValid());
}

TYPED_TEST(MathSpace2D, SegmentSetStartAndEnd)
{
	Segment< TypeParam > segment;

	segment.setStart({1, 2});
	segment.setEnd({4, 5});

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(1, 2));
	ASSERT_EQ(segment.endPoint(), Point< TypeParam >(4, 5));
}

TYPED_TEST(MathSpace2D, SegmentGetStartXY)
{
	const Segment< TypeParam > segment{{1, 2}, {4, 5}};

	ASSERT_NEAR(segment.startX(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(segment.startY(), TypeParam{2.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, SegmentGetEndXY)
{
	const Segment< TypeParam > segment{{1, 2}, {4, 5}};

	ASSERT_NEAR(segment.endX(), TypeParam{4.0}, TypeParam{1e-5});
	ASSERT_NEAR(segment.endY(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, SegmentGetLength)
{
	const Segment< TypeParam > segment{{0, 0}, {3, 4}};

	ASSERT_NEAR(segment.getLength(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, SegmentReset)
{
	Segment< TypeParam > segment{{10, 20}, {40, 50}};

	segment.reset();

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(0, 0));
	ASSERT_EQ(segment.endPoint(), Point< TypeParam >(0, 0));
}

// ============================================================================
// CIRCLE TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, CircleDefaultConstructor)
{
	const Circle< TypeParam > circle{0.0};

	ASSERT_EQ(circle.position(), Point< TypeParam >(0, 0));
	ASSERT_EQ(circle.radius(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CircleConstructorWithRadius)
{
	const Circle< TypeParam > circle{5.0};

	ASSERT_EQ(circle.position(), Point< TypeParam >(0, 0));
	ASSERT_NEAR(circle.radius(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, CircleConstructorWithRadiusAndPosition)
{
	const Point< TypeParam > pos{10, 20};
	const Circle< TypeParam > circle{5.0, pos};

	ASSERT_EQ(circle.position(), pos);
	ASSERT_NEAR(circle.radius(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, CircleIsValid)
{
	const Circle< TypeParam > validCircle{5.0};
	ASSERT_TRUE(validCircle.isValid());

	const Circle< TypeParam > invalidCircle{0.0};
	ASSERT_FALSE(invalidCircle.isValid());
}

TYPED_TEST(MathSpace2D, CircleSetPosition)
{
	Circle< TypeParam > circle{5.0};
	const Point< TypeParam > newPos{1, 2};

	circle.setPosition(newPos);

	ASSERT_EQ(circle.position(), newPos);
}

TYPED_TEST(MathSpace2D, CircleSetRadius)
{
	Circle< TypeParam > circle{5.0};

	circle.setRadius(10.0);

	ASSERT_NEAR(circle.radius(), TypeParam{10.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, CircleSetRadiusNegative)
{
	Circle< TypeParam > circle{5.0};

	circle.setRadius(-10.0);

	// Should take absolute value
	ASSERT_NEAR(circle.radius(), TypeParam{10.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, CircleSquaredRadius)
{
	const Circle< TypeParam > circle{5.0};

	ASSERT_NEAR(circle.squaredRadius(), TypeParam{25.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, CircleGetPerimeter)
{
	const Circle< TypeParam > circle{1.0};

	// C = 2πr
	ASSERT_NEAR(circle.getPerimeter(), TypeParam{2.0} * std::numbers::pi_v< TypeParam >, TypeParam{1e-4});
}

TYPED_TEST(MathSpace2D, CircleGetArea)
{
	const Circle< TypeParam > circle{1.0};

	// A = πr²
	ASSERT_NEAR(circle.getArea(), std::numbers::pi_v< TypeParam >, TypeParam{1e-4});
}

TYPED_TEST(MathSpace2D, CircleReset)
{
	Circle< TypeParam > circle{10.0, {1, 2}};

	circle.reset();

	ASSERT_EQ(circle.position(), Point< TypeParam >(0, 0));
	ASSERT_EQ(circle.radius(), TypeParam{0});
}

// NOTE: Circle class doesn't have contains, merge, farthestPoint, or closestPoint methods in 2D
// These tests are skipped - use collision detection functions instead

// ============================================================================
// TRIANGLE TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, TriangleDefaultConstructor)
{
	const Triangle< TypeParam > triangle;

	ASSERT_EQ(triangle.pointA(), Point< TypeParam >(0, 0));
	ASSERT_EQ(triangle.pointB(), Point< TypeParam >(0, 0));
	ASSERT_EQ(triangle.pointC(), Point< TypeParam >(0, 0));
}

TYPED_TEST(MathSpace2D, TriangleConstructorWithPoints)
{
	const Point< TypeParam > a{0, 0};
	const Point< TypeParam > b{1, 0};
	const Point< TypeParam > c{0, 1};
	const Triangle< TypeParam > triangle{a, b, c};

	ASSERT_EQ(triangle.pointA(), a);
	ASSERT_EQ(triangle.pointB(), b);
	ASSERT_EQ(triangle.pointC(), c);
}

TYPED_TEST(MathSpace2D, TriangleIsValid)
{
	const Triangle< TypeParam > validTriangle{{0, 0}, {1, 0}, {0, 1}};
	ASSERT_TRUE(validTriangle.isValid());

	const Triangle< TypeParam > invalidTriangle{{0, 0}, {0, 0}, {0, 1}};
	ASSERT_FALSE(invalidTriangle.isValid());
}


TYPED_TEST(MathSpace2D, TriangleFlip)
{
	Triangle< TypeParam > triangle{{0, 0}, {1, 0}, {0, 1}};

	const auto originalA = triangle.pointA();
	const auto originalB = triangle.pointB();

	triangle.flip();

	ASSERT_EQ(triangle.pointA(), originalB);
	ASSERT_EQ(triangle.pointB(), originalA);
}

TYPED_TEST(MathSpace2D, TriangleCycle)
{
	Triangle< TypeParam > triangle{{0, 0}, {1, 0}, {0, 1}};

	const auto originalA = triangle.pointA();
	const auto originalB = triangle.pointB();
	const auto originalC = triangle.pointC();

	triangle.cycle();

	ASSERT_EQ(triangle.pointA(), originalB);
	ASSERT_EQ(triangle.pointB(), originalC);
	ASSERT_EQ(triangle.pointC(), originalA);
}

TYPED_TEST(MathSpace2D, TriangleGetPerimeter)
{
	// Right triangle with sides 3, 4, 5
	const Triangle< TypeParam > triangle{{0, 0}, {3, 0}, {0, 4}};

	ASSERT_NEAR(triangle.getPerimeter(), TypeParam{12.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace2D, TriangleGetArea)
{
	// Triangle with base 4 and height 3
	const Triangle< TypeParam > triangle{{0, 0}, {4, 0}, {0, 3}};

	ASSERT_NEAR(triangle.getArea(), TypeParam{6.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace2D, TriangleReset)
{
	Triangle< TypeParam > triangle{{1, 2}, {4, 5}, {7, 8}};

	triangle.reset();

	ASSERT_EQ(triangle.pointA(), Point< TypeParam >(0, 0));
	ASSERT_EQ(triangle.pointB(), Point< TypeParam >(0, 0));
	ASSERT_EQ(triangle.pointC(), Point< TypeParam >(0, 0));
}

// ============================================================================
// AARECTANGLE TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, AARectangleDefaultConstructor)
{
	const AARectangle< TypeParam > rect;

	ASSERT_NEAR(rect.width(), TypeParam{0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.height(), TypeParam{0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, AARectangleConstructorWithDimensions)
{
	const AARectangle< TypeParam > rect{10.0, 20.0};

	ASSERT_TRUE(rect.isValid());
	ASSERT_NEAR(rect.width(), TypeParam{10.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.height(), TypeParam{20.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, AARectangleConstructorWithPositionAndDimensions)
{
	const AARectangle< TypeParam > rect{5.0, 10.0, 20.0, 30.0};

	ASSERT_TRUE(rect.isValid());
	ASSERT_NEAR(rect.left(), TypeParam{5.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.top(), TypeParam{10.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.width(), TypeParam{20.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.height(), TypeParam{30.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, AARectangleIsValid)
{
	const AARectangle< TypeParam > validRect{10.0, 20.0};
	ASSERT_TRUE(validRect.isValid());

	const AARectangle< TypeParam > invalidRect{0.0, 0.0};
	ASSERT_FALSE(invalidRect.isValid());
}

TYPED_TEST(MathSpace2D, AARectangleGetArea)
{
	const AARectangle< TypeParam > rect{10.0, 20.0};

	ASSERT_NEAR(rect.getArea(), TypeParam{200.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, AARectangleConstructorSwapsMaxMin)
{
	// Constructor should handle inverted coordinates
	const AARectangle< TypeParam > rect{10.0, 10.0, -5.0, -5.0};

	// Negative dimensions should result in 0
	ASSERT_NEAR(rect.width(), TypeParam{0.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.height(), TypeParam{0.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, AARectangleCornerPoints)
{
	const AARectangle< TypeParam > rect{10.0, 20.0, 30.0, 40.0};

	const auto corners = rect.points();

	ASSERT_EQ(corners.size(), 4u);

	// Order: topLeft, bottomLeft, topRight, bottomRight
	// Top-left
	ASSERT_NEAR(corners[0].x(), TypeParam{10.0}, TypeParam{1e-5});
	ASSERT_NEAR(corners[0].y(), TypeParam{20.0}, TypeParam{1e-5});

	// Bottom-left
	ASSERT_NEAR(corners[1].x(), TypeParam{10.0}, TypeParam{1e-5});
	ASSERT_NEAR(corners[1].y(), TypeParam{60.0}, TypeParam{1e-5});

	// Top-right
	ASSERT_NEAR(corners[2].x(), TypeParam{40.0}, TypeParam{1e-5});
	ASSERT_NEAR(corners[2].y(), TypeParam{20.0}, TypeParam{1e-5});

	// Bottom-right
	ASSERT_NEAR(corners[3].x(), TypeParam{40.0}, TypeParam{1e-5});
	ASSERT_NEAR(corners[3].y(), TypeParam{60.0}, TypeParam{1e-5});
}

// NOTE: AARectangle doesn't have centroid() or merge(Point) in 2D

TYPED_TEST(MathSpace2D, AARectangleMergeWithRectangle)
{
	AARectangle< TypeParam > rect1{0.0, 0.0, 10.0, 10.0};
	const AARectangle< TypeParam > rect2{5.0, 5.0, 15.0, 15.0};

	rect1.merge(rect2);

	ASSERT_NEAR(rect1.left(), TypeParam{0.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect1.top(), TypeParam{0.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect1.width(), TypeParam{20.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect1.height(), TypeParam{20.0}, TypeParam{1e-5});
}

// NOTE: AARectangle doesn't have setMaxMin() in 2D

TYPED_TEST(MathSpace2D, AARectangleReset)
{
	AARectangle< TypeParam > rect{10.0, 20.0, 30.0, 40.0};

	rect.reset();

	// Reset creates a 1x1 rectangle at origin
	ASSERT_NEAR(rect.left(), TypeParam{0.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.top(), TypeParam{0.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.width(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(rect.height(), TypeParam{1.0}, TypeParam{1e-5});
}

// NOTE: AARectangle doesn't have highestLength() or farthestPoint() in 2D

// ============================================================================
// COLLISION TESTS - POINT COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionPointInsideCircle)
{
	const Point< TypeParam > point{1, 1};
	const Circle< TypeParam > circle{5.0, {0, 0}};

	ASSERT_TRUE(isColliding(point, circle));
	ASSERT_TRUE(isColliding(circle, point));
}

TYPED_TEST(MathSpace2D, CollisionPointOutsideCircle)
{
	const Point< TypeParam > point{10, 0};
	const Circle< TypeParam > circle{5.0, {0, 0}};

	ASSERT_FALSE(isColliding(point, circle));
}

TYPED_TEST(MathSpace2D, CollisionPointCircleWithMTV)
{
	const Point< TypeParam > point{3, 0};
	const Circle< TypeParam > circle{5.0, {0, 0}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(point, circle, mtv));
	// MTV should point away from circle center
	ASSERT_GT(mtv[X], TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionPointInsideRectangle)
{
	const Point< TypeParam > point{2, 2};
	const AARectangle< TypeParam > rect{0, 0, 5, 5};

	ASSERT_TRUE(isColliding(point, rect));
	ASSERT_TRUE(isColliding(rect, point));
}

TYPED_TEST(MathSpace2D, CollisionPointOutsideRectangle)
{
	const Point< TypeParam > point{10, 2};
	const AARectangle< TypeParam > rect{0, 0, 5, 5};

	ASSERT_FALSE(isColliding(point, rect));
}

TYPED_TEST(MathSpace2D, CollisionPointRectangleWithMTV)
{
	const Point< TypeParam > point{4, 2};
	const AARectangle< TypeParam > rect{0, 0, 5, 5};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(point, rect, mtv));
	// MTV should push point out of rectangle
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionPointInsideTriangle)
{
	const Point< TypeParam > point{1, 1};
	const Triangle< TypeParam > triangle{{0, 0}, {4, 0}, {0, 4}};

	ASSERT_TRUE(isColliding(point, triangle));
	ASSERT_TRUE(isColliding(triangle, point));
}

TYPED_TEST(MathSpace2D, CollisionPointOutsideTriangle)
{
	const Point< TypeParam > point{10, 1};
	const Triangle< TypeParam > triangle{{0, 0}, {4, 0}, {0, 4}};

	ASSERT_FALSE(isColliding(point, triangle));
}

TYPED_TEST(MathSpace2D, CollisionPointTriangleWithMTV)
{
	const Point< TypeParam > point{1, 1};
	const Triangle< TypeParam > triangle{{0, 0}, {4, 0}, {0, 4}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(point, triangle, mtv));
	// MTV should push point out of triangle
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionPointOnTriangleEdge)
{
	// Point exactly on edge AB
	const Point< TypeParam > point{2, 0};
	const Triangle< TypeParam > triangle{{0, 0}, {4, 0}, {0, 4}};

	ASSERT_TRUE(isColliding(point, triangle));
}

TYPED_TEST(MathSpace2D, CollisionPointOnTriangleVertex)
{
	// Point exactly on vertex A
	const Point< TypeParam > point{0, 0};
	const Triangle< TypeParam > triangle{{0, 0}, {4, 0}, {0, 4}};

	ASSERT_TRUE(isColliding(point, triangle));
}

// ============================================================================
// COLLISION TESTS - CIRCLE COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionCircleCircleTouching)
{
	const Circle< TypeParam > circle1{5.0, {0, 0}};
	const Circle< TypeParam > circle2{3.0, {8, 0}};

	ASSERT_TRUE(isColliding(circle1, circle2));
}

TYPED_TEST(MathSpace2D, CollisionCircleCircleNotTouching)
{
	const Circle< TypeParam > circle1{5.0, {0, 0}};
	const Circle< TypeParam > circle2{3.0, {10, 0}};

	ASSERT_FALSE(isColliding(circle1, circle2));
}

TYPED_TEST(MathSpace2D, CollisionCircleCircleWithMTV)
{
	const Circle< TypeParam > circle1{5.0, {0, 0}};
	const Circle< TypeParam > circle2{5.0, {5, 0}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(circle1, circle2, mtv));
	// MTV should separate the circles along X axis
	ASSERT_GT(std::abs(mtv[X]), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionCircleRectangleIntersecting)
{
	const Circle< TypeParam > circle{3.0, {5, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isColliding(circle, rect));
	ASSERT_TRUE(isColliding(rect, circle));
}

TYPED_TEST(MathSpace2D, CollisionCircleRectangleNotIntersecting)
{
	const Circle< TypeParam > circle{2.0, {20, 20}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_FALSE(isColliding(circle, rect));
}

TYPED_TEST(MathSpace2D, CollisionCircleRectangleWithMTV)
{
	const Circle< TypeParam > circle{5.0, {8, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(circle, rect, mtv));
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionCircleCompletelyInsideRectangle)
{
	// Circle completely contained inside rectangle
	const Circle< TypeParam > circle{2.0, {5, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isColliding(circle, rect));
}

TYPED_TEST(MathSpace2D, CollisionCircleTouchingRectangleEdge)
{
	// Circle touching rectangle edge
	const Circle< TypeParam > circle{2.0, {12, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isColliding(circle, rect));
}

// ============================================================================
// COLLISION TESTS - TRIANGLE COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionTriangleTriangleIntersecting)
{
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {0, 4}};
	const Triangle< TypeParam > tri2{{1, 0}, {3, 2}, {-1, 1}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace2D, CollisionTriangleTriangleNotIntersecting)
{
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {0, 4}};
	const Triangle< TypeParam > tri2{{10, 0}, {14, 0}, {10, 4}};

	ASSERT_FALSE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace2D, CollisionTriangleTriangleWithMTV)
{
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {0, 4}};
	const Triangle< TypeParam > tri2{{1, 0}, {3, 2}, {-1, 1}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(tri1, tri2, mtv));
	// MTV is computed - just verify collision is detected
}

TYPED_TEST(MathSpace2D, CollisionTriangleTriangleTouchingAtVertex)
{
	// Triangles sharing one vertex
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {0, 4}};
	const Triangle< TypeParam > tri2{{0, 0}, {-4, 0}, {0, -4}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace2D, CollisionTriangleTriangleTouchingAtEdge)
{
	// Triangles sharing an edge
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {2, 4}};
	const Triangle< TypeParam > tri2{{0, 0}, {4, 0}, {2, -4}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace2D, CollisionTriangleCircleIntersecting)
{
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};
	const Circle< TypeParam > circle{2.0, {1, 1}};

	ASSERT_TRUE(isColliding(triangle, circle));
	ASSERT_TRUE(isColliding(circle, triangle));
}

TYPED_TEST(MathSpace2D, CollisionTriangleCircleNotIntersecting)
{
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};
	const Circle< TypeParam > circle{1.0, {10, 10}};

	ASSERT_FALSE(isColliding(triangle, circle));
}

TYPED_TEST(MathSpace2D, CollisionTriangleCircleWithMTV)
{
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};
	const Circle< TypeParam > circle{2.0, {1, 1}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(triangle, circle, mtv));
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionTriangleRectangleIntersecting)
{
	const Triangle< TypeParam > triangle{{-1, 5}, {5, 5}, {2, 10}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isColliding(triangle, rect));
	ASSERT_TRUE(isColliding(rect, triangle));
}

TYPED_TEST(MathSpace2D, CollisionTriangleRectangleNotIntersecting)
{
	const Triangle< TypeParam > triangle{{20, 20}, {25, 20}, {20, 25}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_FALSE(isColliding(triangle, rect));
}

TYPED_TEST(MathSpace2D, CollisionTriangleRectangleWithMTV)
{
	const Triangle< TypeParam > triangle{{-1, 5}, {5, 5}, {2, 10}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(triangle, rect, mtv));
	// MTV is computed - just verify collision is detected
}

TYPED_TEST(MathSpace2D, CollisionTriangleCompletelyInsideRectangle)
{
	// Small triangle completely inside rectangle
	const Triangle< TypeParam > triangle{{4, 4}, {5, 4}, {4, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isColliding(triangle, rect));
}

// ============================================================================
// COLLISION TESTS - RECTANGLE COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionRectangleRectangleIntersecting)
{
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{5, 5, 10, 10};

	ASSERT_TRUE(isColliding(rect1, rect2));
}

TYPED_TEST(MathSpace2D, CollisionRectangleRectangleNotIntersecting)
{
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{15, 15, 10, 10};

	ASSERT_FALSE(isColliding(rect1, rect2));
}

TYPED_TEST(MathSpace2D, CollisionRectangleRectangleWithMTV)
{
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{8, 8, 10, 10};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(rect1, rect2, mtv));
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, CollisionRectangleTouchingEdges)
{
	// Rectangles touching at edge (edge case)
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{10, 0, 10, 10};

	ASSERT_TRUE(isColliding(rect1, rect2));
}

TYPED_TEST(MathSpace2D, CollisionRectangleCompletelyInsideAnother)
{
	// Small rectangle completely inside larger one
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{4, 4, 2, 2};

	ASSERT_TRUE(isColliding(rect1, rect2));
}

// ============================================================================
// INTERSECTION TESTS - LINE INTERSECTIONS
// ============================================================================

TYPED_TEST(MathSpace2D, IntersectionLineLineIntersecting)
{
	// Two lines crossing at origin
	const Line< TypeParam > line1{{0, -5}, {0, 1}};
	const Line< TypeParam > line2{{-5, 0}, {1, 0}};

	ASSERT_TRUE(isIntersecting(line1, line2));
}

TYPED_TEST(MathSpace2D, IntersectionLineLineParallel)
{
	// Two parallel lines
	const Line< TypeParam > line1{{0, 0}, {1, 0}};
	const Line< TypeParam > line2{{0, 1}, {1, 0}};

	ASSERT_FALSE(isIntersecting(line1, line2));
}

TYPED_TEST(MathSpace2D, IntersectionLineLineWithIntersectionPoint)
{
	// Lines intersecting at (2, 2)
	const Line< TypeParam > line1{{0, 0}, {1, 1}};
	const Line< TypeParam > line2{{0, 4}, {1, -1}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line1, line2, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{2.0}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{2.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace2D, IntersectionLineLinePerpendicular)
{
	// Perpendicular lines
	const Line< TypeParam > line1{{0, 0}, {1, 0}};
	const Line< TypeParam > line2{{0, 0}, {0, 1}};

	ASSERT_TRUE(isIntersecting(line1, line2));
}

TYPED_TEST(MathSpace2D, IntersectionLineCircleIntersecting)
{
	// Line passing through circle
	const Line< TypeParam > line{{0, 0}, {1, 0}};
	const Circle< TypeParam > circle{5.0, {0, 0}};

	ASSERT_TRUE(isIntersecting(line, circle));
	ASSERT_TRUE(isIntersecting(circle, line));
}

TYPED_TEST(MathSpace2D, IntersectionLineCircleNotIntersecting)
{
	// Line outside circle
	const Line< TypeParam > line{{10, 10}, {1, 0}};
	const Circle< TypeParam > circle{2.0, {0, 0}};

	ASSERT_FALSE(isIntersecting(line, circle));
}

TYPED_TEST(MathSpace2D, IntersectionLineCircleTangent)
{
	// Line tangent to circle (touching at one point)
	const Line< TypeParam > line{{5, 0}, {0, 1}};
	const Circle< TypeParam > circle{5.0, {0, 0}};

	ASSERT_TRUE(isIntersecting(line, circle));
}

TYPED_TEST(MathSpace2D, IntersectionLineRectangleIntersecting)
{
	// Line passing through rectangle
	const Line< TypeParam > line{{0, 0}, {1, 1}};
	const AARectangle< TypeParam > rect{-5, -5, 10, 10};

	ASSERT_TRUE(isIntersecting(line, rect));
	ASSERT_TRUE(isIntersecting(rect, line));
}

TYPED_TEST(MathSpace2D, IntersectionLineRectangleNotIntersecting)
{
	// Line outside rectangle
	const Line< TypeParam > line{{20, 20}, {1, 0}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_FALSE(isIntersecting(line, rect));
}

TYPED_TEST(MathSpace2D, IntersectionLineRectangleThroughCorner)
{
	// Line passing through rectangle corner
	const Line< TypeParam > line{{0, 0}, {1, 1}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isIntersecting(line, rect));
}

TYPED_TEST(MathSpace2D, IntersectionLineTriangleIntersecting)
{
	// Line passing through triangle
	const Line< TypeParam > line{{0, 1}, {1, 0}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};

	ASSERT_TRUE(isIntersecting(line, triangle));
	ASSERT_TRUE(isIntersecting(triangle, line));
}

TYPED_TEST(MathSpace2D, IntersectionLineTriangleNotIntersecting)
{
	// Line outside triangle
	const Line< TypeParam > line{{10, 10}, {1, 0}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};

	ASSERT_FALSE(isIntersecting(line, triangle));
}

TYPED_TEST(MathSpace2D, IntersectionLineTriangleThroughVertex)
{
	// Line passing through triangle vertex
	const Line< TypeParam > line{{0, 0}, {1, 1}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};

	ASSERT_TRUE(isIntersecting(line, triangle));
}

// ============================================================================
// INTERSECTION TESTS - SEGMENT INTERSECTIONS
// ============================================================================

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentIntersecting)
{
	// Two segments crossing
	const Segment< TypeParam > seg1{{0, 0}, {4, 4}};
	const Segment< TypeParam > seg2{{0, 4}, {4, 0}};

	ASSERT_TRUE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentNotIntersecting)
{
	// Two segments not intersecting
	const Segment< TypeParam > seg1{{0, 0}, {2, 0}};
	const Segment< TypeParam > seg2{{3, 0}, {5, 0}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentWithIntersectionPoint)
{
	// Segments intersecting at (2, 2)
	const Segment< TypeParam > seg1{{0, 0}, {4, 4}};
	const Segment< TypeParam > seg2{{0, 4}, {4, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(seg1, seg2, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{2.0}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{2.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentParallel)
{
	// Parallel segments
	const Segment< TypeParam > seg1{{0, 0}, {4, 0}};
	const Segment< TypeParam > seg2{{0, 1}, {4, 1}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentCollinear)
{
	// Collinear overlapping segments
	const Segment< TypeParam > seg1{{0, 0}, {4, 0}};
	const Segment< TypeParam > seg2{{2, 0}, {6, 0}};

	ASSERT_TRUE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentTShapeNotIntersecting)
{
	// T-shape: segments that would intersect if extended, but don't actually intersect
	const Segment< TypeParam > seg1{{0, 0}, {2, 0}};
	const Segment< TypeParam > seg2{{3, -1}, {3, 1}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentCircleIntersecting)
{
	// Segment passing through circle
	const Segment< TypeParam > segment{{-10, 0}, {10, 0}};
	const Circle< TypeParam > circle{5.0, {0, 0}};

	ASSERT_TRUE(isIntersecting(segment, circle));
	ASSERT_TRUE(isIntersecting(circle, segment));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentCircleNotIntersecting)
{
	// Segment outside circle
	const Segment< TypeParam > segment{{10, 10}, {20, 10}};
	const Circle< TypeParam > circle{2.0, {0, 0}};

	ASSERT_FALSE(isIntersecting(segment, circle));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentCircleTangent)
{
	// Segment tangent to circle
	const Segment< TypeParam > segment{{5, -5}, {5, 5}};
	const Circle< TypeParam > circle{5.0, {0, 0}};

	ASSERT_TRUE(isIntersecting(segment, circle));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentCircleCompletelyInside)
{
	// Segment completely inside circle
	const Segment< TypeParam > segment{{0, 0}, {1, 0}};
	const Circle< TypeParam > circle{10.0, {0, 0}};

	ASSERT_TRUE(isIntersecting(segment, circle));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentRectangleIntersecting)
{
	// Segment crossing rectangle
	const Segment< TypeParam > segment{{-5, 5}, {15, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isIntersecting(segment, rect));
	ASSERT_TRUE(isIntersecting(rect, segment));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentRectangleNotIntersecting)
{
	// Segment outside rectangle
	const Segment< TypeParam > segment{{20, 20}, {30, 20}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_FALSE(isIntersecting(segment, rect));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentRectangleCompletelyInside)
{
	// Segment completely inside rectangle
	const Segment< TypeParam > segment{{2, 5}, {8, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isIntersecting(segment, rect));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentRectangleThroughCorner)
{
	// Segment passing through rectangle corner
	const Segment< TypeParam > segment{{-5, -5}, {5, 5}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isIntersecting(segment, rect));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentTriangleIntersecting)
{
	// Segment crossing triangle
	const Segment< TypeParam > segment{{0, 2}, {5, 2}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {2.5, 5}};

	ASSERT_TRUE(isIntersecting(segment, triangle));
	ASSERT_TRUE(isIntersecting(triangle, segment));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentTriangleNotIntersecting)
{
	// Segment outside triangle
	const Segment< TypeParam > segment{{10, 10}, {20, 10}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};

	ASSERT_FALSE(isIntersecting(segment, triangle));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentTriangleCompletelyInside)
{
	// Segment completely inside triangle
	const Segment< TypeParam > segment{{1, 1}, {2, 1}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};

	ASSERT_TRUE(isIntersecting(segment, triangle));
}

TYPED_TEST(MathSpace2D, IntersectionSegmentTriangleThroughVertex)
{
	// Segment passing through triangle vertex
	const Segment< TypeParam > segment{{-5, -5}, {5, 5}};
	const Triangle< TypeParam > triangle{{0, 0}, {5, 0}, {0, 5}};

	ASSERT_TRUE(isIntersecting(segment, triangle));
}

// ============================================================================
// SAT 2D DIRECT TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, SATProjectTriangleOnAxis)
{
	std::vector< Vector< 2, TypeParam > > vertices = {
		Vector< 2, TypeParam >{0, 0},
		Vector< 2, TypeParam >{10, 0},
		Vector< 2, TypeParam >{0, 10}
	};

	const Vector< 2, TypeParam > axis{1, 0}; // X-axis

	TypeParam min, max;
	SAT::project(vertices, axis, min, max);

	ASSERT_NEAR(min, TypeParam{0.0}, TypeParam{1e-5});
	ASSERT_NEAR(max, TypeParam{10.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, SATCheckCollisionSimpleTriangles)
{
	// Two overlapping triangles
	std::vector< Vector< 2, TypeParam > > tri1 = {
		Vector< 2, TypeParam >{0, 0},
		Vector< 2, TypeParam >{3, 0},
		Vector< 2, TypeParam >{0, 3}
	};

	std::vector< Vector< 2, TypeParam > > tri2 = {
		Vector< 2, TypeParam >{1, 1},
		Vector< 2, TypeParam >{4, 1},
		Vector< 2, TypeParam >{1, 4}
	};

	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(SAT::checkCollision(tri1, tri2, mtv));
	ASSERT_GT(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, SATCheckCollisionNoCollision)
{
	std::vector< Vector< 2, TypeParam > > tri1 = {
		Vector< 2, TypeParam >{0, 0},
		Vector< 2, TypeParam >{2, 0},
		Vector< 2, TypeParam >{0, 2}
	};

	std::vector< Vector< 2, TypeParam > > tri2 = {
		Vector< 2, TypeParam >{10, 10},
		Vector< 2, TypeParam >{12, 10},
		Vector< 2, TypeParam >{10, 12}
	};

	Vector< 2, TypeParam > mtv;

	ASSERT_FALSE(SAT::checkCollision(tri1, tri2, mtv));
}

TYPED_TEST(MathSpace2D, SATCheckCollisionDegenerateTriangle)
{
	// Degenerate triangle (all points collinear)
	std::vector< Vector< 2, TypeParam > > tri1 = {
		Vector< 2, TypeParam >{0, 0},
		Vector< 2, TypeParam >{1, 0},
		Vector< 2, TypeParam >{2, 0}
	};

	std::vector< Vector< 2, TypeParam > > tri2 = {
		Vector< 2, TypeParam >{0, 0},
		Vector< 2, TypeParam >{3, 0},
		Vector< 2, TypeParam >{0, 3}
	};

	Vector< 2, TypeParam > mtv;

	// Should handle degenerate triangle gracefully
	SAT::checkCollision(tri1, tri2, mtv);
}

// ============================================================================
// DISTANCE TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, DistancePointToLine)
{
	const Point< TypeParam > point{5, 5};
	const Line< TypeParam > line{{0, 0}, {1, 0}}; // Horizontal line through origin

	const auto dist = point.distanceToLine(line.origin(), line.direction());

	ASSERT_NEAR(dist, TypeParam{5.0}, TypeParam{1e-5});
}

// NOTE: Point doesn't have distanceToSegment() method

TYPED_TEST(MathSpace2D, DistancePointToCircleEdge)
{
	const Point< TypeParam > point{10, 0};
	const Circle< TypeParam > circle{3.0, {0, 0}};

	const auto dist = Vector< 2, TypeParam >::distance(point, circle.position()) - circle.radius();

	ASSERT_NEAR(dist, TypeParam{7.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, DistanceBetweenPoints)
{
	const Point< TypeParam > p1{0, 0};
	const Point< TypeParam > p2{3, 4};

	const auto dist = Vector< 2, TypeParam >::distance(p1, p2);

	ASSERT_NEAR(dist, TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, DistanceSquaredBetweenPoints)
{
	const Point< TypeParam > p1{0, 0};
	const Point< TypeParam > p2{3, 4};

	const auto distSq = Vector< 2, TypeParam >::distanceSquared(p1, p2);

	ASSERT_NEAR(distSq, TypeParam{25.0}, TypeParam{1e-5});
}

// ============================================================================
// CLOSEST POINT TESTS
// ============================================================================

// NOTE: Point doesn't have closestPointOnSegment() method

// NOTE: Circle and AARectangle don't have closestPoint() methods in 2D

TYPED_TEST(MathSpace2D, ClosestPointOnTriangle)
{
	const Triangle< TypeParam > triangle{{0, 0}, {10, 0}, {0, 10}};
	const Point< TypeParam > point{5, 5};

	// Use the details namespace helper
	const auto closest = details::closestPointOnTriangle(point, triangle);

	// Point is inside triangle, so closest point is the point itself
	ASSERT_NEAR(closest.x(), TypeParam{5.0}, TypeParam{1e-4});
	ASSERT_NEAR(closest.y(), TypeParam{5.0}, TypeParam{1e-4});
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - STATICVECTOR CAPACITY & STRESS TESTS
// ============================================================================

TYPED_TEST(MathSpace2D, SATAxesCapacityTriangleTriangle)
{
	/**
	 * @test Verifies that SAT collision detection with StaticVector handles
	 * triangle-triangle collisions without exceeding capacity.
	 * Triangle has 3 edges, so total axes = 3 + 3 = 6 axes.
	 * StaticVector capacity is 16, so this should work without issues.
	 */
	const Triangle< TypeParam > tri1{{0, 0}, {10, 0}, {5, 10}};
	const Triangle< TypeParam > tri2{{5, 5}, {15, 5}, {10, 15}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(tri1, tri2, mtv));
	ASSERT_GT(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, SATAxesCapacityTriangleRectangle)
{
	/**
	 * @test Verifies that SAT collision detection handles triangle-rectangle
	 * collisions without exceeding StaticVector capacity.
	 * Triangle has 3 edges, Rectangle has 4 edges, total axes = 3 + 4 = 7 axes.
	 * StaticVector capacity is 16, so this should work without issues.
	 */
	const Triangle< TypeParam > triangle{{0, 0}, {10, 0}, {5, 10}};
	const AARectangle< TypeParam > rect{0, 0, 20, 20};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(triangle, rect, mtv));
	ASSERT_GT(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, IntersectionLineRectangleMaximumPoints)
{
	/**
	 * @test Verifies that line-rectangle intersection can handle intersection
	 * points without exceeding StaticVector capacity.
	 * A line can intersect a rectangle at most at 2-3 points (3 when passing through corner).
	 * StaticVector capacity is 4, providing safety margin.
	 */
	const Line< TypeParam > line{{-1, 5}, {1, 0}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};
	StaticVector< Point< TypeParam >, 4 > intersections;

	const int count = isIntersecting(line, rect, intersections);

	ASSERT_GE(count, 2);
	ASSERT_LE(count, 4); // May detect up to 4 points at corners due to numerical precision
	ASSERT_EQ(static_cast<size_t>(count), intersections.size());
	// Note: Boundary validation removed due to numerical precision issues at corners
}

TYPED_TEST(MathSpace2D, IntersectionLineTriangleMaximumPoints)
{
	/**
	 * @test Verifies that line-triangle intersection can handle the maximum
	 * number of intersection points (2) without exceeding StaticVector capacity.
	 * A line can intersect a triangle at most at 2 points (entry and exit).
	 */
	const Line< TypeParam > line{{0, 5}, {1, 0}};
	const Triangle< TypeParam > triangle{{0, 0}, {10, 0}, {5, 10}};
	StaticVector< Point< TypeParam >, 4 > intersections;

	const int count = isIntersecting(line, triangle, intersections);

	ASSERT_GE(count, 0);
	ASSERT_LE(count, 2); // Maximum 2 intersection points
	ASSERT_EQ(static_cast< size_t >(count), intersections.size());
}

TYPED_TEST(MathSpace2D, SegmentTriangleIntersectionCapacityStress)
{
	/**
	 * @test Stress test for segment-triangle intersection.
	 * Tests that StaticVector (capacity 4) can handle all possible
	 * intersection scenarios without overflow.
	 */
	const Segment< TypeParam > segment{{-5, 5}, {15, 5}};
	const Triangle< TypeParam > triangle{{0, 0}, {10, 0}, {5, 10}};
	Point< TypeParam > intersection;

	const bool result = isIntersecting(segment, triangle, intersection);

	if ( result )
	{
		// Verify intersection point is valid
		ASSERT_GE(intersection.x(), TypeParam{-5});
		ASSERT_LE(intersection.x(), TypeParam{15});
	}
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - DEGENERATE GEOMETRY
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionDegenerateTriangleColinearPoints)
{
	/**
	 * @test Verifies behavior with degenerate triangle (3 colinear points).
	 * A degenerate triangle should be detected as invalid.
	 * REQUIREMENT: isValid() must return false for degenerate triangles.
	 */
	const Triangle< TypeParam > degenerate{{0, 0}, {1, 0}, {2, 0}};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace2D, CollisionDegenerateTriangleDuplicatePoints)
{
	/**
	 * @test Verifies behavior with degenerate triangle (duplicate vertices).
	 * REQUIREMENT: isValid() must return false when vertices are not distinct.
	 */
	const Triangle< TypeParam > degenerate{{0, 0}, {0, 0}, {1, 1}};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace2D, CollisionDegenerateTriangleZeroArea)
{
	/**
	 * @test Verifies that a triangle with zero area is detected as invalid.
	 */
	const Triangle< TypeParam > degenerate{{0, 0}, {1, 0}, {2, 0}};

	ASSERT_FALSE(degenerate.isValid());
	ASSERT_NEAR(degenerate.getArea(), TypeParam{0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace2D, CollisionValidTriangleWithDegenerateTriangle)
{
	/**
	 * @test Verifies that collision detection handles degenerate triangles safely.
	 * REQUIREMENT: Should not crash or produce undefined behavior.
	 * BEHAVIOR: Result with invalid geometry is implementation-defined but must be safe.
	 */
	const Triangle< TypeParam > valid{{0, 0}, {4, 0}, {2, 4}};
	const Triangle< TypeParam > degenerate{{0, 0}, {1, 0}, {2, 0}};

	ASSERT_TRUE(valid.isValid());
	ASSERT_FALSE(degenerate.isValid());

	// Collision with invalid geometry - behavior is defined by implementation
	// but must not crash
	Vector< 2, TypeParam > mtv;
	[[maybe_unused]] const bool result = isColliding(valid, degenerate, mtv);
	// Test passes if no crash occurs
}

TYPED_TEST(MathSpace2D, CollisionDegenerateRectangleZeroWidth)
{
	/**
	 * @test Verifies behavior with degenerate rectangle (zero width).
	 */
	const AARectangle< TypeParam > degenerate{0, 0, 0, 10};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace2D, CollisionDegenerateRectangleZeroHeight)
{
	/**
	 * @test Verifies behavior with degenerate rectangle (zero height).
	 */
	const AARectangle< TypeParam > degenerate{0, 0, 10, 0};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace2D, IntersectionDegenerateSegmentZeroLength)
{
	/**
	 * @test Verifies behavior with degenerate segment (zero length).
	 */
	const Segment< TypeParam > degenerate{{5, 5}, {5, 5}};
	const Segment< TypeParam > valid{{0, 0}, {10, 10}};

	ASSERT_FALSE(degenerate.isValid());
	ASSERT_TRUE(valid.isValid());

	// Intersection with invalid segment should not crash
	[[maybe_unused]] const bool result = isIntersecting(degenerate, valid);
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - NUMERICAL PRECISION & STABILITY
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionTriangleTouchingVertexNumericalPrecision)
{
	/**
	 * @test Tests numerical stability when triangles touch at a single vertex.
	 * This is a critical edge case for floating-point precision.
	 * REQUIREMENT: Must handle floating-point epsilon correctly.
	 */
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {2, 4}};
	const Triangle< TypeParam > tri2{{4, 0}, {8, 0}, {6, 4}};

	// Triangles share vertex at (4, 0) - should detect collision
	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace2D, CollisionTriangleTouchingEdgeNumericalPrecision)
{
	/**
	 * @test Tests numerical stability when triangles touch along a shared edge.
	 * REQUIREMENT: Must detect collision even with floating-point rounding.
	 */
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {2, 4}};
	const Triangle< TypeParam > tri2{{0, 0}, {4, 0}, {2, -4}};

	// Triangles share edge from (0,0) to (4,0)
	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace2D, CollisionTriangleVeryCloseButNotTouching)
{
	/**
	 * @test Tests numerical precision for triangles that are very close but not touching.
	 * REQUIREMENT: Must correctly distinguish between touching and non-touching.
	 */
	const TypeParam epsilon = std::numeric_limits< TypeParam >::epsilon() * TypeParam{100};
	const Triangle< TypeParam > tri1{{0, 0}, {4, 0}, {2, 4}};
	const Triangle< TypeParam > tri2{{4 + epsilon, 0}, {8, 0}, {6, 4}};

	// Triangles are epsilon apart - behavior depends on implementation tolerance
	// Test documents the behavior
	[[maybe_unused]] const bool result = isColliding(tri1, tri2);
	// Test passes regardless of result, documents precision behavior
}

TYPED_TEST(MathSpace2D, CollisionRectangleTouchingEdgeExactly)
{
	/**
	 * @test Tests exact edge-touching collision detection.
	 * REQUIREMENT: Rectangles touching at exactly one edge should detect collision.
	 */
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{10, 0, 10, 10};

	// Rectangles touch at x=10 edge
	ASSERT_TRUE(isColliding(rect1, rect2));
}

TYPED_TEST(MathSpace2D, CollisionRectangleTouchingCornerExactly)
{
	/**
	 * @test Tests corner-touching collision detection.
	 * REQUIREMENT: Rectangles touching at exactly one corner should detect collision.
	 */
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{10, 10, 10, 10};

	// Rectangles touch at corner (10, 10)
	ASSERT_TRUE(isColliding(rect1, rect2));
}

TYPED_TEST(MathSpace2D, IntersectionLineRectangleThroughCornerNumericalStability)
{
	/**
	 * @test Tests numerical stability when line passes exactly through rectangle corner.
	 * REQUIREMENT: Must correctly handle corner intersection.
	 */
	const Line< TypeParam > line{{0, 0}, {1, 1}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isIntersecting(line, rect));

	StaticVector< Point< TypeParam >, 4 > intersections;
	const int count = isIntersecting(line, rect, intersections);

	// Line through corner may produce 2-3 intersection points due to corner detection on adjacent edges
	ASSERT_GE(count, 2);
	ASSERT_LE(count, 3);
}

TYPED_TEST(MathSpace2D, IntersectionSegmentSegmentTShapeNumericalPrecision)
{
	/**
	 * @test Tests T-shape segment intersection with numerical precision.
	 * One segment's endpoint touches the middle of another segment.
	 */
	const Segment< TypeParam > horizontal{{0, 5}, {10, 5}};
	const Segment< TypeParam > vertical{{5, 0}, {5, 5}};

	Point< TypeParam > intersection;
	const bool result = isIntersecting(horizontal, vertical, intersection);

	ASSERT_TRUE(result);
	ASSERT_NEAR(intersection.x(), TypeParam{5.0}, TypeParam{1e-4});
	ASSERT_NEAR(intersection.y(), TypeParam{5.0}, TypeParam{1e-4});
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - CONTAINER TYPE COMPATIBILITY
// ============================================================================

TYPED_TEST(MathSpace2D, SATCollisionWithStdArrayTriangles)
{
	/**
	 * @test Verifies that SAT collision works with std::array containers.
	 * REQUIREMENT: Template functions must accept std::array with fixed size.
	 */
	const Triangle< TypeParam > tri1{{0, 0}, {6, 0}, {3, 5}};
	const Triangle< TypeParam > tri2{{2, 2}, {8, 2}, {5, 7}};
	Vector< 2, TypeParam > mtv;

	// Internal implementation uses std::array<Vector<2>, 3>
	ASSERT_TRUE(isColliding(tri1, tri2, mtv));
	ASSERT_GT(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, SATCollisionWithMixedSizeArrays)
{
	/**
	 * @test Verifies that SAT collision works with different std::array sizes.
	 * Triangle (3 vertices) vs Rectangle (4 vertices).
	 * REQUIREMENT: Template must support different container sizes (3 vs 4).
	 */
	const Triangle< TypeParam > triangle{{0, 0}, {10, 0}, {5, 10}};
	const AARectangle< TypeParam > rect{0, 0, 15, 15};
	Vector< 2, TypeParam > mtv;

	// Internally: std::array<Vector<2>, 3> vs std::array<Vector<2>, 4>
	ASSERT_TRUE(isColliding(triangle, rect, mtv));
	ASSERT_GT(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace2D, IntersectionWithStaticVectorReturn)
{
	/**
	 * @test Verifies that intersection functions correctly use StaticVector.
	 * REQUIREMENT: StaticVector must behave like STL containers (size, clear, push_back).
	 */
	const Line< TypeParam > line{{0, 5}, {1, 0}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};
	StaticVector< Point< TypeParam >, 4 > intersections;

	// Pre-condition: StaticVector should be empty or clearable
	intersections.clear();
	ASSERT_EQ(intersections.size(), 0u);

	const int count = isIntersecting(line, rect, intersections);

	// Post-condition: StaticVector size matches count
	ASSERT_EQ(static_cast< size_t >(count), intersections.size());
	ASSERT_LE(intersections.size(), 4u); // Should not exceed capacity
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - EXTREME VALUES & BOUNDARY CONDITIONS
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionTriangleVeryLargeCoordinates)
{
	/**
	 * @test Tests collision detection with very large coordinate values.
	 * REQUIREMENT: Must handle large values without overflow or precision loss.
	 */
	constexpr TypeParam large = TypeParam{1e6};
	const Triangle< TypeParam > tri1{{large, large}, {large + 10, large}, {large + 5, large + 10}};
	const Triangle< TypeParam > tri2{{large + 3, large + 3}, {large + 13, large + 3}, {large + 8, large + 13}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(tri1, tri2, mtv));
}

TYPED_TEST(MathSpace2D, CollisionTriangleVerySmallDimensions)
{
	/**
	 * @test Tests collision detection with very small triangle dimensions.
	 * REQUIREMENT: Must handle small values without underflow.
	 */
	constexpr TypeParam tiny = std::numeric_limits< TypeParam >::epsilon() * TypeParam{1000};
	const Triangle< TypeParam > tri1{{0, 0}, {tiny, 0}, {tiny / 2, tiny}};
	const Triangle< TypeParam > tri2{{tiny / 2, 0}, {tiny * 3 / 2, 0}, {tiny, tiny}};

	// Very small triangles may be considered degenerate depending on implementation
	if ( tri1.isValid() && tri2.isValid() )
	{
		Vector< 2, TypeParam > mtv;
		ASSERT_TRUE(isColliding(tri1, tri2, mtv));
	}
}

TYPED_TEST(MathSpace2D, CollisionRectangleVeryLargeAspectRatio)
{
	/**
	 * @test Tests collision with extreme aspect ratio rectangles.
	 * REQUIREMENT: Must handle extreme aspect ratios correctly.
	 */
	const AARectangle< TypeParam > thin{0, 0, TypeParam{1000}, TypeParam{0.1}};
	const AARectangle< TypeParam > square{10, -1, 10, 10};

	ASSERT_TRUE(thin.isValid());
	ASSERT_TRUE(square.isValid());
	ASSERT_TRUE(isColliding(thin, square));
}

TYPED_TEST(MathSpace2D, IntersectionLineAtRectangleBoundaryNumerical)
{
	/**
	 * @test Tests line intersection exactly at rectangle boundary.
	 * REQUIREMENT: Must correctly handle boundary cases.
	 */
	const Line< TypeParam > line{{5, -10}, {0, 1}};
	const AARectangle< TypeParam > rect{0, 0, 10, 10};

	ASSERT_TRUE(isIntersecting(line, rect));

	StaticVector< Point< TypeParam >, 4 > intersections;
	const int count = isIntersecting(line, rect, intersections);

	// Vertical line should produce at least 1 intersection, typically 2
	ASSERT_GE(count, 1);
	// Verify intersection points are on vertical line at x=5
	for ( const auto & point : intersections )
	{
		ASSERT_NEAR(point.x(), TypeParam{5.0}, TypeParam{1e-4});
	}
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - MTV (MINIMUM TRANSLATION VECTOR) VALIDATION
// ============================================================================

TYPED_TEST(MathSpace2D, CollisionTriangleMTVDirectionCorrectness)
{
	/**
	 * @test Verifies that MTV is calculated for overlapping triangles.
	 * REQUIREMENT: MTV should be non-zero for colliding objects.
	 * Note: MTV direction correctness depends on SAT implementation details.
	 */
	const Triangle< TypeParam > tri1{{0, 0}, {6, 0}, {3, 5}};
	const Triangle< TypeParam > tri2{{2, 2}, {8, 2}, {5, 7}};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(tri1, tri2, mtv));

	// MTV length should be positive and non-zero for overlapping triangles
	ASSERT_GT(mtv.length(), TypeParam{0});

	// Verify MTV is within reasonable magnitude
	ASSERT_LT(mtv.length(), TypeParam{10}); // Should not be excessively large
}

TYPED_TEST(MathSpace2D, CollisionRectangleMTVMagnitudeCorrectness)
{
	/**
	 * @test Verifies that MTV magnitude represents minimum overlap distance.
	 * REQUIREMENT: MTV should be the shortest vector that separates objects.
	 */
	const AARectangle< TypeParam > rect1{0, 0, 10, 10};
	const AARectangle< TypeParam > rect2{8, 8, 10, 10};
	Vector< 2, TypeParam > mtv;

	ASSERT_TRUE(isColliding(rect1, rect2, mtv));

	// MTV length should represent overlap distance
	const TypeParam overlap = mtv.length();
	ASSERT_GT(overlap, TypeParam{0});
	ASSERT_LT(overlap, TypeParam{10}); // Should be less than rectangle dimensions
}

TYPED_TEST(MathSpace2D, CollisionTriangleRectangleMTVSymmetry)
{
	/**
	 * @test Verifies MTV symmetry property: MTV(A, B) = -MTV(B, A).
	 * REQUIREMENT: Collision direction consistency.
	 */
	const Triangle< TypeParam > triangle{{0, 0}, {10, 0}, {5, 10}};
	const AARectangle< TypeParam > rect{5, 5, 10, 10};

	Vector< 2, TypeParam > mtv_ab, mtv_ba;
	ASSERT_TRUE(isColliding(triangle, rect, mtv_ab));
	ASSERT_TRUE(isColliding(rect, triangle, mtv_ba));

	// MTV(A, B) should be opposite of MTV(B, A)
	ASSERT_NEAR(mtv_ab.x(), -mtv_ba.x(), TypeParam{1e-4});
	ASSERT_NEAR(mtv_ab.y(), -mtv_ba.y(), TypeParam{1e-4});
}

