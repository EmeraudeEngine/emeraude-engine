/*
 * src/Animations/SkeletalAnimator.cpp
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

#include "SkeletalAnimator.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>

/* Local inclusions. */
#include "SkeletonResource.hpp"
#include "AnimationClipResource.hpp"
#include "Libs/Animation/AnimationChannel.hpp"
#include "Libs/Math/TransformUtils.hpp"

namespace EmEn::Animations
{
	using namespace Libs::Animation;
	using namespace Libs::Math;

	/* ---- Keyframe sampling helpers ---- */

	/**
	 * @brief Finds the index of the last keyframe at or before the given time.
	 * @return The index, or 0 if time is before all keyframes.
	 */
	template< typename KeyFrameType >
	static
	size_t
	findKeyFrameIndex (const std::vector< KeyFrameType > & keyFrames, float time) noexcept
	{
		if ( keyFrames.size() <= 1 )
		{
			return 0;
		}

		/* Binary search for the last keyframe with time <= given time. */
		size_t low = 0;
		size_t high = keyFrames.size() - 1;

		while ( low < high - 1 )
		{
			const auto mid = (low + high) / 2;

			if ( keyFrames[mid].time <= time )
			{
				low = mid;
			}
			else
			{
				high = mid;
			}
		}

		return low;
	}

	static
	Vector< 3, float >
	sampleVectorChannel (const AnimationChannel< float > & channel, float time) noexcept
	{
		const auto & kf = channel.vectorKeyFrames;

		if ( kf.empty() )
		{
			return {};
		}

		if ( kf.size() == 1 || time <= kf.front().time )
		{
			return kf.front().value;
		}

		if ( time >= kf.back().time )
		{
			return kf.back().value;
		}

		const auto idx = findKeyFrameIndex(kf, time);
		const auto & a = kf[idx];
		const auto & b = kf[idx + 1];

		if ( channel.interpolation == ChannelInterpolation::Step )
		{
			return a.value;
		}

		/* Linear interpolation. */
		const auto span = b.time - a.time;
		const auto t = (span > 0.0F) ? (time - a.time) / span : 0.0F;

		return Vector< 3, float >{
			a.value[0] + (b.value[0] - a.value[0]) * t,
			a.value[1] + (b.value[1] - a.value[1]) * t,
			a.value[2] + (b.value[2] - a.value[2]) * t
		};
	}

	static
	Quaternion< float >
	sampleQuaternionChannel (const AnimationChannel< float > & channel, float time) noexcept
	{
		const auto & kf = channel.quaternionKeyFrames;

		if ( kf.empty() )
		{
			return {};
		}

		if ( kf.size() == 1 || time <= kf.front().time )
		{
			return kf.front().value;
		}

		if ( time >= kf.back().time )
		{
			return kf.back().value;
		}

		const auto idx = findKeyFrameIndex(kf, time);
		const auto & a = kf[idx];
		const auto & b = kf[idx + 1];

		if ( channel.interpolation == ChannelInterpolation::Step )
		{
			return a.value;
		}

		/* Spherical linear interpolation. */
		const auto span = b.time - a.time;
		const auto t = (span > 0.0F) ? (time - a.time) / span : 0.0F;

		return Quaternion< float >::slerp(a.value, b.value, t, 0.05F);
	}

	/* ---- Setup ---- */

	void
	SkeletalAnimator::setSkeleton (const std::shared_ptr< SkeletonResource > & skeleton) noexcept
	{
		m_skeleton = skeleton;

		if ( m_skeleton != nullptr )
		{
			const auto jointCount = m_skeleton->skeleton().jointCount();

			m_localPoses.resize(jointCount);
			m_worldMatrices.resize(jointCount);
		}
	}

	void
	SkeletalAnimator::setSkin (Skin< float > skin) noexcept
	{
		m_skin = std::move(skin);
		m_skinningMatrices.resize(m_skin.jointCount());
	}

	void
	SkeletalAnimator::addClip (const std::shared_ptr< AnimationClipResource > & clip) noexcept
	{
		if ( clip != nullptr )
		{
			m_clips[clip->clip().name()] = clip;
		}
	}

	/* ---- Direct mode ---- */

	bool
	SkeletalAnimator::play (const std::string & clipName, PlaybackWrap wrap) noexcept
	{
		const auto it = m_clips.find(clipName);

		if ( it == m_clips.end() )
		{
			return false;
		}

		m_activeClip = it->second;
		m_currentTime = 0.0F;
		m_wrap = wrap;
		m_playing = true;
		m_paused = false;

		return true;
	}

	void
	SkeletalAnimator::stop () noexcept
	{
		m_playing = false;
		m_paused = false;
		m_currentTime = 0.0F;
		m_activeClip = nullptr;
		m_skinningMatrices.clear();
	}

	void
	SkeletalAnimator::pause () noexcept
	{
		m_paused = true;
	}

	void
	SkeletalAnimator::resume () noexcept
	{
		m_paused = false;
	}

	/* ---- Evaluation ---- */

	void
	SkeletalAnimator::update (float deltaTimeSeconds) noexcept
	{
		if ( !m_playing || m_paused || m_activeClip == nullptr || m_skeleton == nullptr )
		{
			return;
		}

		m_currentTime += deltaTimeSeconds * m_speed;

		const auto duration = m_activeClip->clip().duration();

		if ( duration > 0.0F )
		{
			m_currentTime = this->wrapTime(m_currentTime, duration);
		}

		this->evaluateAtTime(m_currentTime);
	}

	void
	SkeletalAnimator::evaluate (float timeSeconds) noexcept
	{
		if ( m_activeClip == nullptr || m_skeleton == nullptr )
		{
			return;
		}

		this->evaluateAtTime(timeSeconds);
	}

	/* ---- Internal pipeline ---- */

	void
	SkeletalAnimator::evaluateAtTime (float timeSeconds) noexcept
	{
		this->sampleClip(timeSeconds);
		this->computeWorldMatrices();
		this->computeSkinningMatrices();
	}

	void
	SkeletalAnimator::sampleClip (float timeSeconds) noexcept
	{
		const auto & skeleton = m_skeleton->skeleton();
		const auto jointCount = skeleton.jointCount();

		/* Start with bind pose for all joints. */
		for ( size_t i = 0; i < jointCount; ++i )
		{
			const auto & joint = skeleton.joint(i);

			m_localPoses[i].translation = joint.translation;
			m_localPoses[i].rotation = joint.rotation;
			m_localPoses[i].scale = joint.scale;
		}

		/* Override with sampled keyframes from the active clip. */
		const auto & clip = m_activeClip->clip();

		for ( size_t c = 0; c < clip.channelCount(); ++c )
		{
			const auto & channel = clip.channel(c);

			if ( channel.jointIndex < 0 || static_cast< size_t >(channel.jointIndex) >= jointCount )
			{
				continue;
			}

			auto & pose = m_localPoses[static_cast< size_t >(channel.jointIndex)];

			switch ( channel.target )
			{
				case ChannelTarget::Translation :
					pose.translation = sampleVectorChannel(channel, timeSeconds);
					break;

				case ChannelTarget::Rotation :
					pose.rotation = sampleQuaternionChannel(channel, timeSeconds);
					break;

				case ChannelTarget::Scale :
					pose.scale = sampleVectorChannel(channel, timeSeconds);
					break;
			}
		}
	}

	void
	SkeletalAnimator::computeWorldMatrices () noexcept
	{
		const auto & skeleton = m_skeleton->skeleton();
		const auto jointCount = skeleton.jointCount();

		/* Forward pass: parents always have lower indices (topological order). */
		for ( size_t i = 0; i < jointCount; ++i )
		{
			const auto & pose = m_localPoses[i];

			auto localMatrix = composeTRS(pose.translation, pose.rotation, pose.scale);

			const auto parentIndex = skeleton.joint(i).parentIndex;

			if ( parentIndex == NoParent )
			{
				m_worldMatrices[i] = localMatrix;
			}
			else
			{
				m_worldMatrices[i] = m_worldMatrices[static_cast< size_t >(parentIndex)] * localMatrix;
			}
		}
	}

	void
	SkeletalAnimator::computeSkinningMatrices () noexcept
	{
		const auto skinJointCount = m_skin.jointCount();

		m_skinningMatrices.resize(skinJointCount);

		for ( size_t i = 0; i < skinJointCount; ++i )
		{
			const auto skeletonIndex = static_cast< size_t >(m_skin.skeletonJointIndex(i));

			m_skinningMatrices[i] = m_worldMatrices[skeletonIndex] * m_skin.inverseBindMatrix(i);
		}
	}

	float
	SkeletalAnimator::wrapTime (float time, float duration) noexcept
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
			{
				return std::fmod(time, duration);
			}

			case PlaybackWrap::PingPong :
			{
				const auto cycle = std::fmod(time, duration * 2.0F);

				if ( cycle > duration )
				{
					return duration * 2.0F - cycle;
				}

				return cycle;
			}
		}

		return time;
	}
}
