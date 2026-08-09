/*
 * src/Scenes/InstanceCluster.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics::Renderable
	{
		class Abstract;
	}

	namespace Scenes
	{
		class Scene;
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief Options driving the spatial partitioning of an instance set.
	 */
	struct EMEN_API InstanceClusterOptions
	{
		/**
		 * @brief How many instances a cell should hold, on average.
		 *
		 * @note This is the whole trade-off, and it has no universally right value:
		 * - **too few per cell** and the entity count explodes, each carrying its own draw call
		 *   and its own octree membership — the CPU pays what the GPU saves;
		 * - **too many per cell** and a cell survives culling as soon as one of its instances is
		 *   visible, so the GPU draws thousands of instances for a corner of the screen.
		 *
		 * @warning ⚠️⚠️ **This used to be a LENGTH (`cellSize`, 32 units) and that was wrong.**
		 * A length has no meaning without the content's scale, and one asset spans both extremes:
		 * measured on Jungle Ruins with a fixed 32-unit grid, the moss packed 6 649 instances per
		 * cell while the queen forest — the same code, the same frame — got **3.3**, turning
		 * 613 806 instances into **184 733 scene entities**. The content's density varied by a
		 * factor of two thousand; the constant did not move. A COUNT keeps its meaning whatever
		 * the scale, which is why the knob is expressed this way.
		 *
		 * The edge length is derived from it and from the set's own extent — see
		 * `buildInstanceClusters()`.
		 */
		size_t targetInstancesPerCell{1024};
		/**
		 * @brief Bounds clamping the derived edge length, in world units.
		 *
		 * @note They exist for the degenerate cases the derivation cannot handle on its own: a
		 * set collapsed onto a point or a line has no usable area, and a handful of instances
		 * spread over kilometres would ask for a cell larger than the scene.
		 */
		float minimumCellSize{1.0F};
		float maximumCellSize{4096.0F};
		/**
		 * @brief Declares whether the instances belong on the LIT path.
		 *
		 * @warning ⚠️⚠️ A renderable instance is born UNLIT. Leaving this to the renderer's
		 * default is what turns a whole forest into black silhouettes, with nothing in the log
		 * to say so — the geometry, the placement and the materials are all correct.
		 *
		 * @note Default true, matching `MeshDescriptor::lightingEnabled`. Set it to false only
		 * for content whose lighting is already baked into its vertices, which the ambient and
		 * IBL terms would otherwise count twice.
		 */
		bool lightingEnabled{true};
	};

	/**
	 * @brief Splits an instance set into spatial cells, each becoming its own scene entity.
	 *
	 * A forest must never be one instanced object holding every transform: a single entity
	 * declares a single visual extent, so it is either entirely drawn or entirely culled, and
	 * "entirely drawn" means the whole forest goes to the GPU whenever a leaf of it is on screen.
	 *
	 * Splitting it into cells hands the problem to machinery the engine already has: each cell is
	 * an ordinary entity with its own bounding box, and the rendering octree culls it like
	 * anything else. No new culling path, no per-instance test.
	 *
	 * @note Transforms are stored RELATIVE to their cell, whose entity carries the world position.
	 * Two reasons, and the second is the one that bites: the bounding boxes stay tight, and the
	 * floating-point precision of an instance no longer depends on how far the scene sits from
	 * the origin. Jungle Ruins already lives 800 units out; a real forest goes far further.
	 *
	 * @note Cells are anchored on the CENTROID of the instances they hold rather than on the grid
	 * intersection, which keeps the relative coordinates as small as the content allows. The cell
	 * frame is a pure translation, so expressing an instance relative to it is an exact
	 * subtraction — no inverse, no accumulated error.
	 *
	 * @param scene A reference to the scene receiving the entities.
	 * @param baseName The name prefix; each cell appends its index.
	 * @param renderable A reference to the renderable every instance draws.
	 * @param instances The world-space transform of every instance.
	 * @param options The partitioning options.
	 * @return size_t The number of cells actually created.
	 */
	[[nodiscard]]
	EMEN_API
	size_t buildInstanceClusters (Scene & scene, const std::string & baseName, const std::shared_ptr< Graphics::Renderable::Abstract > & renderable, const std::vector< Base::Math::CartesianFrame< float > > & instances, const InstanceClusterOptions & options = {}) noexcept;
}
