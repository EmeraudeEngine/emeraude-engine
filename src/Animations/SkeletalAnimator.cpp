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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "SkeletalAnimator.hpp"

/* STL inclusions. */
#include <cmath>
#include <utility>

/* Local inclusions. */
#include "AnimationClipResource.hpp"
#include "Animation/AnimationChannel.hpp"
#include "Math/Base.hpp"
#include "Math/TransformUtils.hpp"
#include "SkeletonResource.hpp"

namespace EmEn::Animations
{
	using namespace Base::Animation;
	using namespace Base::Math;

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

	std::string
	SkeletalAnimator::activeClipName () const noexcept
	{
		if ( m_activeClip != nullptr )
		{
			return m_activeClip->clip().name();
		}

		return {};
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

		/* ⚠️ EVALUATE the bind pose rather than dropping the matrices: the consumer only stages
		 * a pose when hasPose() is true, and the render side keeps uploading whatever it staged
		 * last. Clearing froze the model on its last animated frame. */
		if ( m_skeleton == nullptr )
		{
			m_skinningMatrices.clear();

			return;
		}

		this->sampleBindPose();
		this->computeWorldMatrices();
		this->computeSkinningMatrices();
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
	SkeletalAnimator::sampleBindPose () noexcept
	{
		const auto & skeleton = m_skeleton->skeleton();
		const auto jointCount = skeleton.jointCount();

		for ( size_t i = 0; i < jointCount; ++i )
		{
			const auto & joint = skeleton.joint(i);

			m_localPoses[i].translation = joint.translation;
			m_localPoses[i].rotation = joint.rotation;
			m_localPoses[i].scale = joint.scale;
		}
	}

	void
	SkeletalAnimator::sampleClip (float timeSeconds) noexcept
	{
		const auto & skeleton = m_skeleton->skeleton();
		const auto jointCount = skeleton.jointCount();

		/* Start with bind pose for all joints. */
		this->sampleBindPose();

		/* Override with sampled keyframes from the active clip. */
		const auto & clip = m_activeClip->clip();

		for ( size_t c = 0; c < clip.channelCount(); ++c )
		{
			const auto & channel = clip.channel(c);

			if ( channel.targetIndex < 0 || static_cast< size_t >(channel.targetIndex) >= jointCount )
			{
				continue;
			}

			auto & pose = m_localPoses[static_cast< size_t >(channel.targetIndex)];

			switch ( channel.target )
			{
				case ChannelTarget::Translation :
					pose.translation = channel.sampleVector(timeSeconds);
					break;

				case ChannelTarget::Rotation :
					pose.rotation = channel.sampleQuaternion(timeSeconds);
					break;

				case ChannelTarget::Scale :
					pose.scale = channel.sampleVector(timeSeconds);
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

		/* Animated joints bounding box (model space): the culling volume of a skinned mesh
		 * must FOLLOW the animation — a bind-pose volume culls the parts the pose moved
		 * outside of it (wings vanishing at the screen edge, measured on the dragon). */
		m_jointsBoundingBox.reset();

		for ( size_t i = 0; i < jointCount; ++i )
		{
			m_jointsBoundingBox.merge(m_worldMatrices[i].column(3));
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
