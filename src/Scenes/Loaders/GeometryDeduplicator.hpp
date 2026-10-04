/*
 * src/Scenes/Loaders/GeometryDeduplicator.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>

/* Local inclusions for usages. */
#include "VertexFactory/Shape.hpp"

/* Forward declarations. */
namespace EmEn::Graphics::Geometry
{
	class IndexedVertexResource;
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief Shares identical geometry within one load: the same shape, met again, returns the geometry already built.
	 * @note Keyed by CONTENT — vertex count, triangle count and a 64-bit hash of the positions, normals, texture
	 * coordinates, vertex colours, triangles and groups — never by name or source identity, which composition and
	 * per-instancer prototypes lose (owner decision 2026-10-04). JungleRuins built 6 tree meshes 65 times each, 23 GB
	 * of GPU memory, without it.
	 * @note Called where a loader creates a geometry, BEFORE the resource exists: a duplicate never computes its
	 * tangents nor reaches the GPU. Thread-safe.
	 * @note Two different shapes sharing counts and a 64-bit hash would be merged: a probability of about 2^-64 per pair,
	 * accepted (comparing every byte would mean keeping every shape in memory until the load ends).
	 */
	class GeometryDeduplicator final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GeometryDeduplicator"};

			/** @brief What identifies a shape's content. */
			struct Key
			{
				uint64_t hash{0};
				uint64_t vertexCount{0};
				uint64_t triangleCount{0};
				uint32_t flags{0};

				/**
				 * @brief Orders keys (std::map).
				 * @param other The other key.
				 * @return bool
				 */
				[[nodiscard]]
				bool
				operator< (const Key & other) const noexcept
				{
					return std::tie(hash, vertexCount, triangleCount, flags) < std::tie(other.hash, other.vertexCount, other.triangleCount, other.flags);
				}
			};

			/**
			 * @brief Computes the key of a shape.
			 * @param shape The shape.
			 * @param flags The geometry flags it will be created with (part of the identity).
			 * @return Key
			 */
			[[nodiscard]]
			static Key key (const Base::VertexFactory::Shape< float > & shape, uint32_t flags) noexcept;

			/**
			 * @brief Returns the geometry already built for this content, or builds it with 'create' and remembers it.
			 * @tparam create_t A callable `std::shared_ptr< IndexedVertexResource > ()`.
			 * @param shape The shape.
			 * @param flags The geometry flags.
			 * @param create Builds the geometry when the content is new.
			 * @return std::shared_ptr< Graphics::Geometry::IndexedVertexResource > Nullptr when 'create' failed.
			 */
			template< typename create_t >
			[[nodiscard]]
			std::shared_ptr< Graphics::Geometry::IndexedVertexResource >
			getOrCreate (const Base::VertexFactory::Shape< float > & shape, uint32_t flags, create_t && create) noexcept
			{
				const auto contentKey = GeometryDeduplicator::key(shape, flags);

				{
					const std::scoped_lock lock{m_access};

					if ( const auto it = m_geometries.find(contentKey); it != m_geometries.end() )
					{
						++m_sharedCount;
						m_sharedVertexCount += contentKey.vertexCount;

						return it->second;
					}
				}

				auto geometry = std::forward< create_t >(create)();

				if ( geometry != nullptr )
				{
					const std::scoped_lock lock{m_access};

					m_geometries.emplace(contentKey, geometry);
				}

				return geometry;
			}

			/**
			 * @brief Returns how many geometries were shared instead of built.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			sharedCount () const noexcept
			{
				const std::scoped_lock lock{m_access};

				return m_sharedCount;
			}

			/**
			 * @brief Returns how many vertices those shared geometries would have built again.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			sharedVertexCount () const noexcept
			{
				const std::scoped_lock lock{m_access};

				return m_sharedVertexCount;
			}

		private:

			mutable std::mutex m_access;
			std::map< Key, std::shared_ptr< Graphics::Geometry::IndexedVertexResource > > m_geometries;
			size_t m_sharedCount{0};
			uint64_t m_sharedVertexCount{0};
	};
}
