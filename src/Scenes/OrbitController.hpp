/*
 * src/Scenes/OrbitController.hpp
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
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Input/PointerListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Node;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Orbits a camera node around a fixed target point from pointer events.
	 * @details A click-drag on the primary button rotates the node around the target
	 * (azimuth and elevation), the mouse wheel moves the node closer or further (dolly).
	 * The node position is always recomputed from the canonical state
	 * (target, azimuth, elevation, distance), never integrated incrementally, and the
	 * dolly distance derives from an integer step index to stay exactly reproducible.
	 * The controller is inert without a controlled node and consumes no event.
	 * @note The controlled node must be a direct child of the scene root : the controller
	 * positions it in parent space, which equals world space only at the first level.
	 * @extends EmEn::Input::PointerListenerInterface
	 */
	class EMEN_API OrbitController final : public Input::PointerListenerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OrbitController"};

			/** @brief Rotation applied per pixel of pointer drag, in radians. */
			static constexpr auto RotationStep{0.005F};

			/** @brief Multiplicative dolly factor for one wheel step. */
			static constexpr auto DollyStepFactor{1.2F};

			/**
			 * @brief Constructs an orbit controller.
			 * @note Absolute pointer mode, no propagation of consumed events.
			 */
			OrbitController () noexcept
				: PointerListenerInterface{false, false, false}
			{

			}

			/**
			 * @brief Sets the camera node to move around the target.
			 * @note The node must be a direct child of the scene root.
			 * @param node A reference to a node smart pointer.
			 * @return void
			 */
			void
			controlNode (const std::shared_ptr< Node > & node) noexcept
			{
				m_controlledNode = node;

				this->applyToNode();
			}

			/**
			 * @brief Releases the controlled node.
			 * @return void
			 */
			void
			releaseNode () noexcept
			{
				m_controlledNode.reset();

				m_dragActive = false;
			}

			/**
			 * @brief Returns whether a node is controlled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasNode () const noexcept
			{
				return m_controlledNode != nullptr;
			}

			/**
			 * @brief Returns the controlled node.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			node () const noexcept
			{
				return m_controlledNode;
			}

			/**
			 * @brief Sets the world point the node orbits around and looks at.
			 * @param target A reference to a vector.
			 * @return void
			 */
			void setTarget (const Base::Math::Vector< 3, float > & target) noexcept;

			/**
			 * @brief Returns the orbited world point.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			target () const noexcept
			{
				return m_target;
			}

			/**
			 * @brief Sets the distance between the node and the target.
			 * @note This re-anchors the dolly : the value becomes the new reference
			 * distance and the wheel step index restarts from zero.
			 * @param distance The distance to the target.
			 * @return void
			 */
			void setDistance (float distance) noexcept;

			/**
			 * @brief Returns the current distance between the node and the target.
			 * @return float
			 */
			[[nodiscard]]
			float distance () const noexcept;

			/**
			 * @brief Sets the dolly distance limits.
			 * @param minimum The closest allowed distance.
			 * @param maximum The furthest allowed distance.
			 * @return void
			 */
			void setDistanceLimits (float minimum, float maximum) noexcept;

			/**
			 * @brief Sets the orbit angles.
			 * @param azimuth The angle around the world Y axis, in radians.
			 * @param elevation The angle above the target horizontal plane, in radians.
			 * Positive looks from above. Clamped near the poles to keep the view stable.
			 * @return void
			 */
			void setOrientation (float azimuth, float elevation) noexcept;

			/**
			 * @brief Returns the angle around the world Y axis, in radians.
			 * @return float
			 */
			[[nodiscard]]
			float
			azimuth () const noexcept
			{
				return m_azimuth;
			}

			/**
			 * @brief Returns the angle above the target horizontal plane, in radians.
			 * @return float
			 */
			[[nodiscard]]
			float
			elevation () const noexcept
			{
				return m_elevation;
			}

		private:

			/** @copydoc EmEn::Input::PointerListenerInterface::onPointerMove() */
			bool onPointerMove (float positionX, float positionY) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonPress() */
			bool onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonRelease() */
			bool onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override;

			/** @copydoc EmEn::Input::PointerListenerInterface::onMouseWheel() */
			bool onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers) noexcept override;

			/**
			 * @brief Recomputes the node position and aim from the canonical state.
			 * @return void
			 */
			void applyToNode () noexcept;

			/** @brief Gap kept between the elevation and the poles, in radians.
			 * @note The node up vector is derived from the aim direction, an orbit
			 * crossing a pole would flip the view. */
			static constexpr auto PoleGap{0.01F};

			std::shared_ptr< Node > m_controlledNode;
			Base::Math::Vector< 3, float > m_target;
			float m_azimuth{0.0F};
			float m_elevation{0.0F};
			float m_referenceDistance{10.0F};
			float m_minimumDistance{0.1F};
			float m_maximumDistance{4096.0F};
			int32_t m_dollyStepIndex{0};
			float m_lastPointerX{0.0F};
			float m_lastPointerY{0.0F};
			/** @brief Fractional wheel motion kept until a whole dolly step is reached (touchpads). */
			float m_pendingDollySteps{0.0F};
			bool m_dragActive{false};
	};
}
