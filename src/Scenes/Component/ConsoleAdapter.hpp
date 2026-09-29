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
#include <map>
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
	class AbstractEntity;
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
	/*
	 * ADDRESSING (owner decision 2026-09-27): an entity is named by the SHORTEST SUFFIX of its node path that
	 * is unique in the scene — `Head` when there is one, `ACTOR_…09/Head` when six actors each carry a `Head`.
	 * Sibling names are unique (Node::children() is a map), so the full path from the root always resolves.
	 * An ambiguous address is REFUSED with its candidates: acting on the first node found was acting on a
	 * random one. A static entity has no parent: its address is its name.
	 */

	/**
	 * @brief Finds an entity of a scene by its address: a node path suffix (`Parent/Child`, a bare name when
	 * unique), or a static entity name.
	 * @param scene The scene.
	 * @param address The entity address.
	 * @param entity Receives the entity.
	 * @param error Receives the reason of a failure (not found, ambiguous address and its candidates).
	 * @return bool
	 */
	[[nodiscard]]
	bool resolveEntity (const Scene & scene, const std::string & address, std::shared_ptr< AbstractEntity > & entity, std::string & error) noexcept;

	/**
	 * @brief Returns the shortest unambiguous address of every entity of a scene (nodes and static entities).
	 * @note An entity no address can single out (a static entity sharing its name with a root node) maps to its
	 * name and is refused by resolveEntity() as ambiguous.
	 * @param scene The scene.
	 * @return std::map< const AbstractEntity *, std::string >
	 */
	[[nodiscard]]
	std::map< const AbstractEntity *, std::string > entityAddresses (const Scene & scene) noexcept;

	/**
	 * @brief Finds an entity of a scene by its address (see resolveEntity()) and one of its components.
	 * @param scene The scene.
	 * @param entityAddress The entity address.
	 * @param componentName The component name on that entity.
	 * @param component Receives the component.
	 * @param error Receives the reason of a failure (not found, ambiguous address).
	 * @return bool
	 */
	[[nodiscard]]
	bool resolveComponent (const Scene & scene, const std::string & entityAddress, const std::string & componentName, std::shared_ptr< Abstract > & component, std::string & error) noexcept;

	/**
	 * @brief Returns the `entity` parameter every component command starts with.
	 * @param holds What the entity holds, for the description (e.g. "the camera").
	 * @return Console::Parameter
	 */
	[[nodiscard]]
	Console::Parameter entityParameter (const std::string & holds) noexcept;

	/**
	 * @brief Returns the `component` parameter every component command takes second.
	 * @param componentType The component type, for the description (e.g. "Camera").
	 * @return Console::Parameter
	 */
	[[nodiscard]]
	Console::Parameter componentParameter (const std::string & componentType) noexcept;

	/**
	 * @brief The answer of a setter: the confirmation, then the component's NEW state as a JSON output.
	 * @note The state is what a machine client reads (MCP structuredContent): it confirms the values actually
	 * applied without a second getState() call.
	 * @param message The confirmation.
	 * @param stateJSON The component state, as the type's getState command answers it.
	 * @return Console::CommandResult
	 */
	[[nodiscard]]
	Console::CommandResult changedState (std::string message, std::string stateJSON) noexcept;

	/**
	 * @brief Runs an action on a component of the active scene, under EXCLUSIVE access to that scene.
	 * @note Exclusive, because the component is read by the logic and render threads; a console command
	 * runs on the main thread between two cycles.
	 * @param sceneManager The scene manager.
	 * @param entityName The entity address (see resolveEntity()).
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
			 * @param entityName The entity address (see resolveEntity()).
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

			/**
			 * @brief Returns the scene manager, for a command that does not address one component (e.g. the active camera).
			 * @return const Manager &
			 */
			[[nodiscard]]
			const Manager &
			sceneManager () const noexcept
			{
				return m_sceneManager;
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

	/**
	 * @brief Creates the console adapter of the Camera component.
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendCameraConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;

	/**
	 * @brief Creates the console adapters of the sky and weather components (SunCourse, SkyFollowsSun, CloudVolume).
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendEnvironmentConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;

	/**
	 * @brief Creates the console adapters of the animated components (NodeAnimation, ParticlesEmitter).
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendAnimationConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;

	/**
	 * @brief Creates the console adapters of the physics components (DirectionalPushModifier, SphericalPushModifier, Weight).
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendPhysicsConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;

	/**
	 * @brief Creates the console adapter of the SoundEmitter component.
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendAudioConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;

	/**
	 * @brief Creates the console adapters of the drawn components (Visual, MultipleVisuals).
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendVisualConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;

	/**
	 * @brief Creates the console adapter of the Beam component (lasers, electric arcs).
	 * @param sceneManager The scene manager.
	 * @param adapters Receives the adapters (the caller registers and owns them).
	 * @return void
	 */
	void appendBeamConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept;
}
