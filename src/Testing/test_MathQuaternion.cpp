/*
 * src/Testing/test_MathQuaternion.cpp
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
#include "Libs/Math/Quaternion.hpp"

using namespace EmEn::Libs::Math;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathQuaternion
	: testing::Test
{
};

TYPED_TEST_SUITE(MathQuaternion, MathTypeList);

/* Helper function for floating-point comparison */
template< typename T >
constexpr T
epsilon ()
{
	return static_cast< T >(1e-5);
}

template< typename T >
bool
nearEqual (T a, T b, T eps = epsilon< T >())
{
	return std::abs(a - b) < eps;
}

/* Helper to compare quaternions element-wise */
template< typename T >
void
assertQuaternionNear (const Quaternion< T > & a, const Quaternion< T > & b, T eps = epsilon< T >())
{
	ASSERT_NEAR(a[X], b[X], eps) << "Mismatch at X";
	ASSERT_NEAR(a[Y], b[Y], eps) << "Mismatch at Y";
	ASSERT_NEAR(a[Z], b[Z], eps) << "Mismatch at Z";
	ASSERT_NEAR(a[W], b[W], eps) << "Mismatch at W";
}

// ============================================================================
// CONSTRUCTION AND INITIALIZATION TESTS
// ============================================================================

TYPED_TEST(MathQuaternion, DefaultConstruction)
{
	const auto quat = Quaternion< TypeParam >{};

	ASSERT_EQ(quat[X], TypeParam{0});
	ASSERT_EQ(quat[Y], TypeParam{0});
	ASSERT_EQ(quat[Z], TypeParam{0});
	ASSERT_EQ(quat[W], TypeParam{1});
}

TYPED_TEST(MathQuaternion, ValueConstruction)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};

	ASSERT_EQ(quat[X], TypeParam{1});
	ASSERT_EQ(quat[Y], TypeParam{2});
	ASSERT_EQ(quat[Z], TypeParam{3});
	ASSERT_EQ(quat[W], TypeParam{4});
}

TYPED_TEST(MathQuaternion, ArrayConstruction)
{
	const std::array< TypeParam, 4 > data{5, 6, 7, 8};
	const Quaternion< TypeParam > quat{data};

	ASSERT_EQ(quat[X], TypeParam{5});
	ASSERT_EQ(quat[Y], TypeParam{6});
	ASSERT_EQ(quat[Z], TypeParam{7});
	ASSERT_EQ(quat[W], TypeParam{8});
}

TYPED_TEST(MathQuaternion, Vector3Construction)
{
	const Vector< 3, TypeParam > vec{1, 2, 3};
	const Quaternion< TypeParam > quat{vec, TypeParam{4}};

	ASSERT_EQ(quat[X], TypeParam{1});
	ASSERT_EQ(quat[Y], TypeParam{2});
	ASSERT_EQ(quat[Z], TypeParam{3});
	ASSERT_EQ(quat[W], TypeParam{4});
}

TYPED_TEST(MathQuaternion, Vector4Construction)
{
	const Vector< 4, TypeParam > vec{1, 2, 3, 4};
	const Quaternion< TypeParam > quat{vec};

	ASSERT_EQ(quat[X], TypeParam{1});
	ASSERT_EQ(quat[Y], TypeParam{2});
	ASSERT_EQ(quat[Z], TypeParam{3});
	ASSERT_EQ(quat[W], TypeParam{4});
}

TYPED_TEST(MathQuaternion, IdentityQuaternion)
{
	const auto quat = Quaternion< TypeParam >{};

	// Identity quaternion is (0, 0, 0, 1)
	ASSERT_EQ(quat[X], TypeParam{0});
	ASSERT_EQ(quat[Y], TypeParam{0});
	ASSERT_EQ(quat[Z], TypeParam{0});
	ASSERT_EQ(quat[W], TypeParam{1});
}

TYPED_TEST(MathQuaternion, ResetToIdentity)
{
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	quat.reset();

	ASSERT_EQ(quat[X], TypeParam{0});
	ASSERT_EQ(quat[Y], TypeParam{0});
	ASSERT_EQ(quat[Z], TypeParam{0});
	ASSERT_EQ(quat[W], TypeParam{1});
}

// ============================================================================
// ARITHMETIC OPERATIONS
// ============================================================================

TYPED_TEST(MathQuaternion, Addition)
{
	const Quaternion< TypeParam > a{1, 2, 3, 4};
	const Quaternion< TypeParam > b{5, 6, 7, 8};
	const Quaternion< TypeParam > expected{6, 8, 10, 12};

	const auto result = a + b;
	assertQuaternionNear(result, expected);
}

TYPED_TEST(MathQuaternion, AdditionAssignment)
{
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > other{5, 6, 7, 8};
	const Quaternion< TypeParam > expected{6, 8, 10, 12};

	quat += other;
	assertQuaternionNear(quat, expected);
}

TYPED_TEST(MathQuaternion, Subtraction)
{
	const Quaternion< TypeParam > a{10, 9, 8, 7};
	const Quaternion< TypeParam > b{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{9, 7, 5, 3};

	const auto result = a - b;
	assertQuaternionNear(result, expected);
}

TYPED_TEST(MathQuaternion, SubtractionAssignment)
{
	Quaternion< TypeParam > quat{10, 9, 8, 7};
	const Quaternion< TypeParam > other{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{9, 7, 5, 3};

	quat -= other;
	assertQuaternionNear(quat, expected);
}

TYPED_TEST(MathQuaternion, ScalarMultiplication)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{3, 6, 9, 12};

	const auto result = quat * TypeParam{3};
	assertQuaternionNear(result, expected);
}

TYPED_TEST(MathQuaternion, ScalarMultiplicationAssignment)
{
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{2, 4, 6, 8};

	quat *= TypeParam{2};
	assertQuaternionNear(quat, expected);
}

TYPED_TEST(MathQuaternion, ScalarDivision)
{
	const Quaternion< TypeParam > quat{6, 8, 10, 12};
	const Quaternion< TypeParam > expected{3, 4, 5, 6};

	const auto result = quat / TypeParam{2};
	assertQuaternionNear(result, expected);
}

TYPED_TEST(MathQuaternion, ScalarDivisionAssignment)
{
	Quaternion< TypeParam > quat{10, 20, 30, 40};
	const Quaternion< TypeParam > expected{2, 4, 6, 8};

	quat /= TypeParam{5};
	assertQuaternionNear(quat, expected);
}

TYPED_TEST(MathQuaternion, UnaryPlus)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const auto result = +quat;

	assertQuaternionNear(result, quat);
}

TYPED_TEST(MathQuaternion, UnaryMinus)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{-1, -2, -3, -4};

	const auto result = -quat;
	assertQuaternionNear(result, expected);
}

TYPED_TEST(MathQuaternion, QuaternionProduct)
{
	// Identity quaternion multiplication
	const Quaternion< TypeParam > q1{0, 0, 0, 1};
	const Quaternion< TypeParam > q2{1, 2, 3, 4};

	const auto result = q1 * q2;
	assertQuaternionNear(result, q2);
}

TYPED_TEST(MathQuaternion, QuaternionProductNonCommutative)
{
	// Quaternion multiplication is non-commutative
	const Quaternion< TypeParam > q1{1, 0, 0, 0};
	const Quaternion< TypeParam > q2{0, 1, 0, 0};

	const auto result1 = q1 * q2;
	const auto result2 = q2 * q1;

	// Results should be different (negatives)
	ASSERT_NEAR(result1[X], -result2[X], epsilon< TypeParam >());
	ASSERT_NEAR(result1[Y], -result2[Y], epsilon< TypeParam >());
	ASSERT_NEAR(result1[Z], -result2[Z], epsilon< TypeParam >());
	ASSERT_NEAR(result1[W], -result2[W], epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, QuaternionProductAssignment)
{
	Quaternion< TypeParam > quat{0, 0, 0, 1};
	const Quaternion< TypeParam > other{1, 2, 3, 4};

	quat *= other;
	assertQuaternionNear(quat, other);
}

// ============================================================================
// COMPARISON OPERATIONS
// ============================================================================

TYPED_TEST(MathQuaternion, Equality)
{
	const Quaternion< TypeParam > q1{1, 2, 3, 4};
	const Quaternion< TypeParam > q2{1, 2, 3, 4};

	ASSERT_TRUE(q1 == q2);
}

TYPED_TEST(MathQuaternion, Inequality)
{
	const Quaternion< TypeParam > q1{1, 2, 3, 4};
	const Quaternion< TypeParam > q2{5, 6, 7, 8};

	ASSERT_TRUE(q1 != q2);
}

// ============================================================================
// QUATERNION-SPECIFIC OPERATIONS
// ============================================================================

TYPED_TEST(MathQuaternion, Conjugate)
{
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{-1, -2, -3, 4};

	quat.conjugate();
	assertQuaternionNear(quat, expected);
}

TYPED_TEST(MathQuaternion, Conjugated)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > expected{-1, -2, -3, 4};

	const auto result = quat.conjugated();
	assertQuaternionNear(result, expected);

	// Original should be unchanged
	ASSERT_EQ(quat[X], TypeParam{1});
}

TYPED_TEST(MathQuaternion, Length)
{
	const Quaternion< TypeParam > quat{1, 0, 0, 0};
	const auto length = quat.length();

	ASSERT_NEAR(length, TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, SquaredLength)
{
	const Quaternion< TypeParam > quat{2, 3, 4, 5};
	const auto squaredLength = quat.squaredLength();

	// 2^2 + 3^2 + 4^2 + 5^2 = 4 + 9 + 16 + 25 = 54
	ASSERT_NEAR(squaredLength, TypeParam{54}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, Normalize)
{
	Quaternion< TypeParam > quat{2, 0, 0, 0};
	quat.normalize();

	const auto length = quat.length();
	ASSERT_NEAR(length, TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, Normalized)
{
	const Quaternion< TypeParam > quat{3, 0, 0, 0};
	const auto normalized = quat.normalized();

	const auto length = normalized.length();
	ASSERT_NEAR(length, TypeParam{1}, epsilon< TypeParam >());

	// Original should be unchanged
	ASSERT_EQ(quat[X], TypeParam{3});
}

TYPED_TEST(MathQuaternion, NormalizeIdentity)
{
	// Identity quaternion is already normalized
	Quaternion< TypeParam > quat{0, 0, 0, 1};
	quat.normalize();

	ASSERT_NEAR(quat.length(), TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, Inverse)
{
	// For unit quaternions, inverse = conjugate
	Quaternion< TypeParam > quat{0, 0, 0, 1};
	quat.inverse();

	ASSERT_NEAR(quat[X], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(quat[Y], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(quat[Z], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(quat[W], TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, Inversed)
{
	const Quaternion< TypeParam > quat{0, 0, 0, 1};
	const auto inversed = quat.inversed();

	ASSERT_NEAR(inversed[X], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(inversed[Y], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(inversed[Z], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(inversed[W], TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, DoubleInverse)
{
	// (q^-1)^-1 = q
	const Quaternion< TypeParam > original{1, 2, 3, 4};
	const auto inversed = original.inversed();
	const auto doubleInversed = inversed.inversed();

	assertQuaternionNear(doubleInversed, original, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, DotProduct)
{
	const Quaternion< TypeParam > q1{1, 0, 0, 0};
	const Quaternion< TypeParam > q2{1, 0, 0, 0};

	const auto dot = Quaternion< TypeParam >::dotProduct(q1, q2);
	ASSERT_NEAR(dot, TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, DotProductOrthogonal)
{
	const Quaternion< TypeParam > q1{1, 0, 0, 0};
	const Quaternion< TypeParam > q2{0, 1, 0, 0};

	const auto dot = Quaternion< TypeParam >::dotProduct(q1, q2);
	ASSERT_NEAR(dot, TypeParam{0}, epsilon< TypeParam >());
}

// ============================================================================
// ROTATIONS AND ANGLE-AXIS
// ============================================================================

TYPED_TEST(MathQuaternion, FromAngleAxisIdentity)
{
	// Zero rotation should give identity quaternion
	Quaternion< TypeParam > quat;
	const Vector< 3, TypeParam > axis{0, 1, 0};
	quat.fromAngleAxis(TypeParam{0}, axis);

	ASSERT_NEAR(quat[X], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(quat[Y], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(quat[Z], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(quat[W], TypeParam{1}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, FromAngleAxis90Degrees)
{
	// 90° rotation around Y axis
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2;

	Quaternion< TypeParam > quat;
	const Vector< 3, TypeParam > axis{0, 1, 0};
	quat.fromAngleAxis(Angle, axis);

	// For 90° rotation: sin(45°) ≈ 0.707, cos(45°) ≈ 0.707
	ASSERT_NEAR(quat[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Y], static_cast< TypeParam >(0.707), static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Z], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[W], static_cast< TypeParam >(0.707), static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, ToAngleAxis)
{
	// Create quaternion from known angle-axis
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2; // 90°
	const Vector< 3, TypeParam > inputAxis{0, 1, 0}; // Y-axis

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, inputAxis);

	// Recover angle and axis
	TypeParam outputAngle;
	Vector< 3, TypeParam > outputAxis;
	quat.toAngleAxis(outputAngle, outputAxis);

	// Verify angle
	ASSERT_NEAR(outputAngle, Angle, static_cast< TypeParam >(0.01));

	// Verify axis (should be unit vector along Y)
	ASSERT_NEAR(outputAxis[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(outputAxis[Y], TypeParam{1}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(outputAxis[Z], TypeParam{0}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, EulerAnglesZero)
{
	const Quaternion< TypeParam > quat{TypeParam{0}, TypeParam{0}, TypeParam{0}};

	// Zero rotation should give identity quaternion
	ASSERT_NEAR(quat[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Y], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Z], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[W], TypeParam{1}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, EulerAnglesRoundTrip)
{
	const Vector< 3, TypeParam > inputAngles{
		std::numbers::pi_v< TypeParam > / 6, // 30°
		std::numbers::pi_v< TypeParam > / 4, // 45°
		std::numbers::pi_v< TypeParam > / 3 // 60°
	};

	Quaternion< TypeParam > quat{inputAngles};
	const auto outputAngles = quat.eulerAngles();

	// Note: Euler angles are not unique, so we can't expect exact match
	// but the quaternion should represent the same rotation
	Quaternion< TypeParam > quat2{outputAngles};

	// Compare quaternions with tolerance for double conversion
	assertQuaternionNear(quat, quat2, static_cast< TypeParam >(0.01));
}

// ============================================================================
// VECTOR ROTATION
// ============================================================================

TYPED_TEST(MathQuaternion, RotateVectorIdentity)
{
	const Quaternion< TypeParam > quat{}; // Identity
	const Vector< 3, TypeParam > vec{1, 0, 0};

	const auto result = quat * vec;

	ASSERT_NEAR(result[X], TypeParam{1}, epsilon< TypeParam >());
	ASSERT_NEAR(result[Y], TypeParam{0}, epsilon< TypeParam >());
	ASSERT_NEAR(result[Z], TypeParam{0}, epsilon< TypeParam >());
}

TYPED_TEST(MathQuaternion, RotateVector90DegreesY)
{
	// 90° rotation around Y axis
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2;

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	const Vector< 3, TypeParam > vec{1, 0, 0};
	const auto result = quat * vec;

	// (1,0,0) rotated 90° around Y should be (0,0,-1)
	ASSERT_NEAR(result[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], TypeParam{-1}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, RotatedVector)
{
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2;

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 0, 1});

	const Vector< 3, TypeParam > vec{1, 0, 0};
	// Use operator* instead of rotatedVector to test
	const auto result = quat * vec;

	// (1,0,0) rotated 90° around Z should be (0,1,0)
	ASSERT_NEAR(result[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], TypeParam{1}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], TypeParam{0}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, RotationPreservesLength)
{
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 3;

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, Vector< 3, TypeParam >{1, 1, 1}.normalized());

	const Vector< 3, TypeParam > vec{3, 4, 5};
	const auto originalLength = vec.length();

	const auto result = quat * vec;
	const auto resultLength = result.length();

	ASSERT_NEAR(originalLength, resultLength, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, RotationFromToSameVector)
{
	Quaternion< TypeParam > quat;
	const Vector< 3, TypeParam > vec{1, 0, 0};
	quat.rotationFromTo(vec, vec);

	// Same vector should give identity quaternion
	ASSERT_NEAR(quat[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Y], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Z], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[W], TypeParam{1}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, RotationFromTo90Degrees)
{
	Quaternion< TypeParam > quat;
	const Vector< 3, TypeParam > from{1, 0, 0};
	const Vector< 3, TypeParam > to{0, 1, 0};
	quat.rotationFromTo(from, to);

	// Apply rotation to 'from' vector
	const auto result = quat * from;

	// Result should be close to 'to' vector
	ASSERT_NEAR(result[X], to[X], static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], to[Y], static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], to[Z], static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, RotationFromToOpposite)
{
	Quaternion< TypeParam > quat;
	const Vector< 3, TypeParam > from{1, 0, 0};
	const Vector< 3, TypeParam > to{-1, 0, 0};
	quat.rotationFromTo(from, to);

	// Apply rotation to 'from' vector
	const auto result = quat * from;

	// Result should be close to 'to' vector
	ASSERT_NEAR(result[X], to[X], static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], to[Y], static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], to[Z], static_cast< TypeParam >(0.01));
}

// ============================================================================
// INTERPOLATION
// ============================================================================

TYPED_TEST(MathQuaternion, LerpStartPoint)
{
	const Quaternion< TypeParam > q1{0, 0, 0, 1};
	const Quaternion< TypeParam > q2{1, 0, 0, 0};

	const auto result = Quaternion< TypeParam >::lerp(q1, q2, TypeParam{0});

	assertQuaternionNear(result, q1);
}

TYPED_TEST(MathQuaternion, LerpEndPoint)
{
	const Quaternion< TypeParam > q1{0, 0, 0, 1};
	const Quaternion< TypeParam > q2{1, 0, 0, 0};

	const auto result = Quaternion< TypeParam >::lerp(q1, q2, TypeParam{1});

	assertQuaternionNear(result, q2);
}

TYPED_TEST(MathQuaternion, LerpMidpoint)
{
	const Quaternion< TypeParam > q1{0, 0, 0, 1};
	const Quaternion< TypeParam > q2{1, 0, 0, 1};

	const auto result = Quaternion< TypeParam >::lerp(q1, q2, static_cast< TypeParam >(0.5));

	// Midpoint should be average
	ASSERT_NEAR(result[X], static_cast< TypeParam >(0.5), static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[W], TypeParam{1}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, SlerpStartPoint)
{
	const Quaternion< TypeParam > q1{0, 0, 0, 1};
	const Quaternion< TypeParam > q2{1, 0, 0, 0};

	// Use 4-parameter version to avoid ambiguity
	const auto result = Quaternion< TypeParam >::slerp(q1, q2, TypeParam{0}, static_cast< TypeParam >(0.05));

	assertQuaternionNear(result, q1, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, SlerpEndPoint)
{
	const Quaternion< TypeParam > q1{0, 0, 0, 1};
	const Quaternion< TypeParam > q2{1, 0, 0, 0};

	// Use 4-parameter version to avoid ambiguity
	const auto result = Quaternion< TypeParam >::slerp(q1, q2, TypeParam{1}, static_cast< TypeParam >(0.05));

	assertQuaternionNear(result, q2, static_cast< TypeParam >(0.01));
}

// ============================================================================
// MATRIX CONVERSIONS
// ============================================================================

TYPED_TEST(MathQuaternion, RotationMatrixIdentity)
{
	const Quaternion< TypeParam > quat{0, 0, 0, 1}; // Identity quaternion
	const auto matrix = quat.rotationMatrix();

	// Identity quaternion should produce identity matrix
	ASSERT_NEAR(matrix[0], TypeParam{1}, static_cast< TypeParam >(0.01)); // m00
	ASSERT_NEAR(matrix[1], TypeParam{0}, static_cast< TypeParam >(0.01)); // m01
	ASSERT_NEAR(matrix[2], TypeParam{0}, static_cast< TypeParam >(0.01)); // m02
	ASSERT_NEAR(matrix[3], TypeParam{0}, static_cast< TypeParam >(0.01)); // m10
	ASSERT_NEAR(matrix[4], TypeParam{1}, static_cast< TypeParam >(0.01)); // m11
	ASSERT_NEAR(matrix[5], TypeParam{0}, static_cast< TypeParam >(0.01)); // m12
	ASSERT_NEAR(matrix[6], TypeParam{0}, static_cast< TypeParam >(0.01)); // m20
	ASSERT_NEAR(matrix[7], TypeParam{0}, static_cast< TypeParam >(0.01)); // m21
	ASSERT_NEAR(matrix[8], TypeParam{1}, static_cast< TypeParam >(0.01)); // m22
}

TYPED_TEST(MathQuaternion, RotationMatrix90DegreesZ)
{
	// Create quaternion for 90° rotation around Z axis
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2; // 90°
	const Vector< 3, TypeParam > axis{0, 0, 1}; // Z-axis

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, axis);

	const auto matrix = quat.rotationMatrix();

	// 90° Z rotation should give: [0 -1 0; 1 0 0; 0 0 1]
	ASSERT_NEAR(matrix[0], TypeParam{0}, static_cast< TypeParam >(0.01)); // m00
	ASSERT_NEAR(matrix[1], TypeParam{-1}, static_cast< TypeParam >(0.01)); // m01
	ASSERT_NEAR(matrix[2], TypeParam{0}, static_cast< TypeParam >(0.01)); // m02
	ASSERT_NEAR(matrix[3], TypeParam{1}, static_cast< TypeParam >(0.01)); // m10
	ASSERT_NEAR(matrix[4], TypeParam{0}, static_cast< TypeParam >(0.01)); // m11
	ASSERT_NEAR(matrix[5], TypeParam{0}, static_cast< TypeParam >(0.01)); // m12
	ASSERT_NEAR(matrix[6], TypeParam{0}, static_cast< TypeParam >(0.01)); // m20
	ASSERT_NEAR(matrix[7], TypeParam{0}, static_cast< TypeParam >(0.01)); // m21
	ASSERT_NEAR(matrix[8], TypeParam{1}, static_cast< TypeParam >(0.01)); // m22
}

TYPED_TEST(MathQuaternion, FromMatrix)
{
	// Create a known rotation matrix (90° around Z)
	Matrix< 4, TypeParam > matrix{
		0, -1, 0, 0,
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};

	const Quaternion< TypeParam > quat{matrix};

	// Verify the quaternion represents a 90° Z rotation
	// For 90° Z rotation: q = [0, 0, sin(45°), cos(45°)] = [0, 0, 0.707, 0.707]
	ASSERT_NEAR(quat[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Y], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[Z], static_cast< TypeParam >(0.707), static_cast< TypeParam >(0.01));
	ASSERT_NEAR(quat[W], static_cast< TypeParam >(0.707), static_cast< TypeParam >(0.01));
}

// ============================================================================
// ACCESSORS AND SETTERS
// ============================================================================

TYPED_TEST(MathQuaternion, ComplexGetter)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const auto complex = quat.complex();

	ASSERT_EQ(complex[X], TypeParam{1});
	ASSERT_EQ(complex[Y], TypeParam{2});
	ASSERT_EQ(complex[Z], TypeParam{3});
}

TYPED_TEST(MathQuaternion, RealGetter)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const auto real = quat.real();

	ASSERT_EQ(real, TypeParam{4});
}

TYPED_TEST(MathQuaternion, SetComplex)
{
	Quaternion< TypeParam > quat{};
	quat.setComplex(TypeParam{1}, TypeParam{2}, TypeParam{3});

	ASSERT_EQ(quat[X], TypeParam{1});
	ASSERT_EQ(quat[Y], TypeParam{2});
	ASSERT_EQ(quat[Z], TypeParam{3});
	ASSERT_EQ(quat[W], TypeParam{1}); // W unchanged
}

TYPED_TEST(MathQuaternion, SetComplexVector)
{
	Quaternion< TypeParam > quat{};
	const Vector< 3, TypeParam > vec{5, 6, 7};
	quat.setComplex(vec);

	ASSERT_EQ(quat[X], TypeParam{5});
	ASSERT_EQ(quat[Y], TypeParam{6});
	ASSERT_EQ(quat[Z], TypeParam{7});
}

TYPED_TEST(MathQuaternion, SetReal)
{
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	quat.setReal(TypeParam{10});

	ASSERT_EQ(quat[W], TypeParam{10});
	ASSERT_EQ(quat[X], TypeParam{1}); // X unchanged
}

TYPED_TEST(MathQuaternion, SetFromVector)
{
	Quaternion< TypeParam > quat{};
	const Vector< 4, TypeParam > vec{1, 2, 3, 4};
	quat.set(vec);

	ASSERT_EQ(quat[X], TypeParam{1});
	ASSERT_EQ(quat[Y], TypeParam{2});
	ASSERT_EQ(quat[Z], TypeParam{3});
	ASSERT_EQ(quat[W], TypeParam{4});
}

TYPED_TEST(MathQuaternion, GetAsVector4)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const auto vec = quat.getAsVector4();

	ASSERT_EQ(vec[X], TypeParam{1});
	ASSERT_EQ(vec[Y], TypeParam{2});
	ASSERT_EQ(vec[Z], TypeParam{3});
	ASSERT_EQ(vec[W], TypeParam{4});
}

// ============================================================================
// EDGE CASES AND ROBUSTNESS
// ============================================================================

TYPED_TEST(MathQuaternion, DivisionByZero)
{
	const Quaternion< TypeParam > quat{1, 2, 3, 4};
	const auto result = quat / TypeParam{0};

	// Should return identity quaternion
	ASSERT_EQ(result[X], TypeParam{0});
	ASSERT_EQ(result[Y], TypeParam{0});
	ASSERT_EQ(result[Z], TypeParam{0});
	ASSERT_EQ(result[W], TypeParam{1});
}

TYPED_TEST(MathQuaternion, DivisionByZeroAssignment)
{
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	const Quaternion< TypeParam > original = quat;

	quat /= TypeParam{0};

	// Should remain unchanged
	assertQuaternionNear(quat, original);
}

TYPED_TEST(MathQuaternion, NormalizeZeroQuaternion)
{
	Quaternion< TypeParam > quat{0, 0, 0, 0};
	quat.normalize();

	// Should remain unchanged (zero length)
	ASSERT_EQ(quat[X], TypeParam{0});
	ASSERT_EQ(quat[Y], TypeParam{0});
	ASSERT_EQ(quat[Z], TypeParam{0});
	ASSERT_EQ(quat[W], TypeParam{0});
}

TYPED_TEST(MathQuaternion, NormalizedZeroQuaternion)
{
	const Quaternion< TypeParam > quat{0, 0, 0, 0};
	const auto result = quat.normalized();

	// Should return identity quaternion
	ASSERT_EQ(result[X], TypeParam{0});
	ASSERT_EQ(result[Y], TypeParam{0});
	ASSERT_EQ(result[Z], TypeParam{0});
	ASSERT_EQ(result[W], TypeParam{1});
}

TYPED_TEST(MathQuaternion, InverseZeroQuaternion)
{
	Quaternion< TypeParam > quat{0, 0, 0, 0};
	quat.inverse();

	// Should remain unchanged (no valid inverse)
	ASSERT_EQ(quat[X], TypeParam{0});
	ASSERT_EQ(quat[Y], TypeParam{0});
	ASSERT_EQ(quat[Z], TypeParam{0});
	ASSERT_EQ(quat[W], TypeParam{0});
}

TYPED_TEST(MathQuaternion, InversedZeroQuaternion)
{
	const Quaternion< TypeParam > quat{0, 0, 0, 0};
	const auto result = quat.inversed();

	// Should return identity quaternion
	ASSERT_EQ(result[X], TypeParam{0});
	ASSERT_EQ(result[Y], TypeParam{0});
	ASSERT_EQ(result[Z], TypeParam{0});
	ASSERT_EQ(result[W], TypeParam{1});
}

TYPED_TEST(MathQuaternion, QuaternionProductIdentity)
{
	const Quaternion< TypeParam > identity{};
	const Quaternion< TypeParam > quat{1, 2, 3, 4};

	const auto result1 = identity * quat;
	const auto result2 = quat * identity;

	assertQuaternionNear(result1, quat);
	assertQuaternionNear(result2, quat);
}

TYPED_TEST(MathQuaternion, ConjugateInverseUnitQuaternion)
{
	// For unit quaternions, conjugate == inverse
	Quaternion< TypeParam > quat{1, 2, 3, 4};
	quat.normalize();

	const auto conjugated = quat.conjugated();
	const auto inversed = quat.inversed();

	assertQuaternionNear(conjugated, inversed, static_cast< TypeParam >(0.01));
}

// ============================================================================
// REAL-WORLD SCENARIOS
// ============================================================================

TYPED_TEST(MathQuaternion, CompositeRotations)
{
	// Rotate 90° around Y, then 90° around Z
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2;

	Quaternion< TypeParam > rotY;
	rotY.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	Quaternion< TypeParam > rotZ;
	rotZ.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 0, 1});

	const auto combined = rotZ * rotY;

	// Test on vector (1,0,0)
	const Vector< 3, TypeParam > vec{1, 0, 0};
	const auto result = combined * vec;

	// After Y rotation: (0,0,-1), after Z rotation: (0,0,-1) stays same
	ASSERT_NEAR(result[X], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], TypeParam{0}, static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], TypeParam{-1}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, UnitQuaternionProperty)
{
	// Unit quaternions should have length 1
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4;

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	ASSERT_NEAR(quat.length(), TypeParam{1}, static_cast< TypeParam >(0.01));
}

TYPED_TEST(MathQuaternion, DoubleRotation180)
{
	// Rotating 180° twice should give identity
	constexpr auto Angle = std::numbers::pi_v< TypeParam >;

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	const auto combined = quat * quat;

	// Should be close to identity (within tolerance for double 180° rotation)
	const Vector< 3, TypeParam > vec{1, 0, 0};
	const auto result = combined * vec;

	ASSERT_NEAR(result[X], vec[X], static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Y], vec[Y], static_cast< TypeParam >(0.01));
	ASSERT_NEAR(result[Z], vec[Z], static_cast< TypeParam >(0.01));
}
