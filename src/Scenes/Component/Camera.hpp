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
#include <cmath>
#include <memory>
#include <mutex>
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
				/* NOTE: There is no FieldOfView entry: the framing is animated through
				 * FocalLength, the two being the same quantity. A zoom IS a focal ramp. */
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

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return Abstract::getWorldCoordinates();
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & worldCoordinates, const Base::Math::Vector< 3, float > & worldVelocity) noexcept override;

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
			 * @note The FRAMING is not a parameter here: it belongs to the optics. A camera is
			 * configured like a real one — `setFocalLength()` and `setSensorWidth()` — and the
			 * projection matrices follow from them. This only declares the projection KIND and how
			 * far the camera sees; switching back from an orthographic projection therefore
			 * restores the lens that was already mounted.
			 * @param distance The distance of view.
			 * @return void
			 */
			void setPerspectiveProjection (float distance) noexcept;

			/**
			 * @brief Pins the field of view of a TECHNICAL camera, in degrees.
			 * @warning NOT a photographic control, and the ONLY way an angle enters the camera. It
			 * exists for cameras whose field of view is a GEOMETRIC constraint rather than a
			 * creative choice: a cubemap face is strictly 90 degrees or the six faces do not join.
			 * Such a camera has no photographic meaning — no format, no lens, no grading — so it
			 * also LOCKS the sensor width, since changing the format would silently break the
			 * angle (see `setSensorWidth()`).
			 * @note Implemented as the focal length that yields the angle on the current sensor, so
			 * there is still a single source of truth. 90 degrees on a 24 mm-high sensor is exactly
			 * 12 mm; the tan/atan round trip costs about 1e-5 degree, four orders of magnitude
			 * below one pixel on a cube face.
			 * @param degrees The required field of view, in degrees.
			 * @return void
			 */
			void setTechnicalFieldOfView (float degrees) noexcept;

			/**
			 * @brief Returns whether this camera's field of view is a pinned geometric constraint.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTechnicalCamera () const noexcept
			{
				return this->isFlagEnabled(TechnicalProjection);
			}

			/**
			 * @brief Returns the vertical field of view in degrees, DERIVED from the optics.
			 * @note Not a setting — a consequence. `fov = 2 * atan(h / (2f))`, `h` being
			 * `sensorHeight()`. This is what the projection matrices consume, which is the only
			 * reason the accessor exists; to change the framing, change the lens.
			 * @return float
			 */
			[[nodiscard]]
			float
			fieldOfView () const noexcept
			{
				return Base::Math::Degree(2.0F * std::atan(this->sensorHeight() / (2.0F * m_focalLength)));
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
			 * and exposure, plus the shutter speed that drives the motion blur length. The engine
			 * materializes the matching post-process effects — DepthOfField, MotionBlur, Bloom and
			 * ToneMapping, in that canonical (physical) order — in the scene chain when enabled
			 * here; when disabled, these options are retained but have no effect (no-op contract).
			 * Every property is readable by the effects each frame: changes apply immediately, no
			 * rebuild. */

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
			 * @brief Enables the lens glare (bloom) for this camera.
			 * @note Materializes the Bloom effect in the scene post-process chain, between the
			 * depth of field and the tone mapping. Veiling glare is scattering INSIDE the lens:
			 * it applies to the image the optics have already formed, so it belongs after the
			 * defocus and the motion smear and before the sensor — which is exactly what that
			 * position gives, and what a bloom sitting early in the scene stack does not.
			 * @param state The state.
			 * @return void
			 */
			void enableBloom (bool state) noexcept;

			/**
			 * @brief Materializes or removes the MOTION BLUR for this camera.
			 * @note Photographic like the depth of field: the blur LENGTH is not a strength slider
			 * but `setShutterSpeed()` divided by the frame duration — the shutter angle, i.e. the
			 * fraction of the frame during which light was collected. 1/48 s at 24 fps is the
			 * cinematic 180-degree rule. This only decides whether the effect EXISTS; the exposure
			 * time decides how long the smear is, which is why the result is framerate-independent.
			 * @param state The state.
			 * @return void
			 */
			void enableMotionBlur (bool state) noexcept;

			/**
			 * @brief Returns whether the motion blur is enabled for this camera.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMotionBlurEnabled () const noexcept
			{
				return this->isFlagEnabled(MotionBlurEnabled);
			}

			/**
			 * @brief Returns whether the lens glare is enabled for this camera.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBloomEnabled () const noexcept
			{
				return this->isFlagEnabled(BloomEnabled);
			}

			/**
			 * @brief Sets the scene luminance above which the lens glares, in nits (cd/m²).
			 * @note An absolute scene luminance, because the glare happens before the sensor: a
			 * wall in a lit interior sits around 15-30 nits, an overcast sky 8000, a bare lamp far
			 * above. A night scene glows from a 20-nit torch-lit wall; a daylight one must not.
			 * @param nits The threshold, in candela per square meter.
			 * @return void
			 */
			void
			setBloomThreshold (float nits) noexcept
			{
				m_bloomThreshold = std::max(0.0F, nits);
			}

			/**
			 * @brief Returns the glare threshold, in nits.
			 * @return float
			 */
			[[nodiscard]]
			float
			bloomThreshold () const noexcept
			{
				return m_bloomThreshold;
			}

			/**
			 * @brief Sets the FRACTION of the above-threshold energy the lens scatters.
			 * @note This is a physical quantity, not an artistic gain: the glare pipeline carries
			 * the full photometric energy of the bright sources (a sunlit wall is ~20000 nits),
			 * and this fraction is what the glass spreads across the image. A clean modern lens
			 * scatters 2-5 percent; 1.0 would mean the lens diffuses ALL of it and sets any
			 * daylight scene ablaze.
			 * @param intensity The scattered fraction (0.03 = 3 percent, the default).
			 * @return void
			 */
			void
			setBloomIntensity (float intensity) noexcept
			{
				m_bloomIntensity = std::max(0.0F, intensity);
			}

			/**
			 * @brief Returns the glare intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			bloomIntensity () const noexcept
			{
				return m_bloomIntensity;
			}

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
			 * @note THE single source of truth for the framing: longer focal length = narrower
			 * field of view AND thinner in-focus plane. This REFRAMES the shot, the field of view
			 * being derived from it (`fieldOfView()`).
			 * @param millimeters The focal length.
			 * @return void
			 */
			void setFocalLength (float millimeters) noexcept;

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
			 * @brief Sets the shutter speed (exposure time) in seconds.
			 * @note Photographic driver of the MOTION BLUR length: the effect blurs along the
			 * velocity buffer over the fraction of the frame the shutter stays open, i.e. the
			 * SHUTTER ANGLE = shutterSpeed / frameTime. `1/48` s at 24 fps is the cinematic
			 * 180-degree rule (angle 0.5). ⚠️ An angle ABOVE 1 is the NORMAL case at high
			 * framerates and must NOT be clamped to 1 — that clamp was a lived regression (the
			 * blur shrank as the framerate rose); the effect only caps it at 128 as hygiene, the
			 * effective ceiling being its maximum blur radius in pixels. Expressing it in seconds
			 * is what makes the blur INDEPENDENT of the framerate: at a fixed shutter speed, a
			 * frame twice as long simply covers twice the motion, exactly as a real camera would.
			 * @param seconds The exposure time (e.g. 1.0F / 60.0F).
			 * @return void
			 */
			void
			setShutterSpeed (float seconds) noexcept
			{
				m_shutterSpeed = std::clamp(seconds, 1.0F / 8000.0F, 1.0F);
			}

			/**
			 * @brief Returns the shutter speed (exposure time) in seconds.
			 * @return float
			 */
			[[nodiscard]]
			float
			shutterSpeed () const noexcept
			{
				return m_shutterSpeed;
			}

			/**
			 * @brief Sets the sensor width, in millimeters.
			 * @note Full-frame (36 mm) by default, which is the format the focal lengths are
			 * implicitly expressed in. It is what converts a circle of confusion — computed in
			 * meters ON the sensor by the thin-lens formula — into a fraction of the image, so it
			 * is the reason the depth of field needs no arbitrary scale factor.
			 * @note CONSTANT LENS: changing the format keeps the focal length and REFRAMES, because
			 * a smaller sensor crops the image circle of the same lens — the physical crop factor
			 * (an APS-C body at 23.6 mm sees 1.53x narrower than full frame). The field of view
			 * being derived, nothing needs synchronizing; the connected render targets are simply
			 * re-notified.
			 * @warning IGNORED on a technical camera (`setTechnicalFieldOfView()`), whose angle is a
			 * geometric constraint: a cubemap face must stay at 90 degrees, and the format is what
			 * would silently break it.
			 * @param millimeters The sensor width (36 = full frame, 23.6 = APS-C).
			 * @return void
			 */
			void setSensorWidth (float millimeters) noexcept;

			/**
			 * @brief Returns the sensor width, in millimeters.
			 * @return float
			 */
			[[nodiscard]]
			float
			sensorWidth () const noexcept
			{
				return m_sensorWidth;
			}

			/**
			 * @brief Returns the sensor height, in millimeters.
			 * @note Derived from the width through the 3:2 still-photography format (36 x 24 mm
			 * full frame), which is the convention the focal lengths are quoted in. It is the
			 * dimension the FIELD OF VIEW is computed from, the engine's field of view being
			 * vertical.
			 * @return float
			 */
			[[nodiscard]]
			float
			sensorHeight () const noexcept
			{
				return m_sensorWidth * (2.0F / 3.0F);
			}

			/**
			 * @brief Sets the sensor sensitivity, in ISO.
			 * @note The third member of the exposure triad, and the ONLY one the metering is
			 * allowed to move: the aperture drives the depth of field and the shutter speed drives
			 * the motion blur, so both are creative controls. With the auto-exposure on, this is
			 * the value the metering lands on, bounded by the sensor range below — which is what
			 * makes auto mode honest: a slow shutter blurs more WITHOUT over-exposing, because the
			 * ISO drops to compensate, exactly as in live-action shooting.
			 * @param iso The sensitivity (100 = the ISO 100 reference).
			 * @return void
			 */
			void
			setSensitivity (float iso) noexcept
			{
				m_sensitivity = std::clamp(iso, m_minSensitivity, m_maxSensitivity);
			}

			/**
			 * @brief Returns the sensor sensitivity, in ISO.
			 * @return float
			 */
			[[nodiscard]]
			float
			sensitivity () const noexcept
			{
				return m_sensitivity;
			}

			/**
			 * @brief Sets the usable sensitivity range of the sensor, in ISO.
			 * @note The bounds of the auto-ISO metering. They REPLACE the arbitrary exposure
			 * clamps a tone mapper would otherwise carry: a real body cannot amplify beyond its
			 * sensor, and expressing the limit in ISO makes it mean something.
			 * @param minimum The lowest usable sensitivity.
			 * @param maximum The highest usable sensitivity.
			 * @return void
			 */
			void
			setSensitivityRange (float minimum, float maximum) noexcept
			{
				m_minSensitivity = std::max(1.0F, minimum);
				m_maxSensitivity = std::max(m_minSensitivity, maximum);
				m_sensitivity = std::clamp(m_sensitivity, m_minSensitivity, m_maxSensitivity);
			}

			/**
			 * @brief Returns the lowest usable sensitivity, in ISO.
			 * @return float
			 */
			[[nodiscard]]
			float
			minSensitivity () const noexcept
			{
				return m_minSensitivity;
			}

			/**
			 * @brief Returns the highest usable sensitivity, in ISO.
			 * @return float
			 */
			[[nodiscard]]
			float
			maxSensitivity () const noexcept
			{
				return m_maxSensitivity;
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
			 * @note 0 = neutral, +1 EV = twice the light, -1 EV = half. With the auto-exposure
			 * ON, the bias shifts the METERING TARGET and therefore saturates at the sensor's
			 * ISO bounds, exactly as on a real auto-ISO body — it does not post-amplify past
			 * the sensor. In manual mode it is a straight EV bias on the APEX exposure.
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
			 * @brief Returns the current lens-effect snapshot, or nullptr when the camera has none.
			 * @note Thread-safe PUBLICATION contract: every mutation replaces the list wholesale
			 * (copy-on-write under the camera's lock), so the returned list is immutable — a
			 * caller iterates its own snapshot, never a vector another thread can reallocate
			 * (the KeyPad style-cycling crash this replaces). GPU lifetime is the RENDERER's
			 * side of the contract: it retains the snapshot it records per frame in flight, so
			 * an effect removed here cannot be destroyed while a command buffer references it.
			 * @return std::shared_ptr< const Graphics::DirectEffectList >
			 */
			[[nodiscard]]
			std::shared_ptr< const Graphics::DirectEffectList >
			lensEffects () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_lensEffectsAccess};

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
				const auto snapshot = this->lensEffects();

				return snapshot != nullptr && std::ranges::find(*snapshot, effect) != snapshot->cend();
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

		protected:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void
			onSuspend () noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void
			onWakeup () noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onOutputDeviceConnected() */
			void onOutputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & targetDevice) noexcept override;

		private:

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
			static constexpr auto BloomEnabled{UnusedFlag + 5UL};
			/** @brief The field of view is a GEOMETRIC constraint (cubemap face): the format is
			 * locked, because changing it would reframe and break the required angle. */
			static constexpr auto TechnicalProjection{UnusedFlag + 6UL};
			static constexpr auto MotionBlurEnabled{UnusedFlag + 7UL};

			/** @brief Guards the PUBLICATION of the lens-effect list (the pointer swap), not its
			 * content: published lists are immutable, mutators build a fresh copy (copy-on-write). */
			mutable std::mutex m_lensEffectsAccess;
			/** @brief Immutable lens-effect snapshot; nullptr = no lens effect. */
			std::shared_ptr< const Graphics::DirectEffectList > m_lensEffects;
			float m_distance{DefaultGraphicsViewDistance};
			float m_near{0.0F};
			float m_far{DefaultGraphicsViewDistance};
			/* Physical camera options (photographic model, consumed by the post-process
			 * effects materialized through enableDepthOfField()/enableHDR()). */
			float m_aperture{2.8F}; /**< Lens aperture, as an f-number. */
			float m_focalLength{DefaultGraphicsFocalLength}; /**< Lens focal length, in millimeters — the SINGLE source of truth for the framing, the field of view being derived from it. */
			float m_focusDistance{10.0F}; /**< Manual focus plane distance, in meters. */
			float m_shutterSpeed{1.0F / 60.0F}; /**< Exposure time, in seconds: drives the motion blur length through the shutter angle (shutterSpeed / frameTime). */
			float m_sensorWidth{36.0F}; /**< Sensor width in millimeters — full frame. */
			float m_sensitivity{100.0F}; /**< Sensor sensitivity in ISO — the third member of the exposure triad. */
			float m_minSensitivity{100.0F}; /**< Lowest usable sensitivity, the auto-ISO floor. */
			float m_maxSensitivity{12800.0F}; /**< Highest usable sensitivity, the auto-ISO ceiling. */
			float m_bloomThreshold{1000.0F}; /**< Scene luminance above which the lens glares, in nits. */
			float m_bloomIntensity{0.03F}; /**< Fraction of the above-threshold energy the lens scatters (a clean lens: 2-5%). */
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
			"Focal length: " << obj.focalLength() << " mm on a " << obj.sensorWidth() << " mm sensor\n"
			"Field of view: " << obj.fieldOfView() << " degrees (derived)\n"
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
