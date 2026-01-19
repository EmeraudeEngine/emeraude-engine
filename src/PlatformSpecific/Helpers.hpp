/*
 * src/PlatformSpecific/Helpers.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <string>
#include <vector>
#if IS_WINDOWS
	#include <map>
#endif

/* Third-party inclusions. */
#if IS_WINDOWS
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif
	#include <Windows.h>
	#include <shtypes.h>
#endif

namespace EmEn::PlatformSpecific
{
#if IS_WINDOWS
	/**
	 * @brief Returns a value in a wide string from the Windows register.
	 * @param regSubKey A reference to a wide string.
	 * @param regValue A reference to a wide string.
	 * @return std::wstring
	 */
	[[nodiscard]]
	std::wstring getStringValueFromHKLM (const std::wstring & regSubKey, const std::wstring & regValue);

	/**
	 * @brief Converts a wide string to an ASCII string.
	 * @param input A reference to a wide string.
	 * @return std::string
	 */
	std::string
	convertWideToANSI (const std::wstring & input);

	/**
	 * @brief Converts an ASCII string to a wide string.
	 * @param input A reference to a string.
	 * @return std::string
	 */
	std::wstring
	convertANSIToWide (const std::string & input);

	/**
	 * @brief Converts a wide string to a UTF-8 string.
	 * @param input A reference to a wide string.
	 * @return std::string
	 */
	std::string
	convertWideToUTF8 (const std::wstring & input);

	/**
	 * @brief Converts a UTF-8 string to a wide string.
	 * @param input A reference to a string.
	 * @return std::string
	 */
	std::wstring
	convertUTF8ToWide (const std::string & input);

	/**
	 * @brief Displays a console.
	 * @warning Used only for Windows OS since the logs are displayed in their own process.
	 * @param title A reference to a string.
	 * @return void
	 */
	bool createConsole (const std::string & title);

	/**
	* @brief Attaches to the parent process console.
	* @warning Used only for Windows OS. Fails if the parent has no console.
	* @return bool
	*/
	bool attachToParentConsole ();

	/**
	 * @brief Waits for a key press before closing the console.
	 * @note Displays "Press any key to close this window..." and waits.
	 * @return void
	 */
	void waitBeforeConsoleClose ();

	/**
	 * @brief Returns the parent process ID on Windows.
	 * @return int
	 */
	[[nodiscard]]
	int getParentProcessId (DWORD pid) noexcept;

	/**
	 * @brief Returns a filter list for windows using std::wstring instead of std::string.
	 * @param filters A reference to a vector.
	 * @param dataHolder A writable reference to a map.
	 * @return std::vector< COMDLG_FILTERSPEC >
	 */
	[[nodiscard]]
	std::vector< COMDLG_FILTERSPEC > createExtensionFilter (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters, std::map< std::wstring, std::wstring > & dataHolder);
#endif

#if IS_LINUX
	/** @brief Extension filter type for file dialogs. */
	using ExtensionFilters = std::vector< std::pair< std::string, std::vector< std::string > > >;

	/**
	 * @brief Checks if a program is available in the system PATH.
	 * @param program The program name to check.
	 * @return bool True if the program exists.
	 */
	[[nodiscard]]
	bool checkProgram (const std::string & program) noexcept;

	/**
	 * @brief Checks if zenity is available on the system.
	 * @note Result is cached after first call.
	 * @return bool
	 */
	[[nodiscard]]
	bool hasZenity () noexcept;

	/**
	 * @brief Checks if kdialog is available on the system.
	 * @note Result is cached after first call.
	 * @return bool
	 */
	[[nodiscard]]
	bool hasKdialog () noexcept;

	/**
	 * @brief Checks if the current desktop environment is KDE.
	 * @return bool
	 */
	[[nodiscard]]
	bool isKdeDesktop () noexcept;

	/**
	 * @brief Escapes a string for safe use as a shell argument.
	 * @param arg The argument to escape.
	 * @return std::string The escaped argument wrapped in single quotes.
	 */
	[[nodiscard]]
	std::string escapeShellArg (const std::string & arg) noexcept;

	/**
	 * @brief Executes a shell command and captures its output.
	 * @param command The command to execute.
	 * @param exitCode Output parameter for the command's exit code.
	 * @return std::string The command's stdout output with trailing newlines removed.
	 */
	[[nodiscard]]
	std::string executeCommand (const std::string & command, int & exitCode) noexcept;

	/**
	 * @brief Builds zenity file filter arguments from extension filters.
	 * @param filters The extension filters.
	 * @return std::string The zenity --file-filter arguments.
	 */
	[[nodiscard]]
	std::string buildZenityFilters (const ExtensionFilters & filters) noexcept;

	/**
	 * @brief Builds kdialog file filter arguments from extension filters.
	 * @param filters The extension filters.
	 * @return std::string The kdialog filter string.
	 */
	[[nodiscard]]
	std::string buildKdialogFilters (const ExtensionFilters & filters) noexcept;
#endif
}
