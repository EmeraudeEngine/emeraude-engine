/*
 * src/Scenes/Component/CameraConsoleAdapter.cpp
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

#include "ConsoleAdapter.hpp"

/* STL inclusions. */
#include <cmath>
#include <optional>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "Camera.hpp"
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Scenes/Manager.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/* The two parameters every command of this file starts with. */
		const Console::Parameter EntityParameter{entityParameter("the camera")};
		const Console::Parameter ComponentParameter{componentParameter(Camera::ClassId)};

		/**
		 * @brief Returns the full state of a camera as a JSON object.
		 * @param camera The camera.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		stateValue (const Camera & camera) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = camera.name();
			state["entity"] = camera.parentEntity().name();
			state["type"] = Camera::ClassId;
			state["viewDistance"] = static_cast< double >(camera.distance());

			/* ⚠️ getNear()/getFar() are the ORTHOGRAPHIC parameters (the stream operator of Camera says why): a
			 * perspective camera derives its near plane, so they are never reported for it. */
			if ( camera.isPerspectiveProjection() )
			{
				state["projection"] = "perspective";
				state["technical"] = camera.isTechnicalCamera();
				state["focalLengthMillimetres"] = static_cast< double >(camera.focalLength());
				state["sensorWidthMillimetres"] = static_cast< double >(camera.sensorWidth());
				state["sensorHeightMillimetres"] = static_cast< double >(camera.sensorHeight());
				state["fieldOfViewDegrees"] = static_cast< double >(camera.fieldOfView());
				state["nearestObjectDistance"] = static_cast< double >(camera.nearestObjectDistance());
			}
			else
			{
				state["projection"] = "orthographic";
				state["near"] = static_cast< double >(camera.getNear());
				state["far"] = static_cast< double >(camera.getFar());
			}

			Json::Value exposure{Json::objectValue};
			exposure["auto"] = camera.isAutoExposureEnabled();
			exposure["aperture"] = static_cast< double >(camera.aperture());
			exposure["shutterSeconds"] = static_cast< double >(camera.shutterSpeed());
			exposure["iso"] = static_cast< double >(camera.sensitivity());
			exposure["minIso"] = static_cast< double >(camera.minSensitivity());
			exposure["maxIso"] = static_cast< double >(camera.maxSensitivity());
			exposure["compensationEV"] = static_cast< double >(camera.exposureCompensation());
			state["exposure"] = std::move(exposure);

			Json::Value focus{Json::objectValue};
			focus["auto"] = camera.isAutoFocusEnabled();
			focus["distance"] = static_cast< double >(camera.focusDistance());
			state["focus"] = std::move(focus);

			Json::Value bloom{Json::objectValue};
			bloom["enabled"] = camera.isBloomEnabled();
			bloom["thresholdNits"] = static_cast< double >(camera.bloomThreshold());
			bloom["intensity"] = static_cast< double >(camera.bloomIntensity());
			state["bloom"] = std::move(bloom);

			/* Read-only here: Core/Graphics/PostProcessing/DepthOfField|MotionBlur/Enabled override the camera
			 * (Renderer syncCameraEffects()), so a console switch would be silently undone. */
			Json::Value effects{Json::objectValue};
			effects["HDR"] = camera.isHDREnabled();
			effects["depthOfField"] = camera.isDepthOfFieldEnabled();
			effects["motionBlur"] = camera.isMotionBlurEnabled();
			state["effects"] = std::move(effects);

			return state;
		}

		/**
		 * @brief Returns the full state of a camera as JSON.
		 * @param camera The camera.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const Camera & camera) noexcept
		{
			return FastJSON::stringify(stateValue(camera));
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the camera's NEW state as JSON.
		 * @param camera The camera, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		[[nodiscard]]
		Console::CommandResult
		changed (const Camera & camera, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(camera));
		}

		/** @brief `Core.SceneManagerService.Camera.*`. */
		class CameraConsoleAdapter final : public ConsoleAdapter< Camera >
		{
			public:

				explicit
				CameraConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					this->bindCommand("getActive", "Returns the camera the active scene renders from, as JSON: its entity `address` and component `name` (what every other Camera command takes as entity and component), then its full state.",
						[this] () {
							auto result = Console::CommandResult::error("No active scene !");

							this->sceneManager().withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
								const auto camera = scene->activeCamera();

								if ( camera == nullptr )
								{
									result = Console::CommandResult::error("The scene '" + scene->name() + "' has no active camera.");

									return;
								}

								/* The ADDRESS, not the name: every actor carries a `Head` node, so the name alone may
								 * designate another camera (measured on animation-debug: six `Head`). */
								auto state = stateValue(*camera);
								state["address"] = entityAddresses(*scene)[&camera->parentEntity()];

								result = Console::CommandResult::json(FastJSON::stringify(state));
							}, true);

							return result;
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("getState", "Returns the state of a camera as JSON: projection and lens (millimetres, degrees), view distance (metres), exposure triad, focus, bloom and the post-process effects it carries.",
						{EntityParameter, ComponentParameter},
						[this] (const std::string & entity, const std::string & component) {
							return this->act(entity, component, [] (Camera & camera) {
								return Console::CommandResult::json(stateOf(camera));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setLens", "Sets the lens of a camera. The field of view is DERIVED from the focal length and the sensor width, never set directly. Refused for a technical camera (a pinned geometric field of view, e.g. a cubemap face).",
						{
							EntityParameter,
							ComponentParameter,
							{"focal", "Focal length, in millimetres, 1 or more (50 mm is the normal lens on the default 36 mm full-frame sensor)."},
							{"sensorWidth", "Sensor width, in millimetres, 1 or more (36 = full frame, 23.6 = APS-C); unchanged when omitted."}
						},
						[this] (const std::string & entity, const std::string & component, float focal, std::optional< float > sensorWidth) {
							if ( !std::isfinite(focal) || focal < 1.0F || ( sensorWidth.has_value() && ( !std::isfinite(*sensorWidth) || *sensorWidth < 1.0F ) ) )
							{
								return Console::CommandResult::error("The focal length and the sensor width must be finite numbers of millimetres, 1 or more.");
							}

							return this->act(entity, component, [focal, sensorWidth] (Camera & camera) {
								if ( !camera.isPerspectiveProjection() )
								{
									return Console::CommandResult::error("Camera '" + camera.name() + "' uses an orthographic projection: it has no lens.");
								}

								if ( camera.isTechnicalCamera() )
								{
									return Console::CommandResult::error("Camera '" + camera.name() + "' is a technical camera: its field of view is a geometric constraint and its lens is locked.");
								}

								camera.setFocalLength(focal);

								if ( sensorWidth.has_value() )
								{
									camera.setSensorWidth(*sensorWidth);
								}

								return changed(camera, "Camera '" + camera.name() + "' lens: " + std::to_string(camera.focalLength()) + " mm on a " + std::to_string(camera.sensorWidth()) + " mm sensor -> " + std::to_string(camera.fieldOfView()) + "°.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setViewDistance", "Sets how far a camera sees (the far plane of a perspective camera).",
						{
							EntityParameter,
							ComponentParameter,
							{"distance", "The view distance, in metres (more than 0)."}
						},
						[this] (const std::string & entity, const std::string & component, float distance) {
							if ( !std::isfinite(distance) || distance <= 0.0F )
							{
								return Console::CommandResult::error("The view distance must be a finite number of metres, more than 0.");
							}

							return this->act(entity, component, [distance] (Camera & camera) {
								camera.setDistance(distance);

								return changed(camera, "Camera '" + camera.name() + "' view distance set to " + std::to_string(camera.distance()) + " m.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setExposure", "PINS the exposure triad of a camera together and switches its auto-exposure off. E.g. setExposure(entity, component, 8, 0.008, 125) for f/8 1/125 s ISO 125. The ISO must lie in the camera's sensitivity range (setSensitivityRange).",
						{
							EntityParameter,
							ComponentParameter,
							{"aperture", "The f-number, 0.5 or more (8 for f/8)."},
							{"shutter", "The exposure time, in seconds, from 1/8000 to 1 (0.008 for 1/125 s)."},
							{"iso", "The sensitivity, in ISO, inside the camera's range (getState: exposure.minIso / maxIso)."}
						},
						[this] (const std::string & entity, const std::string & component, float aperture, float shutter, float iso) {
							if ( !std::isfinite(aperture) || aperture < 0.5F )
							{
								return Console::CommandResult::error("The aperture must be a finite f-number, 0.5 or more.");
							}

							if ( !std::isfinite(shutter) || shutter < Camera::FastestShutterSpeed || shutter > 1.0F )
							{
								return Console::CommandResult::error("The exposure time must lie between 1/8000 s and 1 s.");
							}

							if ( !std::isfinite(iso) || iso < 1.0F )
							{
								return Console::CommandResult::error("The sensitivity must be a finite number of ISO, 1 or more.");
							}

							return this->act(entity, component, [aperture, shutter, iso] (Camera & camera) {
								/* ⚠️ Camera::setSensitivity() CLAMPS to the range: a value outside it would be applied
								 * as the bound, and the image would be exposed differently from what was asked. */
								if ( iso < camera.minSensitivity() || iso > camera.maxSensitivity() )
								{
									return Console::CommandResult::error("ISO " + std::to_string(iso) + " is outside the sensitivity range of camera '" + camera.name() + "' (" + std::to_string(camera.minSensitivity()) + " to " + std::to_string(camera.maxSensitivity()) + "): widen it first with setSensitivityRange.");
								}

								/* ⚠️⚠️ The three are ONE joint calibration and are taken TOGETHER on purpose: moving one
								 * alone blows the content out or sinks it. Pinning them also requires auto-exposure OFF,
								 * because a metered camera would immediately override whatever was just set — and any
								 * "X changed the look" measurement taken on an auto-exposing camera is a claim about the
								 * SENSOR, not about X. */
								camera.setAutoExposure(false);
								camera.setAperture(aperture);
								camera.setShutterSpeed(shutter);
								camera.setSensitivity(iso);

								return changed(camera, "Camera '" + camera.name() + "' exposure PINNED (auto exposure off): f/" + std::to_string(camera.aperture()) + "  " + std::to_string(camera.shutterSpeed()) + " s  ISO " + std::to_string(camera.sensitivity()) + ".");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setAutoExposure", "Switches the auto-exposure of a camera on or off (off keeps the current triad pinned).",
						{
							EntityParameter,
							ComponentParameter,
							{"enabled", "1 (true) meters the whole frame, 0 (false) keeps the current triad pinned."}
						},
						[this] (const std::string & entity, const std::string & component, bool enabled) {
							return this->act(entity, component, [enabled] (Camera & camera) {
								camera.setAutoExposure(enabled);

								return changed(camera, enabled
									? "Camera '" + camera.name() + "' auto exposure ON — ⚠️ it meters the whole frame, so any measurement taken now describes the SENSOR, not the scene."
									: "Camera '" + camera.name() + "' auto exposure OFF (exposure pinned).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setExposureCompensation", "Sets the exposure compensation of a camera, in EV: +1 doubles the exposure, -1 halves it.",
						{
							EntityParameter,
							ComponentParameter,
							{"ev", "The compensation, in EV (positive brightens)."}
						},
						[this] (const std::string & entity, const std::string & component, float ev) {
							if ( !std::isfinite(ev) )
							{
								return Console::CommandResult::error("The exposure compensation must be a finite number of EV.");
							}

							return this->act(entity, component, [ev] (Camera & camera) {
								camera.setExposureCompensation(ev);

								return changed(camera, "Camera '" + camera.name() + "' exposure compensation set to " + std::to_string(camera.exposureCompensation()) + " EV.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSensitivityRange", "Sets the usable sensitivity range of a camera's sensor, in ISO: the bounds of the auto-exposure metering and of setExposure.",
						{
							EntityParameter,
							ComponentParameter,
							{"minimum", "The lowest usable sensitivity, in ISO, 1 or more."},
							{"maximum", "The highest usable sensitivity, in ISO, the minimum or more."}
						},
						[this] (const std::string & entity, const std::string & component, float minimum, float maximum) {
							if ( !std::isfinite(minimum) || !std::isfinite(maximum) || minimum < 1.0F || maximum < minimum )
							{
								return Console::CommandResult::error("The range must satisfy 1 <= minimum <= maximum (ISO).");
							}

							return this->act(entity, component, [minimum, maximum] (Camera & camera) {
								camera.setSensitivityRange(minimum, maximum);

								return changed(camera, "Camera '" + camera.name() + "' sensitivity range set to ISO " + std::to_string(camera.minSensitivity()) + " - " + std::to_string(camera.maxSensitivity()) + ".");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setFocus", "Focuses a camera at a distance (auto focus off), or switches its auto focus back on when the distance is omitted.",
						{
							EntityParameter,
							ComponentParameter,
							{"distance", "The focus distance, in metres (0.01 or more); omit it to re-enable auto focus."}
						},
						[this] (const std::string & entity, const std::string & component, std::optional< float > distance) {
							if ( distance.has_value() && ( !std::isfinite(*distance) || *distance < 0.01F ) )
							{
								return Console::CommandResult::error("The focus distance must be a finite number of metres, 0.01 or more.");
							}

							return this->act(entity, component, [distance] (Camera & camera) {
								if ( !distance.has_value() )
								{
									camera.setAutoFocus(true);

									return changed(camera, "Camera '" + camera.name() + "' auto focus ON.");
								}

								/* An explicit distance means manual focus: leaving auto-focus on would let it pull the
								 * plane away on the next frame. */
								camera.setAutoFocus(false);
								camera.setFocusDistance(*distance);

								return changed(camera, "Camera '" + camera.name() + "' focused manually at " + std::to_string(camera.focusDistance()) + " m.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setBloom", "Sets the lens glare (bloom) of a camera: the scene luminance above which the lens glares, and the fraction of that energy it scatters.",
						{
							EntityParameter,
							ComponentParameter,
							{"threshold", "The luminance threshold, in nits (cd/m²), 0 or more."},
							{"intensity", "The scattered fraction, 0 or more (0.03 = 3 %, the default)."}
						},
						[this] (const std::string & entity, const std::string & component, float threshold, float intensity) {
							if ( !std::isfinite(threshold) || !std::isfinite(intensity) || threshold < 0.0F || intensity < 0.0F )
							{
								return Console::CommandResult::error("The bloom threshold and intensity must be finite numbers, 0 or more.");
							}

							return this->act(entity, component, [threshold, intensity] (Camera & camera) {
								camera.setBloomThreshold(threshold);
								camera.setBloomIntensity(intensity);

								return changed(camera, "Camera '" + camera.name() + "' bloom: threshold " + std::to_string(camera.bloomThreshold()) + " nits, intensity " + std::to_string(camera.bloomIntensity()) + ".");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendCameraConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< CameraConsoleAdapter >(sceneManager));
	}
}
