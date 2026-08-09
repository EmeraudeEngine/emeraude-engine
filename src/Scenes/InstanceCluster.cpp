/*
 * src/Scenes/InstanceCluster.cpp
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

#include "InstanceCluster.hpp"

/* STL inclusions. */
#include <array>
#include <cmath>
#include <map>

/* Local inclusions. */
#include "Component/MultipleVisuals.hpp"
#include "Scene.hpp"
#include "StaticEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Base::Math;

	static constexpr auto TracerTag{"InstanceCluster"};

	size_t
	buildInstanceClusters (Scene & scene, const std::string & baseName, const std::shared_ptr< Graphics::Renderable::Abstract > & renderable, const std::vector< CartesianFrame< float > > & instances, const InstanceClusterOptions & options) noexcept
	{
		if ( renderable == nullptr || instances.empty() )
		{
			return 0;
		}

		if ( !(options.cellSize > 0.0F) )
		{
			TraceError{TracerTag} << "A cell size must be strictly positive, got " << options.cellSize << ".";

			return 0;
		}

		/* An ordered map keyed by the integer cell coordinates: no hash to write, and the
		 * iteration order is deterministic, so the entity names a run produces are stable. */
		using CellKey = std::array< int64_t, 3 >;

		std::map< CellKey, std::vector< size_t > > cells;

		for ( size_t index = 0; index < instances.size(); ++index )
		{
			const auto & position = instances[index].position();

			/* std::floor, NOT a cast: a cast truncates towards zero, which folds the cells on
			 * either side of an axis into one and doubles their size across the origin. */
			const CellKey key{
				static_cast< int64_t >(std::floor(position[X] / options.cellSize)),
				static_cast< int64_t >(std::floor(position[Y] / options.cellSize)),
				static_cast< int64_t >(std::floor(position[Z] / options.cellSize))
			};

			cells[key].push_back(index);
		}

		size_t builtCount = 0;

		for ( const auto & [key, indices] : cells )
		{
			/* The centroid, not the grid intersection: it keeps the relative coordinates as small
			 * as the content allows, which is the point of the whole exercise. */
			Vector< 3, float > centroid;

			for ( const auto index : indices )
			{
				centroid += instances[index].position();
			}

			centroid /= static_cast< float >(indices.size());

			std::vector< CartesianFrame< float > > localFrames;
			localFrames.reserve(indices.size());

			for ( const auto index : indices )
			{
				auto frame = instances[index];

				/* The cell frame is a pure translation, so expressing an instance relative to it
				 * is an exact subtraction — no inverse matrix, no accumulated error. */
				frame.setPosition(frame.position() - centroid);

				localFrames.emplace_back(frame);
			}

			const auto entityName = baseName + "/cell-" + std::to_string(builtCount);

			auto entity = scene.createStaticEntity(entityName, CartesianFrame< float >{centroid});

			if ( entity == nullptr )
			{
				TraceWarning{TracerTag} << "Unable to create the entity of cell " << entityName << ".";

				continue;
			}

			entity->componentBuilder< Component::MultipleVisuals >(entityName + "/Visuals")
				.build(renderable, localFrames);

			builtCount++;
		}

		TraceInfo{TracerTag} <<
			baseName << ": " << instances.size() << " instances split into " << builtCount <<
			" cells of " << options.cellSize << " units (" <<
			( builtCount > 0 ? instances.size() / builtCount : 0 ) << " per cell on average).";

		return builtCount;
	}
}
