/*
 * src/Libs/VertexFactory/ShapeVertex.hpp
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

#pragma once

/* STL inclusions. */
#include <type_traits>

/* Local inclusions. */
#include "Libs/Math/Vector.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The shape vertex class. This structure contains the position, the tangent, the normal and the texture coordinates.
	 * @tparam vertex_data_t The type of floating point number. Default float.
	 */
	template< typename vertex_data_t = float >
	requires (std::is_floating_point_v< vertex_data_t >)
	class ShapeVertex final
	{
		public:

			/**
			 * @brief Constructs a default vertex.
			 */
			ShapeVertex () noexcept = default;

			/**
			 * @brief Constructs a vertex from attributes.
			 * @param position A reference to a vector for the position in space.
			 * @param normal A reference to a vector for the normal. Default Z+.
			 * @param textureCoordinates A reference to a vector for texture coordinates. Default [0, 0, 0].
			 */
			explicit
			ShapeVertex (const Math::Vector< 3, vertex_data_t > & position, const Math::Vector< 3, vertex_data_t > & normal = Math::Vector< 3, vertex_data_t >::positiveZ(), const Math::Vector< 3, vertex_data_t > & textureCoordinates = {}) noexcept
				: m_position(position), m_normal(normal), m_textureCoordinates(textureCoordinates)
			{

			}

			/**
			 * @brief Sets the vertex position.
			 * @param position A reference to a vector for the position in space.
			 * @return void
			 */
			void
			setPosition (const Math::Vector< 3, vertex_data_t > & position) noexcept
			{
				m_position = position;
			}

			/**
			 * @brief Sets the vertex position.
			 * @param position A reference to a vector for the position in space.
			 * @return void
			 */
			void
			setPosition (const Math::Vector< 4, vertex_data_t > & position) noexcept
			{
				m_position[Math::X] = position[Math::X];
				m_position[Math::Y] = position[Math::Y];
				m_position[Math::Z] = position[Math::Z];
			}

			/**
			 * @brief Sets the vertex tangent.
			 * @param tangent A reference to a 3D vector for the tangent.
			 * @return void
			 */
			void
			setTangent (const Math::Vector< 3, vertex_data_t > & tangent) noexcept
			{
				m_tangent = tangent;
			}

			/**
			 * @brief Sets the vertex tangent.
			 * @param tangent A reference to a 4D vector for the tangent.
			 * @return void
			 */
			void
			setTangent (const Math::Vector< 4, vertex_data_t > & tangent) noexcept
			{
				m_tangent[Math::X] = tangent[Math::X];
				m_tangent[Math::Y] = tangent[Math::Y];
				m_tangent[Math::Z] = tangent[Math::Z];
			}

			/**
			 * @brief Sets the vertex normal.
			 * @param normal A reference to a 3D vector for the normal.
			 * @return void
			 */
			void
			setNormal (const Math::Vector< 3, vertex_data_t > & normal) noexcept
			{
				m_normal = normal;
			}

			/**
			 * @brief Sets the vertex normal.
			 * @param normal A reference to a 4D vector for the normal.
			 * @return void
			 */
			void
			setNormal (const Math::Vector< 4, vertex_data_t > & normal) noexcept
			{
				m_normal[Math::X] = normal[Math::X];
				m_normal[Math::Y] = normal[Math::Y];
				m_normal[Math::Z] = normal[Math::Z];
			}

			/**
			 * @brief Sets the vertex texture coordinates.
			 * @param textureCoordinates A reference to a 2D vector for the coordinates.
			 * @return void
			 */
			void
			setTextureCoordinates (const Math::Vector< 2, vertex_data_t > & textureCoordinates) noexcept
			{
				m_textureCoordinates[Math::X] = textureCoordinates[Math::X];
				m_textureCoordinates[Math::Y] = textureCoordinates[Math::Y];
			}

			/**
			 * @brief Sets the vertex texture coordinates.
			 * @param textureCoordinates A reference to a 3D vector for the coordinates.
			 * @return void
			 */
			void
			setTextureCoordinates (const Math::Vector< 3, vertex_data_t > & textureCoordinates) noexcept
			{
				m_textureCoordinates = textureCoordinates;
			}

			/**
			 * @brief Sets the vertex texture coordinates.
			 * @param textureCoordinates A reference to a 4D vector for the coordinates.
			 * @return void
			 */
			void
			setTextureCoordinates (const Math::Vector< 4, vertex_data_t > & textureCoordinates) noexcept
			{
				m_textureCoordinates[Math::X] = textureCoordinates[Math::X];
				m_textureCoordinates[Math::Y] = textureCoordinates[Math::Y];
				m_textureCoordinates[Math::Z] = textureCoordinates[Math::Z];
			}

			/**
			 * @brief Sets vertex influences.
			 * @param influenceA Index to first influence.
			 * @param influenceB Index to second influence. Default none.
			 * @param influenceC Index to third influence. Default none.
			 * @param influenceD Index to fourth influence. Default none.
			 * @return void
			 */
			void
			setInfluences (int32_t influenceA, int32_t influenceB = -1, int32_t influenceC = -1, int32_t influenceD = -1) noexcept
			{
				m_influences[Math::X] = influenceA;
				m_influences[Math::Y] = influenceB;
				m_influences[Math::Z] = influenceC;
				m_influences[Math::W] = influenceD;
			}

			/**
			 * @brief Sets vertex influence weights.
			 * @param weightA Index to first influence weight.
			 * @param weightB Index to second influence weight. Default none.
			 * @param weightC Index to third influence weight. Default none.
			 * @param weightD Index to fourth influence weight. Default none.
			 * @return void
			 */
			void
			setWeights (vertex_data_t weightA, vertex_data_t weightB = 0, vertex_data_t weightC = 0, vertex_data_t weightD = 0) noexcept
			{
				m_weights[Math::X] = weightA;
				m_weights[Math::Y] = weightB;
				m_weights[Math::Z] = weightC;
				m_weights[Math::W] = weightD;
			}

			/**
			 * @brief Returns the vertex position vector.
			 * @return const Math::Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 3, vertex_data_t > &
			position () const noexcept
			{
				return m_position;
			}

			/**
			 * @brief Returns the vertex tangent vector.
			 * @return const Math::Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 3, vertex_data_t > &
			tangent () const noexcept
			{
				return m_tangent;
			}

			/**
			 * @brief Returns the vertex normal vector.
			 * @return const Math::Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 3, vertex_data_t > &
			normal () const noexcept
			{
				return m_normal;
			}

			/**
			 * @brief Returns the vertex bi-normal vector.
			 * @note Produced by the cross product of normal and tangent vectors.
			 * @return const Math::Vector< 3, data_t > &
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			biNormal () const noexcept
			{
				return Math::Vector< 3, vertex_data_t >::crossProduct(m_normal, m_tangent);
			}

			/**
			 * @brief Returns the vertex texture coordinates vector.
			 * @return const Math::Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 3, vertex_data_t > &
			textureCoordinates () const noexcept
			{
				return m_textureCoordinates;
			}

			/**
			 * @brief Returns the vertex influences.
			 * @return const Math::Vector< 4, int32_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 4, int32_t > &
			influences () const noexcept
			{
				return m_influences;
			}

			/**
			 * @brief Returns the vertex weights.
			 * @return const Math::Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Math::Vector< 4, vertex_data_t > &
			weights () const noexcept
			{
				return m_weights;
			}

			/**
			 * @brief Flip the vertex
			 * @return void
			 */
			void
			flip () noexcept
			{
				m_tangent.inverse();
				m_normal.inverse();
			}

			/**
			 * @brief Flips the Y-Axis of attributes.
			 * @return void
			 */
			void
			flipYAxis () noexcept
			{
				m_position[Math::Y] = -m_position[Math::Y];
				m_tangent[Math::Y] = -m_tangent[Math::Y];
				m_normal[Math::Y] = -m_normal[Math::Y];
				m_textureCoordinates[Math::Y] = -m_textureCoordinates[Math::Y];
			}

		private:

			Math::Vector< 3, vertex_data_t > m_position{0, 0, 0};
			Math::Vector< 3, vertex_data_t > m_tangent{1, 0, 0};
			Math::Vector< 3, vertex_data_t > m_normal{0, 0, 1};
			Math::Vector< 3, vertex_data_t > m_textureCoordinates{0, 0, 0};
			Math::Vector< 4, int32_t > m_influences{-1, -1, -1, -1};
			Math::Vector< 4, vertex_data_t > m_weights{0, 0, 0, 0};
	};
}
