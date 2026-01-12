/*
 * src/Testing/test_MathBasics.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include "Libs/Math/Base.hpp"

using namespace EmEn::Libs::Math;

// FIXME: Test disabled due to deprecation.
/*TEST(Math, clamp)
{
	EXPECT_EQ(clamp(-0.3, -1.0, 3.0), -0.3);

	EXPECT_EQ(clamp(67, 10, 50), 50);
}*/

TEST(Math, clampToUnit)
{
	EXPECT_EQ(clampToUnit(-0.3), 0.0);

	EXPECT_EQ(clampToUnit(67.0F), 1.0F);

	EXPECT_EQ(clampToUnit(0.333F), 0.333F);
}

TEST(Math, alignCount)
{
	EXPECT_EQ(alignCount(127, 256), 1);

	EXPECT_EQ(alignCount(256, 256), 1);

	EXPECT_EQ(alignCount(298, 256), 2);

	EXPECT_EQ(alignCount(512, 256), 2);

	EXPECT_EQ(alignCount(640, 256), 3);
}

TEST(Math, timesDivisible)
{
	EXPECT_EQ(timesDivisible(33, 2), 0);

	EXPECT_EQ(timesDivisible(32, 2), 5);

	EXPECT_EQ(timesDivisible(64, 3), 0);

	EXPECT_EQ(timesDivisible(90, 3), 2);

	EXPECT_EQ(timesDivisible(80, 4), 2);
}

// ============================================================================
// ANGLE CONVERSIONS
// ============================================================================

TEST(Math, RadianToDegree)
{
	// Test 0°
	EXPECT_NEAR(Radian(0.0), 0.0, 1e-6);

	// Test 90° = π/2
	EXPECT_NEAR(Radian(90.0), std::numbers::pi / 2.0, 1e-6);

	// Test 180° = π
	EXPECT_NEAR(Radian(180.0), std::numbers::pi, 1e-6);

	// Test 360° = 2π
	EXPECT_NEAR(Radian(360.0), 2.0 * std::numbers::pi, 1e-6);

	// Test negative angle
	EXPECT_NEAR(Radian(-90.0), -std::numbers::pi / 2.0, 1e-6);
}

TEST(Math, DegreeToRadian)
{
	// Test 0 radians
	EXPECT_NEAR(Degree(0.0), 0.0, 1e-6);

	// Test π/2 = 90°
	EXPECT_NEAR(Degree(std::numbers::pi / 2.0), 90.0, 1e-6);

	// Test π = 180°
	EXPECT_NEAR(Degree(std::numbers::pi), 180.0, 1e-6);

	// Test 2π = 360°
	EXPECT_NEAR(Degree(2.0 * std::numbers::pi), 360.0, 1e-6);

	// Test negative radian
	EXPECT_NEAR(Degree(-std::numbers::pi / 2.0), -90.0, 1e-6);
}

TEST(Math, RadianDegreeRoundTrip)
{
	// Round trip: Degree -> Radian -> Degree
	constexpr double testAngle = 45.0;
	EXPECT_NEAR(Degree(Radian(testAngle)), testAngle, 1e-6);

	// Round trip: Radian -> Degree -> Radian
	constexpr double testRadian = std::numbers::pi / 4.0;
	EXPECT_NEAR(Radian(Degree(testRadian)), testRadian, 1e-6);
}

// ============================================================================
// ANGLE CLAMPING
// ============================================================================

TEST(Math, clampRadianInRange)
{
	// Test angle already in range
	double angle = std::numbers::pi;
	EXPECT_TRUE(clampRadian(angle));
	EXPECT_NEAR(angle, std::numbers::pi, 1e-6);
}

TEST(Math, clampRadianTooHigh)
{
	// Test angle > 2π
	double angle = 3.0 * std::numbers::pi;
	EXPECT_FALSE(clampRadian(angle));
	EXPECT_NEAR(angle, std::numbers::pi, 1e-6);
}

TEST(Math, clampRadianTooLow)
{
	// Test angle < -2π
	double angle = -3.0 * std::numbers::pi;
	EXPECT_FALSE(clampRadian(angle));
	EXPECT_NEAR(angle, -std::numbers::pi, 1e-6);
}

TEST(Math, getClampedRadianPositive)
{
	constexpr double angle = 3.0 * std::numbers::pi;
	const double clamped = getClampedRadian(angle);
	EXPECT_NEAR(clamped, std::numbers::pi, 1e-6);
}

TEST(Math, getClampedRadianNegative)
{
	constexpr double angle = -3.0 * std::numbers::pi;
	const double clamped = getClampedRadian(angle);
	EXPECT_NEAR(clamped, -std::numbers::pi, 1e-6);
}

TEST(Math, getClampedRadianInRange)
{
	constexpr double angle = std::numbers::pi / 2.0;
	const double clamped = getClampedRadian(angle);
	EXPECT_NEAR(clamped, angle, 1e-6);
}

// ============================================================================
// TRIGONOMETRIC FUNCTIONS
// ============================================================================

TEST(Math, cotan45Degrees)
{
	// cotan(45°) = 1
	EXPECT_NEAR(cotan(45.0), 1.0, 1e-6);
}

TEST(Math, cotan30Degrees)
{
	// cotan(30°) = √3 ≈ 1.732
	EXPECT_NEAR(cotan(30.0), std::sqrt(3.0), 1e-5);
}

TEST(Math, cotan60Degrees)
{
	// cotan(60°) = 1/√3 ≈ 0.577
	EXPECT_NEAR(cotan(60.0), 1.0 / std::sqrt(3.0), 1e-5);
}

TEST(Math, fastCotan45Degrees)
{
	// fastCotan(45°) ≈ 1
	EXPECT_NEAR(fastCotan(45.0), 1.0, 1e-6);
}

TEST(Math, fastCotanVsCotan)
{
	// Verify fastCotan gives similar results to cotan
	for (double angle = 10.0; angle < 80.0; angle += 10.0)
	{
		EXPECT_NEAR(fastCotan(angle), cotan(angle), 1e-5);
	}
}

// ============================================================================
// INTERPOLATION
// ============================================================================

TEST(Math, linearInterpolationStart)
{
	EXPECT_NEAR(linearInterpolation(0.0, 10.0, 0.0), 0.0, 1e-6);
}

TEST(Math, linearInterpolationEnd)
{
	EXPECT_NEAR(linearInterpolation(0.0, 10.0, 1.0), 10.0, 1e-6);
}

TEST(Math, linearInterpolationMidpoint)
{
	EXPECT_NEAR(linearInterpolation(0.0, 10.0, 0.5), 5.0, 1e-6);
}

TEST(Math, linearInterpolationQuarter)
{
	EXPECT_NEAR(linearInterpolation(0.0, 100.0, 0.25), 25.0, 1e-6);
}

TEST(Math, linearInterpolationNegative)
{
	EXPECT_NEAR(linearInterpolation(-10.0, 10.0, 0.5), 0.0, 1e-6);
}

TEST(Math, cosineInterpolationEndpoints)
{
	// At t=0 and t=1, cosine should match linear
	EXPECT_NEAR(cosineInterpolation(0.0, 10.0, 0.0), 0.0, 1e-6);
	EXPECT_NEAR(cosineInterpolation(0.0, 10.0, 1.0), 10.0, 1e-6);
}

TEST(Math, cosineInterpolationMidpoint)
{
	// At t=0.5, cosine should also give midpoint but with smoother curve
	EXPECT_NEAR(cosineInterpolation(0.0, 10.0, 0.5), 5.0, 1e-5);
}

TEST(Math, cubicInterpolationEndpoints)
{
	// Cubic interpolation at endpoints should match second value
	EXPECT_NEAR(cubicInterpolation(0.0, 5.0, 10.0, 15.0, 0.0), 5.0, 1e-5);
	EXPECT_NEAR(cubicInterpolation(0.0, 5.0, 10.0, 15.0, 1.0), 10.0, 1e-5);
}

TEST(Math, cubicCatmullRomInterpolationEndpoints)
{
	EXPECT_NEAR(cubicCatmullRomInterpolation(0.0, 5.0, 10.0, 15.0, 0.0), 5.0, 1e-5);
	EXPECT_NEAR(cubicCatmullRomInterpolation(0.0, 5.0, 10.0, 15.0, 1.0), 10.0, 1e-5);
}

TEST(Math, hermiteInterpolateEndpoints)
{
	// Test with zero tension and bias
	EXPECT_NEAR(hermiteInterpolate(0.0, 5.0, 10.0, 15.0, 0.0, 0.0, 0.0), 5.0, 1e-5);
	EXPECT_NEAR(hermiteInterpolate(0.0, 5.0, 10.0, 15.0, 1.0, 0.0, 0.0), 10.0, 1e-5);
}

// ============================================================================
// NORMALIZATION
// ============================================================================

TEST(Math, normalizePositiveValue)
{
	EXPECT_NEAR(normalize(50.0, 100.0), 0.5, 1e-6);
}

TEST(Math, normalizeFullScale)
{
	EXPECT_NEAR(normalize(255.0, 255.0), 1.0, 1e-6);
}

TEST(Math, normalizeZeroScale)
{
	EXPECT_EQ(normalize(50.0, 0.0), 0.0);
}

TEST(Math, normalizeIntegerToFloat)
{
	// Integer division happens before cast: 128/256 = 0 (int) -> 0.0f
	constexpr float result = normalize< int, float >(128, 256);
	EXPECT_EQ(result, 0.0f);

	// For proper fractional result, need to use floating point input
	constexpr float result2 = normalize< float, float >(128.0f, 256.0f);
	EXPECT_NEAR(result2, 0.5f, 1e-6f);
}

// ============================================================================
// GEOMETRY
// ============================================================================

TEST(Math, circleCircumferenceUnitRadius)
{
	// C = 2πr, for r=1: C = 2π
	EXPECT_NEAR(circleCircumference(1.0), 2.0 * std::numbers::pi, 1e-6);
}

TEST(Math, circleCircumferenceRadius5)
{
	// C = 2πr, for r=5: C = 10π
	EXPECT_NEAR(circleCircumference(5.0), 10.0 * std::numbers::pi, 1e-5);
}

TEST(Math, circleCircumferenceZero)
{
	EXPECT_EQ(circleCircumference(0.0), 0.0);
}

TEST(Math, circleCircumferenceNegative)
{
	EXPECT_EQ(circleCircumference(-5.0), 0.0);
}

TEST(Math, circleAreaUnitRadius)
{
	// A = πr², for r=1: A = π
	EXPECT_NEAR(circleArea(1.0), std::numbers::pi, 1e-6);
}

TEST(Math, circleAreaRadius3)
{
	// A = πr², for r=3: A = 9π
	EXPECT_NEAR(circleArea(3.0), 9.0 * std::numbers::pi, 1e-5);
}

TEST(Math, circleAreaZero)
{
	EXPECT_EQ(circleArea(0.0), 0.0);
}

TEST(Math, sphereVolumeUnitRadius)
{
	// V = 4/3 πr³, for r=1: V = 4/3 π
	EXPECT_NEAR(sphereVolume(1.0), 4.0 / 3.0 * std::numbers::pi, 1e-6);
}

TEST(Math, sphereVolumeRadius2)
{
	// V = 4/3 πr³, for r=2: V = 32/3 π
	EXPECT_NEAR(sphereVolume(2.0), 32.0 / 3.0 * std::numbers::pi, 1e-5);
}

TEST(Math, sphereVolumeZero)
{
	EXPECT_EQ(sphereVolume(0.0), 0.0);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

TEST(Math, deltaPositive)
{
	EXPECT_EQ(delta(10, 5), 5);
}

TEST(Math, deltaNegative)
{
	EXPECT_EQ(delta(5, 10), 5);
}

TEST(Math, deltaZero)
{
	EXPECT_EQ(delta(7, 7), 0);
}

TEST(Math, deltaFloating)
{
	EXPECT_NEAR(delta(3.5, 1.2), 2.3, 1e-6);
}

TEST(Math, differenceEqualValues)
{
	// When values are equal, difference should be 0
	EXPECT_NEAR(difference(5.0, 5.0), 0.0, 1e-6);
}

TEST(Math, differenceDoubleValue)
{
	// 10 vs 5: difference = (10-5)/5 = 1.0 (100% increase)
	EXPECT_NEAR(difference(10.0, 5.0), 1.0, 1e-6);
}

TEST(Math, differenceHalfValue)
{
	// 5 vs 10: difference = (10-5)/5 = 1.0 (100% of smaller value)
	// The function divides by the smaller value, not the larger
	EXPECT_NEAR(difference(5.0, 10.0), 1.0, 1e-6);
}

TEST(Math, reciprocalTwo)
{
	EXPECT_NEAR(reciprocal(2.0), 0.5, 1e-6);
}

TEST(Math, reciprocalFour)
{
	EXPECT_NEAR(reciprocal(4.0), 0.25, 1e-6);
}

TEST(Math, reciprocalTen)
{
	EXPECT_NEAR(reciprocal(10.0), 0.1, 1e-6);
}

TEST(Math, reciprocalSquareRoot4)
{
	// 1/√4 = 1/2 = 0.5
	EXPECT_NEAR(reciprocalSquareRoot(4.0), 0.5, 1e-6);
}

TEST(Math, reciprocalSquareRoot9)
{
	// 1/√9 = 1/3 ≈ 0.333
	EXPECT_NEAR(reciprocalSquareRoot(9.0), 1.0 / 3.0, 1e-6);
}

TEST(Math, reciprocalSquareRoot16)
{
	// 1/√16 = 1/4 = 0.25
	EXPECT_NEAR(reciprocalSquareRoot(16.0), 0.25, 1e-6);
}

TEST(Math, averageEmptyVector)
{
	std::vector< int > empty;
	EXPECT_EQ(average(empty), 0);
}

TEST(Math, averageSingleValue)
{
	std::vector< double > values{5.0};
	EXPECT_NEAR(average(values), 5.0, 1e-6);
}

TEST(Math, averageMultipleValues)
{
	std::vector< double > values{1.0, 2.0, 3.0, 4.0, 5.0};
	EXPECT_NEAR(average(values), 3.0, 1e-6);
}

TEST(Math, averageNegativeValues)
{
	std::vector< int > values{-10, -5, 0, 5, 10};
	EXPECT_EQ(average(values), 0);
}
