/*
 * src/Core.startup.cpp
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

#include "Core.hpp"

/* STL inclusions. */
#include <algorithm>
#include <optional>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "PlatformSpecific/Desktop/Dialog/Message.hpp"
#include "PlatformSpecific/SystemInfo.hpp"
#include "Tracer.hpp"

/*
 * A startup refused for the graphics hardware or its driver, told to the user
 * (docs/subsystems/vulkan/22-startup-refusals-told-to-the-user.md, owner decisions 2026-10-15: the engine classifies and
 * shows a native dialog in English by default, the application may replace it through Core::onStartupRefused(); the
 * message links the GPU vendor's driver page). Measured in user crash reports of a downstream application, 2026-10-15: 11 of 12 reports
 * were such silent startup refusals.
 */
namespace EmEn
{
	namespace
	{
		/**
		 * @brief Returns the driver download page of a GPU vendor, or nullptr when the vendor is not one of the three.
		 * @param vendorID The PCI vendor id.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		driverPageOf (uint32_t vendorID) noexcept
		{
			switch ( vendorID )
			{
				case 0x10DE :
					return "https://www.nvidia.com/drivers";

				case 0x1002 :
					return "https://www.amd.com/en/support/download/drivers.html";

				case 0x8086 :
					return "https://www.intel.com/content/www/us/en/support/detect.html";

				default :
					return nullptr;
			}
		}

		/**
		 * @brief Returns "Vulkan x.y.z" for a packed API version.
		 * @param apiVersion The packed version.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		vulkanVersionString (uint32_t apiVersion) noexcept
		{
			std::stringstream text;
			text << "Vulkan " << VK_API_VERSION_MAJOR(apiVersion) << '.' << VK_API_VERSION_MINOR(apiVersion) << '.' << VK_API_VERSION_PATCH(apiVersion);

			return text.str();
		}
	}

	std::optional< Core::StartupRefusal >
	Core::diagnoseStartupRefusal () const noexcept
	{
		using Reason = Vulkan::Instance::DeviceRefusal::Reason;

		/* NOTE: The requirement is a major.minor version ("Vulkan 1.3"). */
		std::stringstream requiredText;
		requiredText << "Vulkan " << VK_API_VERSION_MAJOR(Vulkan::Instance::MinimumDeviceAPIVersion) << '.' << VK_API_VERSION_MINOR(Vulkan::Instance::MinimumDeviceAPIVersion);
		const auto requiredVersion = requiredText.str();

		StartupRefusal refusal;
		refusal.refusals = m_vulkanInstance.deviceRefusals();

		/* The GPUs the operating system knows, independently of Vulkan (empty when the platform cannot list them), and
		 * one driver page per vendor among them. A software adapter is not a GPU. */
		std::string knownGPUs;
		std::string driverPages;

		for ( const auto & gpu : m_primaryServices.systemInfo().getGPUDeviceInformation() )
		{
			if ( gpu.software )
			{
				continue;
			}

			knownGPUs += knownGPUs.empty() ? "" : ", ";
			knownGPUs += gpu.deviceName;

			if ( const auto * page = driverPageOf(gpu.vendorID); page != nullptr && driverPages.find(page) == std::string::npos )
			{
				driverPages += driverPages.empty() ? "" : "\n";
				driverPages += page;

				if ( refusal.driverURL.empty() )
				{
					refusal.driverURL = page;
				}
			}
		}

		switch ( m_coreLevelFailure )
		{
			case CoreLevelFailure::VulkanInstance :
				/* NOTE: No driver answers the Vulkan loader, or it answers with no device at all. */
				if ( m_vulkanInstance.creationResult() == VK_ERROR_INCOMPATIBLE_DRIVER || m_vulkanInstance.physicalDevices().empty() )
				{
					refusal.cause = StartupRefusal::Cause::NoVulkanDriver;
					refusal.title = "Graphics driver required";

					std::stringstream message;
					message << "This application needs a graphics driver supporting " << requiredVersion << ", and none was found on this computer.\n\n";

					if ( !knownGPUs.empty() )
					{
						message << "Graphics hardware found: " << knownGPUs << ".\n\n";
					}

					message << "Install or update the driver of your graphics card from its manufacturer's website";
					message << ( driverPages.empty() ? "." : ":\n" + driverPages ) << "\n\n";
					message << "If this computer only has an old integrated graphics chip, it is not compatible.";

					refusal.message = message.str();

					return refusal;
				}

				break;

			case CoreLevelFailure::GraphicsRenderer :
			{
				/* NOTE: A renderer that failed WITH a graphics device is not a hardware / driver refusal. */
				if ( m_vulkanInstance.graphicsDevice() != nullptr )
				{
					break;
				}

				const auto outdated = std::ranges::find_if(refusal.refusals, [] (const auto & entry) {
					return entry.reason == Reason::BelowRequiredAPIVersion;
				});

				if ( outdated != refusal.refusals.end() )
				{
					refusal.cause = StartupRefusal::Cause::OutdatedDriver;
					refusal.title = "Graphics driver too old";

					if ( const auto * page = driverPageOf(outdated->vendorID); page != nullptr )
					{
						refusal.driverURL = page;
					}

					std::stringstream message;
					message <<
						"The graphics driver of '" << outdated->deviceName << "' (version " << outdated->driverVersion << ") is too old: "
						"it supports " << vulkanVersionString(outdated->apiVersion) << ", and this application requires " << requiredVersion << ".\n\n"
						"Update the graphics driver" << ( refusal.driverURL.empty() ? "." : ":\n" + refusal.driverURL );

					refusal.message = message.str();

					return refusal;
				}

				const auto unsupported = std::ranges::find_if(refusal.refusals, [] (const auto & entry) {
					return entry.reason == Reason::MissingFeatures;
				});

				refusal.cause = StartupRefusal::Cause::IncompatibleHardware;
				refusal.title = "Graphics hardware not compatible";

				std::stringstream message;

				if ( unsupported != refusal.refusals.end() )
				{
					if ( const auto * page = driverPageOf(unsupported->vendorID); page != nullptr )
					{
						refusal.driverURL = page;
					}

					message <<
						"'" << unsupported->deviceName << "' does not support features this application requires (" << unsupported->detail << ").\n\n"
						"This graphics hardware is not compatible. If its driver is old, updating it may help" << ( refusal.driverURL.empty() ? "." : ":\n" + refusal.driverURL );
				}
				else
				{
					message << "No graphics device of this computer can run this application.";

					if ( !knownGPUs.empty() )
					{
						message << "\n\nGraphics hardware found: " << knownGPUs << ".";
					}

					if ( !refusal.driverURL.empty() )
					{
						message << "\n\nIf its driver is old, updating it may help:\n" << refusal.driverURL;
					}
				}

				refusal.message = message.str();

				return refusal;
			}

			case CoreLevelFailure::None :
			case CoreLevelFailure::Platform :
			case CoreLevelFailure::Window :
			case CoreLevelFailure::Input :
				break;
		}

		return std::nullopt;
	}

	void
	Core::tellStartupRefusal (const StartupRefusal & refusal) noexcept
	{
		TraceFatal{ClassId} << "Startup refused (" << refusal.title << "): " << refusal.message;

		if ( this->onStartupRefused(refusal) )
		{
			Tracer::info(ClassId, "The application told the user about the refused startup itself.");

			return;
		}

		/* NOTE: The window may not exist yet (the Vulkan instance starts before it): an unparented dialog. */
		PlatformSpecific::Desktop::Dialog::Message dialog{refusal.title, refusal.message, PlatformSpecific::Desktop::Dialog::ButtonLayout::OK, PlatformSpecific::Desktop::Dialog::MessageType::Error};

		static_cast< void >(dialog.execute(m_window, false));
	}
}
