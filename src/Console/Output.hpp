/*
 * src/Console/Output.hpp
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
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "CoreTypes.hpp" // Global types

namespace EmEn::Console
{
	/** @brief What an output carries beyond its text. */
	enum class OutputKind : uint8_t
	{
		/** @brief Plain text for a human or an AI. */
		Text,
		/** @brief The message IS a JSON document (a machine client can parse it as structured data). */
		Json,
		/** @brief A binary payload (bytes + MIME type); the message is a one-line description of it. */
		Binary,
		/** @brief An image FILE on disk (path + MIME type); the message is a one-line description of it.
		 * The bytes are only read by a channel that shows the image (MCP, reduced there): a text channel
		 * prints the message and the path, and a screenshot costs nothing more than before. */
		Image
	};

	/**
	 * @brief Console output class to return command execution info.
	 * @note Every kind keeps a readable message, so a text channel (TCP console, terminal) prints
	 * any output as it is; a machine channel can also use the JSON document or the binary payload.
	 */
	class EMEN_API Output final
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
				: m_message{std::move(message)},
				m_severity{severity}
			{

			}

			/**
			 * @brief Constructs a console return message.
			 * @param severity The message severity.
			 * @param message A reference to a string stream.
			 */
			Output (Severity severity, const std::stringstream & message) noexcept
				: m_message{message.str()},
				m_severity{severity}
			{

			}

			/**
			 * @brief Constructs an output whose message is a JSON document.
			 * @param document A serialized JSON document [std::move].
			 * @return Output
			 */
			[[nodiscard]]
			static
			Output
			json (std::string document) noexcept
			{
				Output output{Severity::Info, std::move(document)};
				output.m_kind = OutputKind::Json;

				return output;
			}

			/**
			 * @brief Constructs an output carrying a binary payload.
			 * @param bytes The payload [std::move].
			 * @param mimeType The MIME type of the payload, e.g. "image/png" [std::move].
			 * @param description A one-line description, printed by text channels [std::move].
			 * @return Output
			 */
			[[nodiscard]]
			static
			Output
			binary (std::vector< uint8_t > bytes, std::string mimeType, std::string description) noexcept
			{
				Output output{Severity::Info, std::move(description)};
				output.m_kind = OutputKind::Binary;
				output.m_mimeType = std::move(mimeType);
				output.m_bytes = std::move(bytes);

				return output;
			}

			/**
			 * @brief Constructs an output referencing an image file on disk.
			 * @param filePath The image file [std::move].
			 * @param mimeType The MIME type of the file, e.g. "image/png" [std::move].
			 * @param description A one-line description, printed by text channels [std::move].
			 * @return Output
			 */
			[[nodiscard]]
			static
			Output
			image (std::filesystem::path filePath, std::string mimeType, std::string description) noexcept
			{
				Output output{Severity::Success, std::move(description)};
				output.m_kind = OutputKind::Image;
				output.m_mimeType = std::move(mimeType);
				output.m_filePath = std::move(filePath);

				return output;
			}

			/**
			 * @brief Returns the image file (empty unless kind() is Image).
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filePath () const noexcept
			{
				return m_filePath;
			}

			/**
			 * @brief Returns what the output carries beyond its text.
			 * @return OutputKind
			 */
			[[nodiscard]]
			OutputKind
			kind () const noexcept
			{
				return m_kind;
			}

			/**
			 * @brief Returns the MIME type of the binary payload or of the image file (empty for Text and Json).
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			mimeType () const noexcept
			{
				return m_mimeType;
			}

			/**
			 * @brief Returns the binary payload (empty unless kind() is Binary).
			 * @return const std::vector< uint8_t > &
			 */
			[[nodiscard]]
			const std::vector< uint8_t > &
			bytes () const noexcept
			{
				return m_bytes;
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

			std::string m_message;
			std::string m_mimeType;
			std::vector< uint8_t > m_bytes;
			std::filesystem::path m_filePath;
			Severity m_severity;
			OutputKind m_kind{OutputKind::Text};
	};

	using Outputs = std::vector< Output >;
}
