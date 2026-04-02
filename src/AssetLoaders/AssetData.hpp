/*
 * src/AssetLoaders/AssetData.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"

/* Forward declarations. */
namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Graphics
{
	namespace Geometry
	{
		class Interface;
	}

	namespace Material
	{
		class Interface;
	}

	namespace Renderable
	{
		class Abstract;
	}
}

namespace EmEn::AssetLoaders
{
	/**
	 * @brief Format-agnostic description of a node in the loaded asset.
	 * @note Contains no Scene/Node/Entity types — purely data.
	 */
	struct NodeDescriptor
	{
		std::string name;
		Libs::Math::CartesianFrame< float > localFrame;
		std::optional< size_t > meshIndex;
		std::vector< size_t > childIndices;
	};

	/**
	 * @brief Describes a loaded mesh with its geometry and materials.
	 */
	struct MeshDescriptor
	{
		std::shared_ptr< Graphics::Renderable::Abstract > renderable;
		std::shared_ptr< Graphics::Geometry::Interface > geometry;
		std::vector< std::shared_ptr< Graphics::Material::Interface > > materials;
	};

	/**
	 * @brief Format-agnostic result of loading a composite asset.
	 * @note All resources are already registered in engine containers.
	 * The node hierarchy is described via NodeDescriptors without any
	 * dependency on the Scenes/ subsystem.
	 */
	struct AssetData
	{
		/* Resources (already in engine containers). */
		std::vector< MeshDescriptor > meshes;
		std::vector< std::shared_ptr< Animations::SkeletonResource > > skeletons;
		std::vector< std::shared_ptr< Animations::AnimationClipResource > > animationClips;

		/* Node hierarchy (format-agnostic). */
		std::vector< NodeDescriptor > nodes;
		std::vector< size_t > rootNodeIndices;
		std::unordered_set< size_t > skinJointNodeIndices;

		/**
		 * @brief Checks if the asset contains exactly one mesh-bearing node.
		 * @note Structural and skeleton joint nodes are ignored.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isSingleMesh () const noexcept
		{
			size_t count = 0;

			for ( const auto & node : nodes )
			{
				if ( node.meshIndex.has_value() )
				{
					count++;

					if ( count > 1 )
					{
						return false;
					}
				}
			}

			return count == 1;
		}

		/**
		 * @brief Returns the index (into nodes[]) of the single mesh-bearing node.
		 * @warning Only valid when isSingleMesh() returns true.
		 * @return size_t
		 */
		[[nodiscard]]
		size_t
		singleMeshNodeIndex () const noexcept
		{
			for ( size_t i = 0; i < nodes.size(); ++i )
			{
				if ( nodes[i].meshIndex.has_value() )
				{
					return i;
				}
			}

			return 0;
		}
	};
}
