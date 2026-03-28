/*
 * src/Testing/test_MathTransformConversions.cpp
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
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/TransformUtils.hpp"

using namespace EmEn::Libs::Math;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathTransformConversions
	: testing::Test
{
};

TYPED_TEST_SUITE(MathTransformConversions, MathTypeList);

/* Helper: epsilon for floating-point comparison. */
template< typename T >
constexpr T
testEpsilon ()
{
	return static_cast< T >(1e-4);
}

/* Helper: compare two quaternions accounting for double-cover (q == -q). */
template< typename T >
void
assertQuaternionEquivalent (const Quaternion< T > & a, const Quaternion< T > & b, T eps = testEpsilon< T >())
{
	/* q and -q represent the same rotation. Check both. */
	const bool directMatch =
		std::abs(a[X] - b[X]) < eps &&
		std::abs(a[Y] - b[Y]) < eps &&
		std::abs(a[Z] - b[Z]) < eps &&
		std::abs(a[W] - b[W]) < eps;

	const bool negatedMatch =
		std::abs(a[X] + b[X]) < eps &&
		std::abs(a[Y] + b[Y]) < eps &&
		std::abs(a[Z] + b[Z]) < eps &&
		std::abs(a[W] + b[W]) < eps;

	ASSERT_TRUE(directMatch || negatedMatch)
		<< "Quaternion mismatch:\n"
		<< "  a = (" << a[X] << ", " << a[Y] << ", " << a[Z] << ", " << a[W] << ")\n"
		<< "  b = (" << b[X] << ", " << b[Y] << ", " << b[Z] << ", " << b[W] << ")";
}

/* Helper: compare two vectors. */
template< size_t dim_t, typename T >
void
assertVectorNear (const Vector< dim_t, T > & a, const Vector< dim_t, T > & b, T eps = testEpsilon< T >())
{
	for ( size_t i = 0; i < dim_t; ++i )
	{
		ASSERT_NEAR(a[i], b[i], eps) << "Vector mismatch at index " << i;
	}
}

/* Helper: compare two matrices. */
template< size_t dim_t, typename T >
void
assertMatrixNear (const Matrix< dim_t, T > & a, const Matrix< dim_t, T > & b, T eps = testEpsilon< T >())
{
	for ( size_t i = 0; i < dim_t * dim_t; ++i )
	{
		ASSERT_NEAR(a[i], b[i], eps) << "Matrix element mismatch at index " << i;
	}
}

// ============================================================================
// QUATERNION -> MATRIX4 -> QUATERNION ROUNDTRIP
// ============================================================================

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_Identity)
{
	const Quaternion< TypeParam > original{0, 0, 0, 1};
	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_90Z)
{
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2;

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 0, 1});

	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_45XYZ)
{
	/* Arbitrary rotation: 45 degrees around (1, 1, 1) normalized. */
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4;
	auto axis = Vector< 3, TypeParam >{1, 1, 1}.normalize();

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, axis);

	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_180X)
{
	/* Edge case: 180 degrees rotation (w near 0). */
	constexpr auto Angle = std::numbers::pi_v< TypeParam >;

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, Vector< 3, TypeParam >{1, 0, 0});

	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_180Y)
{
	constexpr auto Angle = std::numbers::pi_v< TypeParam >;

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_180Z)
{
	constexpr auto Angle = std::numbers::pi_v< TypeParam >;

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 0, 1});

	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_SmallAngle)
{
	/* Near-identity rotation: 0.01 radians around Y. */
	constexpr auto Angle = static_cast< TypeParam >(0.01);

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	const auto mat4 = original.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	assertQuaternionEquivalent(original, restored);
}

TYPED_TEST(MathTransformConversions, QuatToMat4ToQuat_NegatedQuaternion)
{
	/* -q represents the same rotation as q. */
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 3;

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, Vector< 3, TypeParam >{0, 1, 0});

	const auto negated = -original;
	const auto mat4 = negated.toRotationMatrix4();
	const Quaternion< TypeParam > restored{mat4};

	/* Both should represent the same rotation. */
	assertQuaternionEquivalent(original, restored);
}

// ============================================================================
// QUATERNION -> MATRIX3 vs MATRIX4 CONSISTENCY
// ============================================================================

TYPED_TEST(MathTransformConversions, ToRotationMatrix4_MatchesCartesianFrame)
{
	/* toRotationMatrix4() must produce the same matrix as CartesianFrame(pos, quat).getRotationMatrix4(). */
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 3;
	auto axis = Vector< 3, TypeParam >{1, 2, 3}.normalize();

	Quaternion< TypeParam > quat;
	quat.fromAngleAxis(Angle, axis);

	const auto mat4 = quat.toRotationMatrix4();
	const CartesianFrame< TypeParam > frame{{0, 0, 0}, quat};
	const auto frameRot = frame.getRotationMatrix4();

	constexpr auto eps = testEpsilon< TypeParam >();

	/* Compare all 16 elements. */
	for ( size_t i = 0; i < 16; ++i )
	{
		ASSERT_NEAR(mat4[i], frameRot[i], eps) << "Mismatch at index " << i;
	}

	/* The 4th row and column should be identity. */
	ASSERT_NEAR(mat4[M4x4Col0Row3], TypeParam{0}, eps);
	ASSERT_NEAR(mat4[M4x4Col1Row3], TypeParam{0}, eps);
	ASSERT_NEAR(mat4[M4x4Col2Row3], TypeParam{0}, eps);
	ASSERT_NEAR(mat4[M4x4Col3Row0], TypeParam{0}, eps);
	ASSERT_NEAR(mat4[M4x4Col3Row1], TypeParam{0}, eps);
	ASSERT_NEAR(mat4[M4x4Col3Row2], TypeParam{0}, eps);
	ASSERT_NEAR(mat4[M4x4Col3Row3], TypeParam{1}, eps);
}

// ============================================================================
// CARTESIANFRAME <-> QUATERNION ROUNDTRIP
// ============================================================================

TYPED_TEST(MathTransformConversions, CartesianFrameToQuatToFrame_Identity)
{
	const CartesianFrame< TypeParam > original{};
	const auto quat = original.toQuaternion();
	const CartesianFrame< TypeParam > restored{{0, 0, 0}, quat};

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(original.position(), restored.position(), eps);
	assertVectorNear(original.downwardVector(), restored.downwardVector(), eps);
	assertVectorNear(original.backwardVector(), restored.backwardVector(), eps);
}

TYPED_TEST(MathTransformConversions, CartesianFrameToQuatToFrame_Rotated)
{
	/* Create a frame rotated 60 degrees around X. */
	CartesianFrame< TypeParam > original{};
	original.rotate(std::numbers::pi_v< TypeParam > / 3, Vector< 3, TypeParam >{1, 0, 0}, false);

	const auto quat = original.toQuaternion();
	const CartesianFrame< TypeParam > restored{original.position(), quat, original.scalingFactor()};

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(original.downwardVector(), restored.downwardVector(), eps);
	assertVectorNear(original.backwardVector(), restored.backwardVector(), eps);
}

TYPED_TEST(MathTransformConversions, CartesianFrameToQuatToFrame_WithPositionAndScale)
{
	const Vector< 3, TypeParam > pos{10, -5, 3};
	const Vector< 3, TypeParam > scale{2, 0.5, 3};

	/* Construct directly with a quaternion rotation to avoid rotate() side effects on position. */
	Quaternion< TypeParam > rot;
	rot.fromAngleAxis(static_cast< TypeParam >(0.7), Vector< 3, TypeParam >{0, 1, 0});

	const CartesianFrame< TypeParam > original{pos, rot, scale};

	const auto quat = original.toQuaternion();
	const CartesianFrame< TypeParam > restored{pos, quat, scale};

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(original.position(), restored.position(), eps);
	assertVectorNear(original.scalingFactor(), restored.scalingFactor(), eps);
	assertVectorNear(original.downwardVector(), restored.downwardVector(), eps);
	assertVectorNear(original.backwardVector(), restored.backwardVector(), eps);
}

TYPED_TEST(MathTransformConversions, QuatToCartesianFrameToQuat)
{
	constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4;
	auto axis = Vector< 3, TypeParam >{1, 1, 0}.normalize();

	Quaternion< TypeParam > original;
	original.fromAngleAxis(Angle, axis);

	const CartesianFrame< TypeParam > frame{{0, 0, 0}, original};
	const auto restored = frame.toQuaternion();

	assertQuaternionEquivalent(original, restored);
}

// ============================================================================
// CARTESIANFRAME <-> MATRIX4 ROUNDTRIP (via Quaternion path)
// ============================================================================

TYPED_TEST(MathTransformConversions, CartesianFrameModelMatrixRoundtrip)
{
	/* Build a frame, get model matrix, decompose, rebuild, compare model matrix. */
	const Vector< 3, TypeParam > pos{5, -3, 7};
	const Vector< 3, TypeParam > scale{1.5, 2.0, 0.8};

	CartesianFrame< TypeParam > original{pos};
	original.setScalingFactor(Vector< 3, TypeParam >{scale});
	original.rotate(static_cast< TypeParam >(1.2), Vector< 3, TypeParam >{0, 0, 1}, false);
	original.rotate(static_cast< TypeParam >(0.5), Vector< 3, TypeParam >{1, 0, 0}, false);

	const auto modelMatrix = original.getModelMatrix();
	const auto trs = decomposeTRS(modelMatrix);
	const auto recomposed = composeTRS(trs.translation, trs.rotation, trs.scale);

	assertMatrixNear(modelMatrix, recomposed);
}

// ============================================================================
// COMPOSE / DECOMPOSE TRS ROUNDTRIP
// ============================================================================

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_Identity)
{
	const Vector< 3, TypeParam > T{0, 0, 0};
	const Quaternion< TypeParam > R{0, 0, 0, 1};
	const Vector< 3, TypeParam > S{1, 1, 1};

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_TranslationOnly)
{
	const Vector< 3, TypeParam > T{10, -20, 30};
	const Quaternion< TypeParam > R{0, 0, 0, 1};
	const Vector< 3, TypeParam > S{1, 1, 1};

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_RotationOnly)
{
	const Vector< 3, TypeParam > T{0, 0, 0};
	const Vector< 3, TypeParam > S{1, 1, 1};

	Quaternion< TypeParam > R;
	R.fromAngleAxis(std::numbers::pi_v< TypeParam > / 6, Vector< 3, TypeParam >{0, 1, 0});

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_ScaleOnly)
{
	const Vector< 3, TypeParam > T{0, 0, 0};
	const Quaternion< TypeParam > R{0, 0, 0, 1};
	const Vector< 3, TypeParam > S{2, 3, 0.5};

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_FullTRS)
{
	const Vector< 3, TypeParam > T{-5, 12, 0.3};
	const Vector< 3, TypeParam > S{2, 0.5, 1.5};

	Quaternion< TypeParam > R;
	R.fromAngleAxis(static_cast< TypeParam >(1.3), Vector< 3, TypeParam >{1, 0, 0});

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_NonUniformScale)
{
	const Vector< 3, TypeParam > T{1, 2, 3};
	const Vector< 3, TypeParam > S{0.1, 5.0, 0.01};

	Quaternion< TypeParam > R;
	R.fromAngleAxis(static_cast< TypeParam >(2.5), Vector< 3, TypeParam >{0, 0, 1});

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, DecomposeThenCompose_CartesianFrameMatrix)
{
	/* Start from a CartesianFrame model matrix, decompose, recompose, compare. */
	CartesianFrame< TypeParam > frame{{3, -1, 7}};
	frame.setScalingFactor(static_cast< TypeParam >(1.5), static_cast< TypeParam >(1.5), static_cast< TypeParam >(1.5));
	frame.rotate(static_cast< TypeParam >(0.8), Vector< 3, TypeParam >{0, 1, 0}, false);

	const auto original = frame.getModelMatrix();
	const auto trs = decomposeTRS(original);
	const auto recomposed = composeTRS(trs.translation, trs.rotation, trs.scale);

	assertMatrixNear(original, recomposed);
}

// ============================================================================
// COMPOSE TRS vs CARTESIANFRAME MODEL MATRIX
// ============================================================================

TYPED_TEST(MathTransformConversions, ComposeTRSMatchesCartesianFrameModelMatrix)
{
	/* composeTRS should produce the same matrix as CartesianFrame::getModelMatrix()
	 * when given the same TRS components. */
	const Vector< 3, TypeParam > pos{4, -2, 8};
	const Vector< 3, TypeParam > scale{1.2, 0.8, 2.0};

	Quaternion< TypeParam > rotation;
	rotation.fromAngleAxis(static_cast< TypeParam >(0.9), Vector< 3, TypeParam >{0, 0, 1});

	/* Build via CartesianFrame. */
	const CartesianFrame< TypeParam > frame{pos, rotation, scale};
	const auto frameMatrix = frame.getModelMatrix();

	/* Build via composeTRS. */
	const auto composedMatrix = composeTRS(pos, rotation, scale);

	assertMatrixNear(frameMatrix, composedMatrix);
}

TYPED_TEST(MathTransformConversions, ComposeTRSMatchesCartesianFrame_ArbitraryRotation)
{
	const Vector< 3, TypeParam > pos{-10, 0.5, 20};
	const Vector< 3, TypeParam > scale{3.0, 3.0, 3.0};

	Quaternion< TypeParam > rotation;
	rotation.fromAngleAxis(static_cast< TypeParam >(1.7), Vector< 3, TypeParam >{1, 1, 1}.normalize());

	const CartesianFrame< TypeParam > frame{pos, rotation, scale};
	const auto frameMatrix = frame.getModelMatrix();
	const auto composedMatrix = composeTRS(pos, rotation, scale);

	assertMatrixNear(frameMatrix, composedMatrix);
}

// ============================================================================
// 180-DEGREE EDGE CASES
// ============================================================================

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_180AroundX)
{
	const Vector< 3, TypeParam > T{1, 2, 3};
	const Vector< 3, TypeParam > S{1, 1, 1};

	Quaternion< TypeParam > R;
	R.fromAngleAxis(std::numbers::pi_v< TypeParam >, Vector< 3, TypeParam >{1, 0, 0});

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

TYPED_TEST(MathTransformConversions, ComposeThenDecompose_NearlyOppositeRotation)
{
	/* 179 degrees around an arbitrary axis — tests near-180° without hitting the singularity. */
	const Vector< 3, TypeParam > T{0, 0, 0};
	const Vector< 3, TypeParam > S{2, 2, 2};

	Quaternion< TypeParam > R;
	R.fromAngleAxis(std::numbers::pi_v< TypeParam > * static_cast< TypeParam >(0.99), Vector< 3, TypeParam >{1, 1, 0}.normalize());

	const auto matrix = composeTRS(T, R, S);
	const auto trs = decomposeTRS(matrix);

	constexpr auto eps = testEpsilon< TypeParam >();

	assertVectorNear(trs.translation, T, eps);
	assertQuaternionEquivalent(trs.rotation, R, eps);
	assertVectorNear(trs.scale, S, eps);
}

// ============================================================================
// MULTIPLE SEQUENTIAL ROUNDTRIPS (DRIFT TEST)
// ============================================================================

TYPED_TEST(MathTransformConversions, MultipleRoundtrips_NoDrift)
{
	/* Compose -> decompose 10 times and check for accumulated error. */
	Vector< 3, TypeParam > T{7, -3, 12};
	Quaternion< TypeParam > R;
	R.fromAngleAxis(static_cast< TypeParam >(0.42), Vector< 3, TypeParam >{0, 1, 0});
	Vector< 3, TypeParam > S{1.5, 2.0, 0.75};

	for ( int i = 0; i < 10; ++i )
	{
		const auto matrix = composeTRS(T, R, S);
		const auto trs = decomposeTRS(matrix);

		T = trs.translation;
		R = trs.rotation;
		S = trs.scale;
	}

	/* After 10 roundtrips, values should remain stable. */
	constexpr auto eps = static_cast< TypeParam >(1e-3);

	assertVectorNear(T, Vector< 3, TypeParam >{7, -3, 12}, eps);
	assertVectorNear(S, Vector< 3, TypeParam >{1.5, 2.0, 0.75}, eps);

	/* Check rotation by applying to a test vector. */
	Quaternion< TypeParam > originalR;
	originalR.fromAngleAxis(static_cast< TypeParam >(0.42), Vector< 3, TypeParam >{0, 1, 0});

	const auto testVec = Vector< 3, TypeParam >{1, 0, 0};
	const auto originalResult = originalR * testVec;
	const auto driftedResult = R * testVec;

	assertVectorNear(originalResult, driftedResult, eps);
}
