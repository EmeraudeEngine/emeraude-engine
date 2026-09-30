/*
 * src/Scenes/Component/ConsoleAdapter.cpp
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
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <vector>

/* Local inclusions. */
#include "Scenes/Manager.hpp"
#include "Scenes/Node.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/StaticEntity.hpp"

namespace EmEn::Scenes::Component
{
	Console::Parameter
	entityParameter (const std::string & holds) noexcept
	{
		return {"entity", "The address of the entity of the active scene that holds " + holds + ": its name when unique, else the shortest unique path suffix (Parent/Child); SceneManager listEntities() gives every entity's address."};
	}

	Console::Parameter
	componentParameter (const std::string & componentType) noexcept
	{
		return {"component", "The " + componentType + " component's name on that entity (SceneManager listEntityComponents(entity) lists them)."};
	}

	std::optional< std::vector< Base::Math::Vector< 3, float > > >
	parsePointList (const std::string & text) noexcept
	{
		std::vector< Base::Math::Vector< 3, float > > points;
		std::stringstream pointsStream{text};
		std::string item;

		while ( std::getline(pointsStream, item, ';') )
		{
			for ( auto & character : item )
			{
				if ( character == ',' )
				{
					character = ' ';
				}
			}

			std::stringstream coordinates{item};
			float x = 0.0F;
			float y = 0.0F;
			float z = 0.0F;

			if ( !(coordinates >> x >> y >> z) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) )
			{
				/* An empty trailing item ("...;") is not an error. */
				if ( item.find_first_not_of(" \t") == std::string::npos )
				{
					continue;
				}

				return std::nullopt;
			}

			points.emplace_back(x, y, z);
		}

		return points;
	}

	Console::CommandResult
	changedState (std::string message, std::string stateJSON) noexcept
	{
		return Console::CommandResult::success(std::move(message)).add(Console::Output::json(std::move(stateJSON)));
	}

	namespace
	{
		/** @brief An entity of the scene and the names on its path from the root (itself last). */
		struct AddressedEntity
		{
			std::shared_ptr< AbstractEntity > entity;
			std::vector< std::string > path;
		};

		/**
		 * @brief Returns every entity of a scene with its path: the nodes (depth first) then the static entities.
		 * @param scene The scene.
		 * @return std::vector< AddressedEntity >
		 */
		[[nodiscard]]
		std::vector< AddressedEntity >
		collectEntities (const Scene & scene) noexcept
		{
			std::vector< AddressedEntity > entities;
			std::vector< AddressedEntity > pending;

			/* The root itself is not an entity anyone names. */
			for ( const auto & child : scene.root()->children() | std::views::values )
			{
				pending.push_back({child, {child->name()}});
			}

			while ( !pending.empty() )
			{
				auto current = std::move(pending.back());
				pending.pop_back();

				for ( const auto & child : std::static_pointer_cast< Node >(current.entity)->children() | std::views::values )
				{
					auto path = current.path;
					path.emplace_back(child->name());

					pending.push_back({child, std::move(path)});
				}

				entities.emplace_back(std::move(current));
			}

			scene.forEachStaticEntities([&scene, &entities] (const StaticEntity & staticEntity) {
				entities.push_back({scene.findStaticEntity(staticEntity.name()), {staticEntity.name()}});
			});

			return entities;
		}

		/**
		 * @brief Calls a function on every address of an entity, shortest first: its name, then its parent + its
		 * name, up to its full path. The function returns false to stop.
		 * @param path The entity path.
		 * @param process The function.
		 * @return void
		 */
		template< typename function_t >
		void
		forEachSuffix (const std::vector< std::string > & path, function_t && process) noexcept
		{
			std::string suffix;

			for ( auto index = path.size(); index > 0; --index )
			{
				suffix = suffix.empty() ? path[index - 1] : path[index - 1] + '/' + suffix;

				if ( !process(suffix) )
				{
					return;
				}
			}
		}
	}

	bool
	resolveEntity (const Scene & scene, const std::string & address, std::shared_ptr< AbstractEntity > & entity, std::string & error) noexcept
	{
		std::vector< AddressedEntity > matches;

		for ( auto & candidate : collectEntities(scene) )
		{
			bool matched = false;

			/* NOTE: suffixes are built on node boundaries, so a node name that itself contains '/' still matches exactly. */
			forEachSuffix(candidate.path, [&address, &matched] (const std::string & suffix) {
				matched = suffix == address;

				return !matched && suffix.size() < address.size();
			});

			if ( matched )
			{
				matches.emplace_back(std::move(candidate));
			}
		}

		if ( matches.empty() )
		{
			error = "No entity at '" + address + "' in the scene '" + scene.name() + "' (SceneManager listEntities() gives every entity's address).";

			return false;
		}

		if ( matches.size() > 1 )
		{
			const auto addresses = entityAddresses(scene);

			error = "'" + address + "' names " + std::to_string(matches.size()) + " entities of the scene '" + scene.name() + "': give a longer path, one of";

			for ( const auto & match : matches )
			{
				const auto addressIt = addresses.find(match.entity.get());

				error += " '" + ( addressIt != addresses.end() ? addressIt->second : match.path.back() ) + "'";
			}

			error += ".";

			return false;
		}

		entity = std::move(matches.front().entity);

		return true;
	}

	std::map< const AbstractEntity *, std::string >
	entityAddresses (const Scene & scene) noexcept
	{
		const auto entities = collectEntities(scene);

		/* How many entities each suffix designates. */
		std::unordered_map< std::string, size_t > designations;

		for ( const auto & candidate : entities )
		{
			forEachSuffix(candidate.path, [&designations] (const std::string & suffix) {
				++designations[suffix];

				return true;
			});
		}

		std::map< const AbstractEntity *, std::string > addresses;

		for ( const auto & candidate : entities )
		{
			std::string address;

			forEachSuffix(candidate.path, [&designations, &address] (const std::string & suffix) {
				address = suffix;

				return designations[suffix] > 1;
			});

			addresses.emplace(candidate.entity.get(), std::move(address));
		}

		return addresses;
	}

	bool
	resolveComponent (const Scene & scene, const std::string & entityAddress, const std::string & componentName, std::shared_ptr< Abstract > & component, std::string & error) noexcept
	{
		std::shared_ptr< AbstractEntity > entity;

		if ( !resolveEntity(scene, entityAddress, entity, error) )
		{
			return false;
		}

		component = entity->getComponent(componentName);

		if ( component == nullptr )
		{
			error = "'" + entityAddress + "' has no component named '" + componentName + "' (SceneManager listEntityComponents(entity) lists them).";

			return false;
		}

		return true;
	}

	Console::CommandResult
	withComponent (const Manager & sceneManager, const std::string & entityName, const std::string & componentName, const std::function< Console::CommandResult (Abstract &) > & action) noexcept
	{
		auto result = Console::CommandResult::error("No active scene !");

		sceneManager.withExclusiveActiveScene([&] (const std::shared_ptr< Scene > & scene) {
			std::shared_ptr< Abstract > component;
			std::string error;

			if ( !resolveComponent(*scene, entityName, componentName, component, error) )
			{
				result = Console::CommandResult::error(error);

				return;
			}

			result = action(*component);
		}, true);

		return result;
	}
}
