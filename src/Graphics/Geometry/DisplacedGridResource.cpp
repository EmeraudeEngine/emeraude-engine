/*
 * src/Graphics/Geometry/DisplacedGridResource.cpp
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

#include "DisplacedGridResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>

namespace EmEn::Graphics::Geometry
{
	DisplacedGridResource::DisplacedGridResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, float tileSize, uint32_t resourceFlags) noexcept
		: VertexGridResource{serviceProvider, name, resourceFlags | EnableMeshShadingSurface},
		m_tileSize{tileSize > 0.0F ? tileSize : 1.0F}
	{

	}

	size_t
	DisplacedGridResource::classUID () const noexcept
	{
		return getClassUID();
	}

	bool
	DisplacedGridResource::is (size_t classUID) const noexcept
	{
		/* It IS a vertex grid too: everything that asks for one keeps working. */
		return classUID == getClassUID() || classUID == VertexGridResource::getClassUID();
	}

	const char *
	DisplacedGridResource::classLabel () const noexcept
	{
		return ClassId;
	}

	const MeshShadingSurface *
	DisplacedGridResource::meshShadingSurface () const noexcept
	{
		const auto & grid = this->localData();

		if ( !grid.isValid() )
		{
			return nullptr;
		}

		const auto size = grid.squaredSize();
		const auto tileCount = static_cast< uint32_t >(std::max(std::lround(size / m_tileSize), 1L));

		m_surface.originX = -grid.halfSquaredSize();
		m_surface.originZ = -grid.halfSquaredSize();
		m_surface.tileSize = size / static_cast< float >(tileCount);
		m_surface.uvPerMeter = grid.UMultiplier() / size;
		m_surface.tileCountX = tileCount;
		m_surface.tileCountZ = tileCount;

		return &m_surface;
	}
}
