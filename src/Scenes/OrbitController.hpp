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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>

/* Local inclusions for inheritances. */
#include "AbstractNodeController.hpp"
#include "Input/PointerListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "Mutex.hpp"
#include "ThreadSafety.hpp"

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
	 * @note The pointer callbacks and the setters only change the canonical state, under the controller's mutex; the
	 * logic thread places the node from it at its next cycle (the family's contract: AbstractNodeController).
	 * @note The controlled node must be a direct child of the scene root : the controller
	 * positions it in parent space, which equals world space only at the first level.
	 * @extends EmEn::Scenes::AbstractNodeController The controlled node, and who writes it.
	 * @extends EmEn::Input::PointerListenerInterface
	 */
	class EMEN_API OrbitController final : public AbstractNodeController, public Input::PointerListenerInterface
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
			 * @brief Sets the world point the node orbits around and looks at.
			 * @note Any thread; the node follows at the logic thread's next cycle (at once from controlNode()).
			 * @param target A reference to a vector.
			 */
			void setTarget (const Base::Math::Vector< 3, float > & target) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Returns the orbited world point.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > target () const noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Sets the distance between the node and the target.
			 * @note This re-anchors the dolly : the value becomes the new reference
			 * distance and the wheel step index restarts from zero.
			 * @note Any thread; the node follows at the logic thread's next cycle (at once from controlNode()).
			 * @param distance The distance to the target.
			 */
			void setDistance (float distance) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Returns the current distance between the node and the target.
			 * @return float
			 */
			[[nodiscard]]
			float distance () const noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Sets the dolly distance limits.
			 * @note Any thread; the node follows at the logic thread's next cycle (at once from controlNode()).
			 * @param minimum The closest allowed distance.
			 * @param maximum The furthest allowed distance.
			 */
			void setDistanceLimits (float minimum, float maximum) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Sets the orbit angles.
			 * @note Any thread; the node follows at the logic thread's next cycle (at once from controlNode()).
			 * @param azimuth The angle around the world Y axis, in radians.
			 * @param elevation The angle above the target horizontal plane, in radians.
			 * Positive looks from above. Clamped near the poles to keep the view stable.
			 */
			void setOrientation (float azimuth, float elevation) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Returns the angle around the world Y axis, in radians.
			 * @return float
			 */
			[[nodiscard]]
			float azimuth () const noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Returns the angle above the target horizontal plane, in radians.
			 * @return float
			 */
			[[nodiscard]]
			float elevation () const noexcept EMEN_EXCLUDES(m_stateAccess);

		private:

			/** @brief The canonical state the node's position and aim are computed from. */
			struct Orbit final
			{
				Base::Math::Vector< 3, float > target;
				float azimuth{0.0F};
				float elevation{0.0F};
				float referenceDistance{10.0F};
				float minimumDistance{0.1F};
				float maximumDistance{4096.0F};
				int32_t dollyStepIndex{0};

				/**
				 * @brief Returns the distance to the target: the reference distance and the integer dolly index, clamped.
				 * @return float
				 */
				[[nodiscard]]
				float distance () const noexcept;
			};

			/**
			 * @brief Returns an elevation kept off the poles.
			 * @param elevation The angle above the target horizontal plane, in radians.
			 * @return float
			 */
			[[nodiscard]]
			static float clampedElevation (float elevation) noexcept;

			/** @copydoc EmEn::Scenes::AbstractNodeController::onNodeControlled() */
			void onNodeControlled () noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @copydoc EmEn::Scenes::AbstractNodeController::onNodeReleased() */
			void onNodeReleased () noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @copydoc EmEn::Scenes::AbstractNodeController::onUpdate() */
			void onUpdate (Node & node) noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @copydoc EmEn::Input::PointerListenerInterface::onPointerMove() */
			bool onPointerMove (float positionX, float positionY) noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonPress() */
			bool onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @copydoc EmEn::Input::PointerListenerInterface::onButtonRelease() */
			bool onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @copydoc EmEn::Input::PointerListenerInterface::onMouseWheel() */
			bool onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers) noexcept override EMEN_EXCLUDES(m_stateAccess);

			/** @brief Gap kept between the elevation and the poles, in radians.
			 * @note The node up vector is derived from the aim direction, an orbit
			 * crossing a pole would flip the view. */
			static constexpr auto PoleGap{0.01F};

			/** @brief Guards the canonical state and the drag: written by the pointer callbacks (main thread) and the
			 * setters (any thread), read by the logic thread that places the node. */
			mutable Base::Mutex m_stateAccess;
			Orbit m_orbit EMEN_GUARDED_BY(m_stateAccess);
			float m_lastPointerX EMEN_GUARDED_BY(m_stateAccess){0.0F};
			float m_lastPointerY EMEN_GUARDED_BY(m_stateAccess){0.0F};
			/** @brief Fractional wheel motion kept until a whole dolly step is reached (touchpads). */
			float m_pendingDollySteps EMEN_GUARDED_BY(m_stateAccess){0.0F};
			bool m_dragActive EMEN_GUARDED_BY(m_stateAccess){false};
			/** @brief The canonical state changed since the logic thread last placed the node. */
			bool m_orbitChanged EMEN_GUARDED_BY(m_stateAccess){false};
	};
}
