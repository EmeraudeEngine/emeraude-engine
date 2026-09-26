/*
 * src/Console/RemoteProtocol.hpp
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
#include <string>

/* Local inclusions for usages. */
#include "Output.hpp"

/*
 * The wire format of the remote console (TCP, `Core/Console/RemoteListenerPort`), owner decision
 * 2026-09-27: a request is one command line; EVERY request gets exactly ONE response, which is ONE
 * JSON object on ONE line (every newline inside a message is escaped by the JSON writer):
 *
 *   {"ok":true,"outputs":[{"kind":"text","message":"Window resized to 800x600.","severity":"Success"}]}
 *
 * - `ok`: whether the command succeeded (a failed command still answers, with `ok` false).
 * - `outputs`: in display order, possibly empty. Each has `severity` (Debug/Success/Info/Warning/
 *   Error/Fatal), `kind` (`text`, `json` — the message IS a JSON document —, or `binary`) and
 *   `message`; a `binary` output adds `mimeType` and `data` (standard Base64).
 * - The welcome banner sent on connection is a response too, with a top-level `protocol` version.
 *
 * Before this format the server wrote raw text and never said where an answer ended, so clients
 * guessed it from silence — and a command with no output sent nothing at all.
 */
namespace EmEn::Console::RemoteProtocol
{
	/** @brief The version announced by the welcome banner. Bump it on any incompatible change. */
	static constexpr uint32_t Version{1};

	/**
	 * @brief Serializes the response to one request, as one line WITHOUT its terminating newline.
	 * @param succeeded Whether the command succeeded.
	 * @param outputs The outputs, in display order (possibly empty).
	 * @return std::string
	 */
	[[nodiscard]]
	std::string serializeResponse (bool succeeded, const Outputs & outputs) noexcept;

	/**
	 * @brief Serializes a failure the transport itself reports (line too long, queue full, too many
	 * clients), in the same format as a command response.
	 * @param message The reason.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string serializeError (const std::string & message) noexcept;

	/**
	 * @brief Serializes the welcome banner sent on connection: a successful response carrying the
	 * top-level `protocol` version.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string serializeWelcome () noexcept;
}
