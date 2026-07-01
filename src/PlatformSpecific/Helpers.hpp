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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#if IS_WINDOWS
	#include <functional>
	#include <map>
#endif
#include <string>
#include <vector>

/* Third-party inclusions. */
#if IS_WINDOWS
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <Windows.h>
	#include <shtypes.h>
#endif

#if IS_WINDOWS
/* Forward declarations. */
namespace EmEn
{
	class Window;
}
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
	 * @brief Enables ANSI escape sequence processing on stdout and stderr.
	 * @warning Used only for Windows OS. Requires an already attached console.
	 * @return void
	 */
	void enableConsoleANSI ();

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

	/**
	 * @brief Runs a native Windows file-dialog body on a DEDICATED STA thread, owned by the real
	 * application window so the OS provides native modality, centering and Z-order.
	 *
	 * Shared machinery for OpenFile/SaveFile (the dialog-specific COM code is the @a dialogBody).
	 * It reconciles two otherwise-conflicting needs:
	 *
	 * 1. **Perf:** the dialog runs on a fresh STA thread whose message queue is empty, instead of
	 *    the engine's main thread. On the main thread the modal message loop pumps the heavy main
	 *    traffic (rendering, input, CEF), making the Windows shell re-resolve its navigation pane
	 *    thousands of times (empty SyncRootManager / known-folder lookups) — a 3-5 s delay on
	 *    cloud-redirected-known-folder machines. A quiet thread resolves the pane once (~140 ms).
	 *
	 * 2. **Native owner behavior:** the dialog is shown OWNED BY the real main window (passed to
	 *    @a dialogBody as the parent HWND). The OS then disables the owner (modality), centers the
	 *    dialog on it (placement), and keeps it above the owner in Z-order — so it resurfaces with
	 *    the main window after an alt-tab / taskbar click, instead of being buried. The owner is
	 *    cross-thread (dialog on the worker, owner on the main thread); this is made safe by (a) the
	 *    caller PUMPING messages while it waits — see the implementation — so the owner's
	 *    enable/disable/activation sends complete instead of deadlocking, and (b) AttachThreadInput
	 *    merging the two threads' input state for the dialog's lifetime so activation / focus return
	 *    behave as if same-thread. The worker also replicates the owner's DPI awareness context.
	 *
	 * COM (CoInitializeEx STA / CoUninitialize) wraps the body. If the worker thread cannot be
	 * spawned, the body runs inline on the caller thread (slower, but functional).
	 *
	 * @param window A reference to the application window (read on the caller thread only).
	 * @param parentToWindow Whether to own/center on @a window. When false, the body receives a null
	 * owner: the dialog is shown ownerless — no modality, no centering.
	 * @param dialogBody The dialog-specific work. Receives the owner window handle to pass to
	 * IFileDialog::Show() (and the legacy hwndOwner). Runs on the dedicated STA thread. Returns
	 * its own success flag, which becomes this function's return value.
	 * @return bool
	 */
	[[nodiscard]]
	bool runFileDialogOnDedicatedThread (Window & window, bool parentToWindow, const std::function< bool (HWND ownerWindow) > & dialogBody) noexcept;
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
	 * @brief Wraps a desktop-tool command so it runs with a pristine dynamic-loader environment.
	 * @note External GUI helpers (zenity, kdialog) are system programs that must load the system
	 * libraries. When the host application is launched with its bundled library directory pushed into
	 * @c LD_LIBRARY_PATH (AppImage AppRun, development wrappers, Steam-like parents), that value is
	 * inherited by every spawned child. A bundled library then shadows its system counterpart in the
	 * child; for example CEF ships a stripped @c libvulkan.so.1 that does not export
	 * @c vkCreateXlibSurfaceKHR, so a GTK-4 zenity built with the Vulkan renderer (Fedora 43+) aborts
	 * at startup with "undefined symbol: vkCreateXlibSurfaceKHR". Stripping @c LD_LIBRARY_PATH and
	 * @c LD_PRELOAD for the child restores a pristine system environment and avoids the collision.
	 * @param command The desktop-tool command to run.
	 * @return std::string The command prefixed to clear the dynamic-loader environment variables.
	 */
	[[nodiscard]]
	std::string cleanLoaderEnvCommand (const std::string & command) noexcept;

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
