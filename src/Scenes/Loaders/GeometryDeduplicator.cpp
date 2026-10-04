/*
 * src/Scenes/Loaders/GeometryDeduplicator.cpp
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


#include "GeometryDeduplicator.hpp"

/* STL inclusions. */
#include <bit>

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"

namespace EmEn::Scenes::Loaders
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief A 64-bit streaming hash over 64-bit words: FNV-1a's xor-multiply step on whole words, finished by the
		 * splitmix64 avalanche (Steele, Lea, Flood, "Fast splittable pseudorandom number generators", OOPSLA 2014).
		 * @note Word-wise for speed: JungleRuins hashes about ten gigabytes of vertex data in one load.
		 */
		class ContentHash final
		{
			public:

				/**
				 * @brief Mixes one 64-bit word.
				 * @param word The word.
				 * @return void
				 */
				void
				add (uint64_t word) noexcept
				{
					m_state = (m_state ^ word) * Prime;
				}

				/**
				 * @brief Mixes two floats as one word (their bit patterns: -0.0 and 0.0 differ, as they do in memory).
				 * @param first The first float.
				 * @param second The second float.
				 * @return void
				 */
				void
				add (float first, float second) noexcept
				{
					this->add((static_cast< uint64_t >(std::bit_cast< uint32_t >(first)) << 32U) | std::bit_cast< uint32_t >(second));
				}

				/**
				 * @brief Returns the finished hash.
				 * @return uint64_t
				 */
				[[nodiscard]]
				uint64_t
				value () const noexcept
				{
					auto hash = m_state;
					hash = (hash ^ (hash >> 30U)) * 0xBF58476D1CE4E5B9ULL;
					hash = (hash ^ (hash >> 27U)) * 0x94D049BB133111EBULL;

					return hash ^ (hash >> 31U);
				}

			private:

				static constexpr uint64_t Prime{1099511628211ULL};

				uint64_t m_state{14695981039346656037ULL};
		};
	}

	GeometryDeduplicator::Key
	GeometryDeduplicator::key (const VertexFactory::Shape< float > & shape, uint32_t flags) noexcept
	{
		ContentHash hash;

		const auto & vertices = shape.vertices();

		for ( const auto & vertex : vertices )
		{
			const auto & position = vertex.position();
			const auto & normal = vertex.normal();
			const auto & primary = vertex.textureCoordinates();
			const auto & secondary = vertex.secondaryTextureCoordinates();

			hash.add(position[0], position[1]);
			hash.add(position[2], normal[0]);
			hash.add(normal[1], normal[2]);
			hash.add(primary[0], primary[1]);
			hash.add(primary[2], secondary[0]);
			hash.add(secondary[1], secondary[2]);
		}

		for ( const auto & color : shape.vertexColors() )
		{
			hash.add(color[0], color[1]);
			hash.add(color[2], color[3]);
		}

		const auto & triangles = shape.triangles();

		for ( const auto & triangle : triangles )
		{
			hash.add((static_cast< uint64_t >(triangle.vertexIndex(0)) << 32U) | triangle.vertexIndex(1));
			hash.add(triangle.vertexIndex(2));
		}

		for ( const auto & [first, count] : shape.groups() )
		{
			hash.add((static_cast< uint64_t >(first) << 32U) | count);
		}

		return Key{
			.hash = hash.value(),
			.vertexCount = vertices.size(),
			.triangleCount = triangles.size(),
			.flags = flags
		};
	}
}
