/*
 * src/Console/MCP/Protocol.hpp
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
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions for usages. */
#include "Console/Argument.hpp"
#include "Console/Command.hpp"
#include "Console/Output.hpp"

namespace EmEn::Console
{
	class Controller;
}

/*
 * The Model Context Protocol semantics of the engine's MCP server — everything that is not a socket:
 * tool names, tool definitions projected from the typed console commands, JSON arguments → console
 * arguments, console outputs → a tool result, JSON-RPC envelopes. The transport is Server.hpp.
 *
 * Specification: https://modelcontextprotocol.io/specification/2026-07-28 (current, stateless) and the
 * handshake era it stays compatible with (2025-11-25, 2025-06-18, 2025-03-26: `initialize`). The server
 * is DUAL-ERA (owner decision 2026-09-27): Claude Code's v1 client runtime only speaks the handshake era.
 */
namespace EmEn::Console::MCP
{
	/** @brief The current, stateless revision. */
	static constexpr auto ModernVersion{"2026-07-28"};

	/** @brief The handshake-era revisions also served, newest first (the first is answered to an unknown one). */
	static constexpr std::array< const char *, 3 > LegacyVersions{"2025-11-25", "2025-06-18", "2025-03-26"};

	/** @brief The server identity announced in `serverInfo`. */
	static constexpr auto ServerName{"emeraude-engine"};

	/**
	 * @brief The longest tool name exposed. Claude Code calls a tool `mcp__<server>__<tool>` and that name
	 * must fit 64 characters: 64 − `mcp__emeraude__` (15) = 49.
	 */
	static constexpr size_t MaxToolNameLength{49};

	/** @brief The long edge of the reduced image a capture tool returns, in pixels. */
	static constexpr uint32_t ReducedImageMaxEdge{1568};

	/* JSON-RPC and MCP error codes. */
	static constexpr int ParseErrorCode{-32700};
	static constexpr int InvalidRequestCode{-32600};
	static constexpr int MethodNotFoundCode{-32601};
	static constexpr int InvalidParamsCode{-32602};
	static constexpr int InternalErrorCode{-32603};
	static constexpr int HeaderMismatchCode{-32020};
	static constexpr int UnsupportedProtocolVersionCode{-32022};

	/* The `_meta` keys of the modern revision. */
	static constexpr auto MetaProtocolVersionKey{"io.modelcontextprotocol/protocolVersion"};
	static constexpr auto MetaServerInfoKey{"io.modelcontextprotocol/serverInfo"};
	static constexpr auto MetaSubscriptionIdKey{"io.modelcontextprotocol/subscriptionId"};

	/**
	 * @brief One exposed tool: a typed console command under its MCP name.
	 * @note The command pointer is only valid during the main-thread pass that built the list.
	 */
	class Tool final
	{
		public:

			/**
			 * @brief Constructs a tool.
			 * @param name The MCP tool name [std::move].
			 * @param path The console path of the command [std::move].
			 * @param command The command (not owned, main-thread pass only).
			 */
			Tool (std::string name, std::string path, const Command * command) noexcept
				: m_name{std::move(name)},
				m_path{std::move(path)},
				m_command{command}
			{

			}

			/**
			 * @brief Returns the MCP tool name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns the console path of the command.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			path () const noexcept
			{
				return m_path;
			}

			/**
			 * @brief Returns the command.
			 * @return const Command *
			 */
			[[nodiscard]]
			const Command *
			command () const noexcept
			{
				return m_command;
			}

		private:

			std::string m_name;
			std::string m_path;
			const Command * m_command;
	};

	/**
	 * @brief Returns the MCP tool name of a console path: `Core.` dropped, each `Service` suffix dropped,
	 * dots turned into `_` (`Core.SceneManagerService.PostProcess.select` → `SceneManager_PostProcess_select`).
	 * @param consolePath The dotted console path.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string toolName (const std::string & consolePath) noexcept;

	/**
	 * @brief Returns whether a tool name is exposable: 1 to MaxToolNameLength characters of `A-Za-z0-9_-`.
	 * @param name The tool name.
	 * @return bool
	 */
	[[nodiscard]]
	bool isValidToolName (const std::string & name) noexcept;

	/**
	 * @brief Walks the console tree and returns every command as a tool, one per command (aliases merged).
	 * @note Main thread only. A name that is invalid or collides is left out and described in rejections.
	 * @param controller The console controller.
	 * @param rejections Receives one line per command left out.
	 * @return std::vector< Tool >
	 */
	[[nodiscard]]
	std::vector< Tool > collectTools (const Controller & controller, std::vector< std::string > & rejections) noexcept;

	/**
	 * @brief Returns the MCP definition of a tool (name, title, description, inputSchema, annotations).
	 * @param tool The tool.
	 * @return Json::Value
	 */
	[[nodiscard]]
	Json::Value toolDefinition (const Tool & tool) noexcept;

	/**
	 * @brief Converts the named JSON arguments of a tool call to the positional console arguments.
	 * @note An omitted optional argument before a supplied one becomes an Undefined hole, which the typed
	 * binding reads as "not supplied". Type and range checks are left to the binding, which names the
	 * parameter; this only rejects what cannot become an argument at all.
	 * @param signature The command signature.
	 * @param arguments The `arguments` object of the call (null or absent = none).
	 * @param output Receives the positional arguments.
	 * @param error Receives the reason of a refusal.
	 * @return bool
	 */
	[[nodiscard]]
	bool jsonToArguments (const CommandSignature & signature, const Json::Value & arguments, Arguments & output, std::string & error) noexcept;

	/**
	 * @brief Builds the result of a tool call from the command's outputs (reads and reduces image files).
	 * @param succeeded Whether the command succeeded (`isError` is its negation).
	 * @param outputs The command outputs.
	 * @param modern Whether the call used the modern revision.
	 * @return Json::Value
	 */
	[[nodiscard]]
	Json::Value callResult (bool succeeded, const Outputs & outputs, bool modern) noexcept;

	/**
	 * @brief Reads an image file and encodes it as a PNG whose long edge is at most maxEdge (area filter).
	 * @param filePath The image file.
	 * @param maxEdge The longest edge allowed, in pixels.
	 * @param png Receives the PNG bytes.
	 * @param error Receives the reason of a failure.
	 * @return bool
	 */
	[[nodiscard]]
	bool reducedPNG (const std::filesystem::path & filePath, uint32_t maxEdge, std::vector< std::byte > & png, std::string & error) noexcept;

	/**
	 * @brief Returns the protocol versions this server supports, modern first.
	 * @return Json::Value
	 */
	[[nodiscard]]
	Json::Value supportedVersions () noexcept;

	/**
	 * @brief Returns whether a version is one of the handshake-era versions served.
	 * @param version The version.
	 * @return bool
	 */
	[[nodiscard]]
	bool isLegacyVersion (const std::string & version) noexcept;

	/**
	 * @brief Returns the instructions announced to the model (discover, initialize).
	 * @return const char *
	 */
	[[nodiscard]]
	const char * instructions () noexcept;

	/**
	 * @brief Returns a JSON-RPC success envelope. A modern result gets `resultType` and the server identity.
	 * @param id The request id.
	 * @param result The result object [std::move].
	 * @param modern Whether the request used the modern revision.
	 * @return Json::Value
	 */
	[[nodiscard]]
	Json::Value makeResult (const Json::Value & id, Json::Value result, bool modern) noexcept;

	/**
	 * @brief Returns a JSON-RPC error envelope.
	 * @param id The request id (null when unknown).
	 * @param code The error code.
	 * @param message The error message.
	 * @param data Optional error data.
	 * @return Json::Value
	 */
	[[nodiscard]]
	Json::Value makeError (const Json::Value & id, int code, const std::string & message, const Json::Value & data = Json::Value{Json::nullValue}) noexcept;

	/**
	 * @brief Serializes a JSON value compactly, on one line, with full floating point precision.
	 * @note Not FastJSON::stringify(), which writes 5 significant digits: a node position read back
	 * through a tool must not lose its decimals.
	 * @param value The value.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string serialize (const Json::Value & value) noexcept;

	/**
	 * @brief Returns an object's member, or a null value when the holder is not an object or lacks it.
	 * @note ⚠️ The ONLY way to read client JSON here. jsoncpp is built without exceptions: reading a member
	 * of an array, or `asString()` on an object, calls abort() — one malformed request would kill the engine.
	 * @param holder The value that should be an object.
	 * @param key The member name.
	 * @return const Json::Value &
	 */
	[[nodiscard]]
	const Json::Value & member (const Json::Value & holder, const char * key) noexcept;

	/**
	 * @brief Returns an object's string member, or an empty string (see member()).
	 * @param holder The value that should be an object.
	 * @param key The member name.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string stringMember (const Json::Value & holder, const char * key) noexcept;

	/**
	 * @brief Returns an object's boolean member, or a fallback (see member()).
	 * @param holder The value that should be an object.
	 * @param key The member name.
	 * @param fallback The value when absent or not a boolean.
	 * @return bool
	 */
	[[nodiscard]]
	bool boolMember (const Json::Value & holder, const char * key, bool fallback) noexcept;

	/**
	 * @brief Decodes an HTTP header value that may use the MCP Base64 sentinel (`=?base64?…?=`).
	 * @param value The header value.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string decodeHeaderValue (const std::string & value) noexcept;
}
