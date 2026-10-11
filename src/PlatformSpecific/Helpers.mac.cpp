/*
 * src/PlatformSpecific/Helpers.mac.cpp
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
#include <cstdlib>
#include <string>
#include <filesystem>

/* Third-party inclusions. */
#include <CoreFoundation/CoreFoundation.h>
#include <sys/syslimits.h>

/* Local inclusions. */
#include "IO/IO.hpp"

namespace EmEn::PlatformSpecific
{
	std::string
	pinVulkanLoaderToBundledDriver () noexcept
	{
		/* NOTE: An explicit driver choice (a developer pointing the loader at another manifest) always wins.
		 * VK_ICD_FILENAMES is the deprecated spelling the loader still honors. */
		if ( std::getenv("VK_DRIVER_FILES") != nullptr || std::getenv("VK_ICD_FILENAMES") != nullptr )
		{
			return {};
		}

		/* NOTE: Outside a bundle, CoreFoundation answers the executable's directory: the manifest is
		 * then absent and nothing changes. */
		const CFBundleRef mainBundle = CFBundleGetMainBundle();

		if ( mainBundle == nullptr )
		{
			return {};
		}

		const CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);

		if ( resourcesURL == nullptr )
		{
			return {};
		}

		std::array< char, PATH_MAX > resourcesPath{};

		const auto converted = CFURLGetFileSystemRepresentation(resourcesURL, true, reinterpret_cast< UInt8 * >(resourcesPath.data()), static_cast< CFIndex >(resourcesPath.size()));

		CFRelease(resourcesURL);

		if ( converted == 0 )
		{
			return {};
		}

		const auto manifest = std::filesystem::path{resourcesPath.data()} / "vulkan" / "icd.d" / "MoltenVK_icd.json";

		if ( !Base::IO::fileExists(manifest) )
		{
			return {};
		}

		/* NOTE: The loader reads VK_DRIVER_FILES at each driver scan, so it only has to be set before the first one. */
		if ( setenv("VK_DRIVER_FILES", manifest.c_str(), 0) != 0 )
		{
			return {};
		}

		return manifest.string();
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
