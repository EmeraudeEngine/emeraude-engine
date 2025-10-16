/*
 * src/Testing/test_MathCartesianFrame.cpp
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
#include "Libs/Math/CartesianFrame.hpp"

using namespace Libs::Math;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathCartesianFrame
	: testing::Test
{

};

TYPED_TEST_SUITE(MathCartesianFrame, MathTypeList);

TYPED_TEST(MathCartesianFrame, CartesianFrameDefault)
{
	const auto cartesianFrame = CartesianFrame< TypeParam >{};

	/* Check the position */
	{
		constexpr auto Origin = Vector< 3, TypeParam >::origin();

		ASSERT_EQ(cartesianFrame.position(), Origin);
	}

	/* Check the local X axis. */
	{
		constexpr auto Axis = Vector< 3, TypeParam >::positiveX();
		constexpr auto AxisInv = Vector< 3, TypeParam >::negativeX();

		ASSERT_EQ(cartesianFrame.XAxis(), Axis);
		ASSERT_EQ(cartesianFrame.rightVector(), Axis);
		ASSERT_EQ(cartesianFrame.leftVector(), AxisInv);
	}

	/* Check the local Y axis. */
	{
		constexpr auto Axis = Vector< 3, TypeParam >::positiveY();
		constexpr auto AxisInv = Vector< 3, TypeParam >::negativeY();

		ASSERT_EQ(cartesianFrame.YAxis(), Axis);
		ASSERT_EQ(cartesianFrame.downwardVector(), Axis);
		ASSERT_EQ(cartesianFrame.upwardVector(), AxisInv);
	}

	/* Check the local Z axis. */
	{
		constexpr auto Axis = Vector< 3, TypeParam >::positiveZ();
		constexpr auto AxisInv = Vector< 3, TypeParam >::negativeZ();

		ASSERT_EQ(cartesianFrame.ZAxis(), Axis);
		ASSERT_EQ(cartesianFrame.backwardVector(), Axis);
		ASSERT_EQ(cartesianFrame.forwardVector(), AxisInv);
	}

	/* Check the scaling vector. */
	{
		constexpr Vector< 3, TypeParam > NoScale{1, 1, 1};

		ASSERT_EQ(cartesianFrame.scalingFactor(), NoScale);
	}

	/* Check generated matrices from the cartesian frame. */
	{
		constexpr Matrix< 3, TypeParam > IdentityM3{};
		constexpr Matrix< 4, TypeParam > IdentityM4{};

		ASSERT_EQ(cartesianFrame.getTranslationMatrix4(), IdentityM4);
		ASSERT_EQ(cartesianFrame.getRotationMatrix3(), IdentityM3);
		ASSERT_EQ(cartesianFrame.getRotationMatrix4(), IdentityM4);
		ASSERT_EQ(cartesianFrame.getScalingMatrix3(), IdentityM3);
		ASSERT_EQ(cartesianFrame.getScalingMatrix4(), IdentityM4);

		ASSERT_EQ(cartesianFrame.getModelMatrix(), IdentityM4);
		ASSERT_EQ(cartesianFrame.getViewMatrix(), IdentityM4);
		ASSERT_EQ(cartesianFrame.getInfinityViewMatrix(), IdentityM4);
	}
}

TYPED_TEST(MathCartesianFrame, CartesianFrameYaw90)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !" "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto cartesianFrame = CartesianFrame< TypeParam >{};
		cartesianFrame.yaw(Radian< TypeParam >(-90), true);

		/* Check the position */
		{
			constexpr auto Origin = Vector< 3, TypeParam >::origin();

			ASSERT_EQ(cartesianFrame.position(), Origin);
		}

		/* Check the local X axis. */
		{
			constexpr auto Axis = Vector< 3, TypeParam >::positiveZ();
			constexpr auto AxisInv = Vector< 3, TypeParam >::negativeZ();

			ASSERT_EQ(cartesianFrame.XAxis(), Axis);
			ASSERT_EQ(cartesianFrame.rightVector(), Axis);
			ASSERT_EQ(cartesianFrame.leftVector(), AxisInv);
		}

		/* Check the local Y axis. */
		{
			constexpr auto Axis = Vector< 3, TypeParam >::positiveY();
			constexpr auto AxisInv = Vector< 3, TypeParam >::negativeY();

			ASSERT_EQ(cartesianFrame.YAxis(), Axis);
			ASSERT_EQ(cartesianFrame.downwardVector(), Axis);
			ASSERT_EQ(cartesianFrame.upwardVector(), AxisInv);
		}

		/* Check the local Z axis. */
		{
			constexpr auto Axis = Vector< 3, TypeParam >::negativeX();
			constexpr auto AxisInv = Vector< 3, TypeParam >::positiveX();

			ASSERT_EQ(cartesianFrame.ZAxis(), Axis);
			ASSERT_EQ(cartesianFrame.backwardVector(), Axis);
			ASSERT_EQ(cartesianFrame.forwardVector(), AxisInv);
		}
	}
}

TYPED_TEST(MathCartesianFrame, CartesianFrameTransformation)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !" "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		/* Check scaling. */
		{
			constexpr auto Reference = Matrix< 4, TypeParam >{{
				2.0, 0.0, 0.0, 0.0,
				0.0, 2.0, 0.0, 0.0,
				0.0, 0.0, 2.0, 0.0,
				1.2,-1.3, 3.2, 1.0
			}};

			auto cartesianFrame = CartesianFrame< TypeParam >{};
			cartesianFrame.setPosition(1.2, -1.3, 3.2);
			cartesianFrame.setScalingFactor(2.0);

			const auto modelMatrixA = cartesianFrame.getModelMatrix();

			ASSERT_EQ(modelMatrixA, Reference);

			const auto modelMatrixB = cartesianFrame.getModelMatrix() * Matrix< 4, TypeParam >{};

			ASSERT_EQ(modelMatrixB, Reference);
		}

		{
			constexpr auto Reference = Matrix< 4, TypeParam >{{
				2.0, 0.0, 0.0, 0.0,
				0.0, 2.0, 0.0, 0.0,
				0.0, 0.0, 2.0, 0.0,
				0.0,-2.6, 0.0, 1.0
			}};

			const auto frameA = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{0.0, -1.5, 0.0}, 2.0};
			const auto frameB = CartesianFrame< TypeParam >{0.0, -0.55, 0.0};
			const auto modelMatrix = frameA.getModelMatrix() * frameB.getModelMatrix();
			const auto scaling = frameA.scalingFactor() * frameB.scalingFactor();

			ASSERT_EQ(modelMatrix, Reference);

			{
				CartesianFrame< TypeParam > rebuiltFrame{modelMatrix, scaling};

				ASSERT_EQ(rebuiltFrame.getModelMatrix(), Reference);
			}
		}
	}
}
