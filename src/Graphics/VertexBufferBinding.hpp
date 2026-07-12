/*
 * src/Graphics/VertexBufferBinding.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "FlagTrait.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief Vertex buffer binding flags.
	 */
	enum EMEN_API VertexBufferBindingFlagBits : uint8_t
	{
		PerInstance = 1,
		RequestPrimitiveRestart = 2,
		IsPositionAbsolute = 4,
		IsDynamicVertexBuffer = 8
	};

	/**
	 * @brief Describes the data inside a vertex attribute binding. How many elements and the used primitive.
	 * @extends EmEn::Base::FlagTrait
	 */
	class EMEN_API VertexBufferBinding final : public Base::FlagTrait< uint32_t >
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VertexBufferBinding"};

			/**
			 * @brief Constructs a VBO binding format to describe how to use it.
			 * @param binding The binding index.
			 * @param elementCount The element count to make a vertex.
			 * @param topology The type of primitive.
			 * @param bufferFlags The buffer flags.
			 */
			VertexBufferBinding (uint32_t binding, size_t elementCount, Topology topology, uint32_t bufferFlags) noexcept
				: FlagTrait{bufferFlags},
				m_binding{binding},
				m_elementCount{elementCount},
				m_topology{topology}
			{

			}

			/**
			 * @brief Equality operator.
			 * @param operand A reference to another vertex buffer binding.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const VertexBufferBinding & operand) const noexcept
			{
				if ( this != &operand )
				{
					if ( m_binding != operand.m_binding )
					{
						return false;
					}

					if ( m_elementCount != operand.m_elementCount )
					{
						return false;
					}

					if ( m_topology != operand.m_topology )
					{
						return false;
					}

					if ( this->flags() != operand.flags() )
					{
						return false;
					}
				}

				return true;
			}

			/**
			 * @brief Different operator.
			 * @param operand A reference to another vertex buffer binding.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator!= (const VertexBufferBinding & operand) const noexcept
			{
				return !this->operator==(operand);
			}

			/**
			 * @brief Returns the elements count to make a vertex.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			elementCount () const noexcept
			{
				return m_elementCount;
			}

			/**
			 * @brief Returns the vertex size in bytes at a specific binding point.
			 * @note Same as VertexBufferFormat::elementCount() * sizeof(float).
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			size () const noexcept
			{
				return m_elementCount * sizeof(float);
			}

			/**
			 * @brief Returns the type of primitive.
			 * @return Topology
			 */
			[[nodiscard]]
			Topology
			topology () const noexcept
			{
				return m_topology;
			}

			/**
			 * @brief Returns whether the binding is per instance or per vertex.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			perInstance () const noexcept
			{
				return this->isFlagEnabled(VertexBufferBindingFlagBits::PerInstance);
			}

			/**
			 * @brief Returns whether the binding request to active primitive restart.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			requestPrimitiveRestart () const noexcept
			{
				return this->isFlagEnabled(VertexBufferBindingFlagBits::RequestPrimitiveRestart);
			}

			/**
			 * @brief Returns whether the position in vertex attributes are expressed in absolute coordinates.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPositionAbsolute () const noexcept
			{
				return this->isFlagEnabled(VertexBufferBindingFlagBits::IsPositionAbsolute);
			}

			/**
			 * @brief Returns whether the VBO is dynamic.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDynamicVertexBuffer () const noexcept
			{
				return this->isFlagEnabled(VertexBufferBindingFlagBits::IsDynamicVertexBuffer);
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const VertexBufferBinding & obj);

			uint32_t m_binding;
			size_t m_elementCount;
			Topology m_topology;
	};

	/**
	 * @brief Stringify the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	EMEN_API std::string to_string (const VertexBufferBinding & obj) noexcept;
}
