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

using namespace Libs::Math;

using MathTypeList = testing::Types< int, float, double >;

template< typename >
struct MathMatrix
	: testing::Test
{

};

TYPED_TEST_SUITE(MathMatrix, MathTypeList);

TYPED_TEST(MathMatrix, Matrix2Default)
{
	const std::array< TypeParam, 4 > identity{
		1, 0,
		0, 1
	};

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
		0, 0, 1
	};

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
		0, 0, 0, 1
	};

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
	const Matrix< 2, TypeParam > columnMajor{{
		0, 2,
		1, 3
	}};

	const Matrix< 2, TypeParam > rowMajor{
		0, 1,
		2, 3
	};

	for ( size_t i = 0; i < 4; ++i )
	{
		ASSERT_EQ(columnMajor[i], rowMajor[i]);
	}
}

TYPED_TEST(MathMatrix, Constructors3)
{
	const Matrix< 3, TypeParam > columnMajor{{
		0, 3, 6,
		1, 4, 7,
		2, 5, 8
	}};

	const Matrix< 3, TypeParam > rowMajor{
		0, 1, 2,
		3, 4, 5,
		6, 7, 8
	};

	for ( size_t i = 0; i < 9; ++i )
	{
		ASSERT_EQ(columnMajor[i], rowMajor[i]);
	}
}

TYPED_TEST(MathMatrix, Constructors4)
{
	const Matrix< 4, TypeParam > columnMajor{{
		0, 4,  8, 12,
		1, 5,  9, 13,
		2, 6, 10, 14,
		3, 7, 11, 15
	}};

	const Matrix< 4, TypeParam > rowMajor{
		 0,  1,  2,  3,
		 4,  5,  6,  7,
		 8,  9, 10, 11,
		12, 13, 14, 15
	};

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
			15.2, 65.0,
			-3.8, -9.0
		}};

		ASSERT_NEAR(matrix.determinant(), 110.2, 0.001);

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
			-2.0, 4.1, 8.9,
			7.3, -1.0, 3.2,
			9.6, 0.2, 22.0
		}};

		ASSERT_NEAR(matrix.determinant(), -388.794, 0.001);

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
			-56.0, 4.1, 13.5, 1.645,
			7.0, 1.2, 3.1, -6.54,
			9.1, 0.0, -2.5, 0.0,
			-4.0, 7.58, -52.2, 3.54
		}};

		ASSERT_NEAR(matrix.determinant(), -12946.25, 0.001);

		const auto inversedMatrix = matrix.inverse();
		const auto originalMatrix = inversedMatrix.inverse();

		for ( size_t i = 0; i < 16; ++i )
		{
			ASSERT_EQ(inversedMatrix[i], inversedMatrix[i]);
		}
	}
}
