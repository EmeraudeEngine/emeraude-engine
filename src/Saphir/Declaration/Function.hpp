/*
 * src/Saphir/Declaration/Function.hpp
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
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"
#include "../CodeGeneratorInterface.hpp"

/* Local inclusions for usage. */
#include "Libs/BlobTrait.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief The Function class.
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 * @extends EmEn::Saphir::CodeGeneratorInterface
	 */
	class Function final : public Interface, public CodeGeneratorInterface
	{
		public:

			/**
			 * @brief Construct a GLSL function.
			 * @param name A C-string to set the name of the function.
			 * @param returnType A C-string to set the type of the uniform. Use one of Keys::GLSL::* keyword.
			 */
			explicit Function (Key name, Key returnType = Keys::GLSL::Void) noexcept;

			/** @copydoc EmEn::Saphir::Declaration::Interface::isValid() */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				return m_name != nullptr;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::name() */
			[[nodiscard]]
			Key
			name () const noexcept override
			{
				return m_name;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::bytes() */
			[[nodiscard]]
			uint32_t
			bytes () const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::sourceCode() */
			[[nodiscard]]
			std::string sourceCode () const noexcept override;

			/**
			 * @brief returnType
			 * @return Key
			 */
			[[nodiscard]]
			Key
			returnType () const noexcept
			{
				return m_returnType;
			}

			/**
			 * @brief parameters
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Adds an input parameter to the function signature.
			 * @param type The type of the parameter.
			 * @param name The name of the parameter.
			 * @param isConstant Adds "const" before "in" qualifier. Default false.
			 */
			void addInParameter (Key type, Key name, bool isConstant = false) noexcept;

			/**
			 * @brief Adds an output parameter to the function signature.
			 * @param type The type of the parameter.
			 * @param name The name of the parameter.
			 */
			void addOutParameter (Key type, Key name) noexcept;

			/**
			 * @brief Adds a reference parameter to the function signature.
			 * @param type The type of the parameter.
			 * @param name The name of the parameter.
			 */
			void addInOutParameter (Key type, Key name) noexcept;

			/**
			 * @brief Generates the call code.
			 * @param parameters
			 * @return std::string
			 */
			[[nodiscard]]
			std::string callCode (const std::string & parameters) const noexcept;

			/**
			 * @brief Generates the call code.
			 * @param parameters
			 * @return std::string
			 */
			[[nodiscard]]
			std::string callCode (const std::vector< std::string > & parameters) const noexcept;

			/**
			 * @brief Generates the call code.
			 * @param parameters
			 * @return std::string
			 */
			[[nodiscard]]
			std::string callCode (const Libs::BlobTrait & parameters) const noexcept;

		private:

			Key m_name;
			Key m_returnType;
			std::vector< std::string > m_parameters{};
	};
}
