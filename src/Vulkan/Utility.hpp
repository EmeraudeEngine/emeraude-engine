/*
 * src/Vulkan/Utility.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* STL inclusions. */
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace Emeraude::Vulkan
{
	/**
	 * @brief Returns a Vulkan result code to a C-String.
	 * @param code The vulkan code.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * vkResultToCString (VkResult code) noexcept;

	/**
	 * @brief Gets the validation layers available from Vulkan in a string.
	 * @param validationLayers A reference to a validation layer list.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string getItemListAsString (const std::vector< VkLayerProperties > & validationLayers) noexcept;

	/**
	 * @brief Gets the extensions available from Vulkan in a string.
	 * @param type Which type of extensions.
	 * @param extensions A reference to a extension list.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string getItemListAsString (const char * type, const std::vector< VkExtensionProperties > & extensions) noexcept;
}
