/*
 * src/Animations/SkeletalAnimator.hpp
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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/* Local inclusions for usages. */
#include "Libs/Animation/Skin.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Quaternion.hpp"
#include "Libs/Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Animations
{
	/**
	 * @brief Playback mode for wrap behavior when time exceeds clip duration.
	 */
	enum class PlaybackWrap : uint8_t
	{
		Once,    /**< Play once and stop at the last frame. */
		Loop,    /**< Loop back to the beginning. */
		PingPong /**< Alternate forward/backward. */
	};

	/**
	 * @brief Per-instance skeletal animation evaluator.
	 *
	 * The SkeletalAnimator is agnostic about who controls time:
	 * - **Direct mode**: Call update(deltaTime) each frame — the animator advances its own clock.
	 * - **Timeline mode**: Call evaluate(time) — an external timeline drives the time.
	 *
	 * Both modes produce the same output: an array of skinning matrices ready for GPU upload.
	 *
	 * Pipeline: Sample keyframes → Local poses → Forward kinematics → Skinning matrices.
	 */
	class SkeletalAnimator final
	{
		public:

			/**
			 * @brief Constructs an empty animator.
			 */
			SkeletalAnimator () noexcept = default;

			/* ---- Setup ---- */

			/**
			 * @brief Sets the skeleton (shared, from resource system).
			 * @param skeleton The skeleton resource.
			 */
			void setSkeleton (const std::shared_ptr< SkeletonResource > & skeleton) noexcept;

			/**
			 * @brief Sets the skin binding (per-mesh, value).
			 * @param skin The mesh-to-skeleton binding.
			 */
			void setSkin (Libs::Animation::Skin< float > skin) noexcept;

			/**
			 * @brief Adds an animation clip (shared, from resource system).
			 * @param clip The animation clip resource.
			 */
			void addClip (const std::shared_ptr< AnimationClipResource > & clip) noexcept;

			/* ---- Direct mode (gameplay-driven) ---- */

			/**
			 * @brief Starts playing a clip by name.
			 * @param clipName The name of the clip to play.
			 * @param wrap The wrap mode. Default Loop.
			 * @return bool True if the clip was found and started.
			 */
			bool play (const std::string & clipName, PlaybackWrap wrap = PlaybackWrap::Loop) noexcept;

			/**
			 * @brief Stops playback and resets to bind pose.
			 */
			void stop () noexcept;

			/**
			 * @brief Pauses playback at the current time.
			 */
			void pause () noexcept;

			/**
			 * @brief Resumes playback after a pause.
			 */
			void resume () noexcept;

			/**
			 * @brief Sets the playback speed multiplier.
			 * @param speed The speed (1.0 = normal, 0.5 = half, 2.0 = double).
			 */
			void
			setSpeed (float speed) noexcept
			{
				m_speed = speed;
			}

			/**
			 * @brief Returns whether the animator is currently playing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPlaying () const noexcept
			{
				return m_playing;
			}

			/**
			 * @brief Returns the names of all registered animation clips.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			std::vector< std::string >
			clipNames () const noexcept
			{
				std::vector< std::string > names;
				names.reserve(m_clips.size());

				for ( const auto & [name, clip] : m_clips )
				{
					names.push_back(name);
				}

				std::sort(names.begin(), names.end());

				return names;
			}

			/**
			 * @brief Returns the name of the currently active clip.
			 * @return std::string Empty string if no clip is active.
			 */
			[[nodiscard]]
			std::string activeClipName () const noexcept;

			/* ---- Evaluation ---- */

			/**
			 * @brief Advances the internal clock and evaluates the pose.
			 * @note This is for direct/gameplay mode — the animator owns the time.
			 * @param deltaTimeSeconds Time elapsed since last frame, in seconds.
			 */
			void update (float deltaTimeSeconds) noexcept;

			/**
			 * @brief Evaluates the pose at an absolute time.
			 * @note This is for timeline mode — an external clock drives the time.
			 * The internal clock is NOT modified.
			 * @param timeSeconds The absolute time in seconds.
			 */
			void evaluate (float timeSeconds) noexcept;

			/* ---- Output ---- */

			/**
			 * @brief Returns the computed skinning matrices (one per skin joint).
			 * @note These matrices are in model space, ready for GPU upload via SSBO.
			 * @return const std::vector< Libs::Math::Matrix< 4, float > > &
			 */
			[[nodiscard]]
			const std::vector< Libs::Math::Matrix< 4, float > > &
			skinningMatrices () const noexcept
			{
				return m_skinningMatrices;
			}

			/**
			 * @brief Returns true if skinning matrices have been computed at least once.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPose () const noexcept
			{
				return !m_skinningMatrices.empty();
			}

		private:

			/**
			 * @brief Per-joint local transform (T/R/S), used as intermediate evaluation result.
			 */
			struct JointPose
			{
				Libs::Math::Vector< 3, float > translation{};
				Libs::Math::Quaternion< float > rotation{};
				Libs::Math::Vector< 3, float > scale{1.0F, 1.0F, 1.0F};
			};

			/**
			 * @brief Evaluates the full pose at the given time and computes skinning matrices.
			 * @param timeSeconds The time to evaluate at.
			 */
			void evaluateAtTime (float timeSeconds) noexcept;

			/**
			 * @brief Samples all channels of the active clip into m_localPoses.
			 * @param timeSeconds The time to sample at.
			 */
			void sampleClip (float timeSeconds) noexcept;

			/**
			 * @brief Computes world-space matrices from local poses via forward kinematics.
			 */
			void computeWorldMatrices () noexcept;

			/**
			 * @brief Computes skinning matrices from world matrices and inverse bind matrices.
			 */
			void computeSkinningMatrices () noexcept;

			/**
			 * @brief Wraps time according to the current playback mode and clip duration.
			 * @note May set m_playing to false for PlaybackWrap::Once when time exceeds duration.
			 * @param time The raw time.
			 * @param duration The clip duration.
			 * @return float The wrapped time.
			 */
			[[nodiscard]]
			float wrapTime (float time, float duration) noexcept;

			/* ---- Shared data (from resources) ---- */
			std::shared_ptr< SkeletonResource > m_skeleton;
			Libs::Animation::Skin< float > m_skin;
			std::unordered_map< std::string, std::shared_ptr< AnimationClipResource > > m_clips;

			/* ---- Per-instance playback state ---- */
			std::shared_ptr< AnimationClipResource > m_activeClip;
			float m_currentTime{0.0F};
			float m_speed{1.0F};
			PlaybackWrap m_wrap{PlaybackWrap::Loop};
			bool m_playing{false};
			bool m_paused{false};

			/* ---- Evaluated pose buffers ---- */
			std::vector< JointPose > m_localPoses;
			std::vector< Libs::Math::Matrix< 4, float > > m_worldMatrices;
			std::vector< Libs::Math::Matrix< 4, float > > m_skinningMatrices;
	};
}
