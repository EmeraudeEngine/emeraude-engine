/*
 * src/Libs/Animation/Skin.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/Matrix.hpp"

namespace EmEn::Libs::Animation
{
	using namespace Math;

	/**
	 * @brief Binds a mesh to a skeleton for skinning.
	 * @note The Skin establishes the link between vertex bone indices (stored in ShapeVertex)
	 * and the skeleton's joint hierarchy. It contains:
	 * - A joint index remapping table: vertex bone indices [0..N] map to skeleton joint indices.
	 * - Per-joint inverse bind matrices (may differ from the Skeleton's if the asset provides them separately, as in GLTF).
	 *
	 * GLTF context: a "skin" object references a set of joints (nodes) and provides inverseBindMatrices.
	 * The vertex attributes JOINTS_0 contain indices into the skin's joint array, NOT the skeleton's global array.
	 * The Skin provides the indirection.
	 *
	 * Vertex weights are already stored in ShapeVertex (m_influences + m_weights, 4 per vertex).
	 *
	 * @tparam precision_t Floating point type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Skin final
	{
		public:

			/**
			 * @brief Constructs an empty skin.
			 */
			Skin () noexcept = default;

			/**
			 * @brief Constructs a skin with joint remapping and inverse bind matrices.
			 * @param jointIndices Maps skin-local joint index (used by vertex JOINTS_0) to skeleton-global joint index.
			 * @param inverseBindMatrices Per-joint inverse bind matrices. Must be same size as jointIndices.
			 */
			Skin (std::vector< int32_t > jointIndices, std::vector< Matrix< 4, precision_t > > inverseBindMatrices) noexcept
				: m_jointIndices(std::move(jointIndices)),
				  m_inverseBindMatrices(std::move(inverseBindMatrices))
			{

			}

			/**
			 * @brief Returns the number of joints referenced by this skin.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			jointCount () const noexcept
			{
				return m_jointIndices.size();
			}

			/**
			 * @brief Returns true if this skin references no joints.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_jointIndices.empty();
			}

			/**
			 * @brief Maps a skin-local joint index to the skeleton-global joint index.
			 * @param skinLocalIndex The index as stored in vertex JOINTS_0 attribute.
			 * @return int32_t The corresponding index in the Skeleton's joint array.
			 */
			[[nodiscard]]
			int32_t
			skeletonJointIndex (size_t skinLocalIndex) const noexcept
			{
				return m_jointIndices[skinLocalIndex];
			}

			/**
			 * @brief Returns the inverse bind matrix for the given skin-local joint index.
			 * @param skinLocalIndex The index as stored in vertex JOINTS_0 attribute.
			 * @return const Matrix< 4, precision_t > &
			 */
			[[nodiscard]]
			const Matrix< 4, precision_t > &
			inverseBindMatrix (size_t skinLocalIndex) const noexcept
			{
				return m_inverseBindMatrices[skinLocalIndex];
			}

			/**
			 * @brief Returns the entire joint index remapping array.
			 * @return const std::vector< int32_t > &
			 */
			[[nodiscard]]
			const std::vector< int32_t > &
			jointIndices () const noexcept
			{
				return m_jointIndices;
			}

			/**
			 * @brief Returns the entire inverse bind matrix array.
			 * @return const std::vector< Matrix< 4, precision_t > > &
			 */
			[[nodiscard]]
			const std::vector< Matrix< 4, precision_t > > &
			inverseBindMatrices () const noexcept
			{
				return m_inverseBindMatrices;
			}

		private:

			/** @brief Maps skin-local index → skeleton-global joint index. */
			std::vector< int32_t > m_jointIndices{};

			/** @brief Per-joint inverse bind matrices, indexed by skin-local index. */
			std::vector< Matrix< 4, precision_t > > m_inverseBindMatrices{};
	};

	using SkinF = Skin< float >;
	using SkinD = Skin< double >;
}
