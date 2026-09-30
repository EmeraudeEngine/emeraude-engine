/*
 * src/Console/MCP/Protocol.cpp
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

#include "Protocol.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <span>

/* Local inclusions. */
#include "emeraude_config.hpp"
#include "Console/Controller.hpp"
#include "FastJSON.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Processor.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "String.hpp"

namespace EmEn::Console::MCP
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Walks a console sub-tree and appends its typed commands as tools.
		 * @param controllable The root of the sub-tree.
		 * @param path The dotted path of that root.
		 * @param tools The tool list.
		 * @return void
		 */
		void
		appendTools (const ControllableTrait & controllable, const std::string & path, std::vector< Tool > & tools) noexcept
		{
			for ( const auto & [name, command] : controllable.commands() )
			{
				/* One tool per command: an alias (a name other than the first of its list) is skipped. */
				if ( command.primaryName() != name )
				{
					continue;
				}

				const auto consolePath = String::concatenate(path, ".", name);

				tools.emplace_back(toolName(consolePath), consolePath, &command);
			}

			for ( const auto & [subName, subPtr] : controllable.subObjects() )
			{
				if ( subPtr != nullptr )
				{
					appendTools(*subPtr, String::concatenate(path, ".", subName), tools);
				}
			}
		}

		/**
		 * @brief Returns the JSON schema of one scalar parameter type.
		 * @param type The parameter type.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		scalarSchema (ParameterType type) noexcept
		{
			Json::Value schema{Json::objectValue};

			switch ( type )
			{
				case ParameterType::Boolean :
					schema["type"] = "boolean";
					break;

				case ParameterType::Integer :
					schema["type"] = "integer";
					schema["minimum"] = std::numeric_limits< int32_t >::min();
					schema["maximum"] = std::numeric_limits< int32_t >::max();
					break;

				case ParameterType::Float :
					schema["type"] = "number";
					break;

				case ParameterType::String :
					schema["type"] = "string";
					break;

				case ParameterType::Any :
				{
					Json::Value types{Json::arrayValue};
					types.append("boolean");
					types.append("integer");
					types.append("number");
					types.append("string");
					schema["type"] = std::move(types);
				}
					break;
			}

			return schema;
		}

		/**
		 * @brief Returns the JSON value of an argument (a parameter's default value).
		 * @param argument The argument.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		argumentValue (const Argument & argument) noexcept
		{
			if ( const auto * boolean = std::get_if< bool >(&argument.value()) )
			{
				return {*boolean};
			}

			if ( const auto * integer = std::get_if< int32_t >(&argument.value()) )
			{
				return {*integer};
			}

			if ( const auto * number = std::get_if< float >(&argument.value()) )
			{
				return {static_cast< double >(*number)};
			}

			if ( const auto * text = std::get_if< std::string >(&argument.value()) )
			{
				return {*text};
			}

			return {Json::nullValue};
		}

		/**
		 * @brief Converts one scalar JSON value to a console argument.
		 * @param value The JSON value.
		 * @param name The parameter name, for the error.
		 * @param argument Receives the argument.
		 * @param error Receives the reason of a refusal.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		scalarArgument (const Json::Value & value, const std::string & name, Argument & argument, std::string & error) noexcept
		{
			if ( value.isBool() )
			{
				argument = Argument{value.asBool()};

				return true;
			}

			if ( value.isIntegral() )
			{
				if ( !value.isInt() )
				{
					error = "Argument '" + name + "' is outside the 32-bit integer range.";

					return false;
				}

				argument = Argument{static_cast< int32_t >(value.asInt())};

				return true;
			}

			if ( value.isDouble() )
			{
				const auto number = value.asDouble();

				if ( !std::isfinite(number) )
				{
					error = "Argument '" + name + "' is not a finite number.";

					return false;
				}

				/* NOTE: converting a double beyond the float range is undefined behaviour ([conv.double]). */
				if ( std::abs(number) > static_cast< double >(std::numeric_limits< float >::max()) )
				{
					error = "Argument '" + name + "' is outside the 32-bit floating point range.";

					return false;
				}

				argument = Argument{static_cast< float >(number)};

				return true;
			}

			if ( value.isString() )
			{
				argument = Argument{value.asString()};

				return true;
			}

			error = "Argument '" + name + "' must be a boolean, a number or a string.";

			return false;
		}

		/**
		 * @brief Returns the text a severity prefixes to an output message in a tool result.
		 * @param severity The severity.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		severityPrefix (Severity severity) noexcept
		{
			switch ( severity )
			{
				case Severity::Warning :
					return "[Warning] ";

				case Severity::Error :
					return "[Error] ";

				case Severity::Fatal :
					return "[Fatal] ";

				default :
					return "";
			}
		}

		/**
		 * @brief Returns a text content item.
		 * @param text The text.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		textContent (const std::string & text) noexcept
		{
			Json::Value content{Json::objectValue};
			content["type"] = "text";
			content["text"] = text;

			return content;
		}

		/**
		 * @brief Returns an image content item.
		 * @param bytes The encoded image (read in place: no copy).
		 * @param mimeType The MIME type.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		imageContent (std::span< const std::byte > bytes, const std::string & mimeType) noexcept
		{
			Json::Value content{Json::objectValue};
			content["type"] = "image";
			content["data"] = String::encodeBase64(bytes);
			content["mimeType"] = mimeType;

			return content;
		}
	}

	std::string
	toolName (const std::string & consolePath) noexcept
	{
		auto segments = String::explode(consolePath, '.', false);

		if ( segments.size() > 1 && segments.front() == "Core" )
		{
			segments.erase(segments.begin());
		}

		constexpr std::string_view ServiceSuffix{"Service"};

		std::string name;

		for ( size_t index = 0; index < segments.size(); ++index )
		{
			auto segment = segments[index];

			/* NOTE: the command name itself (last segment) is never shortened. */
			if ( index + 1 < segments.size() && segment.size() > ServiceSuffix.size() && segment.ends_with(ServiceSuffix) )
			{
				segment.resize(segment.size() - ServiceSuffix.size());
			}

			if ( !name.empty() )
			{
				name += '_';
			}

			name += segment;
		}

		return name;
	}

	bool
	isValidToolName (const std::string & name) noexcept
	{
		if ( name.empty() || name.size() > MaxToolNameLength )
		{
			return false;
		}

		return std::ranges::all_of(name, [] (char character) {
			return ( character >= 'a' && character <= 'z' ) || ( character >= 'A' && character <= 'Z' ) || ( character >= '0' && character <= '9' ) || character == '_' || character == '-';
		});
	}

	std::vector< Tool >
	collectTools (const Controller & controller, std::vector< std::string > & rejections) noexcept
	{
		std::vector< Tool > candidates;

		for ( const auto & [name, controllable] : controller.objects() )
		{
			if ( controllable != nullptr )
			{
				appendTools(*controllable, name, candidates);
			}
		}

		/* A collision is refused on BOTH sides: exposing one of the two would make the name mean
		 * whichever command happened to be walked first. */
		std::map< std::string, size_t > occurrences;

		for ( const auto & tool : candidates )
		{
			++occurrences[tool.name()];
		}

		std::vector< Tool > tools;
		tools.reserve(candidates.size());

		for ( auto & tool : candidates )
		{
			if ( !isValidToolName(tool.name()) )
			{
				rejections.emplace_back("'" + tool.path() + "' → '" + tool.name() + "': not a valid tool name (1 to " + std::to_string(MaxToolNameLength) + " characters of A-Za-z0-9_-).");

				continue;
			}

			if ( occurrences[tool.name()] > 1 )
			{
				rejections.emplace_back("'" + tool.path() + "' → '" + tool.name() + "': the name collides with another command.");

				continue;
			}

			tools.emplace_back(std::move(tool));
		}

		/* A deterministic order, as the specification asks: it keeps the client's cache and prompt stable. */
		std::ranges::sort(tools, [] (const Tool & left, const Tool & right) {
			return left.name() < right.name();
		});

		return tools;
	}

	Json::Value
	toolDefinition (const Tool & tool) noexcept
	{
		const auto & command = *tool.command();
		const auto & signature = command.signature();

		Json::Value definition{Json::objectValue};
		definition["name"] = tool.name();
		definition["title"] = tool.path();
		definition["description"] = command.description();

		Json::Value schema{Json::objectValue};
		schema["type"] = "object";

		Json::Value properties{Json::objectValue};
		Json::Value required{Json::arrayValue};

		for ( const auto & parameter : signature.parameters() )
		{
			Json::Value property{Json::objectValue};

			if ( parameter.arity() == ParameterArity::Variadic )
			{
				property["type"] = "array";
				property["items"] = scalarSchema(parameter.type());
			}
			else
			{
				property = scalarSchema(parameter.type());
			}

			property["description"] = parameter.description();

			if ( const auto & defaultValue = parameter.defaultValue(); defaultValue.has_value() )
			{
				property["default"] = argumentValue(*defaultValue);
			}

			if ( !parameter.isOmittable() )
			{
				required.append(parameter.name());
			}

			properties[parameter.name()] = std::move(property);
		}

		schema["properties"] = std::move(properties);

		if ( !required.empty() )
		{
			schema["required"] = std::move(required);
		}

		schema["additionalProperties"] = false;
		definition["inputSchema"] = std::move(schema);

		Json::Value annotations{Json::objectValue};
		annotations["readOnlyHint"] = hasHint(signature.hints(), CommandHint::ReadOnly);
		annotations["destructiveHint"] = hasHint(signature.hints(), CommandHint::Destructive);
		annotations["idempotentHint"] = hasHint(signature.hints(), CommandHint::Idempotent);
		/* NOTE: every tool acts on the running engine only, never on an open world. */
		annotations["openWorldHint"] = false;
		definition["annotations"] = std::move(annotations);

		return definition;
	}

	bool
	jsonToArguments (const CommandSignature & signature, const Json::Value & arguments, Arguments & output, std::string & error) noexcept
	{
		output.clear();

		if ( !arguments.isNull() && !arguments.isObject() )
		{
			error = "'arguments' must be an object.";

			return false;
		}

		const auto & parameters = signature.parameters();

		/* Unknown names first: a typo must not silently fall back to a default value. */
		if ( arguments.isObject() )
		{
			for ( const auto & key : arguments.getMemberNames() )
			{
				const auto known = std::ranges::any_of(parameters, [&key] (const Parameter & parameter) {
					return parameter.name() == key;
				});

				if ( !known )
				{
					error = "Unknown argument '" + key + "'.";

					return false;
				}
			}
		}

		for ( const auto & parameter : parameters )
		{
			const auto & value = member(arguments, parameter.name().c_str());

			if ( value.isNull() )
			{
				/* NOTE: a hole, filled only if a later argument is supplied (trimmed below). */
				output.emplace_back();

				continue;
			}

			if ( parameter.arity() == ParameterArity::Variadic )
			{
				if ( !value.isArray() )
				{
					error = "Argument '" + parameter.name() + "' must be an array.";

					return false;
				}

				for ( const auto & element : value )
				{
					Argument argument;

					if ( !scalarArgument(element, parameter.name(), argument, error) )
					{
						return false;
					}

					output.emplace_back(std::move(argument));
				}

				return true;
			}

			Argument argument;

			if ( !scalarArgument(value, parameter.name(), argument, error) )
			{
				return false;
			}

			output.emplace_back(std::move(argument));
		}

		/* Trailing holes carry nothing: drop them, the binding sees omitted arguments. */
		while ( !output.empty() && output.back().type() == ArgumentType::Undefined )
		{
			output.pop_back();
		}

		return true;
	}

	bool
	reducedPNG (const std::filesystem::path & filePath, uint32_t maxEdge, std::vector< std::byte > & png, std::string & error) noexcept
	{
		PixelFactory::Pixmap< uint8_t, uint32_t > source;

		if ( !PixelFactory::FileIO::read(filePath, source) || !source.isValid() )
		{
			error = "unable to read the image file";

			return false;
		}

		const auto longEdge = std::max(source.width(), source.height());

		if ( longEdge <= maxEdge )
		{
			if ( !PixelFactory::StreamIO::write(source, PixelFactory::Pixmap< uint8_t, uint32_t >::Format::PNG, png) )
			{
				error = "unable to encode the image";

				return false;
			}

			return true;
		}

		const auto scale = static_cast< double >(maxEdge) / static_cast< double >(longEdge);
		const auto width = std::max(1U, static_cast< uint32_t >(std::lround(static_cast< double >(source.width()) * scale)));
		const auto height = std::max(1U, static_cast< uint32_t >(std::lround(static_cast< double >(source.height()) * scale)));

		PixelFactory::Pixmap< uint8_t, uint32_t > reduced;

		/* NOTE: the area-weighted box filter, not resize(): a bilinear resample of a 2880-wide frame down
		 * to 1568 would skip source pixels and alias every thin edge. */
		if ( !PixelFactory::Processor< uint8_t, uint32_t >::downsample(source, width, height, reduced) )
		{
			error = "unable to reduce the image";

			return false;
		}

		if ( !PixelFactory::StreamIO::write(reduced, PixelFactory::Pixmap< uint8_t, uint32_t >::Format::PNG, png) )
		{
			error = "unable to encode the image";

			return false;
		}

		return true;
	}

	Json::Value
	callResult (bool succeeded, const Outputs & outputs, bool modern) noexcept
	{
		Json::Value result{Json::objectValue};
		Json::Value content{Json::arrayValue};
		bool structuredSet = false;

		for ( const auto & output : outputs )
		{
			const auto text = std::string{severityPrefix(output.severity())} + output.message();

			switch ( output.kind() )
			{
				case OutputKind::Text :
					content.append(textContent(text));
					break;

				case OutputKind::Json :
				{
					/* The text stays verbatim; the first JSON output is also given as structured data
					 * (an object only in the handshake era, which allowed nothing else). */
					content.append(textContent(text));

					if ( !structuredSet )
					{
						if ( auto parsed = FastJSON::getRootFromString(output.message(), 64, true); parsed.has_value() && ( modern || parsed->isObject() ) )
						{
							result["structuredContent"] = std::move(*parsed);
							structuredSet = true;
						}
					}
				}
					break;

				case OutputKind::Binary :
					if ( output.mimeType().starts_with("image/") )
					{
						content.append(imageContent(std::as_bytes(std::span< const uint8_t >{output.bytes()}), output.mimeType()));
					}

					content.append(textContent(text));
					break;

				case OutputKind::Image :
				{
					std::vector< std::byte > png;
					std::string error;

					if ( reducedPNG(output.filePath(), ReducedImageMaxEdge, png, error) )
					{
						content.append(imageContent(png, "image/png"));
						content.append(textContent(String::concatenate(text, " (shown reduced to ", std::to_string(ReducedImageMaxEdge), " px on its long edge; the file is full resolution)")));
					}
					else
					{
						content.append(textContent(String::concatenate(text, " (the image could not be attached: ", error, ")")));
					}
				}
					break;
			}
		}

		result["content"] = std::move(content);
		result["isError"] = !succeeded;

		return result;
	}

	Json::Value
	supportedVersions () noexcept
	{
		Json::Value versions{Json::arrayValue};
		versions.append(ModernVersion);

		for ( const auto * version : LegacyVersions )
		{
			versions.append(version);
		}

		return versions;
	}

	bool
	isLegacyVersion (const std::string & version) noexcept
	{
		return std::ranges::any_of(LegacyVersions, [&version] (const char * legacy) {
			return version == legacy;
		});
	}

	const char *
	instructions () noexcept
	{
		return
			"Drives a RUNNING Emeraude engine: every tool is one of its console commands, executed on the "
			"engine's main thread between two frames. The world is Y-UP (a lookAt target with a higher Y looks "
			"up). Call SceneManager_targetActiveScene before the node and entity tools. Renderer_screenshot "
			"returns the next presented frame as a reduced image plus the path of the full-resolution PNG. "
			"A setting written with Settings_set is usually read at launch only. Tools with readOnlyHint "
			"change nothing.";
	}

	Json::Value
	makeResult (const Json::Value & id, Json::Value result, bool modern) noexcept
	{
		if ( modern )
		{
			result["resultType"] = "complete";

			Json::Value serverInfo{Json::objectValue};
			serverInfo["name"] = ServerName;
			serverInfo["version"] = VersionString;

			result["_meta"][MetaServerInfoKey] = std::move(serverInfo);
		}

		Json::Value envelope{Json::objectValue};
		envelope["jsonrpc"] = "2.0";
		envelope["id"] = id;
		envelope["result"] = std::move(result);

		return envelope;
	}

	Json::Value
	makeError (const Json::Value & id, int code, const std::string & message, const Json::Value & data) noexcept
	{
		Json::Value error{Json::objectValue};
		error["code"] = code;
		error["message"] = message;

		if ( !data.isNull() )
		{
			error["data"] = data;
		}

		Json::Value envelope{Json::objectValue};
		envelope["jsonrpc"] = "2.0";
		envelope["id"] = id;
		envelope["error"] = std::move(error);

		return envelope;
	}

	std::string
	serialize (const Json::Value & value) noexcept
	{
		Json::StreamWriterBuilder builder{};
		builder["commentStyle"] = "None";
		builder["indentation"] = "";
		builder["enableYAMLCompatibility"] = false;
		builder["dropNullPlaceholders"] = false;
		builder["useSpecialFloats"] = false;
		builder["precision"] = 17;
		builder["precisionType"] = "significant";
		builder["emitUTF8"] = true;

		return Json::writeString(builder, value);
	}

	const Json::Value &
	member (const Json::Value & holder, const char * key) noexcept
	{
		static const Json::Value Null{Json::nullValue};

		if ( !holder.isObject() )
		{
			return Null;
		}

		const auto * found = holder.find(key, key + std::char_traits< char >::length(key));

		return found != nullptr ? *found : Null;
	}

	std::string
	stringMember (const Json::Value & holder, const char * key) noexcept
	{
		const auto & value = member(holder, key);

		return value.isString() ? value.asString() : std::string{};
	}

	bool
	boolMember (const Json::Value & holder, const char * key, bool fallback) noexcept
	{
		const auto & value = member(holder, key);

		return value.isBool() ? value.asBool() : fallback;
	}

	std::string
	decodeHeaderValue (const std::string & value) noexcept
	{
		constexpr std::string_view Prefix{"=?base64?"};
		constexpr std::string_view Suffix{"?="};

		if ( value.size() >= Prefix.size() + Suffix.size() && value.starts_with(Prefix) && value.ends_with(Suffix) )
		{
			return String::decodeBase64(value.substr(Prefix.size(), value.size() - Prefix.size() - Suffix.size()));
		}

		return value;
	}
}
