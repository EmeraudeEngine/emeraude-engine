/*
 * src/Saphir/Declaration/InputAttribute.hpp
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

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief Gets the shader variable type for a vertex attribute.
	 * @param vertexAttribute The predefined vertex attribute type.
	 * @return Key
	 */
	[[nodiscard]]
	Key getVertexAttributeVariableType (Graphics::VertexAttributeType vertexAttribute) noexcept;

	/**
	 * @brief Gets the shader variable name for a vertex attribute.
	 * @param vertexAttribute The predefined vertex attribute type.
	 * @return Key
	 */
	[[nodiscard]]
	Key getVertexAttributeVariableName (Graphics::VertexAttributeType vertexAttribute) noexcept;

	/**
	 * @brief Defines a vertex attribute in a shader.
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 */
	class InputAttribute final : public Interface
	{
		public:

			/**
			 * @brief Constructs an input attribute from VertexAttributeType.
			 * @param vertexAttribute The predefined vertex attribute type.
			 */
			explicit InputAttribute (Graphics::VertexAttributeType vertexAttribute) noexcept;

			/**
			 * @brief Constructs an input attribute from VertexAttributeType with a forced shader variable type.
			 * @param vertexAttribute The predefined vertex attribute type.
			 * @param type A C-string to set the GLSL type of the attribute.
			 */
			InputAttribute (Graphics::VertexAttributeType vertexAttribute, Key type) noexcept;

			/**
			 * @brief Constructs an input attribute.
			 * @param location An integer to specify the location to use in the vertex shader.
			 * @param type A C-string to set the GLSL type of the attribute. Use one of Keys::GLSL::* keyword.
			 * @param name A C-string to set the name of the attribute.
			 * @param arraySize An integer to set the size of the array. Default 0.
			 */
			InputAttribute (uint32_t location, Key type, Key name, uint32_t arraySize = 0) noexcept;

			/** @copydoc EmEn::Saphir::Declaration::Interface::isValid() */
			[[nodiscard]]
			bool isValid () const noexcept override;

			/** @copydoc EmEn::Saphir::Declaration::Interface::name() */
			[[nodiscard]]
			Key
			name () const noexcept override
			{
				return m_name;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::bytes() */
			[[nodiscard]]
			uint32_t bytes () const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::sourceCode() */
			[[nodiscard]]
			std::string sourceCode () const noexcept override;

			/**
			 * @brief Returns the variable location in the shader.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			location () const noexcept
			{
				return m_location;
			}

			/**
			 * @brief Returns the variable type.
			 * @return Key
			 */
			[[nodiscard]]
			Key
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the array size.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			arraySize () const noexcept
			{
				return m_arraySize;
			}

			/**
			 * @brief Returns whether the attribute is part of the model matrices VBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool isModelMatricesVBOAttribute () const noexcept;

		private:

			uint32_t m_location;
			Key m_type;
			Key m_name;
			uint32_t m_arraySize{0};
	};
}
