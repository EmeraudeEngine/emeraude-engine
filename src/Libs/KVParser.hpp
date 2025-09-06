/*
 * src/Libs/KVParser.hpp
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
#include <map>
#include <string>
#include <fstream>

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Libs
{
	class KVVariable final
	{
		public:

			KVVariable () noexcept = default;

			explicit
			KVVariable (std::string value) noexcept
				: m_value(std::move(value)),
				m_undefined(false)
			{

			}

			explicit
			KVVariable (bool value) noexcept
				: m_value(value ? "1" : "0"),
				m_undefined(false)
			{

			}

			explicit
			KVVariable (int value) noexcept
				: m_value(std::to_string(value)),
				m_undefined(false)
			{

			}

			explicit
			KVVariable (float value) noexcept
				: m_value(std::to_string(value)),
				m_undefined(false)
			{

			}

			explicit
			KVVariable (double value) noexcept
				: m_value(std::to_string(value)),
				m_undefined(false)
			{

			}

			[[nodiscard]]
			bool
			isUndefined () const noexcept
			{
				return m_undefined;
			}

			[[nodiscard]]
			bool
			asBoolean () const noexcept
			{
				return m_value == "1" ||
					m_value == "true" ||
					m_value == "True" ||
					m_value == "TRUE" ||
					m_value == "on" ||
					m_value == "On" ||
					m_value == "ON";
			}

			[[nodiscard]]
			int
			asInteger () const noexcept
			{
				return String::toNumber< int >(m_value);
			}

			[[nodiscard]]
			float
			asFloat () const noexcept
			{
				return String::toNumber< float >(m_value);
			}

			[[nodiscard]]
			double
			asDouble () const noexcept
			{
				return String::toNumber< double >(m_value);
			}

			[[nodiscard]]
			const std::string &
			asString () const noexcept
			{
				return m_value;
			}

		private:

			std::string m_value;
			bool m_undefined{true};
	};

	class KVSection final
	{
		public:

			KVSection () noexcept = default;

			void
			addVariable (const std::string & key, const KVVariable & variable) noexcept
			{
				m_variables[key] = variable;
			}

			[[nodiscard]]
			const std::map< std::string, KVVariable > &
			variables () const noexcept
			{
				return m_variables;
			}

			[[nodiscard]]
			KVVariable
			variable (const std::string & key) const noexcept
			{
				if ( const auto variableIt = m_variables.find(key); variableIt != m_variables.cend() )
				{
					return variableIt->second;
				}

				return {};
			}

			void
			write (std::ofstream & file) const noexcept
			{
				for ( const auto & [name, variable] : m_variables )
				{
					file << name << " = " << variable.asString() << "\n";
				}
			}

		private:

			std::map< std::string, KVVariable > m_variables;
	};

	class KVParser final
	{
		public:

			KVParser () noexcept = default;

			[[nodiscard]]
			std::map< std::string, KVSection > &
			sections () noexcept
			{
				return m_sections;
			}

			[[nodiscard]]
			const std::map< std::string, KVSection > &
			sections () const noexcept
			{
				return m_sections;
			}

			[[nodiscard]]
			KVSection & section (const std::string & label) noexcept;

			[[nodiscard]]
			bool read (const std::string & filepath) noexcept;

			[[nodiscard]]
			bool write (const std::string & filepath) const noexcept;

		private:

			enum class LineType
			{
				None = 0,
				Headers = 1,
				Comment = 2,
				SectionTitle = 3,
				Definition = 4
			};

			static std::string parseSectionTitle (const std::string & line) noexcept;

			static LineType getLineType (const std::string & line) noexcept;

			std::map< std::string, KVSection > m_sections;
	};
}
