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
#include <algorithm>
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

		if ( options.targetInstancesPerCell == 0 || !(options.minimumCellSize > 0.0F) || options.maximumCellSize < options.minimumCellSize )
		{
			TraceError{TracerTag} << "Invalid clustering options: target " << options.targetInstancesPerCell << " per cell, bounds [" << options.minimumCellSize << ", " << options.maximumCellSize << "].";

			return 0;
		}

		/* ⚠️⚠️ THE EDGE LENGTH IS DERIVED, NEVER GIVEN. A length is meaningless without the
		 * content's scale, and a single asset spans both extremes: with a fixed 32-unit grid,
		 * Jungle Ruins packed 6 649 instances into a moss cell and 3.3 into a forest one — same
		 * code, same frame — turning 613 806 trees into 184 733 scene entities.
		 *
		 * The set's own extent answers it. Vegetation lies on a surface rather than filling a
		 * volume, so the density that matters is the one measured on the GROUND plane: spreading
		 * `count` instances over `areaXZ` at `target` per cell asks for cells of
		 *
		 *     edge = sqrt(areaXZ * target / count)
		 *
		 * A set collapsed onto a line or a point has no usable area — hence the bounds, which are
		 * a guard against degeneracy and not a tuning knob. */
		Vector< 3, float > minimum = instances[0].position();
		Vector< 3, float > maximum = minimum;

		for ( const auto & instance : instances )
		{
			const auto & position = instance.position();

			for ( size_t axis = 0; axis < 3; ++axis )
			{
				minimum[axis] = std::min(minimum[axis], position[axis]);
				maximum[axis] = std::max(maximum[axis], position[axis]);
			}
		}

		const auto extentX = maximum[X] - minimum[X];
		const auto extentZ = maximum[Z] - minimum[Z];
		const auto areaXZ = extentX * extentZ;

		auto cellSize = options.maximumCellSize;

		if ( areaXZ > 0.0F )
		{
			cellSize = std::sqrt(areaXZ * static_cast< float >(options.targetInstancesPerCell) / static_cast< float >(instances.size()));
		}

		cellSize = std::clamp(cellSize, options.minimumCellSize, options.maximumCellSize);

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
				static_cast< int64_t >(std::floor(position[X] / cellSize)),
				static_cast< int64_t >(std::floor(position[Y] / cellSize)),
				static_cast< int64_t >(std::floor(position[Z] / cellSize))
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

			/* ⚠️⚠️ A renderable instance is created UNLIT: `EnableLighting` is off until someone
			 * turns it on. Building the component and stopping there yields cells rendering as pure
			 * BLACK SILHOUETTES — the very symptom documented for a raw `Component::Visual` builder,
			 * and it was hit on the first vegetation ever drawn from a USD asset.
			 *
			 * Not hard-coded to true: content carrying its own baked lighting must stay OFF the lit
			 * path, or the ambient and IBL terms double-count what is already in the vertices. */
			entity->componentBuilder< Component::MultipleVisuals >(entityName + "/Visuals")
				.setup([lightingEnabled = options.lightingEnabled] (auto & visuals) {
					visuals.getRenderableInstance()->setLightingState(lightingEnabled);
				})
				.build(renderable, localFrames);

			builtCount++;
		}

		TraceInfo{TracerTag} <<
			baseName << ": " << instances.size() << " instances split into " << builtCount <<
			" cells of " << cellSize << " units (" <<
			( builtCount > 0 ? instances.size() / builtCount : 0 ) << " per cell on average).";

		return builtCount;
	}
}
