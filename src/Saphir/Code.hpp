/*
 * src/Saphir/Code.hpp
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
#include <cstdint>
#include <sstream>

/* Local inclusions for usages. */
#include "CodeGeneratorInterface.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn::Saphir
{
	/** @brief Code location type, i.e. which instruction buffer of the generator a Code instance flushes into. */
	enum class Location : std::uint8_t
	{
		Top,	///< Flushed via CodeGeneratorInterface::addTopInstruction(), before the other main() instructions.
		Main,	///< Flushed via CodeGeneratorInterface::addInstruction(), in the ordinary main() body.
		Output	///< Flushed via CodeGeneratorInterface::addOutputInstruction(), at the bottom of main() (e.g. the final output/return statement).
	};

	/** @brief Line ending enumeration. */
	enum class Line : std::uint8_t
	{
		End,	///< Ends the current line and starts a new one, re-indented to the Code instance's depth.
		Blank	///< Ends the current line, inserts one blank line, then starts a new one re-indented to the Code instance's depth.
	};

	/**
	 * @brief The code instruction class.
	 * @note A '\n' character is automatically put at the end of each generated code.
	 * Use Line::End when writing multiple line of code to automatically follow the indentation.
	 * @warning This type is designed to be used as an unnamed temporary, built and streamed into on a single
	 * full expression (e.g. `Code{generator, Location::Output} << "..." << Line::End << "...";`). The
	 * accumulated text is only handed to the generator when the temporary is destructed at the end of that
	 * expression; keeping an instance alive across statements delays the emission of its content accordingly.
	 */
	class Code final
	{
		public:

			/**
			 * @brief Constructs a code.
			 * @param generator A reference to the shader generator that will receive the accumulated instruction once this instance is destructed.
			 * @param type The code location type. Default main instruction.
			 * @param depth The indentation depth. Default 1.
			 */
			explicit
			Code (CodeGeneratorInterface & generator, Location type = Location::Main, size_t depth = 1) noexcept
				: m_generator{generator},
				m_type{type},
				m_indent(depth, '\t')
			{
				m_code << m_indent;
			}

			/**
			 * @brief Copy constructor.
			 * @note Deleted: a copy would flush the same accumulated instruction twice (once per destructed instance).
			 * @param copy A reference to the instance that would have been copied.
			 */
			Code (const Code & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @note Deleted: this type is only meant to be used as a single unnamed temporary (see class note); moving it out is not a supported use case.
			 * @param copy A reference to the instance that would have been moved.
			 */
			Code (Code && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @note Deleted: a copy would flush the same accumulated instruction twice (once per destructed instance).
			 * @param copy A reference to the instance that would have been copied.
			 */
			Code & operator= (const Code & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @note Deleted: this type is only meant to be used as a single unnamed temporary (see class note); moving it out is not a supported use case.
			 * @param copy A reference to the instance that would have been moved.
			 */
			Code & operator= (Code && copy) noexcept = delete;

			/**
			 * @brief Destructs the code.
			 * @note This will generate the code inside the generator.
			 */
			~Code ()
			{
				m_code << '\n';

				switch ( m_type )
				{
					case Location::Top:
						m_generator.addTopInstruction(m_code.str());
						break;

					case Location::Main:
						m_generator.addInstruction(m_code.str());
						break;

					case Location::Output:
						m_generator.addOutputInstruction(m_code.str());
						break;
				}
			}

			/**
			 * @brief Adds a line control token.
			 * @param value The token.
			 * @return Code &
			 */
			Code &
			operator<< (const Line & value) noexcept
			{
				switch ( value )
				{
					/* NOTE: End of the line char + new indent. */
					case Line::End :
						m_code << '\n' << m_indent;
						break;

						/* NOTE: Double end of the line chars + new indent. */
					case Line::Blank :
						m_code << "\n\n" << m_indent;
						break;
				}

				return *this;
			}

			/**
			 * @brief Adds a vector 2 to the code content as a GLSL `vec2(...)` constructor literal.
			 * @param value A reference to the vector.
			 * @return Code &
			 */
			Code &
			operator<< (const Base::Math::Vector< 2, float > & value) noexcept
			{
				m_code << "vec2(" << value.x() << ", " << value.y() << ")";

				return *this;
			}

			/**
			 * @brief Adds a vector 3 to the code content as a GLSL `vec3(...)` constructor literal.
			 * @param value A reference to the vector.
			 * @return Code &
			 */
			Code &
			operator<< (const Base::Math::Vector< 3, float > & value) noexcept
			{
				m_code << "vec3(" << value.x() << ", " << value.y() << ", " << value.z() << ")";

				return *this;
			}

			/**
			 * @brief Adds a vector 4 to the code content as a GLSL `vec4(...)` constructor literal.
			 * @param value A reference to the vector.
			 * @return Code &
			 */
			Code &
			operator<< (const Base::Math::Vector< 4, float > & value) noexcept
			{
				m_code << "vec4(" << value.x() << ", " << value.y() << ", " << value.z() << ", " << value.w() << ")";

				return *this;
			}

			/**
			 * @brief Adds a color to the code content as a GLSL `vec4(r, g, b, a)` constructor literal.
			 * @param value A reference to the color.
			 * @return Code &
			 */
			Code &
			operator<< (const Base::PixelFactory::Color< float > & value) noexcept
			{
				m_code << "vec4(" << value.red() << ", " << value.green() << ", " << value.blue() << ", " << value.alpha() << ")";

				return *this;
			}

			/**
			 * @brief Adds a generic type to the code content.
			 * @tparam data_t The type of the data.
			 * @param value A reference to the data.
			 * @return Code &
			 */
			template< typename data_t >
			Code &
			operator<< (const data_t & value) noexcept
			{
				m_code << value;

				return *this;
			}

		private:

			CodeGeneratorInterface & m_generator;
			Location m_type;
			std::string m_indent;
			std::stringstream m_code;
	};
}
