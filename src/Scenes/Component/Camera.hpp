/*
 * src/Scenes/Component/Camera.hpp
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
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"

/* Local inclusions for usages. */
#include "Graphics/DirectPostProcessEffect.hpp"
#include "Scenes/AVConsole/Types.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief This class defines a physical point of view to capture image in the world.
	 * @note [OBS][SHARED-OBSERVABLE]
	 * @todo Checks if this is the camera to hold the idea of using ortho or perspective projection.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Scenes::AVConsole::AbstractVirtualDevice This is a virtual video device.
	 */
	class EMEN_API Camera final : public Abstract, public AVConsole::AbstractVirtualDevice
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode : std::uint8_t
			{
				LensEffectsChanged,
				/* The enableDepthOfField()/enableHDR() state changed: the scene post-process
				 * chain must (de)materialize the camera-driven photographic effects. */
				PhysicalEffectsToggled,
				/* Enumeration boundary. */
				MaxEnum
			};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				FieldOfView,
				Distance,
				Aperture,
				FocalLength,
				FocusDistance,
				ExposureCompensation
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Camera"};

			/**
			 * @brief Constructs a camera.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param perspective Use a perspective projection.
			 */
			Camera (const std::string & componentName, const AbstractEntity & parentEntity, bool perspective = true) noexcept
				: Abstract{componentName, parentEntity},
				AbstractVirtualDevice{componentName, AVConsole::DeviceType::Video, AVConsole::ConnexionType::Output}
			{
				this->setFlag(PerspectiveProjection, perspective);

				/* Physical camera: the automatic modes are the default behaviour;
				 * manual control (focus distance, exposure bias) is the opt-out. */
				this->enableFlag(AutoFocusEnabled);
				this->enableFlag(AutoExposureEnabled);
			}

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
			void
			move (const Base::Math::CartesianFrame< float > & worldCoordinates) noexcept override
			{
				this->updateDeviceFromCoordinates(worldCoordinates, this->getWorldVelocity());
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void
			processLogics (const Scene & /*scene*/) noexcept override
			{
				this->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::Camera;
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
			 * @param distance The distance of view.
			 * @return void
			 */
			void setPerspectiveProjection (float fov, float distance) noexcept;

			/**
			 * @brief Sets the field of view in degrees.
			 * @param degrees A value between 0.0 and 360.0.
			 * @return void
			 */
			void setFieldOfView (float degrees) noexcept;

			/**
			 * @brief Updates the field of view by degrees.
			 * @param degrees The degrees to add or remove from the current value.
			 * @return void
			 */
			void
			changeFieldOfView (float degrees) noexcept
			{
				this->setFieldOfView(m_fov + degrees);
			}

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
			 * @brief Sets the maximal distance of the view.
			 * @param distance The maximal distance of the view.
			 * @return void
			 */
			void setDistance (float distance) noexcept;

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
			 * @brief Sets an orthographic projection.
			 * @param near The near distance.
			 * @param far The far distance.
			 * @return void
			 */
			void setOrthographicProjection (float near, float far) noexcept;

			/**
			 * @brief Sets the near parameter for an orthographic projection camera.
			 * @param distance A distance.
			 * @return void
			 */
			void setNear (float distance) noexcept;

			/**
			 * @brief Returns the near parameter of an orthographic projection camera.
			 * @return float
			 */
			[[nodiscard]]
			float
			getNear () const noexcept
			{
				return m_near;
			}

			/**
			 * @brief Sets the far parameter for an orthographic projection camera.
			 * @param distance A distance.
			 * @return void
			 */
			void setFar (float distance) noexcept;

			/**
			 * @brief Returns the far parameter of an orthographic projection camera.
			 * @return float
			 */
			[[nodiscard]]
			float
			getFar () const noexcept
			{
				return m_far;
			}

			/* ---- Physical camera (photographic) options ----
			 * The camera is the single source of truth for the photographic behaviour of the
			 * rendered image, like a real camera body: optics (aperture, focal length, focus)
			 * and exposure. The engine materializes the matching post-process effects
			 * (DepthOfField, ToneMapping) in the scene chain when enabled here; when disabled,
			 * these options are retained but have no effect (no-op contract). Every property is
			 * readable by the effects each frame: changes apply immediately, no rebuild. */

			/**
			 * @brief Enables the depth of field for this camera.
			 * @note Materializes the DepthOfField effect in the scene post-process chain.
			 * Auto-focus is enabled by default (see setFocusDistance() to go manual).
			 * @param state The state.
			 * @return void
			 */
			void enableDepthOfField (bool state) noexcept;

			/**
			 * @brief Returns whether the depth of field is enabled for this camera.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDepthOfFieldEnabled () const noexcept
			{
				return this->isFlagEnabled(DepthOfFieldEnabled);
			}

			/**
			 * @brief Enables the HDR rendering (tone mapping) for this camera.
			 * @note Materializes the ToneMapping effect in the scene post-process chain.
			 * Auto-exposure is enabled by default (see setExposureCompensation() to bias it).
			 * @param state The state.
			 * @return void
			 */
			void enableHDR (bool state) noexcept;

			/**
			 * @brief Returns whether the HDR rendering is enabled for this camera.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isHDREnabled () const noexcept
			{
				return this->isFlagEnabled(HDREnabled);
			}

			/**
			 * @brief Sets the lens aperture (f-stop).
			 * @note Lower f-stop = shallower depth of field (stronger blur). Typical range f/1.4 - f/22.
			 * @param fStop The aperture as an f-number.
			 * @return void
			 */
			void
			setAperture (float fStop) noexcept
			{
				m_aperture = std::max(fStop, 0.5F);
			}

			/**
			 * @brief Returns the lens aperture (f-stop).
			 * @return float
			 */
			[[nodiscard]]
			float
			aperture () const noexcept
			{
				return m_aperture;
			}

			/**
			 * @brief Sets the lens focal length in millimeters.
			 * @note Longer focal length = thinner in-focus plane. Does NOT drive the field of
			 * view in this version (decoupled optics).
			 * @param millimeters The focal length.
			 * @return void
			 */
			void
			setFocalLength (float millimeters) noexcept
			{
				m_focalLength = std::max(millimeters, 1.0F);
			}

			/**
			 * @brief Returns the lens focal length in millimeters.
			 * @return float
			 */
			[[nodiscard]]
			float
			focalLength () const noexcept
			{
				return m_focalLength;
			}

			/**
			 * @brief Sets a manual focus distance in meters.
			 * @note Like tapping to focus on a real camera: this DISABLES the auto-focus.
			 * Re-enable it with setAutoFocus(true).
			 * @param meters The distance of the focus plane.
			 * @return void
			 */
			void
			setFocusDistance (float meters) noexcept
			{
				m_focusDistance = std::max(meters, 0.01F);

				this->disableFlag(AutoFocusEnabled);
			}

			/**
			 * @brief Returns the manual focus distance in meters.
			 * @return float
			 */
			[[nodiscard]]
			float
			focusDistance () const noexcept
			{
				return m_focusDistance;
			}

			/**
			 * @brief Enables or disables the auto-focus (enabled by default).
			 * @param state The state.
			 * @return void
			 */
			void
			setAutoFocus (bool state) noexcept
			{
				this->setFlag(AutoFocusEnabled, state);
			}

			/**
			 * @brief Returns whether the auto-focus is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAutoFocusEnabled () const noexcept
			{
				return this->isFlagEnabled(AutoFocusEnabled);
			}

			/**
			 * @brief Sets the exposure compensation in EV (exposure bias).
			 * @note 0 = neutral, +1 EV = twice the light, -1 EV = half. Applies on top of the
			 * auto-exposure when it is enabled, or on the manual exposure otherwise.
			 * @param exposureValue The bias in EV.
			 * @return void
			 */
			void
			setExposureCompensation (float exposureValue) noexcept
			{
				m_exposureCompensation = exposureValue;
			}

			/**
			 * @brief Returns the exposure compensation in EV.
			 * @return float
			 */
			[[nodiscard]]
			float
			exposureCompensation () const noexcept
			{
				return m_exposureCompensation;
			}

			/**
			 * @brief Enables or disables the auto-exposure (enabled by default).
			 * @param state The state.
			 * @return void
			 */
			void
			setAutoExposure (bool state) noexcept
			{
				this->setFlag(AutoExposureEnabled, state);
			}

			/**
			 * @brief Returns whether the auto-exposure is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAutoExposureEnabled () const noexcept
			{
				return this->isFlagEnabled(AutoExposureEnabled);
			}

			/**
			 * @brief Returns the lens effect list.
			 * @return const std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > &
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
			isLensEffectPresent (const std::shared_ptr< Graphics::DirectPostProcessEffect > & effect) const noexcept
			{
				return std::find(m_lensEffects.cbegin(), m_lensEffects.cend(), effect) != m_lensEffects.cend();
			}

			/**
			 * @brief Adds a shader lens effect to the camera.
			 * @note This won't add the same effect twice.
			 * @param effect The effect to add.
			 * @return void
			 */
			void addLensEffect (const std::shared_ptr< Graphics::DirectPostProcessEffect > & effect) noexcept;

			/**
			 * @brief Removes a shader lens effect from the camera.
			 * @param effect The effect to remove.
			 * @return void
			 */
			void removeLensEffect (const std::shared_ptr< Graphics::DirectPostProcessEffect > & effect) noexcept;

			/**
			 * @brief Clears all shader lens effect of the camera.
			 * @return void
			 */
			void clearLensEffects () noexcept;

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return Abstract::getWorldCoordinates();
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & worldCoordinates, const Base::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onOutputDeviceConnected() */
			void onOutputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & targetDevice) noexcept override;

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/**
			 * @brief Updates render targets connected to this camera.
			 * @return void
			 */
			void updateAllVideoDeviceProperties () const noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Camera & obj);

			/* Flag names */
			static constexpr auto PerspectiveProjection{UnusedFlag + 0UL};
			static constexpr auto DepthOfFieldEnabled{UnusedFlag + 1UL};
			static constexpr auto HDREnabled{UnusedFlag + 2UL};
			static constexpr auto AutoFocusEnabled{UnusedFlag + 3UL};
			static constexpr auto AutoExposureEnabled{UnusedFlag + 4UL};

			std::vector< std::shared_ptr< Graphics::DirectPostProcessEffect > > m_lensEffects;
			float m_fov{DefaultGraphicsFieldOfView};
			float m_distance{DefaultGraphicsViewDistance};
			float m_near{0.0F};
			float m_far{DefaultGraphicsViewDistance};
			/* Physical camera options (photographic model, consumed by the post-process
			 * effects materialized through enableDepthOfField()/enableHDR()). */
			float m_aperture{2.8F}; /**< Lens aperture, as an f-number. */
			float m_focalLength{50.0F}; /**< Lens focal length, in millimeters. */
			float m_focusDistance{10.0F}; /**< Manual focus plane distance, in meters. */
			float m_exposureCompensation{0.0F}; /**< Exposure bias, in EV. */
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Camera & obj)
	{
		const auto coordinates = obj.getWorldCoordinates();
		const auto velocity = obj.getWorldVelocity();

		return out <<
			"Video Listener information" "\n"
			"Position: " << coordinates.position() << "\n"
			"Forward: " << coordinates.forwardVector() << "\n"
			"Velocity: " << velocity << "\n"
			"Field of view: " << obj.fieldOfView() << "\n"
			"Size of view: " << obj.distance() << "\n";
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Camera & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
