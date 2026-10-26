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
#include <bit>
#include <chrono>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <limits>
#include <optional>
#include <span>
#include <thread>
#include <type_traits>

/* Third-party inclusions. */
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <sys/syslimits.h>

/* Local inclusions. */
#include "IO/IO.hpp"

namespace EmEn::PlatformSpecific
{
	std::string
	pinVulkanLoaderToBundledDriver (const std::filesystem::path & resourcesDirectory) noexcept
	{
		/* NOTE: An explicit driver choice (a developer pointing the loader at another manifest) always wins.
		 * VK_ICD_FILENAMES is the deprecated spelling the loader still honors. */
		if ( std::getenv("VK_DRIVER_FILES") != nullptr || std::getenv("VK_ICD_FILENAMES") != nullptr )
		{
			return {};
		}

		/* NOTE: The resources of the RESOLVED binary, the root the layers use (PlatformManager::prepareVulkanLoaderEnvironment()):
		 * CFBundleGetMainBundle() kept a symbolic link unresolved — launched through a link, it looked beside the link,
		 * found no manifest, and the loader found no driver at all (macOS peer, 2026-10-19). Outside a bundle that is the
		 * binary directory, inside a bundle Contents/Resources (what CoreFoundation answered): FileSystem::resourcesDirectory(). */
		if ( resourcesDirectory.empty() )
		{
			return {};
		}

		const auto manifest = resourcesDirectory / "vulkan" / "icd.d" / "MoltenVK_icd.json";

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

	bool
	setCurrentThreadTimeConstraint (std::chrono::nanoseconds period, std::chrono::nanoseconds computation) noexcept
	{
		/* NOTE: One second bounds the conversion below (no overflow) and is far above any real-time period. */
		constexpr std::chrono::nanoseconds MaximumPeriod{std::chrono::seconds{1}};

		if ( period.count() <= 0 || period > MaximumPeriod || computation.count() <= 0 || computation > period )
		{
			return false;
		}

		mach_timebase_info_data_t timebase{};

		if ( mach_timebase_info(&timebase) != KERN_SUCCESS || timebase.numer == 0 || timebase.denom == 0 )
		{
			return false;
		}

		/* NOTE: The policy speaks in Mach absolute time units (ticks = ns × denom / numer), in 32 bits. */
		const auto toTicks = [&timebase] (std::chrono::nanoseconds duration) noexcept -> std::optional< uint32_t > {
			const auto ticks = static_cast< uint64_t >(duration.count()) * timebase.denom / timebase.numer;

			if ( ticks == 0 || ticks > std::numeric_limits< uint32_t >::max() )
			{
				return std::nullopt;
			}

			return static_cast< uint32_t >(ticks);
		};

		const auto periodTicks = toTicks(period);
		const auto computationTicks = toTicks(computation);

		if ( !periodTicks || !computationTicks )
		{
			return false;
		}

		thread_time_constraint_policy_data_t policy{};
		policy.period = *periodTicks;
		policy.computation = *computationTicks;
		/* NOTE: The computation may end anywhere in its period. */
		policy.constraint = *periodTicks;
		policy.preemptible = 1;

		/* NOTE: thread_policy_t is an integer_t array: the policy is handed over as its words (std::bit_cast checks
		 * the size at compile time). */
		auto words = std::bit_cast< std::array< integer_t, THREAD_TIME_CONSTRAINT_POLICY_COUNT > >(policy);

		return thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY, words.data(), THREAD_TIME_CONSTRAINT_POLICY_COUNT) == KERN_SUCCESS;
	}
}
