/*
 * src/Libs/Math/TransformUtils.hpp
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

#pragma once

/* Local inclusions. */
#include "Vector.hpp"
#include "Matrix.hpp"
#include "Quaternion.hpp"

namespace EmEn::Libs::Math
{
	/**
	 * @brief Holds the decomposed Translation, Rotation, Scale components of a transform.
	 * @tparam precision_t The data precision. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	struct TRSDecomposition final
	{
		Vector< 3, precision_t > translation{};
		Quaternion< precision_t > rotation{};
		Vector< 3, precision_t > scale{1, 1, 1};
	};

	/**
	 * @brief Decomposes a 4x4 TRS matrix into its Translation, Rotation and Scale components.
	 * @note Assumes the matrix is a valid TRS composition (no shear, no projection).
	 * Scale is extracted from column magnitudes. Negative scale on an odd number of axes
	 * cannot be distinguished from a 180-degree rotation; this function always returns positive scale.
	 * @param matrix A reference to the matrix to decompose.
	 * @return TRSDecomposition< precision_t >
	 */
	template< typename precision_t >
	requires (std::is_floating_point_v< precision_t >)
	[[nodiscard]]
	TRSDecomposition< precision_t >
	decomposeTRS (const Matrix< 4, precision_t > & matrix) noexcept
	{
		TRSDecomposition< precision_t > result;

		/* Extract translation from column 3. */
		result.translation = {matrix[M4x4Col3Row0], matrix[M4x4Col3Row1], matrix[M4x4Col3Row2]};

		/* Extract column vectors (rotation * scale). */
		Vector< 3, precision_t > col0{matrix[M4x4Col0Row0], matrix[M4x4Col0Row1], matrix[M4x4Col0Row2]};
		Vector< 3, precision_t > col1{matrix[M4x4Col1Row0], matrix[M4x4Col1Row1], matrix[M4x4Col1Row2]};
		Vector< 3, precision_t > col2{matrix[M4x4Col2Row0], matrix[M4x4Col2Row1], matrix[M4x4Col2Row2]};

		/* Extract scale from column magnitudes. */
		result.scale = {col0.length(), col1.length(), col2.length()};

		/* Remove scale from columns to get pure rotation. */
		constexpr auto Epsilon = std::numeric_limits< precision_t >::epsilon();

		if ( result.scale[X] > Epsilon ) col0 /= result.scale[X];
		if ( result.scale[Y] > Epsilon ) col1 /= result.scale[Y];
		if ( result.scale[Z] > Epsilon ) col2 /= result.scale[Z];

		/* Build a pure rotation 4x4 matrix in column-major layout and extract quaternion. */
		constexpr auto Zero = static_cast< precision_t >(0);
		constexpr auto One = static_cast< precision_t >(1);

		std::array< precision_t, 16 > rotData{
			col0[X], col0[Y], col0[Z], Zero,
			col1[X], col1[Y], col1[Z], Zero,
			col2[X], col2[Y], col2[Z], Zero,
			Zero, Zero, Zero, One
		};

		result.rotation = Quaternion< precision_t >{Matrix< 4, precision_t >{rotData}};

		return result;
	}

	/**
	 * @brief Composes a 4x4 TRS matrix from Translation, Rotation (quaternion) and Scale components.
	 * @note The result is equivalent to T * R * S in matrix multiplication order.
	 * This matches the convention used by CartesianFrame::getModelMatrix().
	 * @param translation A reference to the translation vector.
	 * @param rotation A reference to the rotation quaternion.
	 * @param scale A reference to the scale vector.
	 * @return Matrix< 4, precision_t >
	 */
	template< typename precision_t >
	requires (std::is_floating_point_v< precision_t >)
	[[nodiscard]]
	Matrix< 4, precision_t >
	composeTRS (const Vector< 3, precision_t > & translation, const Quaternion< precision_t > & rotation, const Vector< 3, precision_t > & scale) noexcept
	{
		/* Start with a pure rotation matrix. */
		auto m = rotation.toRotationMatrix4();

		/* Apply scale to the rotation columns (right-multiplication by scale matrix). */
		if ( !scale.isAllComponentOne() )
		{
			m *= Matrix< 4, precision_t >::scaling(scale);
		}

		/* Set translation in column 3. */
		m[M4x4Col3Row0] = translation[X];
		m[M4x4Col3Row1] = translation[Y];
		m[M4x4Col3Row2] = translation[Z];

		return m;
	}

	using TRSDecompositionF = TRSDecomposition< float >;
	using TRSDecompositionD = TRSDecomposition< double >;
}
