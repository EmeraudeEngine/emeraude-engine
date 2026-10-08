/*
 * src/Console/Controller.cpp
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

#include "Controller.hpp"

/* STL inclusions. */
#include <charconv>
#include <algorithm>
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "RemoteProtocol.hpp"
#include "String.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Console
{
	using namespace Base;

	Controller * Controller::s_instance{nullptr};
	std::atomic< uint64_t > Controller::s_consoleTreeRevision{0};

	bool
	Controller::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();

		/* NOTE: all three keys are read (and written on first run) even when the listener stays
		 * off, so an operator finds them in settings.json without reading the source. */
		/* NOTE: like the console keys, the MCP keys are written on first run even when the server stays off. */
		const auto MCPEnabled = settings.getOrSetDefault< bool >(MCPEnabledKey, DefaultMCPEnabled);
		const auto MCPAddress = settings.getOrSetDefault< std::string >(MCPAddressKey, DefaultMCPAddress);
		const auto MCPPort = settings.getOrSetDefault< uint16_t >(MCPPortKey, DefaultMCPPort);
		auto MCPBearerToken = settings.getOrSetDefault< std::string >(MCPBearerTokenKey, DefaultMCPBearerToken);

		if ( MCPEnabled )
		{
			auto server = std::make_unique< MCP::Server >(MCPAddress, MCPPort, std::move(MCPBearerToken));

			if ( server->isRunning() )
			{
				m_MCPServer = std::move(server);
			}
			else
			{
				TraceWarning{ClassId} << "The MCP server failed to start (see above); MCP clients will be refused.";
			}
		}
		else
		{
			TraceInfo{ClassId} << "MCP server disabled (" << MCPEnabledKey << " = false).";
		}

		const auto remoteListenerEnabled = settings.getOrSetDefault< bool >(ConsoleEnableRemoteListenerKey, DefaultConsoleEnableRemoteListener);
		m_remoteListenerAddress = settings.getOrSetDefault< std::string >(ConsoleRemoteListenerAddressKey, DefaultConsoleRemoteListenerAddress);
		m_remoteListenerPort = settings.getOrSetDefault< uint16_t >(ConsoleRemoteListenerPortKey, DefaultConsoleRemoteListenerPort);

		if ( !remoteListenerEnabled )
		{
			TraceInfo{ClassId} << "Remote console disabled (" << ConsoleEnableRemoteListenerKey << " = false). Set it to true, or press Shift+F10 in the application, to drive it over TCP.";

			return true;
		}

		if ( !this->startRemoteListener(m_remoteListenerAddress, m_remoteListenerPort) )
		{
			TraceWarning{ClassId} << "Remote listener failed to initialize, remote console will be unavailable.";
		}

		return true;
	}

	bool
	Controller::startRemoteListener (const std::string & address, uint16_t port) noexcept
	{
		/* ⚠️ Bind the new listener BEFORE dropping the working one: a refused bind (privileged
		 * port, address in use) used to leave the console closed with no way back in on a
		 * headless host. */
		auto candidate = std::make_unique< RemoteListener >(address, port);

		if ( !candidate->isRunning() )
		{
			return false;
		}

		this->stopRemoteListener();

		m_remoteListener = std::move(candidate);

		return true;
	}

	void
	Controller::stopRemoteListener () noexcept
	{
		if ( m_remoteListener != nullptr )
		{
			TraceInfo{ClassId} << "Stopping the remote console on " << this->remoteListenerEndpoint() << ".";

			m_remoteListener.reset();
		}
	}

	std::string
	Controller::remoteListenerEndpoint () const noexcept
	{
		if ( !this->isRemoteListenerRunning() )
		{
			return {};
		}

		std::stringstream endpoint;
		endpoint << m_remoteListener->address() << ':' << m_remoteListener->port();

		return endpoint.str();
	}

	void
	Controller::requestRemoteListenerRestart (const std::string & address, uint16_t port) noexcept
	{
		m_pendingRemoteListenerRestart = std::make_pair(address, port);
	}

	bool
	Controller::parseEndpoint (const std::string & input, const std::string & defaultAddress, std::string & address, uint16_t & port) noexcept
	{
		const auto text = String::trim(input);

		if ( text.empty() )
		{
			return false;
		}

		std::string portText;

		/* "[::1]:7777" — bracketed IPv6 literal. */
		if ( text.front() == '[' )
		{
			const auto closing = text.find(']');

			if ( closing == std::string::npos || closing + 1 >= text.size() || text[closing + 1] != ':' )
			{
				return false;
			}

			address = text.substr(1, closing - 1);
			portText = text.substr(closing + 2);
		}
		else if ( const auto colon = text.rfind(':'); colon != std::string::npos )
		{
			/* "0.0.0.0:7777" — one colon only, otherwise it is a bare IPv6 literal without a port. */
			if ( text.find(':') != colon )
			{
				return false;
			}

			address = text.substr(0, colon);
			portText = text.substr(colon + 1);
		}
		else
		{
			/* "7777" — the port alone, on the configured address. */
			address = defaultAddress;
			portText = text;
		}

		if ( address.empty() || portText.empty() || portText.size() > 5 || !std::ranges::all_of(portText, [] (char character) {
			return character >= '0' && character <= '9';
		}) )
		{
			return false;
		}

		/* The address must be an IP literal: make_address() does not resolve names, so a hostname
		 * or a typo would silently fall back to loopback while the caller is told it succeeded. */
		asio::error_code addressError;

		static_cast< void >(asio::ip::make_address(address, addressError));

		if ( addressError )
		{
			return false;
		}

		/* NOTE: std::from_chars, not the throwing std::stoul (the text is digits only, at most five of them). */
		uint32_t value = 0;

		if ( const auto [end, error] = std::from_chars(portText.data(), portText.data() + portText.size(), value); error != std::errc{} || value < 1 || value > 65535 )
		{
			return false;
		}

		port = static_cast< uint16_t >(value);

		return true;
	}

	bool
	Controller::onTerminate () noexcept
	{
		/* The MCP server first: its pending requests reference the console tree cleared below. */
		m_MCPServer.reset();

		if ( m_remoteListener != nullptr )
		{
			m_remoteListener.reset();
		}

		m_consoleObjects.clear();

		return true;
	}

	bool
	Controller::add (ControllableTrait & controllable) noexcept
	{
		if ( m_consoleObjects.contains(controllable.identifier()) )
		{
			TraceError{ClassId} << "Console object named '" << controllable.identifier() << "' already exists !";

			return false;
		}

		const auto inserted = m_consoleObjects.emplace(controllable.identifier(), &controllable).second;

		if ( inserted )
		{
			Controller::markConsoleTreeChanged();
		}

		return inserted;
	}

	bool
	Controller::remove (const std::string & identifier) noexcept
	{
		const auto objectIt = m_consoleObjects.find(identifier);

		if ( objectIt == m_consoleObjects.cend() )
		{
			return false;
		}

		m_consoleObjects.erase(objectIt);

		Controller::markConsoleTreeChanged();

		return true;
	}

	bool
	Controller::remove (const ControllableTrait & pointer) noexcept
	{
		for ( auto it = m_consoleObjects.begin(); it != m_consoleObjects.end(); ++it )
		{
			if ( it->second == &pointer )
			{
				m_consoleObjects.erase(it);

				Controller::markConsoleTreeChanged();

				return true;
			}
		}

		return false;
	}

	void
	Controller::complete (std::string & input) const noexcept
	{
		if ( input.empty() )
		{
			return;
		}

		/* Checks form multiple commands. */
		const auto commandsList = String::explode(input, ';', false);

		/* We only check the last term. */
		Expression expression(*commandsList.crbegin());

		if ( expression.isValid() )
		{
			return;
		}

		auto identifier = expression.nextIdentifier();

		std::vector< std::string > terms;

		/* For each register object in the console. */
		loopOverObjectsName(m_consoleObjects, expression, identifier, terms);

		/* Shows the result */
		switch ( terms.size() )
		{
			case 1 :
				input += String::right(*terms.begin(), identifier);

			break;

			case 0 :

				Tracer::warning(ClassId, "No match found !");

			break;

			default:

				for ( auto & term : terms )
				{
					Tracer::info(ClassId, term);
				}

			break;
		}
	}

	void
	Controller::executeCommand (const std::string & fullCommand) noexcept
	{
		Outputs tempOutputs;

		this->executeCommand(fullCommand, tempOutputs);

		for ( const auto & output : tempOutputs )
		{
			if ( output.severity() == Severity::Error || output.severity() == Severity::Fatal )
			{
				Tracer::error(ClassId, output.message());
			}
			else if ( output.severity() == Severity::Warning )
			{
				Tracer::warning(ClassId, output.message());
			}
			else
			{
				Tracer::info(ClassId, output.message());
			}
		}
	}

	void
	Controller::poll () noexcept
	{
		if ( m_MCPServer != nullptr )
		{
			m_MCPServer->processPendingRequests(*this);
		}

		/* A restart asked by a console command is applied here, before draining: the command's
		 * response already left on the previous listener. */
		if ( m_pendingRemoteListenerRestart )
		{
			const auto [address, port] = *m_pendingRemoteListenerRestart;

			m_pendingRemoteListenerRestart.reset();

			if ( this->startRemoteListener(address, port) )
			{
				TraceSuccess{ClassId} << "Remote console restarted on " << this->remoteListenerEndpoint() << ".";
			}
			else
			{
				TraceError{ClassId} << "Remote console restart on " << address << ':' << port << " failed (port in use, privileged or address not local); staying on " << ( this->isRemoteListenerRunning() ? this->remoteListenerEndpoint() : std::string{"nothing — the console is closed"} ) << ".";
			}
		}

		if ( m_remoteListener != nullptr )
		{
			RemoteListener::PendingCommand pending;

			while ( m_remoteListener->popCommand(pending) )
			{
				Outputs outputs;
				bool succeeded = false;

				/* JSON input: route to the registered JSON handler. */
				if ( !pending.command.empty() && pending.command[0] == '{' && m_jsonHandler )
				{
					succeeded = m_jsonHandler(pending.command, outputs);
				}
				else
				{
					succeeded = this->executeCommand(pending.command, outputs);
				}

				/* ⚠️ ALWAYS answer, even with no output: a client waits for exactly one line per request,
				 * and a silent command used to leave it hanging until its timeout. */
				if ( pending.client != nullptr )
				{
					m_remoteListener->respond(pending.client, RemoteProtocol::serializeResponse(succeeded, outputs));
				}
			}
		}
	}

	bool
	Controller::executeCommand (const std::string & command, Outputs & outputs) noexcept
	{
		TraceInfo{ClassId} << "Executing command: " << command;

		/* Checks for built-in command first ! */
		if ( command.find('.') == std::string::npos )
		{
			return this->executeBuiltInCommand(command, outputs);
		}

		/* Parse the command expression. */
		Expression expression(command);

		if ( !expression.isValid() )
		{
			outputs.emplace_back(Severity::Error, "Invalid command !");

			return false;
		}

		/* Gets the first identifier.
		 * NOTE: It is useless to check if it's empty here. */
		const auto identifier = expression.nextIdentifier();

		/* Retrieve the base object. */
		const auto objectIt = m_consoleObjects.find(identifier);

		if ( objectIt == m_consoleObjects.cend() )
		{
			outputs.emplace_back(Severity::Error, std::stringstream{} << "Console object '" << identifier << "' not found !");

			return false;
		}

		/* Send the command to this object. */
		return objectIt->second->execute(expression, outputs);
	}

	bool
	Controller::loopOverObjectsName (const std::map< std::string, ControllableTrait * > & objects, Expression & expression, std::string & identifier, std::vector< std::string > & suggestions) noexcept
	{
		for ( const auto & [name, controllable] : objects )
		{
			/* Perfect match. */
			if ( identifier == name )
			{
				/* As we check a new depth, we don't want old suggestions */
				suggestions.clear();

				controllable->complete(expression, identifier, suggestions);

				return true;
			}

			if ( name.starts_with(identifier) )
			{
				suggestions.emplace_back(name);
			}
		}

		return false;
	}

	void
	Controller::dumpControllable (const ControllableTrait & controllable, std::stringstream & out, const std::string & path) noexcept
	{
		const auto & commands = controllable.commands();

		if ( !commands.empty() )
		{
			for ( const auto & [name, command] : commands )
			{
				out << "  " << path << "." << name << "()\n"
					<< "	  " << command.help() << "\n";

				const auto & signature = command.signature();

				for ( const auto & parameter : signature.parameters() )
				{
					out << "	    " << parameter.name() << " (" << to_cstring(parameter.type());

					if ( parameter.arity() == ParameterArity::Variadic )
					{
						out << ", zero or more";
					}
					else if ( parameter.isOmittable() )
					{
						out << ", optional";
					}

					out << ") " << parameter.description() << "\n";
				}

				if ( const auto hints = signature.hintsText(); !hints.empty() )
				{
					out << "	    [" << hints << "]\n";
				}
			}
		}

		for ( const auto & [subName, subPtr] : controllable.subObjects() )
		{
			if ( subPtr != nullptr )
			{
				Controller::dumpControllable(*subPtr, out, String::concatenate(path, ".", subName));
			}
		}
	}

	namespace
	{
		/**
		 * @brief Converts an argument (a parameter's default value) to its JSON value.
		 * @param argument The argument.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		argumentToJson (const Argument & argument) noexcept
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
		 * @brief Appends the description of every command of a sub-tree to a JSON array.
		 * @param controllable The root of the sub-tree.
		 * @param path The dotted path of that root.
		 * @param commands The JSON array.
		 */
		void
		describeControllable (const ControllableTrait & controllable, const std::string & path, Json::Value & commands) noexcept
		{
			for ( const auto & [name, command] : controllable.commands() )
			{
				Json::Value entry{Json::objectValue};
				entry["path"] = String::concatenate(path, ".", name);
				entry["help"] = command.help();

				const auto & signature = command.signature();


				Json::Value parameters{Json::arrayValue};

				for ( const auto & parameter : signature.parameters() )
				{
					Json::Value item{Json::objectValue};
					item["name"] = parameter.name();
					item["type"] = to_cstring(parameter.type());
					item["description"] = parameter.description();

					switch ( parameter.arity() )
					{
						case ParameterArity::Required :
							item["arity"] = parameter.defaultValue().has_value() ? "optional" : "required";
							break;

						case ParameterArity::Optional :
							item["arity"] = "optional";
							break;

						case ParameterArity::Variadic :
							item["arity"] = "variadic";
							break;
					}

					if ( const auto & defaultValue = parameter.defaultValue(); defaultValue.has_value() )
					{
						item["default"] = argumentToJson(*defaultValue);
					}

					parameters.append(std::move(item));
				}

				entry["parameters"] = std::move(parameters);

				Json::Value hints{Json::arrayValue};

				if ( hasHint(signature.hints(), CommandHint::ReadOnly) )
				{
					hints.append("readOnly");
				}

				if ( hasHint(signature.hints(), CommandHint::Destructive) )
				{
					hints.append("destructive");
				}

				if ( hasHint(signature.hints(), CommandHint::Idempotent) )
				{
					hints.append("idempotent");
				}

				entry["hints"] = std::move(hints);

				commands.append(std::move(entry));
			}

			for ( const auto & [subName, subPtr] : controllable.subObjects() )
			{
				if ( subPtr != nullptr )
				{
					describeControllable(*subPtr, String::concatenate(path, ".", subName), commands);
				}
			}
		}
	}

	std::string
	Controller::describeCommands () const noexcept
	{
		Json::Value commands{Json::arrayValue};

		for ( const auto & [name, controllable] : m_consoleObjects )
		{
			if ( controllable != nullptr )
			{
				describeControllable(*controllable, name, commands);
			}
		}

		return FastJSON::stringify(commands);
	}

	bool
	Controller::executeBuiltInCommand (const std::string & command, Outputs & outputs) noexcept
	{
		if ( command == "help" || command == "help()" || command == "lsfunc()" )
		{
			std::stringstream message;

			message <<
				"Remote Console — available commands\n"
				"\n"
				"Top-level built-ins:\n"
				"  help, help(), lsfunc()  Show this recursive command reference\n"
				"  listObjects, lsobj()	List top-level controllable objects\n"
				"  exit, quit, shutdown	Graceful shutdown (saves settings)\n"
				"  hardExit				Immediate shutdown (no save)\n"
				"  describeCommands()      Every command as JSON (parameters, types, hints)\n"
				"\n"
				"Per-object built-ins (any depth):\n"
				"  <path>.lsfunc()		 List commands bound at that level\n"
				"  <path>.lsobj()		  List sub-objects at that level\n"
				"  <path>.help()		   Recursive dump from that level\n"
				"\n"
				"Registered objects and commands:\n";

			for ( const auto & [name, controllable] : m_consoleObjects )
			{
				if ( controllable != nullptr )
				{
					Controller::dumpControllable(*controllable, message, name);
				}
			}

			outputs.emplace_back(Severity::Info, message);

			return true;
		}

		if ( command == "listObjects" || command == "lsobj()" )
		{
			std::stringstream message;

			for ( const auto & objectName : std::ranges::views::keys(m_consoleObjects) )
			{
				message << "'" << objectName << "'" "\n";
			}

			outputs.emplace_back(Severity::Info, message);

			return true;
		}

		if ( command == "describeCommands" || command == "describeCommands()" )
		{
			outputs.emplace_back(Output::json(this->describeCommands()));

			return true;
		}

		if ( command == "exit" || command == "quit" || command == "shutdown" )
		{
			outputs.emplace_back(Severity::Success, "Goodbye !");

			this->notify(Exit);

			return true;
		}

		if ( command == "hardExit" )
		{
			outputs.emplace_back(Severity::Warning, "Wild exit command invoked !");

			this->notify(HardExit);

			return true;
		}

		outputs.emplace_back(Severity::Error, "Invalid command !");

		return false;
	}
}
