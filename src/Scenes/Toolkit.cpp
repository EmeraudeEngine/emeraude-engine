/*
 * src/Scenes/Toolkit.cpp
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

#include "Toolkit.hpp"

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/BlobTrait.hpp"
#include "Libs/Utility.hpp"
#include "Node.hpp"
#include "Scenes/Scene.hpp"
#include "StaticEntity.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Libs::VertexFactory;
	using namespace EmEn::Graphics;

	size_t Toolkit::s_autoEntityCount{0};

	std::shared_ptr< Node >
	Toolkit::generateNode (const std::string & entityName, GenPolicy genPolicy, bool movable) noexcept
	{
		const auto name = String::incrementalLabel(entityName.empty() ? "AutoNode" : entityName, s_autoEntityCount);

		if ( m_scene == nullptr )
		{
			TraceError{ClassId} << "There is no linked scene to create a scene node '" << name << "' !";

			return nullptr;
		}

		std::shared_ptr< Node > parent{};

		switch ( m_nodeGenerationPolicy )
		{
			/* NOTE: A reusable scene node has been set. Returning it ... */
			case GenPolicy::Reusable :
				if ( m_previousNode == nullptr )
				{
					break;
				}

				return m_previousNode;

			/* NOTE: A parent scene node has been set. Returning it ... */
			case GenPolicy::Parent :
				if ( m_previousNode == nullptr )
				{
					break;
				}

				parent = m_previousNode->createChild(name, m_scene->lifetimeMS(), m_cursorFrame);
				break;

			default:
				parent = m_scene->root();
				break;
		}

		auto childNode = parent->createChild(name, m_scene->lifetimeMS(), m_cursorFrame);

		if ( childNode == nullptr )
		{
			TraceError{ClassId} << "Unable to create a scene node '" << name << "' !";

			return nullptr;
		}

		childNode->setMovingAbility(movable);

		/* Change the new policy if requested. */
		switch ( genPolicy )
		{
			case GenPolicy::Reusable :
				this->setReusableNode(childNode);
				break;

			case GenPolicy::Parent :
				this->setParentNode(childNode);
				break;

			default:
				break;
		}

		return childNode;
	}

	std::shared_ptr< Node >
	Toolkit::generateNode (const Vector< 3, float > & pointTo, const std::string & entityName, GenPolicy genPolicy, bool movable) noexcept
	{
		auto node = this->generateNode(entityName, genPolicy, movable);

		if ( node == nullptr )
		{
			return nullptr;
		}

		node->lookAt(pointTo, true);

		return node;
	}

	std::shared_ptr< StaticEntity >
	Toolkit::generateStaticEntity (const std::string & entityName, GenPolicy genPolicy) noexcept
	{
		const auto name = String::incrementalLabel(entityName.empty() ? "AutoNode" : entityName, s_autoEntityCount);

		if ( m_scene == nullptr )
		{
			TraceError{ClassId} << "There is no linked scene to create a static entity '" << name << "' !";

			return nullptr;
		}

		if ( m_staticEntityGenerationPolicy == GenPolicy::Reusable && m_previousStaticEntity != nullptr )
		{
			return m_previousStaticEntity;
		}

		auto staticEntity = m_scene->createStaticEntity(name, m_cursorFrame);

		if ( staticEntity == nullptr )
		{
			TraceError{ClassId} << "Unable to create a static entity '" << name << "' !";

			return nullptr;
		}

		/* Change the new policy if requested. */
		if ( genPolicy == GenPolicy::Reusable )
		{
			this->setReusableStaticEntity(staticEntity);
		}

		return staticEntity;
	}

	std::shared_ptr< StaticEntity >
	Toolkit::generateStaticEntity (const Vector< 3, float > & pointTo, const std::string & entityName, GenPolicy genPolicy) noexcept
	{
		auto staticEntity = this->generateStaticEntity(entityName, genPolicy);

		if ( staticEntity == nullptr )
		{
			return nullptr;
		}

		staticEntity->lookAt(pointTo, true);

		return staticEntity;
	}

	std::vector< CartesianFrame< float > >
	Toolkit::generateRandomCoordinates (size_t count, float min, float max) noexcept
	{
		Randomizer< float > randomizer;

		std::vector< CartesianFrame< float > > coordinates{count};

		for ( auto & coordinate : coordinates )
		{
			coordinate.setPosition(
				randomizer.value(min, max),
				randomizer.value(min, max),
				randomizer.value(min, max)
			);
		}

		return coordinates;
	}
}
