/*
 * src/Libs/Animation/AnimationClip.hpp
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
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

/* Local inclusions. */
#include "AnimationChannel.hpp"

namespace EmEn::Libs::Animation
{
	/**
	 * @brief A named animation clip containing channels that animate joint transforms over time.
	 * @note Clips are independent of any specific Skeleton. They reference joints by index,
	 * allowing the same clip to be shared between skeletons with compatible joint layouts.
	 * Clip data is immutable after construction (keyframes are static).
	 * @tparam precision_t Floating point type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class AnimationClip final
	{
		public:

			/**
			 * @brief Constructs an empty unnamed clip.
			 */
			AnimationClip () noexcept = default;

			/**
			 * @brief Constructs a named clip with channels.
			 * @param name The clip name (e.g., "Walk", "Idle", "Attack").
			 * @param channels The animation channels.
			 */
			AnimationClip (std::string name, std::vector< AnimationChannel< precision_t > > channels) noexcept
				: m_name(std::move(name)),
				  m_channels(std::move(channels))
			{
				this->computeDuration();
			}

			/**
			 * @brief Returns the clip name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns the total duration of the clip in seconds.
			 * @note This is the maximum timestamp across all channels.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			duration () const noexcept
			{
				return m_duration;
			}

			/**
			 * @brief Returns the number of channels in the clip.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			channelCount () const noexcept
			{
				return m_channels.size();
			}

			/**
			 * @brief Returns true if the clip has no channels.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_channels.empty();
			}

			/**
			 * @brief Returns the channel at the given index.
			 * @param index The channel index.
			 * @return const AnimationChannel< precision_t > &
			 */
			[[nodiscard]]
			const AnimationChannel< precision_t > &
			channel (size_t index) const noexcept
			{
				return m_channels[index];
			}

			/**
			 * @brief Returns the entire channel array.
			 * @return const std::vector< AnimationChannel< precision_t > > &
			 */
			[[nodiscard]]
			const std::vector< AnimationChannel< precision_t > > &
			channels () const noexcept
			{
				return m_channels;
			}

		private:

			/**
			 * @brief Computes the clip duration from the maximum channel duration.
			 */
			void
			computeDuration () noexcept
			{
				m_duration = precision_t{0};

				for ( const auto & ch : m_channels )
				{
					m_duration = std::max(m_duration, ch.duration());
				}
			}

			std::string m_name{};
			std::vector< AnimationChannel< precision_t > > m_channels{};
			precision_t m_duration{0};
	};

	using AnimationClipF = AnimationClip< float >;
	using AnimationClipD = AnimationClip< double >;
}