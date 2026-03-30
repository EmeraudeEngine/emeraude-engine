/*
 * src/Libs/Animation/Joint.hpp
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
#include <cstdint>
#include <string>

/* Local inclusions. */
#include "Libs/Math/Vector.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Quaternion.hpp"

namespace EmEn::Libs::Animation
{
	using namespace Math;

	/** @brief Sentinel value indicating a root joint (no parent). */
	static constexpr int32_t NoParent = -1;

	/**
	 * @brief Represents a single joint in a skeletal hierarchy.
	 * @note This is a pure data structure. The joint stores its local bind-pose transform
	 * (Translation + Rotation + Scale) and the inverse bind matrix used for skinning.
	 * @tparam precision_t Floating point type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	struct Joint final
	{
		/** @brief Human-readable joint name (e.g., "LeftShoulder", "Spine1"). Used for lookup. */
		std::string name{};

		/** @brief Index of the parent joint in the Skeleton's joint array. NoParent (-1) for root joints. */
		int32_t parentIndex{NoParent};

		/** @brief Local bind-pose translation relative to parent joint. */
		Vector< 3, precision_t > translation{};

		/** @brief Local bind-pose rotation relative to parent joint. */
		Quaternion< precision_t > rotation{};

		/** @brief Local bind-pose scale relative to parent joint. */
		Vector< 3, precision_t > scale{1, 1, 1};

		/**
		 * @brief Inverse bind matrix.
		 * @note Transforms a vertex from model space to the joint's local space in bind pose.
		 * Used in skinning: finalMatrix = jointWorldMatrix * inverseBindMatrix.
		 * This is typically provided by the asset (e.g., GLTF skin.inverseBindMatrices).
		 */
		Matrix< 4, precision_t > inverseBindMatrix{};

		/**
		 * @brief Returns true if this joint is a root joint (no parent).
		 * @return bool
		 */
		[[nodiscard]]
		constexpr bool
		isRoot () const noexcept
		{
			return parentIndex == NoParent;
		}
	};

	using JointF = Joint< float >;
	using JointD = Joint< double >;
}
