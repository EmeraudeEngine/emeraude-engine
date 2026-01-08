/*
 * src/Testing/test_MathMatrix.cpp
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
#include "Libs/Math/Matrix.hpp"

using namespace EmEn::Libs::Math;

using MathTypeList = testing::Types< int, float, double >;

template< typename >
struct MathMatrix
	: testing::Test
{
};

TYPED_TEST_SUITE(MathMatrix, MathTypeList);

/* Helper function for floating-point comparison */
template< typename T >
constexpr T
epsilon ()
{
	if constexpr ( std::is_floating_point_v< T > )
	{
		return static_cast< T >(1e-5);
	}
	else
	{
		return T{0};
	}
}

template< typename T >
bool
nearEqual (T a, T b)
{
	if constexpr ( std::is_floating_point_v< T > )
	{
		return std::abs(a - b) < epsilon< T >();
	}
	else
	{
		return a == b;
	}
}

/* Helper to compare matrices element-wise */
template< size_t dim, typename T >
void
assertMatrixNear (const Matrix< dim, T > & a, const Matrix< dim, T > & b, T eps = epsilon< T >())
{
	for ( size_t i = 0; i < dim * dim; ++i )
	{
		if constexpr ( std::is_floating_point_v< T > )
		{
			ASSERT_NEAR(a[i], b[i], eps) << "Mismatch at index " << i;
		}
		else
		{
			ASSERT_EQ(a[i], b[i]) << "Mismatch at index " << i;
		}
	}
}

// ============================================================================
// CONSTRUCTION AND INITIALIZATION TESTS
// ============================================================================

TYPED_TEST(MathMatrix, Matrix2Default)
{
	const std::array< TypeParam, 4 > identity{
		1, 0,
		0, 1};

	auto test = Matrix< 2, TypeParam >{};

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}

	test = Matrix< 2, TypeParam >::identity();

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}

	test.reset();

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}
}

TYPED_TEST(MathMatrix, Matrix3Default)
{
	const std::array< TypeParam, 9 > identity{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1};

	auto test = Matrix< 3, TypeParam >{};

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}

	test = Matrix< 3, TypeParam >::identity();

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}

	test.reset();

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}
}

TYPED_TEST(MathMatrix, Matrix4Default)
{
	const std::array< TypeParam, 16 > identity{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};

	auto test = Matrix< 4, TypeParam >{};

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}

	test = Matrix< 4, TypeParam >::identity();

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}

	test.reset();

	for ( size_t i = 0; i < identity.size(); ++i )
	{
		ASSERT_EQ(test[i], identity[i]);
	}
}

TYPED_TEST(MathMatrix, Constructors2)
{
	const Matrix< 2, TypeParam > columnMajor{{0, 2,
											  1, 3}};

	const Matrix< 2, TypeParam > rowMajor{
		0, 1,
		2, 3};

	for ( size_t i = 0; i < 4; ++i )
	{
		ASSERT_EQ(columnMajor[i], rowMajor[i]);
	}
}

TYPED_TEST(MathMatrix, Constructors3)
{
	const Matrix< 3, TypeParam > columnMajor{{0, 3, 6,
											  1, 4, 7,
											  2, 5, 8}};

	const Matrix< 3, TypeParam > rowMajor{
		0, 1, 2,
		3, 4, 5,
		6, 7, 8};

	for ( size_t i = 0; i < 9; ++i )
	{
		ASSERT_EQ(columnMajor[i], rowMajor[i]);
	}
}

TYPED_TEST(MathMatrix, Constructors4)
{
	const Matrix< 4, TypeParam > columnMajor{{0, 4, 8, 12,
											  1, 5, 9, 13,
											  2, 6, 10, 14,
											  3, 7, 11, 15}};

	const Matrix< 4, TypeParam > rowMajor{
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15};

	for ( size_t i = 0; i < 16; ++i )
	{
		ASSERT_EQ(columnMajor[i], rowMajor[i]);
	}
}

TYPED_TEST(MathMatrix, Rotation2)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		/* NOTE: 2D rotation (or around Z axis) */
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 6; // 30°
		const auto rotation = Matrix< 2, TypeParam >::rotationZ(Angle);
		const auto rotationCustom = Matrix< 3, TypeParam >::rotation(Angle, 0, 0, 1).toMatrix2();

		for ( size_t i = 0; i < 4; ++i )
		{
			ASSERT_EQ(rotation[i], rotationCustom[i]);
		}
	}
}

TYPED_TEST(MathMatrix, Rotation3)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		/* NOTE: 3D rotation around X axis. */
		{
			constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4; // 45°
			const auto rotation = Matrix< 3, TypeParam >::rotationX(Angle);
			const auto rotationCustom = Matrix< 3, TypeParam >::rotation(Angle, 1, 0, 0);

			for ( size_t i = 0; i < 9; ++i )
			{
				ASSERT_EQ(rotation[i], rotationCustom[i]);
			}
		}

		/* NOTE: 3D rotation around Y axis. */
		{
			constexpr auto Angle = 3 * std::numbers::pi_v< TypeParam > / 4; // 135°
			const auto rotation = Matrix< 3, TypeParam >::rotationY(Angle);
			const auto rotationCustom = Matrix< 3, TypeParam >::rotation(Angle, 0, 1, 0);

			for ( size_t i = 0; i < 9; ++i )
			{
				ASSERT_EQ(rotation[i], rotationCustom[i]);
			}
		}

		/* NOTE: 3D rotation around Z axis. */
		{
			constexpr auto Angle = 7 * std::numbers::pi_v< TypeParam > / 4; // 315°
			const auto rotation = Matrix< 3, TypeParam >::rotationZ(Angle);
			const auto rotationCustom = Matrix< 3, TypeParam >::rotation(Angle, 0, 0, 1);

			for ( size_t i = 0; i < 9; ++i )
			{
				ASSERT_EQ(rotation[i], rotationCustom[i]);
			}
		}
	}
}

TYPED_TEST(MathMatrix, Rotation4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		/* NOTE: 3D rotation around X axis. */
		{
			constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4; // 120°
			const auto rotation = Matrix< 4, TypeParam >::rotationX(Angle);
			const auto rotationCustom = Matrix< 4, TypeParam >::rotation(Angle, 1, 0, 0);

			for ( size_t i = 0; i < 16; ++i )
			{
				ASSERT_EQ(rotation[i], rotationCustom[i]);
			}
		}

		/* NOTE: 3D rotation around Y axis. */
		{
			constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4; // 225°
			const auto rotation = Matrix< 4, TypeParam >::rotationY(Angle);
			const auto rotationCustom = Matrix< 4, TypeParam >::rotation(Angle, 0, 1, 0);

			for ( size_t i = 0; i < 16; ++i )
			{
				ASSERT_EQ(rotation[i], rotationCustom[i]);
			}
		}

		/* NOTE: 3D rotation around Z axis. */
		{
			constexpr auto Angle = 1; // 57,17°
			const auto rotation = Matrix< 4, TypeParam >::rotationZ(Angle);
			const auto rotationCustom = Matrix< 4, TypeParam >::rotation(Angle, 0, 0, 1);

			for ( size_t i = 0; i < 16; ++i )
			{
				ASSERT_EQ(rotation[i], rotationCustom[i]);
			}
		}
	}
}

TYPED_TEST(MathMatrix, DeterminantInverse2)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Matrix< 2, TypeParam > matrix{std::array< TypeParam, 4 >{
			static_cast< TypeParam >(15.2), static_cast< TypeParam >(65.0),
			static_cast< TypeParam >(-3.8), static_cast< TypeParam >(-9.0)}};

		ASSERT_NEAR(matrix.determinant(), static_cast< TypeParam >(110.2), static_cast< TypeParam >(0.001));

		const auto inversedMatrix = matrix.inverse();
		const auto originalMatrix = inversedMatrix.inverse();

		for ( size_t i = 0; i < 4; ++i )
		{
			ASSERT_EQ(inversedMatrix[i], inversedMatrix[i]);
		}
	}
}

TYPED_TEST(MathMatrix, DeterminantInverse3)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Matrix< 3, TypeParam > matrix{std::array< TypeParam, 9 >{
			static_cast< TypeParam >(-2.0), static_cast< TypeParam >(4.1), static_cast< TypeParam >(8.9),
			static_cast< TypeParam >(7.3), static_cast< TypeParam >(-1.0), static_cast< TypeParam >(3.2),
			static_cast< TypeParam >(9.6), static_cast< TypeParam >(0.2), static_cast< TypeParam >(22.0)}};

		ASSERT_NEAR(matrix.determinant(), static_cast< TypeParam >(-388.794), static_cast< TypeParam >(0.001));

		const auto inversedMatrix = matrix.inverse();
		const auto originalMatrix = inversedMatrix.inverse();

		for ( size_t i = 0; i < 9; ++i )
		{
			ASSERT_EQ(inversedMatrix[i], inversedMatrix[i]);
		}
	}
}

TYPED_TEST(MathMatrix, DeterminantInverse4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Matrix< 4, TypeParam > matrix{std::array< TypeParam, 16 >{
			static_cast< TypeParam >(-56.0), static_cast< TypeParam >(4.1), static_cast< TypeParam >(13.5), static_cast< TypeParam >(1.645),
			static_cast< TypeParam >(7.0), static_cast< TypeParam >(1.2), static_cast< TypeParam >(3.1), static_cast< TypeParam >(-6.54),
			static_cast< TypeParam >(9.1), static_cast< TypeParam >(0.0), static_cast< TypeParam >(-2.5), static_cast< TypeParam >(0.0),
			static_cast< TypeParam >(-4.0), static_cast< TypeParam >(7.58), static_cast< TypeParam >(-52.2), static_cast< TypeParam >(3.54)}};

		ASSERT_NEAR(matrix.determinant(), static_cast< TypeParam >(-12946.25), static_cast< TypeParam >(0.001));

		const auto inversedMatrix = matrix.inverse();
		const auto originalMatrix = inversedMatrix.inverse();

		for ( size_t i = 0; i < 16; ++i )
		{
			ASSERT_EQ(inversedMatrix[i], inversedMatrix[i]);
		}
	}
}

// ============================================================================
// MATRIX ARITHMETIC OPERATIONS
// ============================================================================

TYPED_TEST(MathMatrix, Addition2)
{
	const Matrix< 2, TypeParam > a{
		1, 2,
		3, 4};

	const Matrix< 2, TypeParam > b{
		5, 6,
		7, 8};

	const Matrix< 2, TypeParam > expected{
		6, 8,
		10, 12};

	const auto result = a + b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Addition3)
{
	const Matrix< 3, TypeParam > a{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	const Matrix< 3, TypeParam > b{
		9, 8, 7,
		6, 5, 4,
		3, 2, 1};

	const Matrix< 3, TypeParam > expected{
		10, 10, 10,
		10, 10, 10,
		10, 10, 10};

	const auto result = a + b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Addition4)
{
	const Matrix< 4, TypeParam > a{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Matrix< 4, TypeParam > b{
		16, 15, 14, 13,
		12, 11, 10, 9,
		8, 7, 6, 5,
		4, 3, 2, 1};

	const Matrix< 4, TypeParam > expected{
		17, 17, 17, 17,
		17, 17, 17, 17,
		17, 17, 17, 17,
		17, 17, 17, 17};

	const auto result = a + b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Subtraction2)
{
	const Matrix< 2, TypeParam > a{
		10, 9,
		8, 7};

	const Matrix< 2, TypeParam > b{
		1, 2,
		3, 4};

	const Matrix< 2, TypeParam > expected{
		9, 7,
		5, 3};

	const auto result = a - b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Subtraction3)
{
	const Matrix< 3, TypeParam > a{
		10, 10, 10,
		10, 10, 10,
		10, 10, 10};

	const Matrix< 3, TypeParam > b{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	const Matrix< 3, TypeParam > expected{
		9, 8, 7,
		6, 5, 4,
		3, 2, 1};

	const auto result = a - b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Subtraction4)
{
	const Matrix< 4, TypeParam > a{
		20, 20, 20, 20,
		20, 20, 20, 20,
		20, 20, 20, 20,
		20, 20, 20, 20};

	const Matrix< 4, TypeParam > b{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Matrix< 4, TypeParam > expected{
		19, 18, 17, 16,
		15, 14, 13, 12,
		11, 10, 9, 8,
		7, 6, 5, 4};

	const auto result = a - b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, ScalarMultiplication2)
{
	const Matrix< 2, TypeParam > matrix{
		1, 2,
		3, 4};

	const Matrix< 2, TypeParam > expected{
		3, 6,
		9, 12};

	const auto result = matrix * TypeParam{3};
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, ScalarMultiplication3)
{
	const Matrix< 3, TypeParam > matrix{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	const Matrix< 3, TypeParam > expected{
		2, 4, 6,
		8, 10, 12,
		14, 16, 18};

	const auto result = matrix * TypeParam{2};
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, ScalarMultiplication4)
{
	const Matrix< 4, TypeParam > matrix{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Matrix< 4, TypeParam > expected{
		5, 10, 15, 20,
		25, 30, 35, 40,
		45, 50, 55, 60,
		65, 70, 75, 80};

	const auto result = matrix * TypeParam{5};
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, MatrixMultiplication2)
{
	const Matrix< 2, TypeParam > a{
		1, 2,
		3, 4};

	const Matrix< 2, TypeParam > b{
		5, 6,
		7, 8};

	const Matrix< 2, TypeParam > expected{
		19, 22,
		43, 50};

	const auto result = a * b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, MatrixMultiplication3)
{
	const Matrix< 3, TypeParam > a{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	const Matrix< 3, TypeParam > b{
		9, 8, 7,
		6, 5, 4,
		3, 2, 1};

	const Matrix< 3, TypeParam > expected{
		30, 24, 18,
		84, 69, 54,
		138, 114, 90};

	const auto result = a * b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, MatrixMultiplication4)
{
	const Matrix< 4, TypeParam > a{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Matrix< 4, TypeParam > b{
		16, 15, 14, 13,
		12, 11, 10, 9,
		8, 7, 6, 5,
		4, 3, 2, 1};

	const Matrix< 4, TypeParam > expected{
		80, 70, 60, 50,
		240, 214, 188, 162,
		400, 358, 316, 274,
		560, 502, 444, 386};

	const auto result = a * b;
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, IdentityMultiplication2)
{
	const Matrix< 2, TypeParam > matrix{
		5, 7,
		11, 13};

	const auto identity = Matrix< 2, TypeParam >::identity();
	const auto result1 = matrix * identity;
	const auto result2 = identity * matrix;

	assertMatrixNear(result1, matrix);
	assertMatrixNear(result2, matrix);
}

TYPED_TEST(MathMatrix, IdentityMultiplication3)
{
	const Matrix< 3, TypeParam > matrix{
		2, 3, 5,
		7, 11, 13,
		17, 19, 23};

	const auto identity = Matrix< 3, TypeParam >::identity();
	const auto result1 = matrix * identity;
	const auto result2 = identity * matrix;

	assertMatrixNear(result1, matrix);
	assertMatrixNear(result2, matrix);
}

TYPED_TEST(MathMatrix, IdentityMultiplication4)
{
	const Matrix< 4, TypeParam > matrix{
		2, 3, 5, 7,
		11, 13, 17, 19,
		23, 29, 31, 37,
		41, 43, 47, 53};

	const auto identity = Matrix< 4, TypeParam >::identity();
	const auto result1 = matrix * identity;
	const auto result2 = identity * matrix;

	assertMatrixNear(result1, matrix);
	assertMatrixNear(result2, matrix);
}

// ============================================================================
// MATRIX-VECTOR MULTIPLICATION
// ============================================================================

TYPED_TEST(MathMatrix, MatrixVectorMultiplication2)
{
	const Matrix< 2, TypeParam > matrix{
		1, 2,
		3, 4};

	const Vector< 2, TypeParam > vec{5, 6};
	const Vector< 2, TypeParam > expected{17, 39};

	const auto result = matrix * vec;

	ASSERT_TRUE(nearEqual(result[0], expected[0]));
	ASSERT_TRUE(nearEqual(result[1], expected[1]));
}

TYPED_TEST(MathMatrix, MatrixVectorMultiplication3)
{
	const Matrix< 3, TypeParam > matrix{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	const Vector< 3, TypeParam > vec{2, 3, 4};
	const Vector< 3, TypeParam > expected{20, 47, 74};

	const auto result = matrix * vec;

	ASSERT_TRUE(nearEqual(result[0], expected[0]));
	ASSERT_TRUE(nearEqual(result[1], expected[1]));
	ASSERT_TRUE(nearEqual(result[2], expected[2]));
}

TYPED_TEST(MathMatrix, MatrixVectorMultiplication4)
{
	const Matrix< 4, TypeParam > matrix{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Vector< 4, TypeParam > vec{1, 2, 3, 4};
	const Vector< 4, TypeParam > expected{30, 70, 110, 150};

	const auto result = matrix * vec;

	ASSERT_TRUE(nearEqual(result[0], expected[0]));
	ASSERT_TRUE(nearEqual(result[1], expected[1]));
	ASSERT_TRUE(nearEqual(result[2], expected[2]));
	ASSERT_TRUE(nearEqual(result[3], expected[3]));
}

TYPED_TEST(MathMatrix, IdentityVectorMultiplication2)
{
	const auto identity = Matrix< 2, TypeParam >::identity();
	const Vector< 2, TypeParam > vec{7, 11};

	const auto result = identity * vec;

	ASSERT_TRUE(nearEqual(result[0], vec[0]));
	ASSERT_TRUE(nearEqual(result[1], vec[1]));
}

TYPED_TEST(MathMatrix, IdentityVectorMultiplication3)
{
	const auto identity = Matrix< 3, TypeParam >::identity();
	const Vector< 3, TypeParam > vec{2, 3, 5};

	const auto result = identity * vec;

	ASSERT_TRUE(nearEqual(result[0], vec[0]));
	ASSERT_TRUE(nearEqual(result[1], vec[1]));
	ASSERT_TRUE(nearEqual(result[2], vec[2]));
}

TYPED_TEST(MathMatrix, IdentityVectorMultiplication4)
{
	const auto identity = Matrix< 4, TypeParam >::identity();
	const Vector< 4, TypeParam > vec{7, 11, 13, 17};

	const auto result = identity * vec;

	ASSERT_TRUE(nearEqual(result[0], vec[0]));
	ASSERT_TRUE(nearEqual(result[1], vec[1]));
	ASSERT_TRUE(nearEqual(result[2], vec[2]));
	ASSERT_TRUE(nearEqual(result[3], vec[3]));
}

// ============================================================================
// TRANSPOSE OPERATIONS
// ============================================================================

TYPED_TEST(MathMatrix, Transpose2)
{
	auto matrix = Matrix< 2, TypeParam >{
		1, 2,
		3, 4};

	const Matrix< 2, TypeParam > expected{
		1, 3,
		2, 4};

	const auto result = matrix.transpose();
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Transpose3)
{
	auto matrix = Matrix< 3, TypeParam >{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	const Matrix< 3, TypeParam > expected{
		1, 4, 7,
		2, 5, 8,
		3, 6, 9};

	const auto result = matrix.transpose();
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, Transpose4)
{
	auto matrix = Matrix< 4, TypeParam >{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Matrix< 4, TypeParam > expected{
		1, 5, 9, 13,
		2, 6, 10, 14,
		3, 7, 11, 15,
		4, 8, 12, 16};

	const auto result = matrix.transpose();
	assertMatrixNear(result, expected);
}

TYPED_TEST(MathMatrix, TransposeSymmetric2)
{
	const Matrix< 2, TypeParam > original{
		5, 3,
		3, 7};

	auto matrix = original;
	const auto result = matrix.transpose();
	assertMatrixNear(result, original);
}

TYPED_TEST(MathMatrix, TransposeSymmetric3)
{
	const Matrix< 3, TypeParam > original{
		1, 2, 3,
		2, 5, 6,
		3, 6, 9};

	auto matrix = original;
	const auto result = matrix.transpose();
	assertMatrixNear(result, original);
}

TYPED_TEST(MathMatrix, TransposeSymmetric4)
{
	const Matrix< 4, TypeParam > original{
		1, 2, 3, 4,
		2, 5, 6, 7,
		3, 6, 8, 9,
		4, 7, 9, 10};

	auto matrix = original;
	const auto result = matrix.transpose();
	assertMatrixNear(result, original);
}

TYPED_TEST(MathMatrix, DoubleTranspose2)
{
	const Matrix< 2, TypeParam > original{
		7, 11,
		13, 17};

	auto matrix = original;
	const auto result = matrix.transpose().transpose();
	assertMatrixNear(result, original);
}

TYPED_TEST(MathMatrix, DoubleTranspose3)
{
	const Matrix< 3, TypeParam > original{
		1, 2, 3,
		4, 5, 6,
		7, 8, 9};

	auto matrix = original;
	const auto result = matrix.transpose().transpose();
	assertMatrixNear(result, original);
}

TYPED_TEST(MathMatrix, DoubleTranspose4)
{
	const Matrix< 4, TypeParam > original{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	auto matrix = original;
	const auto result = matrix.transpose().transpose();
	assertMatrixNear(result, original);
}

// ============================================================================
// IDENTITY AND PROPERTIES
// ============================================================================

TYPED_TEST(MathMatrix, IsIdentity2)
{
	auto matrix = Matrix< 2, TypeParam >::identity();
	ASSERT_TRUE(matrix.isIdentity());

	matrix = Matrix< 2, TypeParam >{
		1, 0,
		0, 1};
	ASSERT_TRUE(matrix.isIdentity());

	matrix = Matrix< 2, TypeParam >{
		2, 0,
		0, 1};
	ASSERT_FALSE(matrix.isIdentity());
}

TYPED_TEST(MathMatrix, IsIdentity3)
{
	auto matrix = Matrix< 3, TypeParam >::identity();
	ASSERT_TRUE(matrix.isIdentity());

	matrix = Matrix< 3, TypeParam >{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1};
	ASSERT_TRUE(matrix.isIdentity());

	matrix = Matrix< 3, TypeParam >{
		1, 0, 0,
		0, 2, 0,
		0, 0, 1};
	ASSERT_FALSE(matrix.isIdentity());
}

TYPED_TEST(MathMatrix, IsIdentity4)
{
	auto matrix = Matrix< 4, TypeParam >::identity();
	ASSERT_TRUE(matrix.isIdentity());

	matrix = Matrix< 4, TypeParam >{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};
	ASSERT_TRUE(matrix.isIdentity());

	matrix = Matrix< 4, TypeParam >{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 1,
		0, 0, 0, 1};
	ASSERT_FALSE(matrix.isIdentity());
}

TYPED_TEST(MathMatrix, InverseIdentity2)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto identity = Matrix< 2, TypeParam >::identity();
		const auto result = identity.inverse();
		assertMatrixNear(result, identity);
	}
}

TYPED_TEST(MathMatrix, InverseIdentity3)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto identity = Matrix< 3, TypeParam >::identity();
		const auto result = identity.inverse();
		assertMatrixNear(result, identity);
	}
}

TYPED_TEST(MathMatrix, InverseIdentity4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto identity = Matrix< 4, TypeParam >::identity();
		const auto result = identity.inverse();
		assertMatrixNear(result, identity);
	}
}

TYPED_TEST(MathMatrix, DoubleInverse2)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Matrix< 2, TypeParam > matrix{
			4, 7,
			2, 6};

		const auto inverse = matrix.inverse();
		const auto doubleInverse = inverse.inverse();

		assertMatrixNear(doubleInverse, matrix);
	}
}

TYPED_TEST(MathMatrix, DoubleInverse3)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Matrix< 3, TypeParam > matrix{
			1, 2, 3,
			0, 1, 4,
			5, 6, 0};

		const auto inverse = matrix.inverse();
		const auto doubleInverse = inverse.inverse();

		assertMatrixNear(doubleInverse, matrix, static_cast< TypeParam >(0.001));
	}
}

TYPED_TEST(MathMatrix, DoubleInverse4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Matrix< 4, TypeParam > matrix{
			1, 0, 2, 0,
			0, 3, 0, 4,
			5, 0, 6, 0,
			0, 7, 0, 8};

		const auto inverse = matrix.inverse();
		const auto doubleInverse = inverse.inverse();

		assertMatrixNear(doubleInverse, matrix, static_cast< TypeParam >(0.001));
	}
}
// ============================================================================
// TRANSFORMATION MATRICES
// ============================================================================

TYPED_TEST(MathMatrix, Scaling3)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto scale = Matrix< 3, TypeParam >::scaling(2, 3, 4);
		const Vector< 3, TypeParam > vec{1, 1, 1};
		const Vector< 3, TypeParam > expected{2, 3, 4};

		const auto result = scale * vec;

		ASSERT_TRUE(nearEqual(result[0], expected[0]));
		ASSERT_TRUE(nearEqual(result[1], expected[1]));
		ASSERT_TRUE(nearEqual(result[2], expected[2]));
	}
}

TYPED_TEST(MathMatrix, Scaling4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto scale = Matrix< 4, TypeParam >::scaling(2, 3, 4);
		const Vector< 4, TypeParam > vec{1, 1, 1, 1};
		const Vector< 4, TypeParam > expected{2, 3, 4, 1};

		const auto result = scale * vec;

		ASSERT_TRUE(nearEqual(result[0], expected[0]));
		ASSERT_TRUE(nearEqual(result[1], expected[1]));
		ASSERT_TRUE(nearEqual(result[2], expected[2]));
		ASSERT_TRUE(nearEqual(result[3], expected[3]));
	}
}

TYPED_TEST(MathMatrix, Translation4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto translation = Matrix< 4, TypeParam >::translation(5, 7, 11);
		const Vector< 4, TypeParam > point{1, 2, 3, 1};
		const Vector< 4, TypeParam > expected{6, 9, 14, 1};

		const auto result = translation * point;

		ASSERT_TRUE(nearEqual(result[0], expected[0]));
		ASSERT_TRUE(nearEqual(result[1], expected[1]));
		ASSERT_TRUE(nearEqual(result[2], expected[2]));
		ASSERT_TRUE(nearEqual(result[3], expected[3]));
	}
}

TYPED_TEST(MathMatrix, TransformationComposition)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Create a transformation: scale -> rotate -> translate
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2; // 90°

		const auto scale = Matrix< 4, TypeParam >::scaling(2, 2, 2);
		const auto rotation = Matrix< 4, TypeParam >::rotationZ(Angle);
		const auto translation = Matrix< 4, TypeParam >::translation(10, 0, 0);

		// Combined transformation (remember: matrix multiplication is right-to-left)
		const auto transform = translation * rotation * scale;

		// Test point (1, 0, 0)
		const Vector< 4, TypeParam > point{1, 0, 0, 1};

		// Expected: (1,0,0) -> scale(2) -> (2,0,0) -> rotate90° -> (0,2,0) -> translate(10,0,0) -> (10,2,0)
		const auto result = transform * point;

		ASSERT_NEAR(static_cast< double >(result[0]), 10.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[1]), 2.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[2]), 0.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[3]), 1.0, 0.01);
	}
}

TYPED_TEST(MathMatrix, RotationComposition3)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Test that rotating by X then Y gives expected combined result
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 2; // 90°

		const auto rotX = Matrix< 3, TypeParam >::rotationX(Angle);
		const auto rotY = Matrix< 3, TypeParam >::rotationY(Angle);

		const auto combined = rotY * rotX;

		const Vector< 3, TypeParam > point{1, 0, 0};
		const auto result = combined * point;

		// After 90° X rotation: (1,0,0) stays (1,0,0)
		// After 90° Y rotation: (1,0,0) becomes (0,0,-1)
		ASSERT_NEAR(static_cast< double >(result[0]), 0.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[1]), 0.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[2]), -1.0, 0.01);
	}
}

TYPED_TEST(MathMatrix, RotationComposition4)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Test Euler angles composition
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4; // 45°

		const auto rotX = Matrix< 4, TypeParam >::rotationX(Angle);
		const auto rotY = Matrix< 4, TypeParam >::rotationY(Angle);
		const auto rotZ = Matrix< 4, TypeParam >::rotationZ(Angle);

		const auto combined = rotZ * rotY * rotX;

		// Verify the combined matrix is still a valid rotation (determinant ≈ 1)
		const auto det = combined.determinant();
		ASSERT_NEAR(static_cast< double >(det), 1.0, 0.01);
	}
}

// ============================================================================
// PROJECTION MATRICES
// ============================================================================

TYPED_TEST(MathMatrix, OrthographicProjection)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto projection = Matrix< 4, TypeParam >::orthographicProjection(-10, 10, -10, 10, static_cast< TypeParam >(0.1), static_cast< TypeParam >(100));

		// Test point in the middle of the frustum
		const Vector< 4, TypeParam > point{0, 0, -50, 1};
		const auto result = projection * point;

		// In orthographic projection, z is mapped linearly
		ASSERT_TRUE(result[3] != TypeParam{0}); // w should not be zero

		// After perspective divide, x and y should be close to 0
		const auto x = result[0] / result[3];
		const auto y = result[1] / result[3];

		ASSERT_NEAR(static_cast< double >(x), 0.0, 0.01);
		ASSERT_NEAR(static_cast< double >(y), 0.0, 0.01);
	}
}

TYPED_TEST(MathMatrix, PerspectiveProjection)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const auto fov = std::numbers::pi_v< TypeParam > / 2; // 90° FOV
		const auto aspectRatio = static_cast< TypeParam >(16.0 / 9.0);
		const auto projection = Matrix< 4, TypeParam >::perspectiveProjection(fov, aspectRatio, static_cast< TypeParam >(0.1), static_cast< TypeParam >(100));

		// Test that the projection matrix exists and is not degenerate
		const auto det = projection.determinant();
		ASSERT_TRUE(det != TypeParam{0}); // Non-degenerate matrix
	}
}

TYPED_TEST(MathMatrix, LookAtMatrix)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		const Vector< 3, TypeParam > eye{0, 0, 5};
		const Vector< 3, TypeParam > center{0, 0, 0};
		const Vector< 3, TypeParam > up{0, 1, 0};

		const auto view = Matrix< 4, TypeParam >::lookAt(eye, center, up);

		// The lookAt matrix should be valid (non-zero determinant)
		const auto det = view.determinant();
		ASSERT_TRUE(det != TypeParam{0});

		// Test that it transforms the eye position correctly
		const Vector< 4, TypeParam > eyePos{0, 0, 5, 1};
		const auto result = view * eyePos;

		// In view space, the camera is at origin
		ASSERT_NEAR(static_cast< double >(result[0]), 0.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[1]), 0.0, 0.01);
		ASSERT_NEAR(static_cast< double >(result[2]), 0.0, 0.01);
	}
}

// ============================================================================
// 3D GRAPHICS REAL-WORLD SCENARIOS
// ============================================================================

TYPED_TEST(MathMatrix, ModelViewProjectionPipeline)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Complete MVP pipeline test
		// Model matrix: scale by 2, rotate 45° around Y, translate to (10, 0, -20)
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 4;

		const auto modelScale = Matrix< 4, TypeParam >::scaling(2, 2, 2);
		const auto modelRotate = Matrix< 4, TypeParam >::rotationY(Angle);
		const auto modelTranslate = Matrix< 4, TypeParam >::translation(10, 0, -20);
		const auto model = modelTranslate * modelRotate * modelScale;

		// View matrix: camera at (0, 5, 10) looking at origin
		const Vector< 3, TypeParam > eye{0, 5, 10};
		const Vector< 3, TypeParam > center{0, 0, 0};
		const Vector< 3, TypeParam > up{0, 1, 0};
		const auto view = Matrix< 4, TypeParam >::lookAt(eye, center, up);

		// Projection matrix: perspective with 60° FOV
		const auto fov = std::numbers::pi_v< TypeParam > / 3;
		const auto projection = Matrix< 4, TypeParam >::perspectiveProjection(fov, static_cast< TypeParam >(16.0 / 9.0), static_cast< TypeParam >(0.1), static_cast< TypeParam >(100));

		// Combined MVP matrix
		const auto mvp = projection * view * model;

		// Verify the MVP matrix is valid
		const auto det = mvp.determinant();
		ASSERT_TRUE(det != TypeParam{0});
	}
}

TYPED_TEST(MathMatrix, RotationMatrixPreservesLength)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Rotation should preserve vector length
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 3; // 60°

		const auto rotation = Matrix< 3, TypeParam >::rotationZ(Angle);
		const Vector< 3, TypeParam > vec{3, 4, 0};

		const auto length = vec.length();
		const auto result = rotation * vec;
		const auto resultLength = result.length();

		ASSERT_NEAR(static_cast< double >(length), static_cast< double >(resultLength), 0.01);
	}
}

TYPED_TEST(MathMatrix, RotationMatrixDeterminant)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Rotation matrices should have determinant = 1
		constexpr auto Angle = std::numbers::pi_v< TypeParam > / 6;

		const auto rotationX = Matrix< 3, TypeParam >::rotationX(Angle);
		const auto rotationY = Matrix< 3, TypeParam >::rotationY(Angle);
		const auto rotationZ = Matrix< 3, TypeParam >::rotationZ(Angle);

		ASSERT_NEAR(static_cast< double >(rotationX.determinant()), 1.0, 0.01);
		ASSERT_NEAR(static_cast< double >(rotationY.determinant()), 1.0, 0.01);
		ASSERT_NEAR(static_cast< double >(rotationZ.determinant()), 1.0, 0.01);
	}
}

TYPED_TEST(MathMatrix, TransformHierarchy)
{
	if constexpr ( !std::is_integral_v< TypeParam > )
	{
		// Test parent-child transformation hierarchy
		// Parent: translated to (5, 0, 0)
		const auto parentTransform = Matrix< 4, TypeParam >::translation(5, 0, 0);

		// Child: translated to (0, 3, 0) relative to parent
		const auto childLocalTransform = Matrix< 4, TypeParam >::translation(0, 3, 0);

		// World transform of child
		const auto childWorldTransform = parentTransform * childLocalTransform;

		// Point at child's origin
		const Vector< 4, TypeParam > childOrigin{0, 0, 0, 1};
		const auto worldPosition = childWorldTransform * childOrigin;

		// Should be at (5, 3, 0) in world space
		ASSERT_NEAR(static_cast< double >(worldPosition[0]), 5.0, 0.01);
		ASSERT_NEAR(static_cast< double >(worldPosition[1]), 3.0, 0.01);
		ASSERT_NEAR(static_cast< double >(worldPosition[2]), 0.0, 0.01);
	}
}

// ============================================================================
// EDGE CASES AND ROBUSTNESS
// ============================================================================

TYPED_TEST(MathMatrix, ZeroMatrix2)
{
	const Matrix< 2, TypeParam > zero{
		0, 0,
		0, 0};

	const Matrix< 2, TypeParam > other{
		5, 7,
		11, 13};

	const auto result = zero + other;
	assertMatrixNear(result, other);
}

TYPED_TEST(MathMatrix, ZeroMatrix3)
{
	const Matrix< 3, TypeParam > zero{
		0, 0, 0,
		0, 0, 0,
		0, 0, 0};

	const Matrix< 3, TypeParam > other{
		2, 3, 5,
		7, 11, 13,
		17, 19, 23};

	const auto result = zero + other;
	assertMatrixNear(result, other);
}

TYPED_TEST(MathMatrix, ZeroMatrix4)
{
	const Matrix< 4, TypeParam > zero{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0};

	const Matrix< 4, TypeParam > other{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const auto result = zero + other;
	assertMatrixNear(result, other);
}

TYPED_TEST(MathMatrix, ZeroScalar2)
{
	const Matrix< 2, TypeParam > matrix{
		7, 11,
		13, 17};

	const auto result = matrix * TypeParam{0};

	for ( size_t i = 0; i < 4; ++i )
	{
		ASSERT_EQ(result[i], TypeParam{0});
	}
}

TYPED_TEST(MathMatrix, ZeroScalar3)
{
	const Matrix< 3, TypeParam > matrix{
		2, 3, 5,
		7, 11, 13,
		17, 19, 23};

	const auto result = matrix * TypeParam{0};

	for ( size_t i = 0; i < 9; ++i )
	{
		ASSERT_EQ(result[i], TypeParam{0});
	}
}

TYPED_TEST(MathMatrix, ZeroScalar4)
{
	const Matrix< 4, TypeParam > matrix{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const auto result = matrix * TypeParam{0};

	for ( size_t i = 0; i < 16; ++i )
	{
		ASSERT_EQ(result[i], TypeParam{0});
	}
}

TYPED_TEST(MathMatrix, MatrixVectorZero2)
{
	const Matrix< 2, TypeParam > matrix{
		7, 11,
		13, 17};

	const Vector< 2, TypeParam > zero{0, 0};
	const auto result = matrix * zero;

	ASSERT_EQ(result[0], TypeParam{0});
	ASSERT_EQ(result[1], TypeParam{0});
}

TYPED_TEST(MathMatrix, MatrixVectorZero3)
{
	const Matrix< 3, TypeParam > matrix{
		2, 3, 5,
		7, 11, 13,
		17, 19, 23};

	const Vector< 3, TypeParam > zero{0, 0, 0};
	const auto result = matrix * zero;

	ASSERT_EQ(result[0], TypeParam{0});
	ASSERT_EQ(result[1], TypeParam{0});
	ASSERT_EQ(result[2], TypeParam{0});
}

TYPED_TEST(MathMatrix, MatrixVectorZero4)
{
	const Matrix< 4, TypeParam > matrix{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16};

	const Vector< 4, TypeParam > zero{0, 0, 0, 0};
	const auto result = matrix * zero;

	ASSERT_EQ(result[0], TypeParam{0});
	ASSERT_EQ(result[1], TypeParam{0});
	ASSERT_EQ(result[2], TypeParam{0});
	ASSERT_EQ(result[3], TypeParam{0});
}
