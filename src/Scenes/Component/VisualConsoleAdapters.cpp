/*
 * src/Scenes/Component/VisualConsoleAdapters.cpp
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
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "MultipleVisuals.hpp"
#include "Visual.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns the state both visual types share, as a JSON object.
		 * @param visual The visual component.
		 * @param type The component type.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		visualState (const Abstract & visual, const char * type) noexcept
		{
			Json::Value state{Json::objectValue};
			state["name"] = visual.name();
			state["entity"] = visual.parentEntity().name();
			state["type"] = type;

			const auto instance = visual.getRenderableInstance();

			if ( instance == nullptr )
			{
				return state;
			}

			if ( const auto * renderable = instance->renderable(); renderable != nullptr )
			{
				state["renderable"] = renderable->name();
				state["levelOfDetailCount"] = renderable->levelOfDetailCount();
			}

			/* Decided when the instance is built (shader generation, ray-tracing lists): reported, not settable. */
			state["lighting"] = instance->isLightingEnabled();
			state["shadowCasting"] = instance->isShadowCastingEnabled();
			state["shadowReceiving"] = instance->isShadowReceivingEnabled();
			state["rayTracing"] = !instance->isRayTracingDisabled();
			state["bakeOnly"] = instance->isBakeOnly();

			state["drawNearDistance"] = static_cast< double >(instance->drawNearDistance());
			state["drawFarDistance"] = static_cast< double >(instance->drawFarDistance());
			state["shadowCastingDistance"] = static_cast< double >(instance->shadowCastingDistance());
			state["shadowLevelOfDetailBias"] = instance->shadowLevelOfDetailBias();

			return state;
		}

		/**
		 * @brief Returns the full state of a visual as JSON.
		 * @param visual The visual.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const Visual & visual) noexcept
		{
			return FastJSON::stringify(visualState(visual, Visual::ClassId));
		}

		/**
		 * @brief Returns the full state of a multiple visuals component as JSON.
		 * @param visuals The multiple visuals component.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const MultipleVisuals & visuals) noexcept
		{
			auto state = visualState(visuals, MultipleVisuals::ClassId);
			state["instanceCount"] = static_cast< Json::UInt64 >(visuals.localCoordinates().size());

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
		 * @brief The console adapter of a drawn component: its state, and the instance settings that stay reversible.
		 * @note Lighting, shadow casting/receiving and ray tracing are decided when the instance is BUILT (shader
		 * generation, ray-tracing lists) and are reported only: a console switch could not undo them.
		 * @tparam visual_t Visual or MultipleVisuals.
		 */
		template< typename visual_t >
		class VisualConsoleAdapter final : public ConsoleAdapter< visual_t >
		{
			public:

				explicit
				VisualConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter< visual_t >{sceneManager}
				{

				}

			private:

				/**
				 * @brief Runs an action on the renderable instance of a visual, refusing a visual that has none.
				 * @param entityName The entity name.
				 * @param componentName The component name.
				 * @param action A callable taking `visual_t &` and `Graphics::RenderableInstance::Abstract &`.
				 * @return Console::CommandResult
				 */
				template< typename action_t >
				[[nodiscard]]
				Console::CommandResult
				withInstance (const std::string & entityName, const std::string & componentName, action_t && action) const noexcept
				{
					return this->act(entityName, componentName, [&action] (visual_t & visual) {
						const auto instance = visual.getRenderableInstance();

						if ( instance == nullptr )
						{
							return Console::CommandResult::error("Visual '" + visual.name() + "' has no renderable instance.");
						}

						return action(visual, *instance);
					});
				}

				void
				onRegisterToConsole () noexcept override
				{
					const std::string type{visual_t::ClassId};
					const Console::Parameter entity{entityParameter("the visual")};
					const Console::Parameter component{componentParameter(type)};

					this->bindCommand("getState", "Returns the state of a " + type + " as JSON: its renderable and level-of-detail count, the build-time switches (lighting, shadow casting and receiving, ray tracing), and the draw and shadow distances (metres).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (visual_t & visual) {
								return Console::CommandResult::json(stateOf(visual));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setDrawDistance", "Restricts where a " + type + " is drawn to a range of distances from the camera (the mesh/imposter switch). The ray-tracing lists honour the far limit too.",
						{
							entity,
							component,
							{"near", "The distance under which it is not drawn, in metres, 0 or more."},
							{"far", "The distance beyond which it is not drawn, in metres; 0 for no far limit, else more than near."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float nearDistance, float farDistance) {
							if ( !std::isfinite(nearDistance) || !std::isfinite(farDistance) || nearDistance < 0.0F || farDistance < 0.0F || ( farDistance > 0.0F && farDistance <= nearDistance ) )
							{
								return Console::CommandResult::error("The range must satisfy 0 <= near < far, or far = 0 for no far limit (metres).");
							}

							return this->withInstance(entityName, componentName, [nearDistance, farDistance] (visual_t & visual, Graphics::RenderableInstance::Abstract & instance) {
								/* NOTE: never name these near/far — windef.h defines both as macros. */
								instance.setDrawDistanceRange(nearDistance, farDistance);

								return changed(visual, "Visual '" + visual.name() + "' draw range set.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setShadowDistance", "Limits the shadow of a " + type + " to a distance from the viewer: beyond it, it is left out of every shadow map.",
						{
							entity,
							component,
							{"distance", "The distance, in metres; 0 for no limit."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float distance) {
							if ( !std::isfinite(distance) || distance < 0.0F )
							{
								return Console::CommandResult::error("The shadow distance must be a finite number of metres, 0 or more.");
							}

							return this->withInstance(entityName, componentName, [distance] (visual_t & visual, Graphics::RenderableInstance::Abstract & instance) {
								instance.setShadowCastingDistance(distance);

								return changed(visual, "Visual '" + visual.name() + "' shadow distance set to " + std::to_string(distance) + " m.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setShadowLODBias", "Draws the shadow of a " + type + " this many levels of detail coarser than the level the shadow pass selects (fewer caster triangles).",
						{
							entity,
							component,
							{"bias", "The number of extra levels, 0 (the selected level) up to the renderable's level-of-detail count minus 1 (getState)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, int32_t bias) {
							if ( bias < 0 )
							{
								return Console::CommandResult::error("The shadow level-of-detail bias must be 0 or more.");
							}

							return this->withInstance(entityName, componentName, [bias] (visual_t & visual, Graphics::RenderableInstance::Abstract & instance) {
								/* ⚠️ The shadow pass clamps the level to the coarsest one: a bias past it would be accepted and change nothing. */
								if ( const auto * renderable = instance.renderable(); renderable != nullptr && static_cast< uint32_t >(bias) >= renderable->levelOfDetailCount() )
								{
									return Console::CommandResult::error("Visual '" + visual.name() + "' has " + std::to_string(renderable->levelOfDetailCount()) + " level(s) of detail: the bias can be at most " + std::to_string(renderable->levelOfDetailCount() - 1U) + ".");
								}

								instance.setShadowLevelOfDetailBias(static_cast< uint32_t >(bias));

								return changed(visual, "Visual '" + visual.name() + "' shadow level-of-detail bias set to " + std::to_string(bias) + ".");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendVisualConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< VisualConsoleAdapter< Visual > >(sceneManager));
		adapters.emplace_back(std::make_unique< VisualConsoleAdapter< MultipleVisuals > >(sceneManager));
	}
}
