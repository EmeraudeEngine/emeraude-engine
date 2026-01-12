/*
 * src/Saphir/CodeGeneratorInterface.hpp
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
#include <string>
#include <vector>

namespace EmEn::Saphir
{
	/**
	 * @brief The code generator interface class.
	 */
	class CodeGeneratorInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			CodeGeneratorInterface (const CodeGeneratorInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			CodeGeneratorInterface (CodeGeneratorInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return CodeGeneratorInterface &
			 */
			CodeGeneratorInterface & operator= (const CodeGeneratorInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return CodeGeneratorInterface &
			 */
			CodeGeneratorInterface & operator= (CodeGeneratorInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the code generator interface.
			 */
			virtual ~CodeGeneratorInterface () = default;

			/**
			 * @brief Appends a line of instruction to the main() shader method at top-level.
			 * @param code A string of code execution.
			 * @return void
			 */
			virtual
			void
			addTopInstruction (const std::string & code) noexcept
			{
				m_topInstructions.emplace_back(code);
			}

			/**
			 * @brief Appends a line of instruction to the main() shader method.
			 * @param code A reference to a string.
			 * @return void
			 */
			virtual
			void
			addInstruction (const std::string & code) noexcept
			{
				m_instructions.emplace_back(code);
			}

			/**
			 * @brief Appends a line of instruction at the bottom of main() shader method.
			 * @param code A string of code execution.
			 * @return void
			 */
			virtual
			void
			addOutputInstruction (const std::string & code) noexcept
			{
				m_outputInstructions.emplace_back(code);
			}

			/**
			 * @brief Appends a line of comment in the flow of th main code.
			 * @param comment A reference to a string.
			 * @param depth A number for indentation level. default 1.
			 * @return void
			 */
			virtual void addComment (const std::string & comment, size_t depth = 1) noexcept;

		protected:

			/**
			 * @brief Constructs a code generator interface.
			 */
			CodeGeneratorInterface () noexcept = default;

			/**
			 * @brief Returns the code into a string.
		 	 * @param prependTopInstructions A reference to a string. Default none.
		 	 * @param prependOutputInstructions A reference to a string. Default none.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getCode (const std::string & prependTopInstructions = {}, const std::string & prependOutputInstructions = {}) const noexcept;

		private:

			/* main() code. */
			std::vector< std::string > m_topInstructions{};
			std::vector< std::string > m_instructions{};
			std::vector< std::string > m_outputInstructions{};
	};
}
