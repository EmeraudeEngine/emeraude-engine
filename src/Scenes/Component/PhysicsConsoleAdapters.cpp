/*
 * src/Scenes/Component/PhysicsConsoleAdapters.cpp
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
#include "DirectionalPushModifier.hpp"
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "SphericalPushModifier.hpp"
#include "Weight.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns a vector as a JSON array.
		 * @param vector The vector.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		toJSON (const Math::Vector< 3, float > & vector) noexcept
		{
			Json::Value array{Json::arrayValue};
			array.append(static_cast< double >(vector.x()));
			array.append(static_cast< double >(vector.y()));
			array.append(static_cast< double >(vector.z()));

			return array;
		}

		/**
		 * @brief Returns the state every push modifier shares, as a JSON object.
		 * @param modifier The modifier.
		 * @param type The component type.
		 * @param magnitude The push magnitude.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		modifierState (const AbstractModifier & modifier, const char * type, float magnitude) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = modifier.name();
			state["entity"] = modifier.parentEntity().name();
			state["type"] = type;
			state["enabled"] = modifier.isEnabled();
			state["magnitude"] = static_cast< double >(magnitude);
			state["influenceArea"] = modifier.hasInfluenceArea();

			return state;
		}

		/**
		 * @brief Returns the full state of a directional push modifier as JSON.
		 * @param modifier The modifier.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const DirectionalPushModifier & modifier) noexcept
		{
			auto state = modifierState(modifier, DirectionalPushModifier::ClassId, modifier.magnitude());
			state["direction"] = toJSON(modifier.direction());
			state["followsEntity"] = modifier.followsEntity();

			return FastJSON::stringify(state);
		}

		/**
		 * @brief Returns the full state of a spherical push modifier as JSON.
		 * @param modifier The modifier.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const SphericalPushModifier & modifier) noexcept
		{
			return FastJSON::stringify(modifierState(modifier, SphericalPushModifier::ClassId, modifier.magnitude()));
		}

		/**
		 * @brief Returns the full state of a weight as JSON.
		 * @param weight The weight.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const Weight & weight) noexcept
		{
			const auto & physics = weight.bodyPhysicalProperties();
			const auto & box = weight.localBoundingBox();

			Json::Value state{Json::objectValue};
			state["name"] = weight.name();
			state["entity"] = weight.parentEntity().name();
			state["type"] = Weight::ClassId;
			state["mass"] = static_cast< double >(physics.mass());
			state["surface"] = static_cast< double >(physics.surface());
			state["dragCoefficient"] = static_cast< double >(physics.dragCoefficient());
			state["angularDragCoefficient"] = static_cast< double >(physics.angularDragCoefficient());
			state["bounciness"] = static_cast< double >(physics.bounciness());
			state["stickiness"] = static_cast< double >(physics.stickiness());

			/* An unset bound is reported as null, never as its reset sentinel (±FLT_MAX for a box). */
			if ( const auto & sphere = weight.localBoundingSphere(); sphere.isValid() )
			{
				state["sphereRadius"] = static_cast< double >(sphere.radius());
			}
			else
			{
				state["sphereRadius"] = Json::Value{Json::nullValue};
			}

			if ( box.isValid() )
			{
				state["boxMinimum"] = toJSON(box.minimum());
				state["boxMaximum"] = toJSON(box.maximum());
			}
			else
			{
				state["boxMinimum"] = Json::Value{Json::nullValue};
				state["boxMaximum"] = Json::Value{Json::nullValue};
			}

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the component's NEW state as JSON.
		 * @tparam component_t The component type.
		 * @param component The component, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		template< typename component_t >
		[[nodiscard]]
		Console::CommandResult
		changed (const component_t & component, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(component));
		}

		/**
		 * @brief The commands both push modifiers share: getState, enable, magnitude.
		 * @tparam modifier_t The modifier type.
		 * @tparam adapter_t The adapter type.
		 * @param adapter The adapter binding them.
		 * @param entity The entity parameter.
		 * @param component The component parameter.
		 * @param forceDescription What the magnitude pushes, for the descriptions.
		 */
		template< typename modifier_t, typename adapter_t >
		void
		bindCommonModifierCommands (adapter_t & adapter, const Console::Parameter & entity, const Console::Parameter & component, const std::string & forceDescription) noexcept
		{
			adapter.bindCommand("getState", "Returns the state of a " + forceDescription + " as JSON: enabled, magnitude, whether an influence area limits it.",
				{entity, component},
				[&adapter] (const std::string & entityName, const std::string & componentName) {
					return adapter.act(entityName, componentName, [] (modifier_t & modifier) {
						return Console::CommandResult::json(stateOf(modifier));
					});
				}, Console::CommandHint::ReadOnly);

			adapter.bindCommand("setEnabled", "Switches a " + forceDescription + " on or off.",
				{
					entity,
					component,
					{"enabled", "1 (true) pushes, 0 (false) stops pushing."}
				},
				[&adapter] (const std::string & entityName, const std::string & componentName, bool enabled) {
					return adapter.act(entityName, componentName, [enabled] (modifier_t & modifier) {
						modifier.enable(enabled);

						return changed(modifier, std::string{"Modifier '"} + modifier.name() + ( enabled ? "' on." : "' off." ));
					});
				}, Console::CommandHint::Idempotent);

			adapter.bindCommand("setMagnitude", "Sets the strength of a " + forceDescription + ".",
				{
					entity,
					component,
					{"magnitude", "The push magnitude, a finite number (negative pulls)."}
				},
				[&adapter] (const std::string & entityName, const std::string & componentName, float magnitude) {
					if ( !std::isfinite(magnitude) )
					{
						return Console::CommandResult::error("The magnitude must be a finite number.");
					}

					return adapter.act(entityName, componentName, [magnitude] (modifier_t & modifier) {
						modifier.setMagnitude(magnitude);

						return changed(modifier, "Modifier '" + modifier.name() + "' magnitude set to " + std::to_string(magnitude) + ".");
					});
				}, Console::CommandHint::Idempotent);
		}

		/** @brief `Core.SceneManagerService.DirectionalPushModifier.*`. */
		class DirectionalPushModifierConsoleAdapter final : public ConsoleAdapter< DirectionalPushModifier >
		{
			public:

				explicit
				DirectionalPushModifierConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

				using ConsoleAdapter::act;
				using ControllableTrait::bindCommand;

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the modifier")};
					const Console::Parameter component{componentParameter(DirectionalPushModifier::ClassId)};

					bindCommonModifierCommands< DirectionalPushModifier >(*this, entity, component, "directional push (a wind-like force along one direction)");

					this->bindCommand("setDirection", "Sets the direction a directional push blows toward, in world space; omit the three components to make it follow the entity again (its backward vector).",
						{
							entity,
							component,
							{"x", "X component of the direction (normalised here)."},
							{"y", "Y component of the direction (UP is +Y)."},
							{"z", "Z component of the direction."}
						},
						[this] (const std::string & entityName, const std::string & componentName, std::optional< float > x, std::optional< float > y, std::optional< float > z) {
							const auto given = static_cast< int >(x.has_value()) + static_cast< int >(y.has_value()) + static_cast< int >(z.has_value());

							if ( given == 0 )
							{
								return this->act(entityName, componentName, [] (DirectionalPushModifier & modifier) {
									modifier.disableCustomDirection();

									return changed(modifier, "Modifier '" + modifier.name() + "' follows its entity again.");
								});
							}

							if ( given != 3 || !std::isfinite(*x) || !std::isfinite(*y) || !std::isfinite(*z) )
							{
								return Console::CommandResult::error("Give the three finite components x, y and z together, or none.");
							}

							Math::Vector< 3, float > direction{*x, *y, *z};

							if ( direction.lengthSquared() < 1e-12F )
							{
								return Console::CommandResult::error("The direction must not be the null vector.");
							}

							direction.normalize();

							return this->act(entityName, componentName, [&direction] (DirectionalPushModifier & modifier) {
								modifier.setCustomDirection(direction);

								return changed(modifier, "Modifier '" + modifier.name() + "' direction set.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};

		/** @brief `Core.SceneManagerService.SphericalPushModifier.*`. */
		class SphericalPushModifierConsoleAdapter final : public ConsoleAdapter< SphericalPushModifier >
		{
			public:

				explicit
				SphericalPushModifierConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

				using ConsoleAdapter::act;
				using ControllableTrait::bindCommand;

			private:

				void
				onRegisterToConsole () noexcept override
				{
					bindCommonModifierCommands< SphericalPushModifier >(*this, entityParameter("the modifier"), componentParameter(SphericalPushModifier::ClassId), "spherical push (a blast-like force away from the entity)");
				}
		};

		/** @brief `Core.SceneManagerService.Weight.*`. */
		class WeightConsoleAdapter final : public ConsoleAdapter< Weight >
		{
			public:

				explicit
				WeightConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the weight")};
					const Console::Parameter component{componentParameter(Weight::ClassId)};

					this->bindCommand("getState", "Returns the state of a weight as JSON: the physical properties it gives its entity (mass in kg, surface in m², drag, bounciness, stickiness) and its collision bounds (metres).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (Weight & weight) {
								return Console::CommandResult::json(stateOf(weight));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setRadius", "Sets the radius of the bounding sphere a weight gives its entity for collisions.",
						{
							entity,
							component,
							{"radius", "The radius, in metres, more than 0."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float radius) {
							if ( !std::isfinite(radius) || radius <= 0.0F )
							{
								return Console::CommandResult::error("The radius must be a finite number of metres, more than 0.");
							}

							return this->act(entityName, componentName, [radius] (Weight & weight) {
								weight.setRadius(radius);

								return changed(weight, "Weight '" + weight.name() + "' radius set to " + std::to_string(radius) + " m.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setBoxSize", "Sets the bounding box a weight gives its entity for collisions, centred on the entity: a cube, or three sizes.",
						{
							entity,
							component,
							{"x", "The size along X (or of every side, alone), in metres, more than 0."},
							{"y", "The size along Y, in metres; give it with z, or neither for a cube."},
							{"z", "The size along Z, in metres; give it with y, or neither for a cube."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float x, std::optional< float > y, std::optional< float > z) {
							if ( y.has_value() != z.has_value() )
							{
								return Console::CommandResult::error("Give y and z together, or neither for a cube.");
							}

							const auto valid = [] (float size) {
								return std::isfinite(size) && size > 0.0F;
							};

							if ( !valid(x) || ( y.has_value() && ( !valid(*y) || !valid(*z) ) ) )
							{
								return Console::CommandResult::error("Every size must be a finite number of metres, more than 0.");
							}

							return this->act(entityName, componentName, [x, y, z] (Weight & weight) {
								if ( y.has_value() )
								{
									weight.setBoxSize(x, *y, *z);
								}
								else
								{
									weight.setBoxSize(x);
								}

								return changed(weight, "Weight '" + weight.name() + "' box set.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendPhysicsConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< DirectionalPushModifierConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< SphericalPushModifierConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< WeightConsoleAdapter >(sceneManager));
	}
}
