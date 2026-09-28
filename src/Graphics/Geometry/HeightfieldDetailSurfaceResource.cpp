/*
 * src/Graphics/Geometry/HeightfieldDetailSurfaceResource.cpp
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

#include "HeightfieldDetailSurfaceResource.hpp"

/* STL inclusions. */
#include <cmath>
#include <utility>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Base;
	using namespace Base::Math;

	HeightfieldDetailSurfaceResource::HeightfieldDetailSurfaceResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, std::shared_ptr< const CDLODTerrainResource > terrain) noexcept
		: VertexGridResource{serviceProvider, name, EnableMeshShadingSurface | EnableHeightfieldSurface},
		m_terrain{std::move(terrain)}
	{

	}

	size_t
	HeightfieldDetailSurfaceResource::classUID () const noexcept
	{
		return getClassUID();
	}

	bool
	HeightfieldDetailSurfaceResource::is (size_t classUID) const noexcept
	{
		return classUID == getClassUID() || classUID == VertexGridResource::getClassUID();
	}

	const char *
	HeightfieldDetailSurfaceResource::classLabel () const noexcept
	{
		return ClassId;
	}

	bool
	HeightfieldDetailSurfaceResource::loadFromTerrain () noexcept
	{
		if ( m_terrain == nullptr || !m_terrain->isDetailWindowEnabled() )
		{
			TraceError{ClassId} << "Detail surface '" << this->name() << "': the terrain has no detail window !";

			return false;
		}

		/* A placeholder the resource machinery needs (a created geometry with a buffer): one cell over the terrain's
		 * extent, never drawn — the task + mesh stages build the window from the clipmap. */
		const auto & box = m_terrain->boundingBox();

		return this->load(std::max(box.width(), box.depth()), 1, 1.0F);
	}

	const Space3D::AACuboid< float > &
	HeightfieldDetailSurfaceResource::boundingBox () const noexcept
	{
		/* The window may be anywhere on the terrain: the renderable is culled like the terrain it stands on. */
		return m_terrain->boundingBox();
	}

	const Space3D::Sphere< float > &
	HeightfieldDetailSurfaceResource::boundingSphere () const noexcept
	{
		return m_terrain->boundingSphere();
	}

	const Vulkan::DescriptorSet *
	HeightfieldDetailSurfaceResource::surfaceDescriptorSet () const noexcept
	{
		/* The terrain's own clipmap set: the window reads the heights the CDLOD draws with. */
		return m_terrain->surfaceDescriptorSet();
	}

	std::optional< MeshShadingSurface >
	HeightfieldDetailSurfaceResource::meshShadingSurfaceFor (const Vector< 3, float > & cameraPosition) const noexcept
	{
		const auto window = m_terrain->detailWindowFor(cameraPosition);

		if ( !window.has_value() )
		{
			return std::nullopt;
		}

		/* One tile per terrain cell: a flat tile then IS a CDLOD level-0 quad, which is what meets the CDLOD at the
		 * window's border. The UV per metre sizes the relief in metres (the material's height scale is in UV units). */
		const auto cell = m_terrain->cellSize();
		const auto tileCount = static_cast< uint32_t >(std::lround(window->size / cell));

		MeshShadingSurface surface;
		surface.originX = window->originX;
		surface.originZ = window->originZ;
		surface.tileSize = cell;
		surface.uvPerMeter = m_terrain->textureUPerMetre();
		surface.tileCountX = tileCount;
		surface.tileCountZ = tileCount;

		return surface;
	}
}
