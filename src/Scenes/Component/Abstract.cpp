/*
 * src/Scenes/Component/Abstract.cpp
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

#include "Abstract.hpp"

/* Local inclusions. */
#include "Scenes/AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Graphics;
	using namespace Physics;

	constexpr auto TracerTag{"EntityComponent"};

	const size_t Abstract::ClassUID{getClassUID("AbstractEntityComponent")};
	const Space3D::AACuboid< float > Abstract::NullBoundingBox{};
	const Space3D::Sphere< float > Abstract::NullBoundingSphere{};

	Abstract::Abstract (const std::string & name, const AbstractEntity & parentEntity) noexcept
		: NameableTrait(name),
		m_parentEntity(parentEntity)
	{

	}

	bool
	Abstract::initialize (const Json::Value & /*unused*/) noexcept
	{
		Tracer::warning(TracerTag, "This entity didn't reimplement the JSON initialization method !");

		return false;
	}

	bool
	Abstract::isParentEntityMovable () const noexcept
	{
		return m_parentEntity.hasMovableAbility();
	}

	const Renderable::Interface *
	Abstract::getRenderable () const noexcept
	{
		const auto renderableInstance = this->getRenderableInstance();

		if ( renderableInstance == nullptr )
		{
			return nullptr;
		}

		return renderableInstance->renderable();
	}

	CartesianFrame< float >
	Abstract::getWorldCoordinates () const noexcept
	{
		return m_parentEntity.getWorldCoordinates();
	}

	Vector< 3, float >
	Abstract::getWorldVelocity () const noexcept
	{
		const auto * movable = m_parentEntity.getMovableTrait();

		if ( movable == nullptr )
		{
			return {};
		}

		return movable->getWorldVelocity();
	}
}
