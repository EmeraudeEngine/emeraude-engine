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
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <span>
#include <thread>
#include <type_traits>

/* Third-party inclusions. */
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
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

	std::string
	readEnvironmentVariable (const char * name) noexcept
	{
		const char * value = std::getenv(name);

		return value != nullptr ? std::string{value} : std::string{};
	}

	namespace
	{
		/** @brief Upper bound of the online display list (CoreGraphics' own limit is far beyond any real desk). */
		constexpr uint32_t MaxOnlineDisplays{32};

		/**
		 * @brief Returns whether at least one online display is awake.
		 * @param awake Set to the answer when the list was obtained.
		 * @return bool False when the display list could not be obtained.
		 */
		[[nodiscard]]
		bool
		anyDisplayAwake (bool & awake) noexcept
		{
			std::array< CGDirectDisplayID, MaxOnlineDisplays > displays{};
			uint32_t count = 0;

			if ( CGGetOnlineDisplayList(MaxOnlineDisplays, displays.data(), &count) != kCGErrorSuccess )
			{
				return false;
			}

			awake = std::ranges::any_of(std::span{displays}.first(std::min(count, MaxOnlineDisplays)), [] (CGDirectDisplayID display) {
				return CGDisplayIsAsleep(display) == 0;
			});

			return true;
		}
	}

	DisplayWakeResult
	wakeDisplays (uint32_t timeoutMilliseconds) noexcept
	{
		bool awake = false;

		if ( !anyDisplayAwake(awake) )
		{
			return DisplayWakeResult::Failed;
		}

		if ( awake )
		{
			return DisplayWakeResult::AlreadyAwake;
		}

		IOPMAssertionID assertion = kIOPMNullAssertionID;

		if ( IOPMAssertionDeclareUserActivity(CFSTR("Emeraude engine startup: wake the display to create the window"), kIOPMUserActiveLocal, &assertion) != kIOReturnSuccess )
		{
			return DisplayWakeResult::Failed;
		}

		constexpr uint32_t PollIntervalMilliseconds{50};
		const uint32_t maxPolls = timeoutMilliseconds / PollIntervalMilliseconds + 1;

		auto result = DisplayWakeResult::StillAsleep;

		for ( uint32_t poll = 0; poll < maxPolls; ++poll )
		{
			if ( anyDisplayAwake(awake) && awake )
			{
				result = DisplayWakeResult::Woken;

				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds{PollIntervalMilliseconds});
		}

		/* NOTE: The activity was declared; releasing the assertion hands the display back to the user's idle timer. */
		static_cast< void >(IOPMAssertionRelease(assertion));

		return result;
	}

	static_assert(std::is_same_v< IOPMAssertionID, uint32_t >, "DisplaySleepInhibitor stores an IOPMAssertionID as uint32_t.");

	DisplaySleepInhibitor::~DisplaySleepInhibitor ()
	{
		this->release();
	}

	bool
	DisplaySleepInhibitor::acquire () noexcept
	{
		if ( m_active )
		{
			return true;
		}

		IOPMAssertionID assertion = kIOPMNullAssertionID;

		if ( IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn, CFSTR("Emeraude engine: a window is open"), &assertion) != kIOReturnSuccess )
		{
			return false;
		}

		m_assertion = assertion;
		m_active = true;

		return true;
	}

	void
	DisplaySleepInhibitor::release () noexcept
	{
		if ( !m_active )
		{
			return;
		}

		static_cast< void >(IOPMAssertionRelease(m_assertion));

		m_assertion = kIOPMNullAssertionID;
		m_active = false;
	}
}
