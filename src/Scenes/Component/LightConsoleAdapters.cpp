/*
 * src/Scenes/Component/LightConsoleAdapters.cpp
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

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "DirectionalLight.hpp"
#include "FastJSON.hpp"
#include "PointLight.hpp"
#include "SpotLight.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/* The two parameters every command of this file starts with. */
		const Console::Parameter EntityParameter{entityParameter("the light")};
		const Console::Parameter ComponentParameter{componentParameter("light")};

		/**
		 * @brief Returns whether three colour components are a valid light colour.
		 * @param red The red component.
		 * @param green The green component.
		 * @param blue The blue component.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValidColor (float red, float green, float blue) noexcept
		{
			return std::isfinite(red) && std::isfinite(green) && std::isfinite(blue) && red >= 0.0F && green >= 0.0F && blue >= 0.0F && red <= 1.0F && green <= 1.0F && blue <= 1.0F;
		}

		/**
		 * @brief Returns the state every light shares, as a JSON object.
		 * @param light The light.
		 * @param type The component type.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		commonState (const AbstractLightEmitter & light, const char * type) noexcept
		{
			const auto & color = light.authoredColor();

			Json::Value state{Json::objectValue};
			state["name"] = light.name();
			state["type"] = type;
			state["enabled"] = light.isEnabled();

			Json::Value rgb{Json::arrayValue};
			rgb.append(static_cast< double >(color.red()));
			rgb.append(static_cast< double >(color.green()));
			rgb.append(static_cast< double >(color.blue()));
			state["color"] = std::move(rgb);

			/* Shadows: the map exists only if it was requested at creation (resolution 0 = none). */
			Json::Value shadow{Json::objectValue};
			shadow["mapResolution"] = light.shadowMapResolution();
			shadow["casting"] = light.isShadowCastingEnabled();
			shadow["PCFRadius"] = static_cast< double >(light.PCFRadius());
			shadow["bias"] = static_cast< double >(light.shadowBias());
			state["shadow"] = std::move(shadow);

			Json::Value projection{Json::objectValue};
			projection["texture"] = light.hasColorProjectionTexture();
			projection["boost"] = static_cast< double >(light.colorProjectionBoost());
			state["colorProjection"] = std::move(projection);

			return state;
		}

		/**
		 * @brief Returns the full state of a point light as JSON.
		 * @param light The light.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const PointLight & light) noexcept
		{
			auto state = commonState(light, PointLight::ClassId);
			state["intensityCandela"] = static_cast< double >(light.intensity());
			state["radius"] = static_cast< double >(light.radius());

			return FastJSON::stringify(state);
		}

		/**
		 * @brief Returns the full state of a spot light as JSON.
		 * @param light The light.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const SpotLight & light) noexcept
		{
			auto state = commonState(light, SpotLight::ClassId);
			state["intensityCandela"] = static_cast< double >(light.intensity());
			state["radius"] = static_cast< double >(light.radius());
			state["innerAngle"] = static_cast< double >(light.innerAngle());
			state["outerAngle"] = static_cast< double >(light.outerAngle());

			return FastJSON::stringify(state);
		}

		/**
		 * @brief Returns the full state of a directional light as JSON.
		 * @param light The light.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const DirectionalLight & light) noexcept
		{
			auto state = commonState(light, DirectionalLight::ClassId);
			state["illuminanceLux"] = static_cast< double >(light.intensity());

			if ( light.usesCSM() )
			{
				state["shadow"]["cascadeCount"] = light.cascadeCount();
				state["shadow"]["cascadeLambda"] = static_cast< double >(light.cascadeLambda());
			}
			else if ( light.shadowMapResolution() > 0 )
			{
				state["shadow"]["coverageSize"] = static_cast< double >(light.coverageSize());
			}

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the light's NEW state as JSON.
		 * @note The state is what a machine client reads (MCP structuredContent): it confirms the values
		 * actually applied without a second getState() call (suggested by Gemini's review, 2026-09-27).
		 * @tparam light_t The light type.
		 * @param light The light, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		template< typename light_t >
		[[nodiscard]]
		Console::CommandResult
		changed (const light_t & light, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(light));
		}

		/**
		 * @brief The commands every light type shares: enable, colour, shadow filtering, colour projection.
		 * @tparam light_t The light type.
		 * @param adapter The adapter binding them.
		 * @return void
		 */
		template< typename light_t, typename adapter_t >
		void
		bindCommonLightCommands (adapter_t & adapter) noexcept
		{
			adapter.bindCommand("setEnabled", "Switches the light on or off.",
				{
					EntityParameter,
					ComponentParameter,
					{"enabled", "1 (true) emits, 0 (false) stops emitting."}
				},
				[&adapter] (const std::string & entity, const std::string & component, bool enabled) {
					return adapter.act(entity, component, [enabled] (light_t & light) {
						light.enable(enabled);

						return changed(light, std::string{"Light '"} + light.name() + ( enabled ? "' on." : "' off." ));
					});
				}, Console::CommandHint::Idempotent);

			adapter.bindCommand("setColor", "Sets the light colour, a CHROMATICITY: the intensity keeps its photometric value whatever the hue (dim through the intensity command).",
				{
					EntityParameter,
					ComponentParameter,
					{"red", "Red component, 0 to 1."},
					{"green", "Green component, 0 to 1."},
					{"blue", "Blue component, 0 to 1."}
				},
				[&adapter] (const std::string & entity, const std::string & component, float red, float green, float blue) {
					if ( !isValidColor(red, green, blue) )
					{
						return Console::CommandResult::error("Each colour component must be between 0 and 1.");
					}

					return adapter.act(entity, component, [red, green, blue] (light_t & light) {
						light.setColor(PixelFactory::Color< float >{red, green, blue});

						return changed(light, "Light '" + light.name() + "' colour set.");
					});
				}, Console::CommandHint::Idempotent);

			/* ⚠️ The two shadow commands REFUSE a light built without a shadow map: the map is allocated at creation
			 * only. There is deliberately NO shadow on/off command: AbstractLightEmitter::enableShadowCasting() is a
			 * creation-time switch — flipped at runtime it only freezes the CSM cascade fitting while the shadow map
			 * is still drawn and sampled, which put a whole forest floor in shadow (measured 2026-09-27; engine item
			 * light-shadow-runtime-toggle). */
			adapter.bindCommand("setPCFRadius", "Sets the soft-edge filter radius of a light's shadow (percentage-closer filtering). Refused for a light built without a shadow map.",
				{
					EntityParameter,
					ComponentParameter,
					{"radius", "The filter radius, 0 or more: in shadow-map texels for a directional or spot light; a point light scales it by the distance to the light."}
				},
				[&adapter] (const std::string & entity, const std::string & component, float radius) {
					if ( !std::isfinite(radius) || radius < 0.0F )
					{
						return Console::CommandResult::error("The filter radius must be a finite number, 0 or more.");
					}

					return adapter.act(entity, component, [radius] (light_t & light) {
						if ( light.shadowMapResolution() == 0 )
						{
							return Console::CommandResult::error("Light '" + light.name() + "' was built without a shadow map: it has no shadow to filter.");
						}

						light.setPCFRadius(radius);

						return changed(light, "Light '" + light.name() + "' shadow filter radius set to " + std::to_string(radius) + ".");
					});
				}, Console::CommandHint::Idempotent);

			adapter.bindCommand("setShadowBias", "Sets the depth bias of a light's shadow: too small shows acne (self-shadowing stripes), too large detaches the shadow from its caster (peter-panning). Refused for a light built without a shadow map.",
				{
					EntityParameter,
					ComponentParameter,
					{"bias", "The depth bias, 0 or more (default 0.005)."}
				},
				[&adapter] (const std::string & entity, const std::string & component, float bias) {
					if ( !std::isfinite(bias) || bias < 0.0F )
					{
						return Console::CommandResult::error("The shadow bias must be a finite number, 0 or more.");
					}

					return adapter.act(entity, component, [bias] (light_t & light) {
						if ( light.shadowMapResolution() == 0 )
						{
							return Console::CommandResult::error("Light '" + light.name() + "' was built without a shadow map: it has no shadow to bias.");
						}

						light.setShadowBias(bias);

						return changed(light, "Light '" + light.name() + "' shadow bias set to " + std::to_string(bias) + ".");
					});
				}, Console::CommandHint::Idempotent);

			adapter.bindCommand("setProjectionBoost", "Sets how much the bright areas of a light's colour projection texture (a gobo) amplify it: 0 = the texture only filters the light, above 0 = light × (1 + texture × boost). Refused for a light without a projection texture.",
				{
					EntityParameter,
					ComponentParameter,
					{"boost", "The boost factor, 0 or more."}
				},
				[&adapter] (const std::string & entity, const std::string & component, float boost) {
					if ( !std::isfinite(boost) || boost < 0.0F )
					{
						return Console::CommandResult::error("The projection boost must be a finite number, 0 or more.");
					}

					return adapter.act(entity, component, [boost] (light_t & light) {
						if ( !light.hasColorProjectionTexture() )
						{
							return Console::CommandResult::error("Light '" + light.name() + "' has no colour projection texture: a boost would change nothing.");
						}

						light.setColorProjectionBoost(boost);

						return changed(light, "Light '" + light.name() + "' projection boost set to " + std::to_string(boost) + ".");
					});
				}, Console::CommandHint::Idempotent);
		}

		/** @brief `Core.SceneManagerService.PointLight.*`. */
		class PointLightConsoleAdapter final : public ConsoleAdapter< PointLight >
		{
			public:

				explicit
				PointLightConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

				using ConsoleAdapter::act;
				using ControllableTrait::bindCommand;

			private:

				void
				onRegisterToConsole () noexcept override
				{
					this->bindCommand("getState", "Returns the state of a point light as JSON: enabled, colour, intensity (candela), radius (metres).",
						{EntityParameter, ComponentParameter},
						[this] (const std::string & entity, const std::string & component) {
							return this->act(entity, component, [] (PointLight & light) {
								return Console::CommandResult::json(stateOf(light));
							});
						}, Console::CommandHint::ReadOnly);

					bindCommonLightCommands< PointLight >(*this);

					this->bindCommand("setLuminousPower", "Sets the luminous power of a point light, in lumens — the unit a bulb is sold in (an 800 lm bulb ≈ a 60 W incandescent).",
						{
							EntityParameter,
							ComponentParameter,
							{"lumens", "The luminous power, in lumens (0 or more)."}
						},
						[this] (const std::string & entity, const std::string & component, float lumens) {
							if ( !std::isfinite(lumens) || lumens < 0.0F )
							{
								return Console::CommandResult::error("The luminous power must be a finite number of lumens, 0 or more.");
							}

							return this->act(entity, component, [lumens] (PointLight & light) {
								light.setLuminousPower(lumens);

								return changed(light, "Point light '" + light.name() + "' set to " + std::to_string(lumens) + " lm.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setRadius", "Sets the influence radius of a point light: its contribution falls smoothly to exactly zero there, and the light is culled beyond.",
						{
							EntityParameter,
							ComponentParameter,
							{"radius", "The radius, in metres; 0 = unbounded (the inverse square alone)."}
						},
						[this] (const std::string & entity, const std::string & component, float radius) {
							if ( !std::isfinite(radius) || radius < 0.0F )
							{
								return Console::CommandResult::error("The radius must be a finite number of metres, 0 or more.");
							}

							return this->act(entity, component, [radius] (PointLight & light) {
								light.setRadius(radius);

								return changed(light, "Point light '" + light.name() + "' radius set to " + std::to_string(radius) + " m.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};

		/** @brief `Core.SceneManagerService.SpotLight.*`. */
		class SpotLightConsoleAdapter final : public ConsoleAdapter< SpotLight >
		{
			public:

				explicit
				SpotLightConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

				using ConsoleAdapter::act;
				using ControllableTrait::bindCommand;

			private:

				void
				onRegisterToConsole () noexcept override
				{
					this->bindCommand("getState", "Returns the state of a spot light as JSON: enabled, colour, intensity (candela), radius (metres), cone angles (degrees).",
						{EntityParameter, ComponentParameter},
						[this] (const std::string & entity, const std::string & component) {
							return this->act(entity, component, [] (SpotLight & light) {
								return Console::CommandResult::json(stateOf(light));
							});
						}, Console::CommandHint::ReadOnly);

					bindCommonLightCommands< SpotLight >(*this);

					this->bindCommand("setLuminousPower", "Sets the luminous power of a spot light, in lumens, spread over its outer cone (change the cone first: the power is converted with the current angle).",
						{
							EntityParameter,
							ComponentParameter,
							{"lumens", "The luminous power, in lumens (0 or more)."}
						},
						[this] (const std::string & entity, const std::string & component, float lumens) {
							if ( !std::isfinite(lumens) || lumens < 0.0F )
							{
								return Console::CommandResult::error("The luminous power must be a finite number of lumens, 0 or more.");
							}

							return this->act(entity, component, [lumens] (SpotLight & light) {
								light.setLuminousPower(lumens);

								return changed(light, "Spot light '" + light.name() + "' set to " + std::to_string(lumens) + " lm.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setRadius", "Sets the influence radius of a spot light: its contribution falls smoothly to exactly zero there.",
						{
							EntityParameter,
							ComponentParameter,
							{"radius", "The radius, in metres; 0 = unbounded (the inverse square alone)."}
						},
						[this] (const std::string & entity, const std::string & component, float radius) {
							if ( !std::isfinite(radius) || radius < 0.0F )
							{
								return Console::CommandResult::error("The radius must be a finite number of metres, 0 or more.");
							}

							return this->act(entity, component, [radius] (SpotLight & light) {
								light.setRadius(radius);

								return changed(light, "Spot light '" + light.name() + "' radius set to " + std::to_string(radius) + " m.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setConeAngles", "Sets the cone of a spot light: full light inside the inner angle, fading to none at the outer angle.",
						{
							EntityParameter,
							ComponentParameter,
							{"innerAngle", "The inner angle, in degrees (0 to 90)."},
							{"outerAngle", "The outer angle, in degrees (the inner one to 90)."}
						},
						[this] (const std::string & entity, const std::string & component, float innerAngle, float outerAngle) {
							if ( !std::isfinite(innerAngle) || !std::isfinite(outerAngle) || innerAngle < 0.0F || outerAngle > 90.0F || innerAngle > outerAngle )
							{
								return Console::CommandResult::error("The angles must satisfy 0 <= innerAngle <= outerAngle <= 90 degrees.");
							}

							return this->act(entity, component, [innerAngle, outerAngle] (SpotLight & light) {
								light.setConeAngles(innerAngle, outerAngle);

								return changed(light, "Spot light '" + light.name() + "' cone set to " + std::to_string(innerAngle) + "° / " + std::to_string(outerAngle) + "°.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};

		/** @brief `Core.SceneManagerService.DirectionalLight.*`. */
		class DirectionalLightConsoleAdapter final : public ConsoleAdapter< DirectionalLight >
		{
			public:

				explicit
				DirectionalLightConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

				using ConsoleAdapter::act;
				using ControllableTrait::bindCommand;

			private:

				void
				onRegisterToConsole () noexcept override
				{
					this->bindCommand("getState", "Returns the state of a directional light (a sun) as JSON: enabled, colour, illuminance (lux).",
						{EntityParameter, ComponentParameter},
						[this] (const std::string & entity, const std::string & component) {
							return this->act(entity, component, [] (DirectionalLight & light) {
								return Console::CommandResult::json(stateOf(light));
							});
						}, Console::CommandHint::ReadOnly);

					bindCommonLightCommands< DirectionalLight >(*this);

					this->bindCommand("setIlluminance", "Sets the illuminance a directional light casts on a surface facing it, in lux (a clear midday sun ≈ 100 000 lx, an overcast sky ≈ 1 000 lx).",
						{
							EntityParameter,
							ComponentParameter,
							{"lux", "The illuminance, in lux (0 or more)."}
						},
						[this] (const std::string & entity, const std::string & component, float lux) {
							if ( !std::isfinite(lux) || lux < 0.0F )
							{
								return Console::CommandResult::error("The illuminance must be a finite number of lux, 0 or more.");
							}

							return this->act(entity, component, [lux] (DirectionalLight & light) {
								light.setIlluminance(lux);

								return changed(light, "Directional light '" + light.name() + "' set to " + std::to_string(lux) + " lx.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendLightConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< PointLightConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< SpotLightConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< DirectionalLightConsoleAdapter >(sceneManager));
	}
}
