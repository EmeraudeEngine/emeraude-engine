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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <string>
#include <vector>

namespace EmEn::Saphir
{
	/**
	 * @brief The code generator interface class.
	 * @note Accumulates the GLSL statements that make up a shader stage's `main()` body (or a
	 * user-defined function body, see Declaration::Function) as three ordered sections: top
	 * (preparation), main and output. getCode() concatenates them in that fixed order, regardless
	 * of the order in which the add*() methods were called across sections. AbstractShader and
	 * Declaration::Function inherit from this interface to gain that accumulation behavior; the
	 * Code helper (see Code.hpp) is the usual way callers append to a section without holding a
	 * reference to the underlying vectors, which stay private.
	 */
	class EMEN_API CodeGeneratorInterface
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
			 * @note The comment is always inserted into the main-instructions section, interleaved
			 * with addInstruction() calls in call order. There is no equivalent for the top or
			 * output sections.
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
			 * @note Assembles, in fixed order, the top section (prependTopInstructions followed by
			 * every line accumulated through addTopInstruction()), then the main section (lines
			 * accumulated through addInstruction()/addComment()), then the output section
			 * (prependOutputInstructions followed by every line accumulated through
			 * addOutputInstruction()). A section whose prepend string is empty AND whose
			 * accumulated vector is empty is omitted entirely, including its header comment.
			 * Callers (e.g. AbstractShader::generateSourceCode()) use the prepend arguments to
			 * inject code ahead of what was accumulated via the add*Instruction() methods, without
			 * having to go through them.
		 	 * @param prependTopInstructions A reference to a string. Default none.
		 	 * @param prependOutputInstructions A reference to a string. Default none.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getCode (const std::string & prependTopInstructions = {}, const std::string & prependOutputInstructions = {}) const noexcept;

		private:

			/* main() code. */
			std::vector< std::string > m_topInstructions;
			std::vector< std::string > m_instructions;
			std::vector< std::string > m_outputInstructions;
	};
}
