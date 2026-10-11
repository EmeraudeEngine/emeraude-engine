/*
 * src/PlatformSpecific/Helpers.linux.cpp
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

#include "Helpers.hpp"

/* STL inclusions. */
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

/* Local inclusions. */
#include "Thread.hpp"

namespace EmEn::PlatformSpecific
{
	bool
	checkProgram (const std::string & program) noexcept
	{
		const std::string command = "which " + program + " > /dev/null 2>&1";

		return system(command.c_str()) == 0;
	}

	bool
	hasZenity () noexcept
	{
		static const bool result = checkProgram("zenity");

		return result;
	}

	bool
	hasKdialog () noexcept
	{
		static const bool result = checkProgram("kdialog");

		return result;
	}

	bool
	isKdeDesktop () noexcept
	{
		if ( const char * desktop = std::getenv("XDG_CURRENT_DESKTOP"); desktop != nullptr )
		{
			const std::string desktopStr{desktop};

			return desktopStr.find("KDE") != std::string::npos;
		}

		return false;
	}

	std::string
	escapeShellArg (const std::string & arg) noexcept
	{
		std::string escaped;
		escaped.reserve(arg.size() + 2);
		escaped += '\'';

		for ( const char c : arg )
		{
			if ( c == '\'' )
			{
				/* End quote, add escaped quote, restart quote. */
				escaped += "'\\''";
			}
			else
			{
				escaped += c;
			}
		}

		escaped += '\'';

		return escaped;
	}

	std::string
	cleanLoaderEnvCommand (const std::string & command) noexcept
	{
		/* Strip the dynamic-loader variables for the spawned system tool so a bundled
		 * library (e.g. CEF's stripped libvulkan.so.1) cannot shadow the system one. */
		return "env -u LD_LIBRARY_PATH -u LD_PRELOAD " + command;
	}

	std::string
	executeCommand (const std::string & command, int & exitCode) noexcept
	{
		std::string output;

		FILE * pipe = popen(cleanLoaderEnvCommand(command).c_str(), "r");

		if ( pipe == nullptr )
		{
			exitCode = -1;

			return output;
		}

		std::array< char, 4096 > buffer{};

		while ( fgets(buffer.data(), static_cast< int >(buffer.size()), pipe) != nullptr )
		{
			output += buffer.data();
		}

		const int status = pclose(pipe);
		exitCode = WEXITSTATUS(status);

		/* Remove trailing newline if any. */
		while ( !output.empty() && output.back() == '\n' )
		{
			output.pop_back();
		}

		return output;
	}

	std::string
	executeCommandPumpingEvents (const std::string & command, int & exitCode, const std::function< void () > & pumpEvents) noexcept
	{
		/* One frame at 60Hz: often enough for the compositor's ping, cheap enough to be invisible. */
		constexpr auto PumpIntervalMS = 16;

		/* A pumped callback can legitimately open a second dialog. Pumping again from inside would
		 * nest one event loop into another, so the inner call just blocks: it is already covered by
		 * the outer pump, which keeps answering the compositor. */
		static thread_local bool isPumping = false;

		if ( !pumpEvents || isPumping )
		{
			return executeCommand(command, exitCode);
		}

		/* ⚠️ No try/catch in the engine, and it is built with -fno-exceptions: std::async — or a std::thread — would
		 * terminate the process when no thread is available. Base::Thread reports that failure as a value. */
		int childExitCode = -1;
		std::string childOutput;
		std::atomic< bool > finished{false};

		Base::Thread worker;

		if ( !worker.start([&command, &childExitCode, &childOutput, &finished] () noexcept {
			childOutput = executeCommand(command, childExitCode);

			finished.store(true, std::memory_order_release);
		}) )
		{
			/* The child was never spawned, so falling back to the blocking form only costs the
			 * unresponsive-window prompt, never a lost dialog (owner policy, 2026-10-07). */
			return executeCommand(command, exitCode);
		}

		isPumping = true;

		while ( !finished.load(std::memory_order_acquire) )
		{
			pumpEvents();

			std::this_thread::sleep_for(std::chrono::milliseconds{PumpIntervalMS});
		}

		worker.join();

		isPumping = false;

		exitCode = childExitCode;

		return childOutput;
	}

	std::string
	buildZenityFilters (const ExtensionFilters & filters) noexcept
	{
		std::string result;

		for ( const auto & [filterName, extensions] : filters )
		{
			std::string filterArg = filterName;
			filterArg += '|';

			for ( auto it = extensions.cbegin(); it != extensions.cend(); ++it )
			{
				filterArg += "*.";
				filterArg += *it;

				if ( std::next(it) != extensions.cend() )
				{
					filterArg += ' ';
				}
			}

			result += " --file-filter=";
			result += escapeShellArg(filterArg);
		}

		return result;
	}

	std::string
	buildKdialogFilters (const ExtensionFilters & filters) noexcept
	{
		std::string result;

		for ( auto it = filters.cbegin(); it != filters.cend(); ++it )
		{
			const auto & [filterName, extensions] = *it;

			if ( it != filters.cbegin() )
			{
				result += " | ";
			}

			result += filterName;
			result += '(';

			for ( auto extIt = extensions.cbegin(); extIt != extensions.cend(); ++extIt )
			{
				result += "*.";
				result += *extIt;

				if ( std::next(extIt) != extensions.cend() )
				{
					result += ' ';
				}
			}

			result += ')';
		}

		return result;
	}

	bool
	prependVulkanLayerDirectory (const std::filesystem::path & directory) noexcept
	{
		/* NOTE: VK_LAYER_PATH replaces every layer search path: an explicit developer choice, left alone. */
		if ( std::getenv("VK_LAYER_PATH") != nullptr )
		{
			return false;
		}

		std::string value = directory.string();

		if ( const char * current = std::getenv("VK_ADD_LAYER_PATH"); current != nullptr && *current != '\0' )
		{
			value += ':';
			value += current;
		}

		return setenv("VK_ADD_LAYER_PATH", value.c_str(), 1) == 0;
	}
}
