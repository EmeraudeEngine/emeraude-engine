/*
 * src/PlatformSpecific/SystemInfo.linux.cpp
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

#include "SystemInfo.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

/* Third-party inclusions. */
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "String.hpp"

namespace EmEn::PlatformSpecific
{
	namespace
	{
		/**
		 * @brief Multiplies two sysconf() results into a byte count.
		 * @note sysconf() returns -1 when a value is unavailable: the product would then wrap into a huge
		 * size_t (or read as one page). Such a case reads as zero, "unknown".
		 * @param count The first sysconf() result (a page count).
		 * @param size The second sysconf() result (a page size).
		 * @return size_t
		 */
		[[nodiscard]]
		size_t
		sysconfProduct (long count, long size) noexcept
		{
			if ( count <= 0 || size <= 0 )
			{
				return 0;
			}

			return static_cast< size_t >(count) * static_cast< size_t >(size);
		}

		/**
		 * @brief Reads the first line of a small text file (a sysfs attribute), without its line end.
		 * @param path The file.
		 * @return std::string Empty when unreadable.
		 */
		[[nodiscard]]
		std::string
		readFirstLine (const std::filesystem::path & path) noexcept
		{
			/* NOTE: A fixed system path (/proc, /sys, /etc, the pci.ids database), never data: opened directly, without Base::IO::openInputFile() (engine item readers-through-io-open-input-file, 2026-10-18). */
			std::ifstream file{path};
			std::string line;

			if ( !file.is_open() || !std::getline(file, line) )
			{
				return {};
			}

			return line;
		}

		/**
		 * @brief Parses a sysfs hexadecimal identifier ("0x10de").
		 * @param text The text.
		 * @return uint32_t 0 when invalid.
		 */
		[[nodiscard]]
		uint32_t
		parseHexIdentifier (std::string_view text) noexcept
		{
			if ( text.starts_with("0x") || text.starts_with("0X") )
			{
				text.remove_prefix(2);
			}

			uint32_t value = 0;

			if ( const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 16); error != std::errc{} )
			{
				return 0;
			}

			return value;
		}

		/**
		 * @brief Looks a PCI vendor / device name up in the system's pci.ids database (pciutils / hwdata), when present.
		 * @note The file is read line by line once per adapter (~1.5 MB, bounded): a vendor line is "vvvv  Name", its
		 * devices follow as "\tdddd  Name".
		 * @param vendorID The PCI vendor id.
		 * @param deviceID The PCI device id.
		 * @return std::pair< std::string, std::string > The vendor and device names, empty when unknown.
		 */
		[[nodiscard]]
		std::pair< std::string, std::string >
		lookUpPCINames (uint32_t vendorID, uint32_t deviceID) noexcept
		{
			constexpr std::array< const char *, 2 > Databases{"/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids"};

			for ( const auto * database : Databases )
			{
				std::ifstream file{database};

				if ( !file.is_open() )
				{
					continue;
				}

				std::string vendorName;
				std::string line;
				bool inVendor = false;

				while ( std::getline(file, line) )
				{
					if ( line.empty() || line.front() == '#' )
					{
						continue;
					}

					if ( line.front() != '\t' )
					{
						/* A new vendor (or the class section, which ends the vendor list: its lines start with 'C'). */
						if ( inVendor || line.front() == 'C' )
						{
							break;
						}

						if ( line.size() > 6 && parseHexIdentifier(std::string_view{line}.substr(0, 4)) == vendorID )
						{
							inVendor = true;
							vendorName = line.substr(6);
						}

						continue;
					}

					/* A device line of the vendor: "\tdddd  Name" (a sub-device line starts with two tabs). */
					if ( inVendor && line.size() > 7 && line[1] != '\t' && parseHexIdentifier(std::string_view{line}.substr(1, 4)) == deviceID )
					{
						return {vendorName, line.substr(7)};
					}
				}

				return {vendorName, {}};
			}

			return {};
		}
	}

	bool
	SystemInfo::fetchOSInformation () noexcept
	{
		{
			std::ifstream file{"/etc/os-release"};

			if ( file.is_open() )
			{
				std::string line;

				while ( std::getline(file, line) )
				{
					if ( line.starts_with("PRETTY_NAME") )
					{
						auto & name = m_OSInformation.systemName;
						name = Base::String::right(line, "=");
						std::erase(name, '"');
					}
				}
			}
		}

		{
			std::stringstream rawInformation;

			utsname info{};

			if ( uname(&info) != 0 )
			{
				return {};
			}

			rawInformation <<
				"System name : " << info.sysname << "\n"
				"Node name : " << info.nodename << "\n"
				"Release : " << info.release << "\n"
				"Version : " << info.version << "\n"
				"Machine : " << info.machine << "\n";

#ifdef _GNU_SOURCE
			rawInformation << "Domain name : " << info.domainname << "\n";
#endif

			m_OSInformation.systemVersion = info.version;
			m_OSInformation.computerName = info.nodename;
			m_OSInformation.rawInformation = rawInformation.str();
		}

		/* NOTE: Get the system unique identifier. */
		{
			// Call: "cat /var/lib/dbus/machine-id"
			// System: cc4719450adf495083f5ab483b0cebca
			// Requested: cc471945-0adf-4950-83f5-ab483b0cebca
			// FIXME: Node.js module use the second pattern.
			// 9db073cb-6101-4a3b-a425-7b4471697a8b
			// 9db073cb6-1014a-3ba42-57b44-71697a8b

			std::ifstream file{"/var/lib/dbus/machine-id"};

			if ( file.is_open() )
			{
				std::string line;

				std::getline(file, line);

				if ( line.length() == 32 )
				{
					m_OSInformation.machineUUID.resize(36);

					size_t destIndex = 0;

					for ( size_t index = 0; index < 32; ++index )
					{
						m_OSInformation.machineUUID[destIndex++] = line[index];

						if ( index == 9 || index == 15 || index == 21 || index == 27 )
						{
							m_OSInformation.machineUUID[destIndex++] = '-';
						}
					}
				}
				else
				{
					std::cerr << "Failed to get machine UUID !" "\n";
				}
			}
		}

		return true;
	}

	size_t
	SystemInfo::getTotalMemory () noexcept
	{
		return sysconfProduct(sysconf(_SC_PHYS_PAGES), sysconf(_SC_PAGE_SIZE));
	}

	size_t
	SystemInfo::getFreeMemory () noexcept
	{
		/* NOTE: _SC_AVPHYS_PAGES only counts truly-free pages — the reclaimable page
		 * cache is reported as "used", overestimating the memory load by tens of
		 * percents on a busy desktop. Prefer MemAvailable from /proc/meminfo
		 * (kernel >= 3.14): the kernel's own estimate of allocatable memory. */
		std::ifstream meminfo{"/proc/meminfo"};
		std::string line;

		while ( std::getline(meminfo, line) )
		{
			if ( line.starts_with("MemAvailable:") )
			{
				/* Format: "MemAvailable:   12345678 kB" */
				const auto kibibytes = std::strtoull(line.c_str() + line.find(':') + 1, nullptr, 10);

				return static_cast< size_t >(kibibytes) * 1024;
			}
		}

		/* Fallback (pre-3.14 kernel): truly-free pages. */
		return sysconfProduct(sysconf(_SC_AVPHYS_PAGES), sysconf(_SC_PAGESIZE));
	}

	size_t
	SystemInfo::getPeakRSS () noexcept
	{
		rusage rusage{};
		getrusage( RUSAGE_SELF, &rusage);

		/* NOTE: ru_maxrss is in kibibytes on Linux, never negative. */
		return rusage.ru_maxrss > 0 ? static_cast< size_t >(rusage.ru_maxrss) * 1024UL : 0;
	}

	size_t
	SystemInfo::getCurrentRSS () noexcept
	{
		std::ifstream statFile{"/proc/self/statm"};

		if ( !statFile.is_open() )
		{
			return 0;
		}

		/* ie: "92957 20466 16854 854 0 1627 0" */
		std::string line;
		std::getline(statFile, line);

		/* NOTE: We need the second number. */
		const auto position = line.find_first_of(' ');

		if ( position == std::string::npos )
		{
			return 0;
		}

		const auto residentPages = std::strtol(line.c_str() + position, nullptr, 10);

		return sysconfProduct(residentPages, sysconf(_SC_PAGESIZE));
	}

	std::filesystem::path
	SystemInfo::getRealApplicationDir () noexcept
	{
		std::error_code errorCode;

		const std::string realPath = std::filesystem::canonical("/proc/self/exe", errorCode).string();

		/* NOTE: no /proc (a restricted container) — an empty path: the caller keeps its current directory. */
		if ( errorCode )
		{
			return {};
		}

		const auto position = realPath.find_last_of('/');

		return
			position == std::string::npos ?
			realPath :
			realPath.substr(0, position);
	}

	bool
	SystemInfo::fetchGPUInformation () noexcept
	{
		/* NOTE: One DRM card per adapter ("card0", "card1"; "card0-DP-1" are its connectors, "renderD128" its render
		 * node). Bounded by the directory's size and the StaticVector's capacity. */
		const std::filesystem::path drmClass{"/sys/class/drm"};

		if ( !Base::IO::directoryExists(drmClass) )
		{
			return false;
		}

		m_GPUDevices.clear();

		for ( const auto & entry : Base::IO::directoryEntries(drmClass) )
		{
			const auto name = entry.filename().string();

			if ( !name.starts_with("card") || name.size() == 4 || !std::all_of(name.begin() + 4, name.end(), [] (char character) { return character >= '0' && character <= '9'; }) )
			{
				continue;
			}

			if ( m_GPUDevices.full() )
			{
				break;
			}

			const auto device = entry / "device";

			GPUDevice adapter;
			adapter.vendorID = parseHexIdentifier(readFirstLine(device / "vendor"));
			adapter.deviceID = parseHexIdentifier(readFirstLine(device / "device"));

			/* The driver and the bus come from the device's uevent ("DRIVER=nvidia", "PCI_SLOT_NAME=0000:01:00.0"). */
			{
				std::ifstream uevent{device / "uevent"};
				std::string line;

				while ( std::getline(uevent, line) )
				{
					if ( line.starts_with("DRIVER=") )
					{
						adapter.driverName = line.substr(7);
					}
					else if ( line.starts_with("PCI_SLOT_NAME=") )
					{
						adapter.bus = line.substr(14);
					}
				}
			}

			/* NOTE: A kernel module publishes its version there when it has one (nvidia does, the in-tree drivers do not). */
			if ( !adapter.driverName.empty() )
			{
				adapter.driverVersion = readFirstLine(std::filesystem::path{"/sys/module"} / adapter.driverName / "version");
			}

			/* NOTE: A framebuffer driver is not a GPU. */
			adapter.software = adapter.driverName == "simpledrm" || adapter.driverName == "efifb" || adapter.driverName == "vboxvideo";

			/* NOTE: A heuristic, stated as such: an integrated GPU sits on the root PCI bus ("0000:00:..."). */
			const bool rootBus = adapter.bus.size() > 7 && adapter.bus.compare(5, 2, "00") == 0;
			if ( adapter.software )
			{
				adapter.deviceType = GPUDeviceType::Other;
			}
			else
			{
				adapter.deviceType = rootBus ? GPUDeviceType::IntegratedGPU : GPUDeviceType::DiscreteGPU;
			}

			adapter.dedicatedMemory = !rootBus && !adapter.software;

			auto [vendorName, deviceName] = lookUpPCINames(adapter.vendorID, adapter.deviceID);

			if ( !vendorName.empty() )
			{
				adapter.vendorName = std::move(vendorName);
			}

			if ( !deviceName.empty() )
			{
				adapter.deviceName = std::move(deviceName);
			}
			else
			{
				std::stringstream fallback;
				fallback << adapter.vendorName << " device 0x" << std::hex << adapter.deviceID;
				adapter.deviceName = fallback.str();
			}

			m_GPUDevices.emplace_back(std::move(adapter));
		}

		return true;
	}

	std::string
	SystemInfo::platformCPUBrandString () noexcept
	{
		/* NOTE: cpu_features reads the brand string (CPUID) on this platform. */
		return {};
	}
}
