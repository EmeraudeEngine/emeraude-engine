/*
 * src/Scenes/Component/NodeAnimation.hpp
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
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Animations/PlaybackWrap.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Sphere.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Animations
	{
		class AnimationClipResource;
	}

	namespace Scenes
	{
		class Node;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Plays the animation clips that move a NODE HIERARCHY, as opposed to a skeleton.
	 *
	 * A format such as glTF animates plain nodes and skin joints through the very same construct:
	 * a rotating glass cover, a swinging door, a turning wheel are TRS tracks on nodes, with no
	 * skeleton and no skinning anywhere. Those need an evaluator of their own — writing a node's
	 * local frame each logic cycle — and this component is it.
	 *
	 * @note ⚠️ ONE clip drives MANY nodes, which is why this is NOT a per-node property and why it
	 * does not go through Animations::AnimatableInterface: that map is keyed by animation ID, one
	 * animation per node, with no way to select a clip. The component is placed on the ROOT of an
	 * imported hierarchy and holds weak references to its targets, so it dies with them and never
	 * dangles.
	 *
	 * @note The playback surface deliberately MIRRORS Animations::SkeletalAnimator
	 * (play/stop/isPlaying/clipNames/activeClipName): a caller cycling an asset's animations
	 * should not have to care which of the two evaluators the asset happens to need.
	 *
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_API NodeAnimation final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"NodeAnimation"};

			/**
			 * @brief Constructs a node animation component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			NodeAnimation (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{

			}

			/* ---- Setup ---- */

			/**
			 * @brief Binds a target index to the node the clips' channels address.
			 * @note ⚠️ The index is the one the CLIP uses (Base::Animation::AnimationChannel's
			 * targetIndex), which for an imported asset is the node's index in
			 * Scenes::Loaders::SceneData::nodes. The node's current local frame is captured here
			 * as its REST frame: a channel set that only animates the rotation leaves translation
			 * and scale at that value, and stop() restores it.
			 * @param targetIndex The index the clip channels address.
			 * @param node A reference to the node to drive.
			 * @return void
			 */
			void bindTarget (int32_t targetIndex, const std::shared_ptr< Node > & node) noexcept;

			/**
			 * @brief Adds an animation clip, indexed on the clip's own name.
			 * @param clip A reference to the clip resource.
			 * @return void
			 */
			void addClip (const std::shared_ptr< Animations::AnimationClipResource > & clip) noexcept;

			/* ---- Playback ---- */

			/**
			 * @brief Starts playing a clip by name.
			 * @param clipName The name of the clip to play.
			 * @param wrap The wrap mode. Default Loop.
			 * @return bool True if the clip was found and started.
			 */
			bool play (const std::string & clipName, Animations::PlaybackWrap wrap = Animations::PlaybackWrap::Loop) noexcept;

			/**
			 * @brief Stops playback and puts every target back on its rest frame.
			 * @note ⚠️ RESTORES, never merely stops writing: a node keeps whatever frame was last
			 * written to it, so dropping the clip would leave the hierarchy frozen mid-animation.
			 * @return void
			 */
			void stop () noexcept;

			/**
			 * @brief Sets the playback speed multiplier.
			 * @param speed The speed (1.0 = normal, 0.5 = half, 2.0 = double).
			 * @return void
			 */
			void
			setSpeed (float speed) noexcept
			{
				m_speed = speed;
			}

			/**
			 * @brief Returns whether a clip is currently playing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPlaying () const noexcept
			{
				return m_playing;
			}

			/**
			 * @brief Returns the names of every registered clip, sorted.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			std::vector< std::string > clipNames () const noexcept;

			/**
			 * @brief Returns the name of the currently active clip.
			 * @return std::string Empty when no clip is active.
			 */
			[[nodiscard]]
			std::string activeClipName () const noexcept;

			/* ---- Component contract ---- */

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return std::strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::localBoundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept override
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::localBoundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept override
			{
				return m_boundingSphere;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

		private:

			/**
			 * @brief The transform a target holds while the clip says nothing about it.
			 */
			struct Target
			{
				std::weak_ptr< Node > node;
				Base::Math::Vector< 3, float > restTranslation;
				Base::Math::Quaternion< float > restRotation;
				Base::Math::Vector< 3, float > restScale{1.0F, 1.0F, 1.0F};
			};

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/**
			 * @brief Samples the active clip at the given time and writes every target's frame.
			 * @param timeSeconds The time to evaluate at.
			 * @return void
			 */
			void evaluateAtTime (float timeSeconds) noexcept;

			/**
			 * @brief Wraps time according to the current playback mode and clip duration.
			 * @note May stop the playback for PlaybackWrap::Once when time exceeds the duration.
			 * @param time The raw time.
			 * @param duration The clip duration.
			 * @return float
			 */
			[[nodiscard]]
			float wrapTime (float time, float duration) noexcept;

			std::unordered_map< std::string, std::shared_ptr< Animations::AnimationClipResource > > m_clips;
			std::unordered_map< int32_t, Target > m_targets;
			std::shared_ptr< Animations::AnimationClipResource > m_activeClip;
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			float m_currentTime{0.0F};
			float m_speed{1.0F};
			Animations::PlaybackWrap m_wrap{Animations::PlaybackWrap::Loop};
			bool m_playing{false};
	};
}
