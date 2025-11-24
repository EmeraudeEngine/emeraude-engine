/*
 * src/Console/Output.hpp
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
#include <string>
#include <sstream>
#include <vector>

/* Local inclusions for usages. */
#include "../CoreTypes.hpp" // Global types

namespace EmEn::Console
{
	/**
	 * @brief Console output class to return command execution info.
	 */
	class Output final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConsoleOutput"};

			/**
			 * @brief Constructs a console return message.
			 * @param severity The message severity.
			 * @param message A string [std::move].
			 */
			Output (Severity severity, std::string message) noexcept
				: m_severity{severity},
				m_message{std::move(message)}
			{

			}

			/**
			 * @brief Constructs a console return message.
			 * @param severity The message severity.
			 * @param message A reference to a string stream.
			 */
			Output (Severity severity, const std::stringstream & message) noexcept
				: m_severity{severity},
				m_message{message.str()}
			{

			}

			/**
			 * @brief Returns the message severity.
			 * @return Severity
			 */
			[[nodiscard]]
			Severity
			severity () const noexcept
			{
				return m_severity;
			}

			/**
			 * @brief Returns the message.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			message () const noexcept
			{
				return m_message;
			}

		private:

			Severity m_severity;
			std::string m_message;
	};

	using Outputs = std::vector< Output >;
}
