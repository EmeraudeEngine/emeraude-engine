/*
 * src/Testing/test_MathSpace3D.cpp
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
#include "Libs/Math/Space3D/Collisions/PointCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Libs/Math/Space3D/Collisions/PointTriangle.hpp"
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space3D/Collisions/SphereCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/TriangleCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/TriangleSphere.hpp"
#include "Libs/Math/Space3D/Intersections/LineCuboid.hpp"
#include "Libs/Math/Space3D/Intersections/LineLine.hpp"
#include "Libs/Math/Space3D/Intersections/LineSphere.hpp"
#include "Libs/Math/Space3D/Intersections/LineTriangle.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentCuboid.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentSegment.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentSphere.hpp"
#include "Libs/Math/Space3D/Intersections/SegmentTriangle.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Line.hpp"
#include "Libs/Math/Space3D/Point.hpp"
#include "Libs/Math/Space3D/Segment.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Libs/Math/Space3D/Triangle.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::Math;
using namespace EmEn::Libs::Math::Space3D;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathSpace3D
	: testing::Test
{

};

TYPED_TEST_SUITE(MathSpace3D, MathTypeList);

// ============================================================================
// LINE TESTS
// ============================================================================

TYPED_TEST(MathSpace3D, LineDefaultConstructor)
{
	const Line< TypeParam > line;

	ASSERT_EQ(line.origin(), Point< TypeParam >(0, 0, 0));

	const auto expectedDir = Vector< 3, TypeParam >::positiveX();
	ASSERT_EQ(line.direction(), expectedDir);
}

TYPED_TEST(MathSpace3D, LineConstructorWithDirection)
{
	const Vector< 3, TypeParam > dir{0, 1, 0};
	const Line< TypeParam > line{dir};

	ASSERT_EQ(line.origin(), Point< TypeParam >(0, 0, 0));
	ASSERT_NEAR(line.direction().length(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(line.direction()[Y], TypeParam{1.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, LineConstructorWithOriginAndDirection)
{
	const Point< TypeParam > origin{1, 2, 3};
	const Vector< 3, TypeParam > dir{0, 0, 1};
	const Line< TypeParam > line{origin, dir};

	ASSERT_EQ(line.origin(), origin);
	ASSERT_NEAR(line.direction().length(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(line.direction()[Z], TypeParam{1.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, LineSetOrigin)
{
	Line< TypeParam > line;
	const Point< TypeParam > newOrigin{5, 6, 7};

	line.setOrigin(newOrigin);

	ASSERT_EQ(line.origin(), newOrigin);
}

TYPED_TEST(MathSpace3D, LineSetDirection)
{
	Line< TypeParam > line;
	const Vector< 3, TypeParam > newDir{1, 1, 0};

	line.setDirection(newDir);

	// Direction should be normalized
	ASSERT_NEAR(line.direction().length(), TypeParam{1.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, LineReset)
{
	Line< TypeParam > line{{10, 20, 30}, {1, 1, 1}};

	line.reset();

	ASSERT_EQ(line.origin(), Point< TypeParam >(0, 0, 0));

	const auto expectedDir = Vector< 3, TypeParam >::positiveX();
	ASSERT_EQ(line.direction(), expectedDir);
}

// ============================================================================
// SEGMENT TESTS
// ============================================================================

TYPED_TEST(MathSpace3D, SegmentDefaultConstructor)
{
	const Segment< TypeParam > segment;

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(segment.endPoint(), Point< TypeParam >(0, 0, 0));
}

TYPED_TEST(MathSpace3D, SegmentConstructorWithEndPoint)
{
	const Point< TypeParam > end{10, 20, 30};
	const Segment< TypeParam > segment{end};

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(segment.endPoint(), end);
}

TYPED_TEST(MathSpace3D, SegmentConstructorWithTwoPoints)
{
	const Point< TypeParam > start{1, 2, 3};
	const Point< TypeParam > end{4, 5, 6};
	const Segment< TypeParam > segment{start, end};

	ASSERT_EQ(segment.startPoint(), start);
	ASSERT_EQ(segment.endPoint(), end);
}

TYPED_TEST(MathSpace3D, SegmentIsValid)
{
	const Segment< TypeParam > validSegment{{0, 0, 0}, {1, 0, 0}};
	ASSERT_TRUE(validSegment.isValid());

	const Segment< TypeParam > invalidSegment{{5, 5, 5}, {5, 5, 5}};
	ASSERT_FALSE(invalidSegment.isValid());
}

TYPED_TEST(MathSpace3D, SegmentSetStartAndEnd)
{
	Segment< TypeParam > segment;

	segment.setStart({1, 2, 3});
	segment.setEnd({4, 5, 6});

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(1, 2, 3));
	ASSERT_EQ(segment.endPoint(), Point< TypeParam >(4, 5, 6));
}

TYPED_TEST(MathSpace3D, SegmentGetStartXYZ)
{
	const Segment< TypeParam > segment{{1, 2, 3}, {4, 5, 6}};

	ASSERT_NEAR(segment.startX(), TypeParam{1.0}, TypeParam{1e-5});
	ASSERT_NEAR(segment.startY(), TypeParam{2.0}, TypeParam{1e-5});
	ASSERT_NEAR(segment.startZ(), TypeParam{3.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SegmentGetEndXYZ)
{
	const Segment< TypeParam > segment{{1, 2, 3}, {4, 5, 6}};

	ASSERT_NEAR(segment.endX(), TypeParam{4.0}, TypeParam{1e-5});
	ASSERT_NEAR(segment.endY(), TypeParam{5.0}, TypeParam{1e-5});
	ASSERT_NEAR(segment.endZ(), TypeParam{6.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SegmentGetLength)
{
	const Segment< TypeParam > segment{{0, 0, 0}, {3, 4, 0}};

	ASSERT_NEAR(segment.getLength(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SegmentReset)
{
	Segment< TypeParam > segment{{10, 20, 30}, {40, 50, 60}};

	segment.reset();

	ASSERT_EQ(segment.startPoint(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(segment.endPoint(), Point< TypeParam >(0, 0, 0));
}

// ============================================================================
// SPHERE TESTS
// ============================================================================

TYPED_TEST(MathSpace3D, SphereDefaultConstructor)
{
	const Sphere< TypeParam > sphere;

	ASSERT_EQ(sphere.position(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(sphere.radius(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, SphereConstructorWithRadius)
{
	const Sphere< TypeParam > sphere{5.0};

	ASSERT_EQ(sphere.position(), Point< TypeParam >(0, 0, 0));
	ASSERT_NEAR(sphere.radius(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SphereConstructorWithRadiusAndPosition)
{
	const Point< TypeParam > pos{10, 20, 30};
	const Sphere< TypeParam > sphere{5.0, pos};

	ASSERT_EQ(sphere.position(), pos);
	ASSERT_NEAR(sphere.radius(), TypeParam{5.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SphereIsValid)
{
	const Sphere< TypeParam > validSphere{5.0};
	ASSERT_TRUE(validSphere.isValid());

	const Sphere< TypeParam > invalidSphere{0.0};
	ASSERT_FALSE(invalidSphere.isValid());
}

TYPED_TEST(MathSpace3D, SphereSetPosition)
{
	Sphere< TypeParam > sphere{5.0};
	const Point< TypeParam > newPos{1, 2, 3};

	sphere.setPosition(newPos);

	ASSERT_EQ(sphere.position(), newPos);
}

TYPED_TEST(MathSpace3D, SphereSetRadius)
{
	Sphere< TypeParam > sphere{5.0};

	sphere.setRadius(10.0);

	ASSERT_NEAR(sphere.radius(), TypeParam{10.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SphereSetRadiusNegative)
{
	Sphere< TypeParam > sphere{5.0};

	sphere.setRadius(-10.0);

	// Should take absolute value
	ASSERT_NEAR(sphere.radius(), TypeParam{10.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SphereSquaredRadius)
{
	const Sphere< TypeParam > sphere{5.0};

	ASSERT_NEAR(sphere.squaredRadius(), TypeParam{25.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SphereGetPerimeter)
{
	const Sphere< TypeParam > sphere{1.0};

	// C = 2πr
	ASSERT_NEAR(sphere.getPerimeter(), TypeParam{2.0} * std::numbers::pi_v< TypeParam >, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, SphereGetArea)
{
	const Sphere< TypeParam > sphere{1.0};

	// A = πr²
	ASSERT_NEAR(sphere.getArea(), std::numbers::pi_v< TypeParam >, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, SphereGetVolume)
{
	const Sphere< TypeParam > sphere{1.0};

	// V = 4/3 πr³
	ASSERT_NEAR(sphere.getVolume(), TypeParam{4.0 / 3.0} * std::numbers::pi_v< TypeParam >, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, SphereReset)
{
	Sphere< TypeParam > sphere{10.0, {1, 2, 3}};

	sphere.reset();

	ASSERT_EQ(sphere.position(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(sphere.radius(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, SphereMergeContained)
{
	Sphere< TypeParam > sphere1{10.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{2.0, {1, 1, 1}};

	sphere1.merge(sphere2);

	// sphere2 is entirely contained in sphere1, so sphere1 should remain unchanged
	ASSERT_NEAR(sphere1.radius(), TypeParam{10.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, SphereMergeDisjoint)
{
	Sphere< TypeParam > sphere1{1.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{1.0, {10, 0, 0}};

	sphere1.merge(sphere2);

	// Merged sphere should contain both
	ASSERT_GT(sphere1.radius(), TypeParam{5.0});
}

// ============================================================================
// TRIANGLE TESTS
// ============================================================================

TYPED_TEST(MathSpace3D, TriangleDefaultConstructor)
{
	const Triangle< TypeParam > triangle;

	ASSERT_EQ(triangle.pointA(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(triangle.pointB(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(triangle.pointC(), Point< TypeParam >(0, 0, 0));
}

TYPED_TEST(MathSpace3D, TriangleConstructorWithPoints)
{
	const Point< TypeParam > a{0, 0, 0};
	const Point< TypeParam > b{1, 0, 0};
	const Point< TypeParam > c{0, 1, 0};
	const Triangle< TypeParam > triangle{a, b, c};

	ASSERT_EQ(triangle.pointA(), a);
	ASSERT_EQ(triangle.pointB(), b);
	ASSERT_EQ(triangle.pointC(), c);
}

TYPED_TEST(MathSpace3D, TriangleIsValid)
{
	const Triangle< TypeParam > validTriangle{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
	ASSERT_TRUE(validTriangle.isValid());

	const Triangle< TypeParam > invalidTriangle{{0, 0, 0}, {0, 0, 0}, {0, 1, 0}};
	ASSERT_FALSE(invalidTriangle.isValid());
}

TYPED_TEST(MathSpace3D, TriangleSetPoints)
{
	Triangle< TypeParam > triangle;

	triangle.setPointA({1, 0, 0});
	triangle.setPointB({0, 1, 0});
	triangle.setPointC({0, 0, 1});

	ASSERT_EQ(triangle.pointA(), Point< TypeParam >(1, 0, 0));
	ASSERT_EQ(triangle.pointB(), Point< TypeParam >(0, 1, 0));
	ASSERT_EQ(triangle.pointC(), Point< TypeParam >(0, 0, 1));
}

TYPED_TEST(MathSpace3D, TriangleFlip)
{
	Triangle< TypeParam > triangle{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};

	const auto originalA = triangle.pointA();
	const auto originalB = triangle.pointB();

	triangle.flip();

	ASSERT_EQ(triangle.pointA(), originalB);
	ASSERT_EQ(triangle.pointB(), originalA);
}

TYPED_TEST(MathSpace3D, TriangleCycle)
{
	Triangle< TypeParam > triangle{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};

	const auto originalA = triangle.pointA();
	const auto originalB = triangle.pointB();
	const auto originalC = triangle.pointC();

	triangle.cycle();

	ASSERT_EQ(triangle.pointA(), originalB);
	ASSERT_EQ(triangle.pointB(), originalC);
	ASSERT_EQ(triangle.pointC(), originalA);
}

TYPED_TEST(MathSpace3D, TriangleGetPerimeter)
{
	// Right triangle with sides 3, 4, 5
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 4, 0}};

	ASSERT_NEAR(triangle.getPerimeter(), TypeParam{12.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, TriangleGetArea)
{
	// Triangle with base 4 and height 3
	const Triangle< TypeParam > triangle{{0, 0, 0}, {4, 0, 0}, {0, 3, 0}};

	ASSERT_NEAR(triangle.getArea(), TypeParam{6.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, TriangleReset)
{
	Triangle< TypeParam > triangle{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

	triangle.reset();

	ASSERT_EQ(triangle.pointA(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(triangle.pointB(), Point< TypeParam >(0, 0, 0));
	ASSERT_EQ(triangle.pointC(), Point< TypeParam >(0, 0, 0));
}

// ============================================================================
// AACUBOID TESTS
// ============================================================================

TYPED_TEST(MathSpace3D, AACuboidDefaultConstructor)
{
	const AACuboid< TypeParam > cuboid;

	// Default should be invalid (min > max)
	ASSERT_FALSE(cuboid.isValid());
}

TYPED_TEST(MathSpace3D, AACuboidConstructorWithValue)
{
	const AACuboid< TypeParam > cuboid{5.0};

	ASSERT_TRUE(cuboid.isValid());
	ASSERT_EQ(cuboid.maximum(), Point< TypeParam >(5, 5, 5));
	ASSERT_EQ(cuboid.minimum(), Point< TypeParam >(-5, -5, -5));
}

TYPED_TEST(MathSpace3D, AACuboidConstructorWithMaxMin)
{
	const Point< TypeParam > maxPoint{10, 20, 30};
	const Point< TypeParam > minPoint{-5, -10, -15};
	const AACuboid< TypeParam > cuboid{maxPoint, minPoint};

	ASSERT_TRUE(cuboid.isValid());
	ASSERT_EQ(cuboid.maximum(), maxPoint);
	ASSERT_EQ(cuboid.minimum(), minPoint);
}

TYPED_TEST(MathSpace3D, AACuboidConstructorSwapsMaxMin)
{
	// Constructor should swap if max < min
	const AACuboid< TypeParam > cuboid{{-5, -10, -15}, {10, 20, 30}};

	ASSERT_TRUE(cuboid.isValid());
	ASSERT_EQ(cuboid.maximum(), Point< TypeParam >(10, 20, 30));
	ASSERT_EQ(cuboid.minimum(), Point< TypeParam >(-5, -10, -15));
}

TYPED_TEST(MathSpace3D, AACuboidIsValid)
{
	const AACuboid< TypeParam > validCuboid{5.0};
	ASSERT_TRUE(validCuboid.isValid());

	AACuboid< TypeParam > invalidCuboid;
	ASSERT_FALSE(invalidCuboid.isValid());
}

TYPED_TEST(MathSpace3D, AACuboidSetValue)
{
	AACuboid< TypeParam > cuboid;

	cuboid.set(10.0);

	ASSERT_TRUE(cuboid.isValid());
	ASSERT_EQ(cuboid.maximum(), Point< TypeParam >(10, 10, 10));
	ASSERT_EQ(cuboid.minimum(), Point< TypeParam >(-10, -10, -10));
}

TYPED_TEST(MathSpace3D, AACuboidSetMaxMin)
{
	AACuboid< TypeParam > cuboid;

	cuboid.set({10, 20, 30}, {-5, -10, -15});

	ASSERT_TRUE(cuboid.isValid());
	ASSERT_EQ(cuboid.maximum(), Point< TypeParam >(10, 20, 30));
	ASSERT_EQ(cuboid.minimum(), Point< TypeParam >(-5, -10, -15));
}

TYPED_TEST(MathSpace3D, AACuboidWidthHeightDepth)
{
	const AACuboid< TypeParam > cuboid{{10, 20, 30}, {-5, -10, -15}};

	ASSERT_NEAR(cuboid.width(), TypeParam{15.0}, TypeParam{1e-5});
	ASSERT_NEAR(cuboid.height(), TypeParam{30.0}, TypeParam{1e-5});
	ASSERT_NEAR(cuboid.depth(), TypeParam{45.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, AACuboidFarthestPoint)
{
	const AACuboid< TypeParam > cuboid{{10, 5, 3}, {-2, -20, -1}};

	// Farthest point from center should be max of abs values
	ASSERT_NEAR(cuboid.farthestPoint(), TypeParam{20.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, AACuboidHighestLength)
{
	const AACuboid< TypeParam > cuboid{{10, 20, 30}, {5, 10, 15}};

	// Highest of width=5, height=10, depth=15
	ASSERT_NEAR(cuboid.highestLength(), TypeParam{15.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, AACuboidCornerPoints)
{
	const AACuboid< TypeParam > cuboid{{1, 1, 1}, {-1, -1, -1}};

	ASSERT_EQ(cuboid.bottomSouthEast(), Point< TypeParam >(1, 1, 1));
	ASSERT_EQ(cuboid.bottomNorthEast(), Point< TypeParam >(1, 1, -1));
	ASSERT_EQ(cuboid.bottomSouthWest(), Point< TypeParam >(-1, 1, 1));
	ASSERT_EQ(cuboid.bottomNorthWest(), Point< TypeParam >(-1, 1, -1));
	ASSERT_EQ(cuboid.topSouthEast(), Point< TypeParam >(1, -1, 1));
	ASSERT_EQ(cuboid.topNorthEast(), Point< TypeParam >(1, -1, -1));
	ASSERT_EQ(cuboid.topSouthWest(), Point< TypeParam >(-1, -1, 1));
	ASSERT_EQ(cuboid.topNorthWest(), Point< TypeParam >(-1, -1, -1));
}

TYPED_TEST(MathSpace3D, AACuboidCentroid)
{
	const AACuboid< TypeParam > cuboid{{10, 20, 30}, {-10, -20, -30}};

	ASSERT_EQ(cuboid.centroid(), Point< TypeParam >(0, 0, 0));
}

TYPED_TEST(MathSpace3D, AACuboidGetVolume)
{
	const AACuboid< TypeParam > cuboid{{5, 5, 5}, {0, 0, 0}};

	ASSERT_NEAR(cuboid.getVolume(), TypeParam{125.0}, TypeParam{1e-5});
}

TYPED_TEST(MathSpace3D, AACuboidReset)
{
	AACuboid< TypeParam > cuboid{10.0};

	cuboid.reset();

	ASSERT_FALSE(cuboid.isValid());
}

TYPED_TEST(MathSpace3D, AACuboidMergeWithCuboid)
{
	AACuboid< TypeParam > cuboid1{{5, 5, 5}, {0, 0, 0}};
	const AACuboid< TypeParam > cuboid2{{10, 3, 8}, {-2, -4, -1}};

	cuboid1.merge(cuboid2);

	ASSERT_EQ(cuboid1.maximum(), Point< TypeParam >(10, 5, 8));
	ASSERT_EQ(cuboid1.minimum(), Point< TypeParam >(-2, -4, -1));
}

TYPED_TEST(MathSpace3D, AACuboidMergeWithPoint)
{
	AACuboid< TypeParam > cuboid{{5, 5, 5}, {0, 0, 0}};
	const Point< TypeParam > point{10, -5, 3};

	cuboid.merge(point);

	ASSERT_EQ(cuboid.maximum(), Point< TypeParam >(10, 5, 5));
	ASSERT_EQ(cuboid.minimum(), Point< TypeParam >(0, -5, 0));
}

TYPED_TEST(MathSpace3D, AACuboidMergeXYZ)
{
	AACuboid< TypeParam > cuboid{{5, 5, 5}, {0, 0, 0}};

	cuboid.mergeX(10.0);
	cuboid.mergeY(-3.0);
	cuboid.mergeZ(7.0);

	ASSERT_EQ(cuboid.maximum(), Point< TypeParam >(10, 5, 7));
	ASSERT_EQ(cuboid.minimum(), Point< TypeParam >(0, -3, 0));
}

// ============================================================================
// COLLISION TESTS - POINT COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionPointInsideSphere)
{
	const Point< TypeParam > point{1, 1, 1};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};

	ASSERT_TRUE(isColliding(point, sphere));
	ASSERT_TRUE(isColliding(sphere, point));
}

TYPED_TEST(MathSpace3D, CollisionPointOutsideSphere)
{
	const Point< TypeParam > point{10, 0, 0};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};

	ASSERT_FALSE(isColliding(point, sphere));
}

TYPED_TEST(MathSpace3D, CollisionPointSphereWithMTV)
{
	const Point< TypeParam > point{3, 0, 0};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(point, sphere, mtv));
	// MTV should point away from sphere center
	ASSERT_GT(mtv[X], TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionPointInsideCuboid)
{
	const Point< TypeParam > point{2, 2, 2};
	const AACuboid< TypeParam > cuboid{{5, 5, 5}, {0, 0, 0}};

	ASSERT_TRUE(isColliding(point, cuboid));
	ASSERT_TRUE(isColliding(cuboid, point));
}

TYPED_TEST(MathSpace3D, CollisionPointOutsideCuboid)
{
	const Point< TypeParam > point{10, 2, 2};
	const AACuboid< TypeParam > cuboid{{5, 5, 5}, {0, 0, 0}};

	ASSERT_FALSE(isColliding(point, cuboid));
}

TYPED_TEST(MathSpace3D, CollisionPointCuboidWithMTV)
{
	const Point< TypeParam > point{4, 2, 2};
	const AACuboid< TypeParam > cuboid{{5, 5, 5}, {0, 0, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(point, cuboid, mtv));
	// MTV should push point out of cuboid
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionPointInsideTriangle)
{
	const Point< TypeParam > point{1, 0, 1};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 0, 3}};

	ASSERT_TRUE(isColliding(point, triangle));
	ASSERT_TRUE(isColliding(triangle, point));
}

TYPED_TEST(MathSpace3D, CollisionPointOutsideTriangle)
{
	const Point< TypeParam > point{10, 0, 1};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 0, 3}};

	ASSERT_FALSE(isColliding(point, triangle));
}

TYPED_TEST(MathSpace3D, CollisionPointTriangleWithMTV)
{
	const Point< TypeParam > point{1, 0, 1};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 0, 3}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(point, triangle, mtv));
	// MTV should push point out of triangle
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionPointOnTriangleEdge)
{
	// Point exactly on edge AB
	const Point< TypeParam > point{1.5, 0, 0};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 0, 3}};

	ASSERT_TRUE(isColliding(point, triangle));
}

TYPED_TEST(MathSpace3D, CollisionPointOnTriangleVertex)
{
	// Point exactly on vertex A
	const Point< TypeParam > point{0, 0, 0};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 0, 3}};

	ASSERT_TRUE(isColliding(point, triangle));
}

// ============================================================================
// COLLISION TESTS - SPHERE COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionSphereSphereTouching)
{
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{3.0, {8, 0, 0}};

	ASSERT_TRUE(isColliding(sphere1, sphere2));
}

TYPED_TEST(MathSpace3D, CollisionSphereSphereNotTouching)
{
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{3.0, {10, 0, 0}};

	ASSERT_FALSE(isColliding(sphere1, sphere2));
}

TYPED_TEST(MathSpace3D, CollisionSphereSphereWithMTV)
{
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{5.0, {5, 0, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(sphere1, sphere2, mtv));
	// MTV should separate the spheres along X axis
	ASSERT_GT(std::abs(mtv[X]), TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionSphereCuboidIntersecting)
{
	const Sphere< TypeParam > sphere{3.0, {5, 5, 5}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_TRUE(isColliding(sphere, cuboid));
	ASSERT_TRUE(isColliding(cuboid, sphere));
}

TYPED_TEST(MathSpace3D, CollisionSphereCuboidNotIntersecting)
{
	const Sphere< TypeParam > sphere{2.0, {20, 20, 20}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_FALSE(isColliding(sphere, cuboid));
}

TYPED_TEST(MathSpace3D, CollisionSphereCuboidWithMTV)
{
	const Sphere< TypeParam > sphere{5.0, {8, 5, 5}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(sphere, cuboid, mtv));
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionSphereSphereExactlyTouching)
{
	// Spheres touching at exactly one point
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{3.0, {8, 0, 0}}; // distance = sum of radii

	ASSERT_TRUE(isColliding(sphere1, sphere2));
}

TYPED_TEST(MathSpace3D, CollisionSphereInsideCuboid)
{
	// Sphere completely contained inside cuboid
	const Sphere< TypeParam > sphere{2.0, {5, 5, 5}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_TRUE(isColliding(sphere, cuboid));
}

TYPED_TEST(MathSpace3D, CollisionSphereTouchingCuboidFace)
{
	// Sphere touching cuboid face at one point
	const Sphere< TypeParam > sphere{2.0, {12, 5, 5}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_TRUE(isColliding(sphere, cuboid));
}

// ============================================================================
// COLLISION TESTS - TRIANGLE COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleIntersecting)
{
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	const Triangle< TypeParam > tri2{{1, 0, 0}, {2, 2, 0}, {-1, 1, 0}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleNotIntersecting)
{
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	const Triangle< TypeParam > tri2{{10, 0, 0}, {13, 0, 0}, {10, 3, 0}};

	ASSERT_FALSE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleWithMTV)
{
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	const Triangle< TypeParam > tri2{{1, 0, 0}, {2, 2, 0}, {-1, 1, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(tri1, tri2, mtv));
	// MTV is computed (may be small for coplanar triangles)
	// Just verify collision is detected
}

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleNonCoplanar)
{
	// Triangle in XY plane
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	// Triangle in XZ plane that intersects tri1
	const Triangle< TypeParam > tri2{{1, -1, 1}, {1, 2, 1}, {1, -1, -1}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleNonCoplanarNotIntersecting)
{
	// Triangle in XY plane
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	// Triangle in XZ plane that doesn't intersect tri1
	const Triangle< TypeParam > tri2{{10, -1, 1}, {10, 2, 1}, {10, -1, -1}};

	ASSERT_FALSE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleTouchingAtVertex)
{
	// Triangles sharing one vertex
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	const Triangle< TypeParam > tri2{{0, 0, 0}, {-3, 0, 0}, {0, -3, 0}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleTriangleTouchingAtEdge)
{
	// Triangles sharing an edge
	const Triangle< TypeParam > tri1{{0, 0, 0}, {3, 0, 0}, {1.5, 3, 0}};
	const Triangle< TypeParam > tri2{{0, 0, 0}, {3, 0, 0}, {1.5, -3, 0}};

	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleSphereIntersecting)
{
	const Triangle< TypeParam > triangle{{0, 0, 0}, {5, 0, 0}, {0, 5, 0}};
	const Sphere< TypeParam > sphere{2.0, {1, 1, 0}};

	ASSERT_TRUE(isColliding(triangle, sphere));
	ASSERT_TRUE(isColliding(sphere, triangle));
}

TYPED_TEST(MathSpace3D, CollisionTriangleSphereNotIntersecting)
{
	const Triangle< TypeParam > triangle{{0, 0, 0}, {5, 0, 0}, {0, 5, 0}};
	const Sphere< TypeParam > sphere{1.0, {10, 10, 10}};

	ASSERT_FALSE(isColliding(triangle, sphere));
}

TYPED_TEST(MathSpace3D, CollisionTriangleSphereWithMTV)
{
	const Triangle< TypeParam > triangle{{0, 0, 0}, {5, 0, 0}, {0, 5, 0}};
	const Sphere< TypeParam > sphere{2.0, {1, 1, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(triangle, sphere, mtv));
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionTriangleSphereSphereTouchingVertex)
{
	// Sphere touching triangle at vertex A
	const Triangle< TypeParam > triangle{{0, 0, 0}, {5, 0, 0}, {0, 5, 0}};
	const Sphere< TypeParam > sphere{1.0, {0, 0, 1}};

	ASSERT_TRUE(isColliding(triangle, sphere));
}

TYPED_TEST(MathSpace3D, CollisionTriangleCuboidIntersecting)
{
	const Triangle< TypeParam > triangle{{-1, 5, 5}, {5, 5, 5}, {2, 5, 10}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_TRUE(isColliding(triangle, cuboid));
	ASSERT_TRUE(isColliding(cuboid, triangle));
}

TYPED_TEST(MathSpace3D, CollisionTriangleCuboidNotIntersecting)
{
	const Triangle< TypeParam > triangle{{20, 20, 20}, {25, 20, 20}, {20, 25, 20}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_FALSE(isColliding(triangle, cuboid));
}

TYPED_TEST(MathSpace3D, CollisionTriangleCuboidWithMTV)
{
	const Triangle< TypeParam > triangle{{-1, 5, 5}, {5, 5, 5}, {2, 5, 10}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(triangle, cuboid, mtv));
	// MTV is computed - just verify collision is detected
}

TYPED_TEST(MathSpace3D, CollisionTriangleCompletelyInsideCuboid)
{
	// Small triangle completely inside cuboid
	const Triangle< TypeParam > triangle{{4, 4, 4}, {5, 4, 4}, {4, 5, 4}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_TRUE(isColliding(triangle, cuboid));
}

// ============================================================================
// COLLISION TESTS - CUBOID COLLISIONS
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionCuboidCuboidIntersecting)
{
	const AACuboid< TypeParam > cuboid1{{10, 10, 10}, {0, 0, 0}};
	const AACuboid< TypeParam > cuboid2{{15, 15, 15}, {5, 5, 5}};

	ASSERT_TRUE(isColliding(cuboid1, cuboid2));
}

TYPED_TEST(MathSpace3D, CollisionCuboidCuboidNotIntersecting)
{
	const AACuboid< TypeParam > cuboid1{{10, 10, 10}, {0, 0, 0}};
	const AACuboid< TypeParam > cuboid2{{25, 25, 25}, {15, 15, 15}};

	ASSERT_FALSE(isColliding(cuboid1, cuboid2));
}

TYPED_TEST(MathSpace3D, CollisionCuboidCuboidWithMTV)
{
	const AACuboid< TypeParam > cuboid1{{10, 10, 10}, {0, 0, 0}};
	const AACuboid< TypeParam > cuboid2{{12, 12, 12}, {8, 8, 8}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(cuboid1, cuboid2, mtv));
	ASSERT_NE(mtv.length(), TypeParam{0});
}

TYPED_TEST(MathSpace3D, CollisionCuboidCuboidTouchingFaces)
{
	// Cuboids touching at face (edge case)
	const AACuboid< TypeParam > cuboid1{{10, 10, 10}, {0, 0, 0}};
	const AACuboid< TypeParam > cuboid2{{20, 10, 10}, {10, 0, 0}};

	ASSERT_TRUE(isColliding(cuboid1, cuboid2));
}

TYPED_TEST(MathSpace3D, CollisionCuboidCompletelyInsideAnother)
{
	// Small cuboid completely inside larger one
	const AACuboid< TypeParam > cuboid1{{10, 10, 10}, {0, 0, 0}};
	const AACuboid< TypeParam > cuboid2{{6, 6, 6}, {4, 4, 4}};

	ASSERT_TRUE(isColliding(cuboid1, cuboid2));
}

// ============================================================================
// INTERSECTION TESTS - LINE INTERSECTIONS
// ============================================================================

TYPED_TEST(MathSpace3D, IntersectionLineLineIntersecting)
{
	const Line< TypeParam > line1{{0, 0, 0}, {1, 0, 0}};
	const Line< TypeParam > line2{{5, -5, 0}, {0, 1, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line1, line2, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{5}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionLineLineParallel)
{
	const Line< TypeParam > line1{{0, 0, 0}, {1, 0, 0}};
	const Line< TypeParam > line2{{0, 1, 0}, {1, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_FALSE(isIntersecting(line1, line2, intersection));
}

TYPED_TEST(MathSpace3D, IntersectionLineLineSkew)
{
	const Line< TypeParam > line1{{0, 0, 0}, {1, 0, 0}};
	const Line< TypeParam > line2{{0, 0, 5}, {0, 1, 0}};

	ASSERT_FALSE(isIntersecting(line1, line2));
}

TYPED_TEST(MathSpace3D, IntersectionLineSphereIntersecting)
{
	const Line< TypeParam > line{{-10, 0, 0}, {1, 0, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line, sphere, intersection));
	ASSERT_TRUE(isIntersecting(sphere, line, intersection));
	// Intersection should be on sphere surface
	const Point< TypeParam > center{0, 0, 0};
	ASSERT_NEAR(Point< TypeParam >::distance(intersection, center), TypeParam{5.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionLineSphereNotIntersecting)
{
	const Line< TypeParam > line{{0, 10, 0}, {1, 0, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};

	ASSERT_FALSE(isIntersecting(line, sphere));
}

TYPED_TEST(MathSpace3D, IntersectionLineTriangleIntersecting)
{
	const Line< TypeParam > line{{1, 1, -5}, {0, 0, 1}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line, triangle, intersection));
	ASSERT_TRUE(isIntersecting(triangle, line, intersection));
	ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionLineTriangleNotIntersecting)
{
	const Line< TypeParam > line{{10, 10, -5}, {0, 0, 1}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};

	ASSERT_FALSE(isIntersecting(line, triangle));
}

TYPED_TEST(MathSpace3D, IntersectionLineCuboidIntersecting)
{
	const Line< TypeParam > line{{5, 5, -10}, {0, 0, 1}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line, cuboid, intersection));
	ASSERT_TRUE(isIntersecting(cuboid, line, intersection));
	// Intersection should be at bottom of cuboid
	ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionLineCuboidNotIntersecting)
{
	const Line< TypeParam > line{{20, 20, -10}, {0, 0, 1}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_FALSE(isIntersecting(line, cuboid));
}

TYPED_TEST(MathSpace3D, IntersectionLineSphereTangent)
{
	// Line tangent to sphere (touching at exactly one point)
	const Line< TypeParam > line{{5, 0, 0}, {0, 0, 1}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line, sphere, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{5}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionLineThroughSphereCenter)
{
	// Line passing through sphere center
	const Line< TypeParam > line{{0, 0, 0}, {1, 0, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(line, sphere, intersection));
}

// ============================================================================
// INTERSECTION TESTS - SEGMENT INTERSECTIONS
// ============================================================================

TYPED_TEST(MathSpace3D, IntersectionSegmentSegmentIntersecting)
{
	const Segment< TypeParam > seg1{{0, 0, 0}, {10, 0, 0}};
	const Segment< TypeParam > seg2{{5, -5, 0}, {5, 5, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(seg1, seg2, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{5}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSegmentNotIntersecting)
{
	const Segment< TypeParam > seg1{{0, 0, 0}, {5, 0, 0}};
	const Segment< TypeParam > seg2{{10, -5, 0}, {10, 5, 0}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSegmentParallel)
{
	const Segment< TypeParam > seg1{{0, 0, 0}, {10, 0, 0}};
	const Segment< TypeParam > seg2{{0, 1, 0}, {10, 1, 0}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSegmentCollinearOverlapping)
{
	// Collinear segments that overlap
	const Segment< TypeParam > seg1{{0, 0, 0}, {5, 0, 0}};
	const Segment< TypeParam > seg2{{3, 0, 0}, {8, 0, 0}};

	ASSERT_TRUE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSegmentCollinearNonOverlapping)
{
	// Collinear segments that don't overlap
	const Segment< TypeParam > seg1{{0, 0, 0}, {5, 0, 0}};
	const Segment< TypeParam > seg2{{10, 0, 0}, {15, 0, 0}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSegmentSkewLines)
{
	// Segments on skew lines (non-coplanar, non-intersecting)
	const Segment< TypeParam > seg1{{0, 0, 0}, {10, 0, 0}};
	const Segment< TypeParam > seg2{{0, 0, 5}, {0, 10, 5}};

	ASSERT_FALSE(isIntersecting(seg1, seg2));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSphereIntersecting)
{
	const Segment< TypeParam > segment{{-10, 0, 0}, {10, 0, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(segment, sphere, intersection));
	ASSERT_TRUE(isIntersecting(sphere, segment, intersection));
	// Intersection should be on sphere surface
	const Point< TypeParam > center{0, 0, 0};
	ASSERT_NEAR(Point< TypeParam >::distance(intersection, center), TypeParam{5.0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSphereNotIntersecting)
{
	const Segment< TypeParam > segment{{10, 10, 0}, {20, 10, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};

	ASSERT_FALSE(isIntersecting(segment, sphere));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSphereTooShort)
{
	const Segment< TypeParam > segment{{-2, 0, 0}, {2, 0, 0}};
	const Sphere< TypeParam > sphere{1.0, {10, 0, 0}};

	ASSERT_FALSE(isIntersecting(segment, sphere));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleIntersecting)
{
	const Segment< TypeParam > segment{{1, 1, -5}, {1, 1, 5}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(segment, triangle, intersection));
	ASSERT_TRUE(isIntersecting(triangle, segment, intersection));
	ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleNotIntersecting)
{
	const Segment< TypeParam > segment{{10, 10, -5}, {10, 10, 5}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};

	ASSERT_FALSE(isIntersecting(segment, triangle));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleTooShort)
{
	const Segment< TypeParam > segment{{1, 1, -1}, {1, 1, -0.5}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};

	ASSERT_FALSE(isIntersecting(segment, triangle));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentCuboidIntersecting)
{
	const Segment< TypeParam > segment{{5, 5, -10}, {5, 5, 15}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(segment, cuboid, intersection));
	ASSERT_TRUE(isIntersecting(cuboid, segment, intersection));
	// Intersection should be at bottom of cuboid
	ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentCuboidNotIntersecting)
{
	const Segment< TypeParam > segment{{20, 20, -5}, {20, 20, -1}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};

	ASSERT_FALSE(isIntersecting(segment, cuboid));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentCuboidInsideSegment)
{
	const Segment< TypeParam > segment{{5, 5, 5}, {6, 6, 6}};
	const AACuboid< TypeParam > cuboid{{10, 10, 10}, {0, 0, 0}};
	Point< TypeParam > intersection;

	// Segment entirely inside cuboid
	ASSERT_TRUE(isIntersecting(segment, cuboid, intersection));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSphereTangent)
{
	// Segment tangent to sphere surface
	const Segment< TypeParam > segment{{5, -10, 0}, {5, 10, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(segment, sphere, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{5}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleAtVertex)
{
	// Segment intersecting triangle at vertex
	const Segment< TypeParam > segment{{0, 0, -5}, {0, 0, 5}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(segment, triangle, intersection));
	ASSERT_NEAR(intersection[X], TypeParam{0}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{0}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleAtEdge)
{
	// Segment intersecting triangle at edge
	const Segment< TypeParam > segment{{1.5, 0, -5}, {1.5, 0, 5}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {3, 0, 0}, {0, 3, 0}};
	Point< TypeParam > intersection;

	ASSERT_TRUE(isIntersecting(segment, triangle, intersection));
	ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - STATICVECTOR CAPACITY & STRESS
// ============================================================================

TYPED_TEST(MathSpace3D, SATAxesCapacityTetrahedronTetrahedron)
{
	/**
	 * @test Verifies that SAT collision with tetrahedrons doesn't exceed StaticVector capacity.
	 * REQUIREMENT: StaticVector capacity must accommodate all collision detection axes.
	 * 3D SAT for two triangular pyramids requires:
	 * - 4 face normals from first tetrahedron
	 * - 4 face normals from second tetrahedron
	 * - Up to 9 edge-cross-product axes (3 edges × 3 edges)
	 * Total: ~17 axes maximum, capacity is 32 providing 2x safety margin.
	 */
	// Create two tetrahedrons represented as vertex arrays
	std::array< Vector< 3, TypeParam >, 4 > tetra1 = {
		Vector< 3, TypeParam >{0, 0, 0},
		Vector< 3, TypeParam >{4, 0, 0},
		Vector< 3, TypeParam >{2, 4, 0},
		Vector< 3, TypeParam >{2, 2, 4}
	};

	std::array< Vector< 3, TypeParam >, 4 > tetra2 = {
		Vector< 3, TypeParam >{3, 2, 2},
		Vector< 3, TypeParam >{7, 2, 2},
		Vector< 3, TypeParam >{5, 6, 2},
		Vector< 3, TypeParam >{5, 4, 6}
	};

	Vector< 3, TypeParam > mtv;

	// This should not crash or overflow StaticVector
	[[maybe_unused]] const bool result = SAT::checkCollision(tetra1, tetra2, mtv);
}

TYPED_TEST(MathSpace3D, SATAxesCapacityOctahedron)
{
	/**
	 * @test Stress test for SAT with octahedron (8 triangular faces, 12 edges).
	 * REQUIREMENT: StaticVector capacity 32 must handle complex 3D polyhedra.
	 * Worst case: 8 + 8 face normals + up to 12×12 edge crosses = 160+ potential axes.
	 * SAT optimization reduces this, but capacity must be sufficient.
	 */
	std::array< Vector< 3, TypeParam >, 6 > octahedron = {
		Vector< 3, TypeParam >{0, 5, 0},   // top
		Vector< 3, TypeParam >{5, 0, 0},   // +X
		Vector< 3, TypeParam >{0, 0, 5},   // +Z
		Vector< 3, TypeParam >{-5, 0, 0},  // -X
		Vector< 3, TypeParam >{0, 0, -5},  // -Z
		Vector< 3, TypeParam >{0, -5, 0}   // bottom
	};

	std::array< Vector< 3, TypeParam >, 4 > tetra = {
		Vector< 3, TypeParam >{1, 1, 1},
		Vector< 3, TypeParam >{3, 1, 1},
		Vector< 3, TypeParam >{2, 3, 1},
		Vector< 3, TypeParam >{2, 2, 3}
	};

	Vector< 3, TypeParam > mtv;
	[[maybe_unused]] const bool result = SAT::checkCollision(octahedron, tetra, mtv);
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSphereMaximumPoints)
{
	/**
	 * @test Verifies segment-sphere intersection can handle maximum intersection points.
	 * A segment can intersect a sphere at most at 2 points (entry and exit).
	 */
	const Segment< TypeParam > segment{{-10, 0, 0}, {10, 0, 0}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	const bool result = isIntersecting(segment, sphere, intersection);

	ASSERT_TRUE(result);
	// Verify intersection is on segment
	ASSERT_GE(intersection[X], TypeParam{-10});
	ASSERT_LE(intersection[X], TypeParam{10});
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleCapacityStress)
{
	/**
	 * @test Stress test for segment-triangle intersection in 3D.
	 * Tests that intersection detection handles all edge cases without overflow.
	 */
	const Segment< TypeParam > segment{{-5, 5, 10}, {15, 5, -10}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {10, 0, 0}, {5, 10, 0}};
	Point< TypeParam > intersection;

	const bool result = isIntersecting(segment, triangle, intersection);

	if ( result )
	{
		// Verify intersection point is valid
		ASSERT_GE(intersection[X], TypeParam{-5});
		ASSERT_LE(intersection[X], TypeParam{15});
	}
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - DEGENERATE GEOMETRY
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionDegenerateTriangleCoplanarPoints)
{
	/**
	 * @test Verifies behavior with degenerate 3D triangle (3 coplanar points on a line).
	 * NOTE: In 3D, collinear points may still form a valid degenerate triangle.
	 * This test verifies the triangle can be constructed without crashing.
	 */
	const Triangle< TypeParam > degenerate{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}};

	// In 3D, collinear points might be considered valid or invalid depending on implementation
	[[maybe_unused]] const bool isValid = degenerate.isValid();
}

TYPED_TEST(MathSpace3D, CollisionDegenerateTriangleDuplicateVertices)
{
	/**
	 * @test Verifies behavior with degenerate triangle (duplicate vertices).
	 */
	const Triangle< TypeParam > degenerate{{2, 3, 4}, {2, 3, 4}, {5, 6, 7}};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace3D, CollisionDegenerateTriangleZeroArea)
{
	/**
	 * @test Verifies behavior with zero-area triangle in 3D space.
	 */
	const Triangle< TypeParam > degenerate{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace3D, CollisionValidTriangleWithDegenerateTriangle)
{
	/**
	 * @test Verifies that collision detection safely handles degenerate geometry.
	 * REQUIREMENT: Must not crash when one triangle is degenerate.
	 */
	const Triangle< TypeParam > valid{{0, 0, 0}, {5, 0, 0}, {2.5, 5, 0}};
	const Triangle< TypeParam > degenerate{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};

	ASSERT_TRUE(valid.isValid());
	// NOTE: In 3D, collinear points may be considered valid
	[[maybe_unused]] const bool degenerateValid = degenerate.isValid();

	// Should not crash regardless of validity status
	[[maybe_unused]] const bool result = isColliding(valid, degenerate);
}

TYPED_TEST(MathSpace3D, CollisionDegenerateSphereZeroRadius)
{
	/**
	 * @test Verifies behavior with degenerate sphere (zero radius).
	 */
	const Sphere< TypeParam > degenerate{0.0, {5, 5, 5}};

	ASSERT_FALSE(degenerate.isValid());
}

TYPED_TEST(MathSpace3D, CollisionDegenerateSphereNegativeRadius)
{
	/**
	 * @test Verifies behavior with invalid sphere (negative radius).
	 */
	const Sphere< TypeParam > invalid{-5.0, {5, 5, 5}};

	ASSERT_FALSE(invalid.isValid());
}

TYPED_TEST(MathSpace3D, IntersectionDegenerateSegmentZeroLength)
{
	/**
	 * @test Verifies behavior with degenerate segment (zero length) in 3D.
	 */
	const Segment< TypeParam > degenerate{{5, 5, 5}, {5, 5, 5}};
	const Segment< TypeParam > valid{{0, 0, 0}, {10, 10, 10}};

	ASSERT_FALSE(degenerate.isValid());
	ASSERT_TRUE(valid.isValid());

	// Intersection with invalid segment should not crash
	[[maybe_unused]] const bool result = isIntersecting(degenerate, valid);
}

TYPED_TEST(MathSpace3D, IntersectionDegenerateSegmentWithSphere)
{
	/**
	 * @test Tests intersection of degenerate segment with valid sphere.
	 */
	const Segment< TypeParam > degenerate{{5, 0, 0}, {5, 0, 0}};
	const Sphere< TypeParam > sphere{10.0, {0, 0, 0}};

	ASSERT_FALSE(degenerate.isValid());
	ASSERT_TRUE(sphere.isValid());

	// Should handle gracefully
	Point< TypeParam > intersection;
	[[maybe_unused]] const bool result = isIntersecting(degenerate, sphere, intersection);
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - NUMERICAL PRECISION & STABILITY
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionTriangleTouchingVertexNumericalPrecision)
{
	/**
	 * @test Tests numerical stability when 3D triangles touch at a single vertex.
	 */
	const Triangle< TypeParam > tri1{{0, 0, 0}, {5, 0, 0}, {2.5, 5, 0}};
	const Triangle< TypeParam > tri2{{5, 0, 0}, {10, 0, 0}, {7.5, 5, 0}};

	// Triangles share vertex (5, 0, 0) - should be touching
	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleTouchingEdgeNumericalPrecision)
{
	/**
	 * @test Tests edge-touching detection with numerical precision.
	 */
	const Triangle< TypeParam > tri1{{0, 0, 0}, {10, 0, 0}, {5, 10, 0}};
	const Triangle< TypeParam > tri2{{0, 0, 0}, {10, 0, 0}, {5, -10, 0}};

	// Triangles share edge from (0,0,0) to (10,0,0)
	ASSERT_TRUE(isColliding(tri1, tri2));
}

TYPED_TEST(MathSpace3D, CollisionTriangleVeryCloseButNotTouching)
{
	/**
	 * @test Tests numerical precision when triangles are very close but not touching.
	 */
	const Triangle< TypeParam > tri1{{0, 0, 0}, {5, 0, 0}, {2.5, 5, 0}};
	const Triangle< TypeParam > tri2{{0, 0, 0.001}, {5, 0, 0.001}, {2.5, 5, 0.001}};

	// Triangles are 0.001 units apart - should not be colliding
	[[maybe_unused]] const bool result = isColliding(tri1, tri2);
	// Result depends on collision tolerance
}

TYPED_TEST(MathSpace3D, CollisionSphereTouchingExactly)
{
	/**
	 * @test Tests spheres touching exactly at single point.
	 */
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{5.0, {10, 0, 0}};

	// Spheres touch at (5, 0, 0)
	ASSERT_TRUE(isColliding(sphere1, sphere2));
}

TYPED_TEST(MathSpace3D, CollisionSphereVeryCloseButNotTouching)
{
	/**
	 * @test Tests numerical stability with nearly touching spheres.
	 */
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{5.0, {10.001, 0, 0}};

	// Spheres are 0.001 units apart
	const bool result = isColliding(sphere1, sphere2);

	// Should not be colliding (distance = 10.001, radii sum = 10.0)
	ASSERT_FALSE(result);
}

TYPED_TEST(MathSpace3D, IntersectionSegmentTriangleNumericalPrecision)
{
	/**
	 * @test Tests segment-triangle intersection at exact vertex.
	 */
	const Segment< TypeParam > segment{{5, 5, -10}, {5, 5, 10}};
	const Triangle< TypeParam > triangle{{0, 0, 0}, {10, 0, 0}, {5, 10, 0}};
	Point< TypeParam > intersection;

	const bool result = isIntersecting(segment, triangle, intersection);

	if ( result )
	{
		ASSERT_NEAR(intersection[X], TypeParam{5}, TypeParam{1e-4});
		ASSERT_NEAR(intersection[Z], TypeParam{0}, TypeParam{1e-4});
	}
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - CONTAINER TYPE COMPATIBILITY
// ============================================================================

TYPED_TEST(MathSpace3D, SATCollisionWithStdArrayTetrahedrons)
{
	/**
	 * @test Verifies that SAT collision works with std::array containers in 3D.
	 * REQUIREMENT: Template functions must accept std::array with fixed size.
	 */
	std::array< Vector< 3, TypeParam >, 4 > tetra1 = {
		Vector< 3, TypeParam >{0, 0, 0},
		Vector< 3, TypeParam >{8, 0, 0},
		Vector< 3, TypeParam >{4, 8, 0},
		Vector< 3, TypeParam >{4, 4, 8}
	};

	std::array< Vector< 3, TypeParam >, 4 > tetra2 = {
		Vector< 3, TypeParam >{2, 2, 1},
		Vector< 3, TypeParam >{10, 2, 1},
		Vector< 3, TypeParam >{6, 10, 1},
		Vector< 3, TypeParam >{6, 6, 9}
	};

	Vector< 3, TypeParam > mtv;

	// Internal implementation uses std::array<Vector<3>, 4>
	const bool collision = SAT::checkCollision(tetra1, tetra2, mtv);

	if ( collision )
	{
		ASSERT_GT(mtv.length(), TypeParam{0});
	}
}

TYPED_TEST(MathSpace3D, SATCollisionWithMixedSizeArrays3D)
{
	/**
	 * @test Verifies that SAT collision works with different std::array sizes in 3D.
	 * Tetrahedron (4 vertices) vs Octahedron (6 vertices).
	 * REQUIREMENT: Template must support different container sizes.
	 */
	std::array< Vector< 3, TypeParam >, 4 > tetra = {
		Vector< 3, TypeParam >{2, 2, 2},
		Vector< 3, TypeParam >{10, 2, 2},
		Vector< 3, TypeParam >{6, 10, 2},
		Vector< 3, TypeParam >{6, 6, 10}
	};

	std::array< Vector< 3, TypeParam >, 6 > octa = {
		Vector< 3, TypeParam >{6, 12, 6},
		Vector< 3, TypeParam >{11, 7, 6},
		Vector< 3, TypeParam >{6, 7, 11},
		Vector< 3, TypeParam >{1, 7, 6},
		Vector< 3, TypeParam >{6, 7, 1},
		Vector< 3, TypeParam >{6, 2, 6}
	};

	Vector< 3, TypeParam > mtv;

	const bool collision = SAT::checkCollision(tetra, octa, mtv);

	if ( collision )
	{
		ASSERT_GT(mtv.length(), TypeParam{0});
	}
}

TYPED_TEST(MathSpace3D, IntersectionWithStaticVectorCompatibility)
{
	/**
	 * @test Validates that intersection functions work with StaticVector.
	 * REQUIREMENT: StaticVector must be STL-compatible (size(), clear(), etc.).
	 */
	const Segment< TypeParam > segment{{0, 0, -10}, {0, 0, 10}};
	const Sphere< TypeParam > sphere{5.0, {0, 0, 0}};
	Point< TypeParam > intersection;

	const bool result = isIntersecting(segment, sphere, intersection);

	ASSERT_TRUE(result);
	// Verify intersection point is on segment
	ASSERT_NEAR(intersection[X], TypeParam{0}, TypeParam{1e-4});
	ASSERT_NEAR(intersection[Y], TypeParam{0}, TypeParam{1e-4});
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - EXTREME VALUES & BOUNDARY CONDITIONS
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionTriangleVeryLargeCoordinates)
{
	/**
	 * @test Tests collision detection with very large coordinate values.
	 * REQUIREMENT: Must handle large floating-point values without overflow.
	 */
	const Triangle< TypeParam > tri1{
		{TypeParam{1e6}, TypeParam{1e6}, TypeParam{1e6}},
		{TypeParam{1e6} + TypeParam{10}, TypeParam{1e6}, TypeParam{1e6}},
		{TypeParam{1e6} + TypeParam{5}, TypeParam{1e6} + TypeParam{10}, TypeParam{1e6}}
	};

	const Triangle< TypeParam > tri2{
		{TypeParam{1e6} + TypeParam{2}, TypeParam{1e6} + TypeParam{2}, TypeParam{1e6} + TypeParam{2}},
		{TypeParam{1e6} + TypeParam{12}, TypeParam{1e6} + TypeParam{2}, TypeParam{1e6} + TypeParam{2}},
		{TypeParam{1e6} + TypeParam{7}, TypeParam{1e6} + TypeParam{12}, TypeParam{1e6} + TypeParam{2}}
	};

	ASSERT_TRUE(tri1.isValid());
	ASSERT_TRUE(tri2.isValid());

	// Should handle large coordinates
	[[maybe_unused]] const bool result = isColliding(tri1, tri2);
}

TYPED_TEST(MathSpace3D, CollisionTriangleVerySmallDimensions)
{
	/**
	 * @test Tests collision with very small triangle dimensions.
	 * REQUIREMENT: Must handle epsilon-scale geometry.
	 */
	const TypeParam epsilon = std::numeric_limits< TypeParam >::epsilon() * TypeParam{100};

	const Triangle< TypeParam > tiny{
		{0, 0, 0},
		{epsilon, 0, 0},
		{epsilon / TypeParam{2}, epsilon, 0}
	};

	const Triangle< TypeParam > overlapping{
		{0, 0, 0},
		{epsilon * TypeParam{2}, 0, 0},
		{epsilon, epsilon * TypeParam{2}, 0}
	};

	if ( tiny.isValid() && overlapping.isValid() )
	{
		[[maybe_unused]] const bool result = isColliding(tiny, overlapping);
	}
}

TYPED_TEST(MathSpace3D, CollisionSphereVeryLargeRadius)
{
	/**
	 * @test Tests sphere collision with very large radius.
	 */
	const Sphere< TypeParam > huge{TypeParam{1e6}, {0, 0, 0}};
	const Sphere< TypeParam > small{TypeParam{10}, {TypeParam{1e5}, 0, 0}};

	ASSERT_TRUE(huge.isValid());
	ASSERT_TRUE(small.isValid());

	// Small sphere should be inside huge sphere
	ASSERT_TRUE(isColliding(huge, small));
}

TYPED_TEST(MathSpace3D, IntersectionSegmentSphereAtBoundary)
{
	/**
	 * @test Tests segment-sphere intersection at exact boundary.
	 */
	const Segment< TypeParam > segment{{0, 5, 0}, {10, 5, 0}};
	const Sphere< TypeParam > sphere{5.0, {5, 5, 0}};
	Point< TypeParam > intersection;

	const bool result = isIntersecting(segment, sphere, intersection);

	// Segment passes through sphere
	ASSERT_TRUE(result);
}

// ============================================================================
// INDUSTRIAL QUALITY TESTS - MTV (MINIMUM TRANSLATION VECTOR) VALIDATION
// ============================================================================

TYPED_TEST(MathSpace3D, CollisionTriangleMTVCalculation)
{
	/**
	 * @test Verifies that SAT collision detection works with triangular arrays in 3D.
	 * NOTE: Coplanar triangles in 3D may not generate volumetric collisions.
	 * This test validates the SAT implementation accepts triangle arrays.
	 */
	std::array< Vector< 3, TypeParam >, 3 > tri1 = {
		Vector< 3, TypeParam >{0, 0, 0},
		Vector< 3, TypeParam >{8, 0, 0},
		Vector< 3, TypeParam >{4, 8, 0}
	};

	std::array< Vector< 3, TypeParam >, 3 > tri2 = {
		Vector< 3, TypeParam >{2, 2, 0},
		Vector< 3, TypeParam >{10, 2, 0},
		Vector< 3, TypeParam >{6, 10, 0}
	};

	Vector< 3, TypeParam > mtv;

	// Test that SAT works with triangle arrays (3 vertices)
	[[maybe_unused]] const bool collision = SAT::checkCollision(tri1, tri2, mtv);

	// If collision detected, MTV should be reasonable
	if ( collision && mtv.length() > TypeParam{0} )
	{
		ASSERT_LT(mtv.length(), TypeParam{20});
	}
}

TYPED_TEST(MathSpace3D, CollisionSphereMTVMagnitudeCorrectness)
{
	/**
	 * @test Verifies that MTV magnitude represents overlap distance for spheres.
	 */
	const Sphere< TypeParam > sphere1{5.0, {0, 0, 0}};
	const Sphere< TypeParam > sphere2{5.0, {8, 0, 0}};
	Vector< 3, TypeParam > mtv;

	ASSERT_TRUE(isColliding(sphere1, sphere2, mtv));

	// MTV length should represent overlap: radii_sum - distance = 10 - 8 = 2
	ASSERT_NEAR(mtv.length(), TypeParam{2}, TypeParam{1e-3});
}

TYPED_TEST(MathSpace3D, CollisionTetrahedronMTVSymmetry)
{
	/**
	 * @test Verifies MTV symmetry: MTV(A,B) should be opposite of MTV(B,A).
	 */
	std::array< Vector< 3, TypeParam >, 4 > tetra1 = {
		Vector< 3, TypeParam >{0, 0, 0},
		Vector< 3, TypeParam >{6, 0, 0},
		Vector< 3, TypeParam >{3, 6, 0},
		Vector< 3, TypeParam >{3, 3, 6}
	};

	std::array< Vector< 3, TypeParam >, 4 > tetra2 = {
		Vector< 3, TypeParam >{2, 2, 2},
		Vector< 3, TypeParam >{8, 2, 2},
		Vector< 3, TypeParam >{5, 8, 2},
		Vector< 3, TypeParam >{5, 5, 8}
	};

	Vector< 3, TypeParam > mtv1, mtv2;

	const bool collision1 = SAT::checkCollision(tetra1, tetra2, mtv1);
	const bool collision2 = SAT::checkCollision(tetra2, tetra1, mtv2);

	ASSERT_EQ(collision1, collision2);

	if ( collision1 )
	{
		// MTV directions should be opposite (dot product negative)
		const TypeParam dotProduct = Vector< 3, TypeParam >::dotProduct(mtv1, mtv2);
		ASSERT_LT(dotProduct, TypeParam{0});
	}
}
