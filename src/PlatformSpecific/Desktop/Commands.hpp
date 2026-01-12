/*
 * src/PlatformSpecific/Desktop/Commands.hpp
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
#include <cstdint>
#include <string>
#include <filesystem>

/* Forward declarations. */
namespace EmEn
{
	class Settings;
	class Window;
}

namespace EmEn::PlatformSpecific::Desktop
{
	/** @brief The progress mode enumeration for Windows. */
	enum class ProgressMode: uint8_t
	{
		None,
		Normal,
		Indeterminate,
		Error,
		Paused
	};

	/**
	 * @brief Converts a string to a progress mode token.
	 * @param string A reference to a string.
	 * @return ProgressMode
	 */
	[[nodiscard]]
	ProgressMode to_ProgressMode (const std::string & string) noexcept;

	/**
	 * @brief Tries to open a URL in an external web browser.
	 * @param url A reference to a string.
	 * @return bool
	 */
	bool openURL (const std::string & url) noexcept;

	/**
	 * @brief Tries to open a file with an external program.
	 * @param filepath A reference to a path.
	 * @return bool
	 */
	bool openFile (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Tries to open a folder in an external program.
	 * @param filepath A reference to a path.
	 * @return bool
	 */
	bool openFolder (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Tries to open a file with an external text editor.
	 * @param settings A reference to the settings.
	 * @param filepath A reference to a path.
	 * @return bool
	 */
	bool openTextFile (Settings & settings, const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Tries to open the directory of a file in an external file browser.
	 * @param filepath A reference to a path.
	 * @return bool
	 */
	bool showInFolder (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Runs a desktop application with arguments.
	 * @note This is used to run an application aside to the user attention.
	 * @param executable A reference to a string.
	 * @param argument A reference to a string.
	 * @return bool
	 */
	bool runDesktopApplication (const std::string & executable, const std::string & argument) noexcept;

	/**
	 * @brief Tries to open a file using the default desktop application.
	 * @note This is used to run an application aside for to the user attention.
	 * @param argument A reference to a string.
	 * @return bool
	 */
	bool runDefaultDesktopApplication (const std::string & argument) noexcept;

	/**
	 * @brief Makes the taskbar icon of the application flashing to alert the user.
	 * @param window A reference to the window.
	 * @param state The flashing state.
	 * @return void
	 */
	void flashTaskbarIcon (const Window & window, bool state) noexcept;

	/**
	 * @brief Sets a progression inside the taskbar icon of the application.
	 * @param window A reference to the window.
	 * @param progress The progression value. Negative number disables the progression.
	 * @param mode The progression mode. Only for Windows.
	 * @return void
	 */
	void setTaskbarIconProgression (const Window & window, float progress, ProgressMode mode) noexcept;
}
