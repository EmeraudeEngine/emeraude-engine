/*
 * src/Scenes/DefinitionResource.cpp
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

#include "DefinitionResource.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Resources/Manager.hpp"
#include "Libs/FastJSON.hpp"
#include "Scene.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Graphics;

	bool
	DefinitionResource::load (Resources::ServiceProvider & /*serviceProvider*/) noexcept
	{
		return false;
	}

	bool
	DefinitionResource::load (Resources::ServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		const auto rootCheck = FastJSON::getRootFromFile(filepath);

		if ( !rootCheck )
		{
			TraceError{ClassId} << "Unable to parse the resource file " << filepath << " !" "\n";

			return false;
		}

		const auto & root = rootCheck.value();

		/* Checks if additional stores before loading (optional) */
		serviceProvider.update(root);

		return this->load(serviceProvider, root);
	}

	bool
	DefinitionResource::load (Resources::ServiceProvider & /*serviceProvider*/, const Json::Value & data) noexcept
	{
		m_root = data;

		return true;
	}

	std::string
	DefinitionResource::getSceneName () const noexcept
	{
		if ( m_root.isMember(FastJSON::NameKey) && m_root[FastJSON::NameKey].isString() )
		{
			return m_root[FastJSON::NameKey].asString();
		}

		return "NoName";
	}

	bool
	DefinitionResource::buildScene (Scene & scene) noexcept
	{
		if ( m_root.empty() )
		{
			Tracer::error(ClassId, "No data ! Load a JSON file or set a JSON string before.");

			return false;
		}

		/* Checks scene properties. */
		this->readProperties(scene);

		/* Checks for the background. */
		this->readBackground(scene);

		/* Checks for the scene area. */
		this->readSceneArea(scene);

		return true;
	}

	Json::Value
	DefinitionResource::getExtraData () const noexcept
	{
		if ( !m_root.isMember(ExtraDataKey) || !m_root[ExtraDataKey].isObject() )
		{
			return {};
		}

		return m_root[ExtraDataKey];
	}

	bool
	DefinitionResource::readProperties (Scene & scene) noexcept
	{
		if ( !m_root.isMember(FastJSON::PropertiesKey) || !m_root[FastJSON::PropertiesKey].isObject() )
		{
			TraceWarning{ClassId} << "There is no '" << FastJSON::PropertiesKey << "' definition or is invalid !";

			return false;
		}

		/* Checks for global scene properties. */
		const auto properties = m_root[FastJSON::PropertiesKey];

		scene.setEnvironmentPhysicalProperties({
			FastJSON::getValue< float >(properties, SurfaceGravityKey).value_or(Physics::Gravity::Earth< float >),
			FastJSON::getValue< float >(properties, AtmosphericDensityKey).value_or(Physics::Density::EarthStandardAir< float >),
			FastJSON::getValue< float >(properties, PlanetRadiusKey).value_or(Physics::Radius::Earth< float >)
		});

		return true;
	}

	bool
	DefinitionResource::readBackground (Scene & /*scene*/) noexcept
	{
		return true;
	}

	bool
	DefinitionResource::readSceneArea (Scene & /*scene*/) noexcept
	{
		return true;
	}
}
