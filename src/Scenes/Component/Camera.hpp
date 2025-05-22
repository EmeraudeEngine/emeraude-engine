/*
 * src/Scenes/Component/Camera.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <cstddef>
#include <string>
#include <memory>

/* Local inclusions for inheritances. */
#include "AVConsole/AbstractVirtualDevice.hpp"
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Saphir/FramebufferEffectInterface.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief This class defines a physical point of view to capture image in the world.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::AVConsole::AbstractVirtualDevice This is a virtual video device.
	 */
	class Camera final : public Abstract, public AVConsole::AbstractVirtualDevice
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				LensEffectsChanged,
				/* Enumeration boundary. */
				MaxEnum
			};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				FieldOfView,
				Distance
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Camera"};

			/**
			 * @brief Constructs a camera.
			 * @param name The name of the component.
			 * @param parentEntity A reference to the parent entity.
			 * @param perspective Use a perspective projection.
			 */
			Camera (const std::string & name, const AbstractEntity & parentEntity, bool perspective = true) noexcept;

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
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::Camera;
			}

			/**
			 * @brief Sets the field of view in degrees.
			 * @param degrees A value between 0.0 and 360.0.
			 * @return void
			 */
			void setFieldOfView (float degrees) noexcept;

			/**
			 * @brief Returns the field of view in degrees.
			 * @return float
			 */
			[[nodiscard]]
			float
			fieldOfView () const noexcept
			{
				return m_fov;
			}

			/**
			 * @brief Returns whether the camera is using a perspective projection.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPerspectiveProjection () const noexcept
			{
				return this->isFlagEnabled(PerspectiveProjection);
			}

			/**
			 * @brief Returns whether the camera is using an orthographic projection.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOrthographicProjection () const noexcept
			{
				return !this->isFlagEnabled(PerspectiveProjection);
			}

			/**
			 * @brief Sets a perspective projection.
			 * @param fov The field of view in degrees.
			 * @param maxViewableDistance The distance of view. If not given, settings will be used.
			 * @return void
			 */
			void setPerspectiveProjection (float fov, float maxViewableDistance = 0.0F) noexcept;

			/**
			 * @brief Sets an orthographic projection.
			 * @param size The size of the render.
			 * @return void
			 */
			void setOrthographicProjection (float size) noexcept;

			/**
			 * @brief Updates the field of view with degrees.
			 * @param degrees The degrees to add or remove from the current value.
			 * @return void
			 */
			void
			changeFieldOfView (float degrees) noexcept
			{
				this->setFieldOfView(m_fov + degrees);
			}

			/**
			 * @brief Returns the maximal distance of the view.
			 * @return float
			 */
			[[nodiscard]]
			float
			distance () const noexcept
			{
				return m_distance;
			}

			/**
			 * @brief Sets the maximal distance of the view.
			 * @param distance The maximal distance of the view.
			 * @return void
			 */
			void setDistance (float distance) noexcept;

			/**
			 * @brief Returns the lens effect list.
			 * @return const Saphir::FramebufferEffectsList &
			 */
			[[nodiscard]]
			const Saphir::FramebufferEffectsList &
			lensEffects () const noexcept
			{
				return m_lensEffects;
			}

			/**
			 * @brief Checks if a shader lens effect is present.
			 * @param effect The effect to test.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLensEffectPresent (const std::shared_ptr< Saphir::FramebufferEffectInterface > & effect) const noexcept
			{
				return m_lensEffects.contains(effect);
			}

			/**
			 * @brief Adds a shader lens effect to the camera.
			 * @note This won't add the same effect twice.
			 * @param effect The effect to add.
			 * @return void
			 */
			void addLensEffect (const std::shared_ptr< Saphir::FramebufferEffectInterface > & effect) noexcept;

			/**
			 * @brief Removes a shader lens effect from the camera.
			 * @param effect The effect to remove.
			 * @return void
			 */
			void removeLensEffect (const std::shared_ptr< Saphir::FramebufferEffectInterface > & effect) noexcept;

			/**
			 * @brief Clears all shader lens effect of the camera.
			 * @return void
			 */
			void clearLensEffects () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Camera & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const Camera & obj) noexcept;

		private:
		
			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateProperties() */
			void updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onTargetConnected() */
			void onTargetConnected (AbstractVirtualDevice * targetDevice) noexcept override;

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/* Flag names */
			static constexpr auto PerspectiveProjection{UnusedFlag + 0UL};
		
			Saphir::FramebufferEffectsList m_lensEffects;
			float m_fov{DefaultGraphicsFieldOfView};
			float m_distance{DefaultGraphicsMaxViewableDistance};
	};
}
