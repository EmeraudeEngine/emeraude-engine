/*
 * src/Graphics/Geometry/Interface.cpp
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

#include "Interface.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iterator>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::VertexFactory;

	constexpr auto TracerTag{"GeometryInterface"};

	Interface::Interface (const std::string & name, uint32_t geometryFlagBits) noexcept
		: ResourceTrait(name, geometryFlagBits)
	{

	}

	bool
	Interface::buildSubGeometries (std::vector< SubGeometry > & subGeometries, uint32_t length) noexcept
	{
		if ( length == 0 )
		{
			return false;
		}

		subGeometries.clear();
		subGeometries.emplace_back(0, length);

		return true;
	}

	bool
	Interface::buildSubGeometries (std::vector< SubGeometry > & subGeometries, const Shape< float > & shape) noexcept
	{
		if ( shape.empty() )
		{
			return false;
		}

		subGeometries.clear();

		if ( shape.hasGroups() )
		{
			//TraceInfo{TracerTag} << "Shape has " << shape.groupCount() << " groups ! Creating sub-geometries ...";

			/* FIXME: Check topology/primitive */
			std::ranges::transform(shape.groups(), std::back_inserter(subGeometries), [] (const auto & group) {
				const auto offset = static_cast< uint32_t >(group.first * 3);
				const auto length = static_cast< uint32_t >(group.second * 3);

				return SubGeometry{offset, length};
			});
		}

		return true;
	}

	bool
	Interface::buildSubGeometries (std::vector< SubGeometry > & subGeometries, const Grid< float > & grid) noexcept
	{
		if ( grid.empty() )
		{
			return false;
		}

		subGeometries.clear();

		// TODO ...

		return true;
	}

	bool
	Interface::onDependenciesLoaded () noexcept
	{
		/* NOTE: Ensure the texture is on the video memory. */
		if ( !this->isCreated() && !this->create() )
		{
			TraceError{TracerTag} << "Unable to send the geometry resource '" << this->name() << "' (" << this->classLabel() << ") into the video memory !";

			return false;
		}

		return true;
	}
}
