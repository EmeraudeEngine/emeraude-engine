/*
 * src/Console/CommandResult.hpp
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
#include <cstdint>
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "Output.hpp"

namespace EmEn::Console
{
	/**
	 * @brief What a typed console command returns: success or failure, and its outputs.
	 * @note Built with the named constructors, then completed with add():
	 * `return CommandResult::json(document).add(Output{Severity::Info, "3 scenes."});`.
	 * A failure is only produced by error(); adding an Error output to a success does not flip it.
	 */
	class EMEN_API CommandResult final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConsoleCommandResult"};

			/**
			 * @brief A success with a confirmation message.
			 * @param message The message [std::move].
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult success (std::string message) noexcept;

			/**
			 * @brief A success whose message is information (a status, a value read back).
			 * @param message The message [std::move].
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult info (std::string message) noexcept;

			/**
			 * @brief A success that carries a warning (done, but not as asked).
			 * @param message The message [std::move].
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult warning (std::string message) noexcept;

			/**
			 * @brief A failure, with the reason a caller can act on.
			 * @param message The reason [std::move].
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult error (std::string message) noexcept;

			/**
			 * @brief A success whose payload is a JSON document.
			 * @param document The serialized JSON document [std::move].
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult json (std::string document) noexcept;

			/**
			 * @brief A success whose payload is binary (an image, a file).
			 * @param bytes The payload [std::move].
			 * @param mimeType The MIME type, e.g. "image/png" [std::move].
			 * @param description A one-line description, printed by text channels [std::move].
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult binary (std::vector< uint8_t > bytes, std::string mimeType, std::string description) noexcept;

			/**
			 * @brief A result made of a prepared output list, for a report of several lines with mixed
			 * severities (a status table, a per-slot listing).
			 * @param outputs The outputs, in display order [std::move].
			 * @param succeeded Whether the command succeeded.
			 * @return CommandResult
			 */
			[[nodiscard]]
			static CommandResult fromOutputs (Outputs outputs, bool succeeded) noexcept;

			/**
			 * @brief Appends an output (lvalue chain).
			 * @param output The output [std::move].
			 * @return CommandResult &
			 */
			CommandResult & add (Output output) & noexcept;

			/**
			 * @brief Appends an output (rvalue chain, for `return CommandResult::…().add(…);`).
			 * @param output The output [std::move].
			 * @return CommandResult &&
			 */
			CommandResult && add (Output output) && noexcept;

			/**
			 * @brief Returns whether the command succeeded.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			succeeded () const noexcept
			{
				return m_succeeded;
			}

			/**
			 * @brief Returns the outputs.
			 * @return const Outputs &
			 */
			[[nodiscard]]
			const Outputs &
			outputs () const noexcept
			{
				return m_outputs;
			}

			/**
			 * @brief Moves the outputs to the end of a console output list.
			 * @param outputs The destination list.
			 * @return bool Whether the command succeeded.
			 */
			bool moveTo (Outputs & outputs) noexcept;

		private:

			/**
			 * @brief Constructs a result from its first output.
			 * @param output The first output [std::move].
			 * @param succeeded Whether the command succeeded.
			 */
			CommandResult (Output output, bool succeeded) noexcept;

			/**
			 * @brief Constructs a result from a prepared output list.
			 * @param outputs The outputs [std::move].
			 * @param succeeded Whether the command succeeded.
			 */
			CommandResult (Outputs outputs, bool succeeded) noexcept;

			Outputs m_outputs;
			bool m_succeeded;
	};
}
