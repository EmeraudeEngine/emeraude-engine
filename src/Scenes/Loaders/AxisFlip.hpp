/*
 * src/Scenes/Loaders/AxisFlip.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <string>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"
#include "Math/Matrix.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Vector.hpp"

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief The axis negation an asset is imported through — the SINGLE authority for the rule.
	 *
	 * Built from `LoaderOptions::swapX/swapY/swapZ`, it materialises the diagonal matrix
	 * `M = diag(±1, ±1, ±1)`. Every loader routes its data through this class instead of
	 * open-coding sign flips, because the rule is NOT "negate the coordinate": a mirror touches
	 * positions, directions, rotations, matrices AND the triangle winding, and getting one of
	 * them wrong produces an asset that looks almost right.
	 *
	 * ## Why an asset ever needs this
	 *
	 * The engine's world→screen mapping is orientation-REVERSING (measured; see
	 * `docs/coordinate-system.md` § OPEN DEFECT). Importers, on the other hand, convert with a
	 * ROTATION — the consumer's 180° X rotation, determinant +1 — and a rotation can never undo a
	 * chirality difference. Chiral content (typically TEXT baked in a texture, or any asset with a
	 * readable left/right) therefore arrives mirrored on screen. Enabling an ODD number of flags
	 * here makes the import itself orientation-reversing, which cancels the pipeline's reversal.
	 *
	 * ⚠️ For a glTF/FBX asset the flag that fixes chirality is **swapZ**, not swapY. The consumer
	 * still applies its 180° X rotation `diag(1,-1,-1)` on top; composed with `diag(1,1,-1)` it
	 * yields `diag(1,-1,1)` — determinant -1 (chirality fixed) with the up axis still up. `swapY`
	 * would give `diag(1,1,-1)`: also determinant -1, but the scene upside down.
	 *
	 * ## The rule, in full
	 *
	 * `M` is diagonal with entries in {-1, +1}, so `M⁻¹ = M`. It follows that:
	 *
	 * | Quantity | Transform | Note |
	 * |---|---|---|
	 * | Point, vector, normal, tangent | `M · v` | The inverse-transpose of `M` IS `M`, so normals need no special case |
	 * | Translation of a TRS | `M · t` | |
	 * | Rotation quaternion `(w, v)` | `(w, det(M) · M · v)` | ⚠️ see below |
	 * | Scale of a TRS | unchanged | `M · diag(s) · M = diag(s)` |
	 * | 4×4 transform `T` | `M · T · M` | Conjugation, so a hierarchy telescopes |
	 * | Triangle winding | swapped iff `det(M) = +1` | ⚠️ see below |
	 *
	 * ⚠️⚠️ **The `det(M)` factor on the quaternion is not decoration.** Conjugating a rotation by
	 * a REFLECTION gives a rotation about the mirrored axis by the OPPOSITE angle. Dropping the
	 * factor yields a rig that animates backwards while its bind pose looks correct — the kind of
	 * defect that gets blamed on the animation blender for days.
	 *
	 * ⚠️⚠️ **Winding follows the PARITY, and the historical swap is the even case.** All loaders
	 * swap indices 1↔2 unconditionally today, and that is correct as long as the import preserves
	 * orientation. An odd number of flags reverses orientation, which reverses the winding too:
	 * keeping the swap there would render every face inside-out. Ask
	 * `mustSwapTriangleWinding()` — never hard-code the swap again.
	 *
	 * @note Conjugating the hierarchy (`M·T·M`) while mirroring the vertices (`M·v`) is what keeps
	 * every node transform a PROPER rotation. The alternative — a single reflection on the root
	 * node — would need a negative scale in the scene graph, which breaks normals, physics and
	 * bounding boxes. That is why the flip belongs to the geometry, not to the transform.
	 */
	class EMEN_API AxisFlip final
	{
		public:

			/**
			 * @brief Constructs an identity flip: nothing is mirrored.
			 */
			constexpr AxisFlip () noexcept = default;

			/**
			 * @brief Constructs a flip from the three per-axis negation flags.
			 * @param flipX Negates the X axis.
			 * @param flipY Negates the Y axis.
			 * @param flipZ Negates the Z axis.
			 */
			constexpr
			AxisFlip (bool flipX, bool flipY, bool flipZ) noexcept
				: m_signs{flipX ? -1.0F : 1.0F, flipY ? -1.0F : 1.0F, flipZ ? -1.0F : 1.0F}
			{

			}

			/**
			 * @brief Returns whether nothing is mirrored at all.
			 * @note A hot loop may branch on this to skip the transform entirely.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			isIdentity () const noexcept
			{
				return m_signs[0] > 0.0F && m_signs[1] > 0.0F && m_signs[2] > 0.0F;
			}

			/**
			 * @brief Returns the determinant of the flip: +1 or -1.
			 * @return float
			 */
			[[nodiscard]]
			constexpr
			float
			determinant () const noexcept
			{
				return m_signs[0] * m_signs[1] * m_signs[2];
			}

			/**
			 * @brief Returns whether the flip reverses orientation (an ODD number of axes negated).
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			invertsOrientation () const noexcept
			{
				return this->determinant() < 0.0F;
			}

			/**
			 * @brief Returns whether the loader must swap triangle indices 1 and 2.
			 * @warning ⚠️ This is the ONLY place that answer may come from. The swap compensates
			 * the engine's orientation-reversing projection, so it is required when the import
			 * preserves orientation and FORBIDDEN when the import reverses it — otherwise every
			 * face renders inside-out.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			mustSwapTriangleWinding () const noexcept
			{
				return !this->invertsOrientation();
			}

			/**
			 * @brief Mirrors a point, a vector, a normal or a tangent.
			 * @param value A reference to the vector.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			vector (const Base::Math::Vector< 3, float > & value) const noexcept
			{
				return {m_signs[0] * value[0], m_signs[1] * value[1], m_signs[2] * value[2]};
			}

			/**
			 * @brief Mirrors the three components of a point, a vector, a normal or a tangent.
			 * @note Component form, for the loaders that read raw attributes straight from a
			 * third-party accessor without building a Vector first.
			 * @param x The X component.
			 * @param y The Y component.
			 * @param z The Z component.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			vector (float x, float y, float z) const noexcept
			{
				return {m_signs[0] * x, m_signs[1] * y, m_signs[2] * z};
			}

			/**
			 * @brief Returns the sign applied to one axis, +1 or -1.
			 * @param axisIndex The axis index: 0 = X, 1 = Y, 2 = Z.
			 * @return float
			 */
			[[nodiscard]]
			constexpr
			float
			sign (size_t axisIndex) const noexcept
			{
				return m_signs[axisIndex];
			}

			/**
			 * @brief Conjugates a rotation quaternion.
			 * @warning Carries the `det(M)` angle inversion — see the class documentation.
			 * @param value A reference to the quaternion.
			 * @return Base::Math::Quaternion< float >
			 */
			[[nodiscard]]
			Base::Math::Quaternion< float >
			rotation (const Base::Math::Quaternion< float > & value) const noexcept
			{
				const auto determinant = this->determinant();
				const auto complex = value.complex();

				return Base::Math::Quaternion< float >{
					Base::Math::Vector< 3, float >{
						determinant * m_signs[0] * complex[0],
						determinant * m_signs[1] * complex[1],
						determinant * m_signs[2] * complex[2]
					},
					value.real()
				};
			}

			/**
			 * @brief Conjugates a 4x4 transformation matrix: `M · T · M`.
			 * @note Also correct for an inverse bind matrix: skinning stays coherent because the
			 * joint world matrices are conjugated the same way and the two `M` cancel.
			 * @param value A reference to the matrix.
			 * @return Base::Math::Matrix< 4, float >
			 */
			[[nodiscard]]
			Base::Math::Matrix< 4, float >
			matrix (const Base::Math::Matrix< 4, float > & value) const noexcept
			{
				auto result = value;

				/* NOTE: (M T M)(row, col) = m_row * T(row, col) * m_col on the linear part, and the
				 * translation column follows m_row alone. The homogeneous row is left untouched. */
				for ( size_t row = 0; row < 3; ++row )
				{
					for ( size_t col = 0; col < 3; ++col )
					{
						result(row, col) = m_signs[row] * value(row, col) * m_signs[col];
					}

					result(row, 3) = m_signs[row] * value(row, 3);
				}

				return result;
			}

			/**
			 * @brief Conjugates a cartesian frame: position mirrored, orientation kept a PROPER rotation.
			 * @param value A reference to the frame.
			 * @return Base::Math::CartesianFrame< float >
			 */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			frame (const Base::Math::CartesianFrame< float > & value) const noexcept
			{
				/* NOTE: Column j of (M R M) is m_j * M * (column j of R), so each stored axis gets
				 * the global mirror AND the sign of its own axis. The X axis is derived by
				 * CartesianFrame from these two, and stays consistent because the conjugate of a
				 * rotation is a rotation of the same handedness. */
				const auto downward = this->vector(value.downwardVector()) * m_signs[1];
				const auto backward = this->vector(value.backwardVector()) * m_signs[2];

				return {
					this->vector(value.position()),
					downward,
					backward,
					value.scalingFactor()
				};
			}

			/**
			 * @brief Returns a suffix identifying this flip, to be appended to every resource name.
			 * @warning ⚠️ MANDATORY on cached resources. Geometries are cached by name: without a
			 * suffix, loading the same asset twice with different flags would silently serve the
			 * first variant to the second caller.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			resourceNameSuffix () const noexcept
			{
				if ( this->isIdentity() )
				{
					return {};
				}

				std::string suffix{"+flip"};

				if ( m_signs[0] < 0.0F )
				{
					suffix += 'X';
				}

				if ( m_signs[1] < 0.0F )
				{
					suffix += 'Y';
				}

				if ( m_signs[2] < 0.0F )
				{
					suffix += 'Z';
				}

				return suffix;
			}

		private:

			/** @brief The diagonal of M: +1 keeps an axis, -1 negates it. */
			std::array< float, 3 > m_signs{1.0F, 1.0F, 1.0F};
	};
}
