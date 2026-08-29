/*
 * src/Scenes/Component/NodeAnimation.cpp
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

#include "NodeAnimation.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <ranges>

/* Local inclusions. */
#include "Animations/AnimationClipResource.hpp"
#include "Constants.hpp"
#include "Math/TransformUtils.hpp"
#include "Scenes/Node.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Base;
	using namespace EmEn::Base::Animation;
	using namespace EmEn::Base::Math;
	using namespace EmEn::Animations;

	void
	NodeAnimation::bindTarget (int32_t targetIndex, const std::shared_ptr< Node > & node) noexcept
	{
		if ( node == nullptr || targetIndex < 0 )
		{
			return;
		}

		const auto & frame = node->localCoordinates();

		m_targets[targetIndex] = Target{
			.node = node,
			.restTranslation = frame.position(),
			.restRotation = frame.toQuaternion(),
			.restScale = frame.scalingFactor()
		};
	}

	void
	NodeAnimation::addClip (const std::shared_ptr< AnimationClipResource > & clip) noexcept
	{
		if ( clip != nullptr )
		{
			/* ⚠️ Keyed on the CLIP's own name, exactly like SkeletalAnimator: a caller holding a
			 * clip list from a loader must be able to use one lookup key for both evaluators. */
			m_clips[clip->clip().name()] = clip;
		}
	}

	bool
	NodeAnimation::play (const std::string & clipName, PlaybackWrap wrap) noexcept
	{
		const auto clipIt = m_clips.find(clipName);

		if ( clipIt == m_clips.end() )
		{
			return false;
		}

		m_activeClip = clipIt->second;
		m_currentTime = 0.0F;
		m_wrap = wrap;
		m_playing = true;

		/* The first frame lands NOW rather than on the next logic cycle: a caller that plays and
		 * screenshots in the same breath must not read the previous clip's last frame. */
		this->evaluateAtTime(0.0F);

		return true;
	}

	void
	NodeAnimation::stop () noexcept
	{
		m_playing = false;
		m_currentTime = 0.0F;
		m_activeClip = nullptr;

		for ( const auto & target : m_targets | std::views::values )
		{
			const auto node = target.node.lock();

			if ( node == nullptr )
			{
				continue;
			}

			node->setLocalCoordinates(CartesianFrame< float >::fromQuaternion(target.restTranslation, target.restRotation, target.restScale));
		}
	}

	std::vector< std::string >
	NodeAnimation::clipNames () const noexcept
	{
		std::vector< std::string > names;
		names.reserve(m_clips.size());

		for ( const auto & name : m_clips | std::views::keys )
		{
			names.push_back(name);
		}

		std::ranges::sort(names);

		return names;
	}

	std::string
	NodeAnimation::activeClipName () const noexcept
	{
		if ( m_activeClip == nullptr )
		{
			return {};
		}

		return m_activeClip->clip().name();
	}

	void
	NodeAnimation::processLogics (const Scene & /*scene*/) noexcept
	{
		if ( !m_playing || m_activeClip == nullptr )
		{
			return;
		}

		m_currentTime += WorldPhysicsUpdateCycleDurationS< float > * m_speed;

		const auto duration = m_activeClip->clip().duration();

		if ( duration > 0.0F )
		{
			m_currentTime = this->wrapTime(m_currentTime, duration);
		}

		this->evaluateAtTime(m_currentTime);
	}

	void
	NodeAnimation::evaluateAtTime (float timeSeconds) noexcept
	{
		if ( m_activeClip == nullptr )
		{
			return;
		}

		/* ⚠️ Start from the REST frame of every target, never from its current one: a clip that
		 * only carries a rotation track must leave translation and scale alone, and reading them
		 * back from the node would accumulate whatever the previous clip left behind. */
		std::unordered_map< int32_t, Target > poses;
		poses.reserve(m_targets.size());

		const auto & clip = m_activeClip->clip();

		for ( size_t index = 0; index < clip.channelCount(); ++index )
		{
			const auto & channel = clip.channel(index);
			const auto targetIt = m_targets.find(channel.targetIndex);

			if ( targetIt == m_targets.end() )
			{
				continue;
			}

			auto [poseIt, inserted] = poses.try_emplace(channel.targetIndex, targetIt->second);

			switch ( channel.target )
			{
				case ChannelTarget::Translation :
					poseIt->second.restTranslation = channel.sampleVector(timeSeconds);
					break;

				case ChannelTarget::Rotation :
					poseIt->second.restRotation = channel.sampleQuaternion(timeSeconds);
					break;

				case ChannelTarget::Scale :
					poseIt->second.restScale = channel.sampleVector(timeSeconds);
					break;
			}
		}

		for ( const auto & pose : poses | std::views::values )
		{
			const auto node = pose.node.lock();

			if ( node == nullptr )
			{
				continue;
			}

			node->setLocalCoordinates(CartesianFrame< float >::fromQuaternion(pose.restTranslation, pose.restRotation, pose.restScale));
		}
	}

	float
	NodeAnimation::wrapTime (float time, float duration) noexcept
	{
		switch ( m_wrap )
		{
			case PlaybackWrap::Once :
			{
				if ( time >= duration )
				{
					m_playing = false;

					return duration;
				}

				return time;
			}

			case PlaybackWrap::Loop :
				return std::fmod(time, duration);

			case PlaybackWrap::PingPong :
			{
				const auto cycle = std::fmod(time, duration * 2.0F);

				if ( cycle > duration )
				{
					return (duration * 2.0F) - cycle;
				}

				return cycle;
			}
		}

		return time;
	}

	bool
	NodeAnimation::playAnimation (uint8_t /*animationID*/, const Variant & /*value*/, size_t /*cycle*/) noexcept
	{
		/* ⚠️ Deliberately empty: this component is NOT driven through AnimatableInterface. Its
		 * clips move OTHER entities' nodes, which that per-ID interface cannot express. */
		return false;
	}
}
