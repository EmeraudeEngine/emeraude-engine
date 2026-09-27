/*
 * src/Console/RemoteProtocol.cpp
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

#include "RemoteProtocol.hpp"

/* Local inclusions. */
#include "FastJSON.hpp"
#include "String.hpp"

namespace EmEn::Console::RemoteProtocol
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns the wire name of an output kind.
		 * @param kind The output kind.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		kindName (OutputKind kind) noexcept
		{
			switch ( kind )
			{
				case OutputKind::Json :
					return "json";

				case OutputKind::Binary :
					return "binary";

				case OutputKind::Image :
					return "image";

				case OutputKind::Text :
					break;
			}

			return "text";
		}

		/**
		 * @brief Builds the response object, the root every serializer starts from.
		 * @param succeeded Whether the command succeeded.
		 * @param outputs The outputs.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		makeResponse (bool succeeded, const Outputs & outputs) noexcept
		{
			Json::Value response{Json::objectValue};
			response["ok"] = succeeded;

			Json::Value list{Json::arrayValue};

			for ( const auto & output : outputs )
			{
				Json::Value entry{Json::objectValue};
				entry["severity"] = to_cstring(output.severity());
				entry["kind"] = kindName(output.kind());
				entry["message"] = output.message();

				if ( output.kind() == OutputKind::Binary )
				{
					entry["mimeType"] = output.mimeType();
					entry["data"] = String::encodeBase64(std::string{output.bytes().begin(), output.bytes().end()});
				}
				else if ( output.kind() == OutputKind::Image )
				{
					/* NOTE: the path only — a text channel never pays for reading the image. */
					entry["mimeType"] = output.mimeType();
					entry["path"] = output.filePath().string();
				}

				list.append(std::move(entry));
			}

			response["outputs"] = std::move(list);

			return response;
		}
	}

	std::string
	serializeResponse (bool succeeded, const Outputs & outputs) noexcept
	{
		/* NOTE: FastJSON::stringify() writes with no indentation, so every newline of a message is
		 * escaped and the response stays on one line — the framing depends on it. */
		return FastJSON::stringify(makeResponse(succeeded, outputs));
	}

	std::string
	serializeError (const std::string & message) noexcept
	{
		return serializeResponse(false, {Output{Severity::Error, message}});
	}

	std::string
	serializeWelcome () noexcept
	{
		auto response = makeResponse(true, {Output{Severity::Info, "Welcome to Emeraude-Engine AI Remote Console. One command per line; one JSON response per line."}});
		response["protocol"] = Version;

		return FastJSON::stringify(response);
	}
}
