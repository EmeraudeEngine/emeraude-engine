/*
 * src/Scenes/Component/ConsoleAdapter.hpp
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

/* STL inclusions. */
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/* Local inclusions for inheritances. */
#include "Console/ControllableTrait.hpp"

/* Local inclusions for usages. */
#include "Scenes/Component/Abstract.hpp"

namespace EmEn::Scenes
{
	class Manager;
	class Scene;
}

/*
 * Driving the components of an entity from the console and the MCP server (owner decision 2026-09-27:
 * one set of TYPED commands per component type, EXPLICIT addressing). Every command names what it acts
 * on — `entity` (a node anywhere in the hierarchy, or a static entity, of the ACTIVE scene) and
 * `component` (its name on that entity) — so there is no hidden targeting state for two clients to
 * steal from each other, which is what the MCP specification recommends for a stateless protocol.
 *
 * A type's commands live in a small adapter next to the component (e.g. LightConsoleAdapters.cpp),
 * registered once under SceneManagerService (`Core.SceneManagerService.PointLight.setLuminousPower`,
 * MCP tool `SceneManager_PointLight_setLuminousPower`). Nothing is added to the component classes: the
 * thousands of components never driven pay nothing.
 */
namespace EmEn::Scenes::Component
{
	/**
	 * @brief Finds an entity of a scene by name (a node anywhere in the hierarchy, or a static entity) and
	 * one of its components.
	 * @param scene The scene.
	 * @param entityName The node or static entity name.
	 * @param componentName The component name on that entity.
	 * @param component Receives the component.
	 * @param error Receives the reason of a failure (not found, ambiguous name).
	 * @return bool
	 */
	[[nodiscard]]
	bool resolveComponent (const Scene & scene, const std::string & entityName, const std::string & componentName, std::shared_ptr< Abstract > & component, std::string & error) noexcept;

	/**
	 * @brief Runs an action on a component of the active scene, under EXCLUSIVE access to that scene.
	 * @note Exclusive, because the component is read by the logic and render threads; a console command
	 * runs on the main thread between two cycles.
	 * @param sceneManager The scene manager.
	 * @param entityName The node or static entity name.
	 * @param componentName The component name.
	 * @param action The action on the component.
	 * @return Console::CommandResult
	 */
	[[nodiscard]]
	Console::CommandResult withComponent (const Manager & sceneManager, const std::string & entityName, const std::string & componentName, const std::function< Console::CommandResult (Abstract &) > & action) noexcept;

	/**
	 * @brief Base of the console adapter of one component type.
	 * @tparam component_t The component type (it provides a `ClassId`).
	 */
	template< typename component_t >
	class ConsoleAdapter : public Console::ControllableTrait
	{
		protected:

			/**
			 * @brief Constructs the adapter; its console identifier is the component type name.
			 * @param sceneManager The scene manager.
			 */
			explicit
			ConsoleAdapter (const Manager & sceneManager) noexcept
				: ControllableTrait{component_t::ClassId},
				m_sceneManager{sceneManager}
			{

			}

			/**
			 * @brief Runs an action on a component of this type, refusing a component of another type.
			 * @param entityName The node or static entity name.
			 * @param componentName The component name.
			 * @param action A callable taking `component_t &` and returning a Console::CommandResult.
			 * @return Console::CommandResult
			 */
			template< typename action_t >
			[[nodiscard]]
			Console::CommandResult
			act (const std::string & entityName, const std::string & componentName, action_t && action) const noexcept
			{
				return withComponent(m_sceneManager, entityName, componentName, [&] (Abstract & component) {
					auto * typed = dynamic_cast< component_t * >(&component);

					if ( typed == nullptr )
					{
						return Console::CommandResult::error("The component '" + componentName + "' of '" + entityName + "' is a " + component.getComponentType() + ", not a " + component_t::ClassId + ".");
					}

					return action(*typed);
				});
			}

		private:

			const Manager & m_sceneManager;
	};

	/**
	 * @brief Creates the console adapters of the light components (PointLight, SpotLight, DirectionalLight).
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendLightConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;
}
