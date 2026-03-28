/*
 * src/Libs/Animation/AnimationChannel.hpp
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
#include "Libs/Math/Vector.hpp"
#include "Libs/Math/Quaternion.hpp"

namespace EmEn::Libs::Animation
{
	using namespace Math;

	/**
	 * @brief Interpolation method for animation keyframes.
	 * @note Matches the GLTF 2.0 animation sampler interpolation types.
	 */
	enum class ChannelInterpolation : uint8_t
	{
		Step,        /**< No interpolation — holds the value until the next keyframe. */
		Linear,      /**< Linear interpolation (LERP for T/S, SLERP for R). */
		CubicSpline  /**< Cubic spline interpolation (GLTF cubic with in/out tangents). */
	};

	/**
	 * @brief The transform component targeted by an animation channel.
	 */
	enum class ChannelTarget : uint8_t
	{
		Translation,
		Rotation,
		Scale
	};

	/**
	 * @brief A single keyframe for a translation or scale channel.
	 * @tparam precision_t Floating point type.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	struct VectorKeyFrame final
	{
		precision_t time{0};
		Vector< 3, precision_t > value{};
	};

	/**
	 * @brief A single keyframe for a rotation channel.
	 * @tparam precision_t Floating point type.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	struct QuaternionKeyFrame final
	{
		precision_t time{0};
		Quaternion< precision_t > value{};
	};

	/**
	 * @brief An animation channel targeting a specific joint's transform component.
	 * @note Each channel contains keyframes for ONE component (translation, rotation, or scale)
	 * of ONE joint. Timestamps are in seconds. Keyframes must be sorted by time (ascending).
	 *
	 * A joint may have 0 to 3 channels in a clip (one per T/R/S component).
	 * If a component has no channel, the bind-pose value is used.
	 *
	 * @tparam precision_t Floating point type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	struct AnimationChannel final
	{
		/** @brief Index of the target joint in the Skeleton's joint array. */
		int32_t jointIndex{-1};

		/** @brief Which transform component this channel animates. */
		ChannelTarget target{ChannelTarget::Translation};

		/** @brief Interpolation method for this channel. */
		ChannelInterpolation interpolation{ChannelInterpolation::Linear};

		/** @brief Keyframes for translation or scale channels. Empty if target is Rotation. */
		std::vector< VectorKeyFrame< precision_t > > vectorKeyFrames{};

		/** @brief Keyframes for rotation channels. Empty if target is Translation or Scale. */
		std::vector< QuaternionKeyFrame< precision_t > > quaternionKeyFrames{};

		/**
		 * @brief Returns the number of keyframes in this channel.
		 * @return size_t
		 */
		[[nodiscard]]
		size_t
		keyFrameCount () const noexcept
		{
			if ( target == ChannelTarget::Rotation )
			{
				return quaternionKeyFrames.size();
			}

			return vectorKeyFrames.size();
		}

		/**
		 * @brief Returns true if this channel has no keyframes.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		empty () const noexcept
		{
			return keyFrameCount() == 0;
		}

		/**
		 * @brief Returns the timestamp of the last keyframe, or 0 if empty.
		 * @return precision_t
		 */
		[[nodiscard]]
		precision_t
		duration () const noexcept
		{
			if ( target == ChannelTarget::Rotation )
			{
				return quaternionKeyFrames.empty() ? precision_t{0} : quaternionKeyFrames.back().time;
			}

			return vectorKeyFrames.empty() ? precision_t{0} : vectorKeyFrames.back().time;
		}
	};

	using AnimationChannelF = AnimationChannel< float >;
	using AnimationChannelD = AnimationChannel< double >;
}