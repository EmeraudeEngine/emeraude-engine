/*
 * src/Vulkan/Utility.hpp
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
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn::Vulkan
{
	/**
	 * @brief Returns a Vulkan result code to a C-String.
	 * @param code The vulkan code.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_API const char * vkResultToCString (VkResult code) noexcept;

	/**
	 * @brief Returns what a Vulkan result code actually means for diagnosis, for the codes that are
	 * systematically misread, or nullptr when there is nothing worth adding.
	 * @note Exists because VK_ERROR_SURFACE_LOST_KHR keeps being investigated as a GPU fault
	 * alongside VK_ERROR_DEVICE_LOST, which sends the reader into the wrong subsystem entirely: the
	 * GPU is fine, the WINDOW is gone. On Wayland it is usually a compositor protocol error killing
	 * the wl_display connection, printed on stderr by libwayland itself just above — nothing the
	 * engine can catch or recover from, since every Wayland object of the process dies with it.
	 * @param code The vulkan code.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_API const char * vkResultDiagnosticHint (VkResult code) noexcept;

	/**
	 * @brief Gets the validation layers available from Vulkan in a string.
	 * @param validationLayers A reference to a validation layer list.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string getItemListAsString (const std::vector< VkLayerProperties > & validationLayers) noexcept;

	/**
	 * @brief Gets the extensions available from Vulkan in a string.
	 * @param type Which type of extensions.
	 * @param extensions A reference to an extension list.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string getItemListAsString (const char * type, const std::vector< VkExtensionProperties > & extensions) noexcept;
}
