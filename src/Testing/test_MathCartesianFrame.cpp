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

using namespace EmEn::Libs::Math;

using MathTypeList = testing::Types< float, double >;

template< typename >
struct MathCartesianFrame
	: testing::Test
{
};

TYPED_TEST_SUITE(MathCartesianFrame, MathTypeList);

/**
 * @brief Helper function to compare matrices with a reasonable tolerance for floating point errors.
 * @tparam dim_t Matrix dimension.
 * @tparam precision_t Floating point type.
 * @param a First matrix.
 * @param b Second matrix.
 * @param epsilon Tolerance (default: 1e-5 for reasonable floating point accumulation).
 */
template< size_t dim_t, typename precision_t >
void
assertMatrixNear (const Matrix< dim_t, precision_t > & a, const Matrix< dim_t, precision_t > & b, precision_t epsilon = static_cast< precision_t >(1e-5))
{
	for ( size_t i = 0; i < dim_t * dim_t; ++i )
	{
		ASSERT_NEAR(a[i], b[i], epsilon) << "Matrix element mismatch at index " << i;
	}
}

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
		std::cout << "No useful test for integral version !"
					 "\n";

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
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		/* Check scaling. */
		{
			constexpr auto Reference = Matrix< 4, TypeParam >{{static_cast< TypeParam >(2.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0),
															   static_cast< TypeParam >(0.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0),
															   static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(0.0),
															   static_cast< TypeParam >(1.2), static_cast< TypeParam >(-1.3), static_cast< TypeParam >(3.2), static_cast< TypeParam >(1.0)}};

			auto cartesianFrame = CartesianFrame< TypeParam >{};
			cartesianFrame.setPosition(static_cast< TypeParam >(1.2), static_cast< TypeParam >(-1.3), static_cast< TypeParam >(3.2));
			cartesianFrame.setScalingFactor(static_cast< TypeParam >(2.0));

			const auto modelMatrixA = cartesianFrame.getModelMatrix();

			ASSERT_EQ(modelMatrixA, Reference);

			const auto modelMatrixB = cartesianFrame.getModelMatrix() * Matrix< 4, TypeParam >{};

			ASSERT_EQ(modelMatrixB, Reference);
		}

		{
			constexpr auto Reference = Matrix< 4, TypeParam >{{static_cast< TypeParam >(2.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0),
															   static_cast< TypeParam >(0.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0),
															   static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(0.0),
															   static_cast< TypeParam >(0.0), static_cast< TypeParam >(-2.6), static_cast< TypeParam >(0.0), static_cast< TypeParam >(1.0)}};

			const auto frameA = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{static_cast< TypeParam >(0.0), static_cast< TypeParam >(-1.5), static_cast< TypeParam >(0.0)}, static_cast< TypeParam >(2.0)};
			const auto frameB = CartesianFrame< TypeParam >{static_cast< TypeParam >(0.0F), static_cast< TypeParam >(-0.55F), static_cast< TypeParam >(0.0F)};
			const auto modelMatrix = frameA.getModelMatrix() * frameB.getModelMatrix();
			const auto scaling = frameA.scalingFactor() * frameB.scalingFactor();

			assertMatrixNear(modelMatrix, Reference);

			{
				CartesianFrame< TypeParam > rebuiltFrame{modelMatrix, scaling};

				assertMatrixNear(rebuiltFrame.getModelMatrix(), Reference);
			}
		}
	}
}

// ============================================================================
// CONSTRUCTORS
// ============================================================================

TYPED_TEST(MathCartesianFrame, ConstructorWithPosition)
{
	const Vector< 3, TypeParam > pos{static_cast< TypeParam >(1.5), static_cast< TypeParam >(2.5), static_cast< TypeParam >(3.5)};
	const auto frame = CartesianFrame< TypeParam >{pos};

	ASSERT_EQ(frame.position(), pos);

	const Vector< 3, TypeParam > expectedScale{1, 1, 1};
	ASSERT_EQ(frame.scalingFactor(), expectedScale);
}

TYPED_TEST(MathCartesianFrame, ConstructorWithPositionAndScaling)
{
	const Vector< 3, TypeParam > pos{static_cast< TypeParam >(1.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(3.0)};
	const TypeParam scale = static_cast< TypeParam >(2.5);
	const auto frame = CartesianFrame< TypeParam >{pos, scale};

	ASSERT_EQ(frame.position(), pos);

	const Vector< 3, TypeParam > expectedScale{scale, scale, scale};
	ASSERT_EQ(frame.scalingFactor(), expectedScale);
}

TYPED_TEST(MathCartesianFrame, ConstructorWithCoordinates)
{
	const auto frame = CartesianFrame< TypeParam >{1.0F, 2.0F, 3.0F};

	ASSERT_NEAR(frame.position()[X], static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Y], static_cast< TypeParam >(2.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Z], static_cast< TypeParam >(3.0), static_cast< TypeParam >(1e-5));
}

TYPED_TEST(MathCartesianFrame, ConstructorFromMatrix)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		// Create a known model matrix (translation + rotation)
		auto originalFrame = CartesianFrame< TypeParam >{};
		originalFrame.setPosition(static_cast< TypeParam >(1.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(3.0));
		originalFrame.yaw(Radian< TypeParam >(45), true);

		const auto modelMatrix = originalFrame.getModelMatrix();
		const auto scale = Vector< 3, TypeParam >{static_cast< TypeParam >(2.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(2.0)};

		// Reconstruct frame from matrix
		const auto rebuiltFrame = CartesianFrame< TypeParam >{modelMatrix, scale};

		ASSERT_NEAR(rebuiltFrame.position()[X], static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(rebuiltFrame.position()[Y], static_cast< TypeParam >(2.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(rebuiltFrame.position()[Z], static_cast< TypeParam >(3.0), static_cast< TypeParam >(1e-5));
		ASSERT_EQ(rebuiltFrame.scalingFactor(), scale);
	}
}

TYPED_TEST(MathCartesianFrame, ConstructorWithVectors)
{
	const Vector< 3, TypeParam > pos{static_cast< TypeParam >(1.0), static_cast< TypeParam >(2.0), static_cast< TypeParam >(3.0)};
	const Vector< 3, TypeParam > down{static_cast< TypeParam >(0.0), static_cast< TypeParam >(1.0), static_cast< TypeParam >(0.0)};
	const Vector< 3, TypeParam > back{static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(1.0)};
	const Vector< 3, TypeParam > scale{static_cast< TypeParam >(2.0), static_cast< TypeParam >(3.0), static_cast< TypeParam >(4.0)};

	const auto frame = CartesianFrame< TypeParam >{pos, down, back, scale};

	ASSERT_EQ(frame.position(), pos);
	ASSERT_EQ(frame.YAxis(), down);
	ASSERT_EQ(frame.ZAxis(), back);
	ASSERT_EQ(frame.scalingFactor(), scale);
}

// ============================================================================
// POSITION SETTERS AND GETTERS
// ============================================================================

TYPED_TEST(MathCartesianFrame, SetPosition)
{
	auto frame = CartesianFrame< TypeParam >{};
	const Vector< 3, TypeParam > newPos{static_cast< TypeParam >(10.0), static_cast< TypeParam >(20.0), static_cast< TypeParam >(30.0)};

	frame.setPosition(newPos);

	ASSERT_EQ(frame.position(), newPos);
}

TYPED_TEST(MathCartesianFrame, SetPositionCoordinates)
{
	auto frame = CartesianFrame< TypeParam >{};

	frame.setPosition(static_cast< TypeParam >(5.0), static_cast< TypeParam >(6.0), static_cast< TypeParam >(7.0));

	ASSERT_NEAR(frame.position()[X], static_cast< TypeParam >(5.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Y], static_cast< TypeParam >(6.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Z], static_cast< TypeParam >(7.0), static_cast< TypeParam >(1e-5));
}

TYPED_TEST(MathCartesianFrame, SetXYZPosition)
{
	auto frame = CartesianFrame< TypeParam >{};

	frame.setXPosition(static_cast< TypeParam >(1.5));
	frame.setYPosition(static_cast< TypeParam >(2.5));
	frame.setZPosition(static_cast< TypeParam >(3.5));

	ASSERT_NEAR(frame.position()[X], static_cast< TypeParam >(1.5), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Y], static_cast< TypeParam >(2.5), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Z], static_cast< TypeParam >(3.5), static_cast< TypeParam >(1e-5));
}

// ============================================================================
// SCALING SETTERS
// ============================================================================

TYPED_TEST(MathCartesianFrame, SetUniformScaling)
{
	auto frame = CartesianFrame< TypeParam >{};

	frame.setScalingFactor(static_cast< TypeParam >(3.0));

	const Vector< 3, TypeParam > expectedScale{static_cast< TypeParam >(3.0), static_cast< TypeParam >(3.0), static_cast< TypeParam >(3.0)};
	ASSERT_EQ(frame.scalingFactor(), expectedScale);
}

TYPED_TEST(MathCartesianFrame, SetScalingVector)
{
	auto frame = CartesianFrame< TypeParam >{};
	const Vector< 3, TypeParam > scale{static_cast< TypeParam >(2.0), static_cast< TypeParam >(3.0), static_cast< TypeParam >(4.0)};

	frame.setScalingFactor(scale);

	ASSERT_EQ(frame.scalingFactor(), scale);
}

TYPED_TEST(MathCartesianFrame, SetScalingCoordinates)
{
	auto frame = CartesianFrame< TypeParam >{};

	frame.setScalingFactor(static_cast< TypeParam >(1.5), static_cast< TypeParam >(2.5), static_cast< TypeParam >(3.5));

	ASSERT_NEAR(frame.scalingFactor()[X], static_cast< TypeParam >(1.5), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.scalingFactor()[Y], static_cast< TypeParam >(2.5), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.scalingFactor()[Z], static_cast< TypeParam >(3.5), static_cast< TypeParam >(1e-5));
}

TYPED_TEST(MathCartesianFrame, SetScalingXYZFactors)
{
	auto frame = CartesianFrame< TypeParam >{};

	frame.setScalingXFactor(static_cast< TypeParam >(2.0));
	frame.setScalingYFactor(static_cast< TypeParam >(3.0));
	frame.setScalingZFactor(static_cast< TypeParam >(4.0));

	ASSERT_NEAR(frame.scalingFactor()[X], static_cast< TypeParam >(2.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.scalingFactor()[Y], static_cast< TypeParam >(3.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.scalingFactor()[Z], static_cast< TypeParam >(4.0), static_cast< TypeParam >(1e-5));
}

// ============================================================================
// ORIENTATION VECTORS
// ============================================================================

TYPED_TEST(MathCartesianFrame, SetBackwardVector)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		const Vector< 3, TypeParam > newBackward{1.0, 0.0, 0.0};

		frame.setBackwardVector(newBackward);

		// Backward should be normalized
		ASSERT_NEAR(frame.backwardVector().length(), static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(frame.backwardVector()[X], static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
	}
}

TYPED_TEST(MathCartesianFrame, SetOrientationVectors)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		const Vector< 3, TypeParam > backward{1.0, 0.0, 0.0};
		const Vector< 3, TypeParam > downward{0.0, 1.0, 0.0};

		frame.setOrientationVectors(backward, downward);

		// Vectors should be normalized
		ASSERT_NEAR(frame.backwardVector().length(), static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(frame.downwardVector().length(), static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
	}
}

TYPED_TEST(MathCartesianFrame, SetOrientationFromFrame)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto sourceFrame = CartesianFrame< TypeParam >{};
		sourceFrame.yaw(Radian< TypeParam >(45), true);

		auto targetFrame = CartesianFrame< TypeParam >{};
		targetFrame.setOrientationVectors(sourceFrame);

		ASSERT_EQ(targetFrame.YAxis(), sourceFrame.YAxis());
		ASSERT_EQ(targetFrame.ZAxis(), sourceFrame.ZAxis());
	}
}

// ============================================================================
// TRANSLATION METHODS
// ============================================================================

TYPED_TEST(MathCartesianFrame, TranslateWorld)
{
	auto frame = CartesianFrame< TypeParam >{};
	const Vector< 3, TypeParam > translation{1.0, 2.0, 3.0};

	frame.translate(translation, false);

	ASSERT_EQ(frame.position(), translation);
}

TYPED_TEST(MathCartesianFrame, TranslateLocal)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.yaw(Radian< TypeParam >(90), true); // Rotate 90 degrees around Y

		// Translate 1 unit in local Z (which should be world X after rotation)
		frame.translate(static_cast< TypeParam >(0.0), static_cast< TypeParam >(0.0), static_cast< TypeParam >(1.0), true);

		// Position should have changed based on local axes
		ASSERT_GT(std::abs(frame.position()[X]), static_cast< TypeParam >(0.5));
	}
}

TYPED_TEST(MathCartesianFrame, TranslateXYZ)
{
	auto frame = CartesianFrame< TypeParam >{};

	frame.translateX(static_cast< TypeParam >(1.0), false);
	frame.translateY(static_cast< TypeParam >(2.0), false);
	frame.translateZ(static_cast< TypeParam >(3.0), false);

	ASSERT_NEAR(frame.position()[X], static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Y], static_cast< TypeParam >(2.0), static_cast< TypeParam >(1e-5));
	ASSERT_NEAR(frame.position()[Z], static_cast< TypeParam >(3.0), static_cast< TypeParam >(1e-5));
}

// ============================================================================
// ROTATION METHODS - PITCH
// ============================================================================

TYPED_TEST(MathCartesianFrame, PitchLocal)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.pitch(Radian< TypeParam >(90), true);

		// After 90 degree pitch, backward should point upward
		ASSERT_NEAR(frame.backwardVector()[Y], TypeParam{-1.0}, static_cast< TypeParam >(0.01));
	}
}

TYPED_TEST(MathCartesianFrame, PitchWorld)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(0.0, 1.0, 0.0);
		frame.pitch(Radian< TypeParam >(90), false);

		// Position should rotate around world X axis
		ASSERT_GT(std::abs(frame.position()[Z]), static_cast< TypeParam >(0.5));
	}
}

// ============================================================================
// ROTATION METHODS - ROLL
// ============================================================================

TYPED_TEST(MathCartesianFrame, RollLocal)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.roll(Radian< TypeParam >(90), true);

		// After 90 degree roll, downward should point left
		ASSERT_NEAR(frame.downwardVector()[X], TypeParam{-1.0}, static_cast< TypeParam >(0.01));
	}
}

TYPED_TEST(MathCartesianFrame, RollWorld)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(0.0, 1.0, 0.0);
		frame.roll(Radian< TypeParam >(90), false);

		// Position should rotate around world Z axis
		ASSERT_GT(std::abs(frame.position()[X]), static_cast< TypeParam >(0.5));
	}
}

// ============================================================================
// ROTATION METHODS - ARBITRARY AXIS
// ============================================================================

TYPED_TEST(MathCartesianFrame, RotateArbitraryAxis)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		const Vector< 3, TypeParam > axis{0.0, 1.0, 0.0};

		frame.rotate(Radian< TypeParam >(90), axis, true);

		// Should behave like yaw
		ASSERT_NEAR(frame.backwardVector()[X], static_cast< TypeParam >(1.0), static_cast< TypeParam >(0.01));
	}
}

// ============================================================================
// LOOKAT FUNCTIONALITY
// ============================================================================

TYPED_TEST(MathCartesianFrame, LookAtTarget)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(0.0, 0.0, 0.0);

		const Vector< 3, TypeParam > target{0.0, 0.0, 10.0};

		// flipAxis = false: backward = position - target, so backward points away from target
		frame.lookAt(target, false);

		// Backward should point in -Z direction (away from target at +Z)
		ASSERT_NEAR(frame.backwardVector()[Z], TypeParam{-1.0}, static_cast< TypeParam >(0.01));

		// Forward should point in +Z direction (toward target at +Z)
		ASSERT_NEAR(frame.forwardVector()[Z], static_cast< TypeParam >(1.0), static_cast< TypeParam >(0.01));
	}
}

TYPED_TEST(MathCartesianFrame, LookAtTargetFlipped)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(0.0, 0.0, 0.0);

		const Vector< 3, TypeParam > target{0.0, 0.0, 10.0};

		// flipAxis = true means Z+ points toward target
		frame.lookAt(target, true);

		// Backward vector (Z+) should point toward target
		ASSERT_NEAR(frame.backwardVector()[Z], static_cast< TypeParam >(1.0), static_cast< TypeParam >(0.01));
	}
}

// ============================================================================
// ANGLE GETTERS
// ============================================================================

TYPED_TEST(MathCartesianFrame, GetPitchAngle)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};

		// Default frame: backward = (0, 0, 1), negativeZ = (0, 0, -1)
		// Angle between them is 180 degrees (π radians)
		const auto initialAngle = frame.getPitchAngle();
		ASSERT_NEAR(initialAngle, std::numbers::pi_v< TypeParam >, static_cast< TypeParam >(0.01));

		// After pitching -90 degrees, backward should point toward negativeZ
		frame.pitch(Radian< TypeParam >(-90), true);
		const auto pitchedAngle = frame.getPitchAngle();

		// After -90 degree pitch, backward should be closer to negativeZ
		ASSERT_LT(pitchedAngle, initialAngle);
	}
}

TYPED_TEST(MathCartesianFrame, GetYawAngle)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};

		const auto initialAngle = frame.getYawAngle();

		// Default backward is Z+, angle to X+ should be 90 degrees
		ASSERT_NEAR(initialAngle, Radian< TypeParam >(90), static_cast< TypeParam >(0.01));
	}
}

TYPED_TEST(MathCartesianFrame, GetRollAngle)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};

		const auto angle = frame.getRollAngle();

		// Default backward is Z+, angle to Y+ should be 90 degrees
		ASSERT_NEAR(angle, Radian< TypeParam >(90), static_cast< TypeParam >(0.01));
	}
}

// ============================================================================
// MATRIX GETTERS
// ============================================================================

TYPED_TEST(MathCartesianFrame, GetViewMatrix)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(1.0, 2.0, 3.0);

		const auto viewMatrix = frame.getViewMatrix();

		// View matrix should be inverse of model matrix (without scaling)
		// Check that translation is negated and rotated
		ASSERT_NE(viewMatrix[M4x4Col3Row0], static_cast< TypeParam >(1.0));
	}
}

TYPED_TEST(MathCartesianFrame, GetInfinityViewMatrix)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(1.0, 2.0, 3.0);

		const auto infinityViewMatrix = frame.getInfinityViewMatrix();

		// Infinity view matrix should have no translation
		ASSERT_NEAR(infinityViewMatrix[M4x4Col3Row0], static_cast< TypeParam >(0.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(infinityViewMatrix[M4x4Col3Row1], static_cast< TypeParam >(0.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(infinityViewMatrix[M4x4Col3Row2], static_cast< TypeParam >(0.0), static_cast< TypeParam >(1e-5));
	}
}

TYPED_TEST(MathCartesianFrame, GetInvertedModelMatrix)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(1.0, 2.0, 3.0);
		frame.setScalingFactor(2.0);

		const auto modelMatrix = frame.getModelMatrix();
		const auto invertedMatrix = frame.getInvertedModelMatrix();

		// Product should be identity (approximately)
		const auto product = modelMatrix * invertedMatrix;

		assertMatrixNear(product, Matrix< 4, TypeParam >{});
	}
}

TYPED_TEST(MathCartesianFrame, GetRotationMatrix3)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.yaw(Radian< TypeParam >(45), true);

		const auto rotMatrix = frame.getRotationMatrix3();

		// Rotation matrix should have determinant of 1 (or -1)
		const auto det = rotMatrix.determinant();
		ASSERT_NEAR(std::abs(det), static_cast< TypeParam >(1.0), static_cast< TypeParam >(0.01));
	}
}

TYPED_TEST(MathCartesianFrame, GetRotationMatrix4)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.yaw(Radian< TypeParam >(45), true);

		const auto rotMatrix = frame.getRotationMatrix4();

		// Translation should be zero
		ASSERT_NEAR(rotMatrix[M4x4Col3Row0], static_cast< TypeParam >(0.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(rotMatrix[M4x4Col3Row1], static_cast< TypeParam >(0.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(rotMatrix[M4x4Col3Row2], static_cast< TypeParam >(0.0), static_cast< TypeParam >(1e-5));
	}
}

// ============================================================================
// RESET AND NORMALIZE
// ============================================================================

TYPED_TEST(MathCartesianFrame, ResetFrame)
{
	auto frame = CartesianFrame< TypeParam >{};
	frame.setPosition(10.0, 20.0, 30.0);
	frame.setScalingFactor(5.0);

	frame.reset();

	const auto expectedOrigin = Vector< 3, TypeParam >::origin();
	ASSERT_EQ(frame.position(), expectedOrigin);

	const Vector< 3, TypeParam > expectedScale{1, 1, 1};
	ASSERT_EQ(frame.scalingFactor(), expectedScale);

	const auto expectedY = Vector< 3, TypeParam >::positiveY();
	ASSERT_EQ(frame.YAxis(), expectedY);

	const auto expectedZ = Vector< 3, TypeParam >::positiveZ();
	ASSERT_EQ(frame.ZAxis(), expectedZ);
}

TYPED_TEST(MathCartesianFrame, ResetRotation)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};
		frame.setPosition(10.0, 20.0, 30.0);
		frame.yaw(Radian< TypeParam >(45), true);

		frame.resetRotation();

		// Position should remain unchanged
		ASSERT_NEAR(frame.position()[X], static_cast< TypeParam >(10.0), static_cast< TypeParam >(1e-5));

		// Rotation should be reset
		const auto expectedY = Vector< 3, TypeParam >::positiveY();
		ASSERT_EQ(frame.YAxis(), expectedY);

		const auto expectedZ = Vector< 3, TypeParam >::positiveZ();
		ASSERT_EQ(frame.ZAxis(), expectedZ);
	}
}

TYPED_TEST(MathCartesianFrame, NormalizeFrame)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		auto frame = CartesianFrame< TypeParam >{};

		// Manually set non-normalized vectors
		frame.setOrientationVectors(Vector< 3, TypeParam >{2.0, 0.0, 0.0}, Vector< 3, TypeParam >{0.0, 3.0, 0.0});

		frame.normalize();

		// Vectors should be normalized
		ASSERT_NEAR(frame.YAxis().length(), static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(frame.ZAxis().length(), static_cast< TypeParam >(1.0), static_cast< TypeParam >(1e-5));
	}
}

// ============================================================================
// INTERPOLATION
// ============================================================================

TYPED_TEST(MathCartesianFrame, LinearInterpolation)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		const auto frameA = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{0.0, 0.0, 0.0}};
		const auto frameB = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{10.0, 10.0, 10.0}};

		const auto interpolated = CartesianFrame< TypeParam >::linearInterpolation(frameA, frameB, static_cast< TypeParam >(0.5));

		ASSERT_NEAR(interpolated.position()[X], static_cast< TypeParam >(5.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(interpolated.position()[Y], static_cast< TypeParam >(5.0), static_cast< TypeParam >(1e-5));
		ASSERT_NEAR(interpolated.position()[Z], static_cast< TypeParam >(5.0), static_cast< TypeParam >(1e-5));
	}
}

TYPED_TEST(MathCartesianFrame, LinearInterpolationEndpoints)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		const auto frameA = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{0.0, 0.0, 0.0}};
		const auto frameB = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{10.0, 10.0, 10.0}};

		// At t=0, should equal frameA
		const auto interpolatedA = CartesianFrame< TypeParam >::linearInterpolation(frameA, frameB, static_cast< TypeParam >(0.0));
		ASSERT_EQ(interpolatedA.position(), frameA.position());

		// At t=1, should equal frameB
		const auto interpolatedB = CartesianFrame< TypeParam >::linearInterpolation(frameA, frameB, static_cast< TypeParam >(1.0));
		ASSERT_EQ(interpolatedB.position(), frameB.position());
	}
}

TYPED_TEST(MathCartesianFrame, CosineInterpolation)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		const auto frameA = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{0.0, 0.0, 0.0}};
		const auto frameB = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{10.0, 10.0, 10.0}};

		const auto interpolated = CartesianFrame< TypeParam >::cosineInterpolation(frameA, frameB, static_cast< TypeParam >(0.5));

		// At t=0.5, should be near midpoint with cosine interpolation
		ASSERT_NEAR(interpolated.position()[X], static_cast< TypeParam >(5.0), static_cast< TypeParam >(0.1));
		ASSERT_NEAR(interpolated.position()[Y], static_cast< TypeParam >(5.0), static_cast< TypeParam >(0.1));
		ASSERT_NEAR(interpolated.position()[Z], static_cast< TypeParam >(5.0), static_cast< TypeParam >(0.1));
	}
}

TYPED_TEST(MathCartesianFrame, CosineInterpolationEndpoints)
{
	if constexpr ( std::is_integral_v< TypeParam > )
	{
		std::cout << "No useful test for integral version !"
					 "\n";

		ASSERT_EQ(true, true);
	}
	else
	{
		const auto frameA = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{0.0, 0.0, 0.0}};
		const auto frameB = CartesianFrame< TypeParam >{Vector< 3, TypeParam >{10.0, 10.0, 10.0}};

		// At t=0, should equal frameA
		const auto interpolatedA = CartesianFrame< TypeParam >::cosineInterpolation(frameA, frameB, static_cast< TypeParam >(0.0));
		ASSERT_NEAR(interpolatedA.position()[X], frameA.position()[X], static_cast< TypeParam >(1e-5));

		// At t=1, should equal frameB
		const auto interpolatedB = CartesianFrame< TypeParam >::cosineInterpolation(frameA, frameB, static_cast< TypeParam >(1.0));
		ASSERT_NEAR(interpolatedB.position()[X], frameB.position()[X], static_cast< TypeParam >(1e-5));
	}
}
