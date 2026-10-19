/*
 * src/PlatformSpecific/SystemInfo.mac.cpp
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
#include <cstdint>
#include <cstdio> // FIXME: Remove this
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

/* Third-party inclusions. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific
{
	namespace
	{
		/**
		 * @brief Multiplies two sysconf() results into a byte count.
		 * @note sysconf() returns -1 when a value is unavailable: the product would then wrap into a huge
		 * size_t (or read as one page). Such a case reads as zero, "unknown" (same as SystemInfo.linux.cpp).
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

		/** @brief Releases a Core Foundation object (a std::unique_ptr deleter). */
		struct CFReleaser final
		{
			void
			operator() (CFTypeRef object) const noexcept
			{
				if ( object != nullptr )
				{
					CFRelease(object);
				}
			}
		};

		/** @brief An owned Core Foundation object (a "Create" / "Copy" result). */
		using CFObject = std::unique_ptr< const void, CFReleaser >;

		/** @brief An owned IOKit object (an io_object_t is a Mach port name, not a pointer): released at destruction. */
		class IOObject final
		{
			public:

				/**
				 * @brief Takes the ownership of an IOKit object.
				 * @param object The object, or IO_OBJECT_NULL.
				 */
				explicit
				IOObject (io_object_t object) noexcept
					: m_object{object}
				{

				}

				IOObject (const IOObject & copy) = delete;
				IOObject & operator= (const IOObject & copy) = delete;

				IOObject (IOObject && other) noexcept
					: m_object{std::exchange(other.m_object, IO_OBJECT_NULL)}
				{

				}

				IOObject &
				operator= (IOObject && other) noexcept
				{
					if ( this != &other )
					{
						this->release();

						m_object = std::exchange(other.m_object, IO_OBJECT_NULL);
					}

					return *this;
				}

				~IOObject ()
				{
					this->release();
				}

				/**
				 * @brief Returns the object.
				 * @return io_object_t IO_OBJECT_NULL when none.
				 */
				[[nodiscard]]
				io_object_t
				get () const noexcept
				{
					return m_object;
				}

			private:

				void
				release () noexcept
				{
					if ( m_object != IO_OBJECT_NULL )
					{
						IOObjectRelease(m_object);

						m_object = IO_OBJECT_NULL;
					}
				}

				io_object_t m_object{IO_OBJECT_NULL};
		};

		/** @brief The PCI vendor id of Apple. */
		constexpr uint32_t AppleVendorID{0x106B};
		/** @brief The PCI vendor id of Intel. */
		constexpr uint32_t IntelVendorID{0x8086};
		/** @brief The PCI vendor id of AMD. */
		constexpr uint32_t AMDVendorID{0x1002};
		/** @brief The PCI vendor id of NVIDIA. */
		constexpr uint32_t NVIDIAVendorID{0x10DE};
		/** @brief How many accelerators are read at most (more than any Mac carries; the list itself holds 8). */
		constexpr size_t MaximumAccelerators{32};
		/** @brief How many registry levels above an accelerator are searched for its PCI device. */
		constexpr size_t MaximumParentDepth{6};

		/**
		 * @brief Reads a registry property as a string: a CFString, or a CFData holding a NUL-terminated string (the
		 * "model" of an IOPCIDevice).
		 * @param entry The registry entry.
		 * @param key The property name.
		 * @return std::string Empty when the property is missing or of another type.
		 */
		[[nodiscard]]
		std::string
		stringProperty (io_registry_entry_t entry, const char * key) noexcept
		{
			const CFObject keyString{CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8)};

			if ( keyString == nullptr )
			{
				return {};
			}

			const CFObject property{IORegistryEntryCreateCFProperty(entry, static_cast< CFStringRef >(keyString.get()), kCFAllocatorDefault, 0)};

			if ( property == nullptr )
			{
				return {};
			}

			if ( CFGetTypeID(property.get()) == CFStringGetTypeID() )
			{
				std::array< char, 256 > text{};

				if ( CFStringGetCString(static_cast< CFStringRef >(property.get()), text.data(), static_cast< CFIndex >(text.size()), kCFStringEncodingUTF8) != 0 )
				{
					return std::string{text.data()};
				}

				return {};
			}

			if ( CFGetTypeID(property.get()) == CFDataGetTypeID() )
			{
				std::array< UInt8, 256 > bytes{};

				const auto * const data = static_cast< CFDataRef >(property.get());
				const auto length = std::min< CFIndex >(CFDataGetLength(data), static_cast< CFIndex >(bytes.size()));

				CFDataGetBytes(data, CFRangeMake(0, length), bytes.data());

				/* NOTE: The text ends at its first NUL, or with the data. */
				std::string text{bytes.begin(), bytes.begin() + length};
				text.resize(std::min(text.find('\0'), text.size()));

				return text;
			}

			return {};
		}

		/**
		 * @brief Reads a registry property as a 32-bit integer: a CFNumber, or a CFData of at least four little-endian
		 * bytes (the "vendor-id" / "device-id" of the registry).
		 * @param entry The registry entry.
		 * @param key The property name.
		 * @return std::optional< uint32_t > None when the property is missing or of another type.
		 */
		[[nodiscard]]
		std::optional< uint32_t >
		integerProperty (io_registry_entry_t entry, const char * key) noexcept
		{
			const CFObject keyString{CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8)};

			if ( keyString == nullptr )
			{
				return std::nullopt;
			}

			const CFObject property{IORegistryEntryCreateCFProperty(entry, static_cast< CFStringRef >(keyString.get()), kCFAllocatorDefault, 0)};

			if ( property == nullptr )
			{
				return std::nullopt;
			}

			if ( CFGetTypeID(property.get()) == CFNumberGetTypeID() )
			{
				int64_t value = 0;

				if ( CFNumberGetValue(static_cast< CFNumberRef >(property.get()), kCFNumberSInt64Type, &value) != 0 && value >= 0 && value <= INT64_C(0xFFFFFFFF) )
				{
					return static_cast< uint32_t >(value);
				}

				return std::nullopt;
			}

			if ( CFGetTypeID(property.get()) == CFDataGetTypeID() )
			{
				const auto * const data = static_cast< CFDataRef >(property.get());

				if ( CFDataGetLength(data) < 4 )
				{
					return std::nullopt;
				}

				std::array< UInt8, 4 > bytes{};

				CFDataGetBytes(data, CFRangeMake(0, 4), bytes.data());

				return static_cast< uint32_t >(bytes[0]) | (static_cast< uint32_t >(bytes[1]) << 8U) | (static_cast< uint32_t >(bytes[2]) << 16U) | (static_cast< uint32_t >(bytes[3]) << 24U);
			}

			return std::nullopt;
		}

		/**
		 * @brief Finds the PCI device an accelerator belongs to, walking up the service plane (an Intel Mac's GPUs).
		 * @note An Apple Silicon GPU has none: it is not on a PCI bus.
		 * @param accelerator The accelerator service.
		 * @return IOObject The IOPCIDevice, or an empty object.
		 */
		[[nodiscard]]
		IOObject
		pciDeviceOf (io_registry_entry_t accelerator) noexcept
		{
			io_registry_entry_t current = accelerator;
			IOObject owned{IO_OBJECT_NULL};

			for ( size_t depth = 0; depth < MaximumParentDepth; ++depth )
			{
				io_registry_entry_t parent = IO_OBJECT_NULL;

				if ( IORegistryEntryGetParentEntry(current, kIOServicePlane, &parent) != KERN_SUCCESS || parent == IO_OBJECT_NULL )
				{
					return IOObject{IO_OBJECT_NULL};
				}

				/* NOTE: The previous parent is released here, once its own parent is held. */
				owned = IOObject{parent};
				current = parent;

				if ( IOObjectConformsTo(parent, "IOPCIDevice") != 0 )
				{
					return owned;
				}
			}

			return IOObject{IO_OBJECT_NULL};
		}

		/**
		 * @brief Returns the vendor name of a PCI vendor id.
		 * @param vendorID The vendor id.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		vendorNameOf (uint32_t vendorID) noexcept
		{
			switch ( vendorID )
			{
				case AppleVendorID :
					return "Apple";

				case IntelVendorID :
					return "Intel";

				case AMDVendorID :
					return "AMD";

				case NVIDIAVendorID :
					return "NVIDIA";

				default :
					return "Unknown";
			}
		}

		/**
		 * @brief Describes one accelerator service.
		 * @param accelerator The accelerator service.
		 * @return GPUDevice
		 */
		[[nodiscard]]
		GPUDevice
		describeAccelerator (io_registry_entry_t accelerator) noexcept
		{
			GPUDevice device;

			const auto pciDevice = pciDeviceOf(accelerator);

			/* NOTE: An Apple Silicon accelerator carries its own "model" ("Apple M2") and "vendor-id"; an Intel Mac's GPU
			 * has them on its PCI device, with the "device-id". */
			auto model = stringProperty(accelerator, "model");

			if ( model.empty() && pciDevice.get() != IO_OBJECT_NULL )
			{
				model = stringProperty(pciDevice.get(), "model");
			}

			if ( !model.empty() )
			{
				device.deviceName = model;
			}

			auto vendorID = integerProperty(accelerator, "vendor-id");

			if ( !vendorID.has_value() && pciDevice.get() != IO_OBJECT_NULL )
			{
				vendorID = integerProperty(pciDevice.get(), "vendor-id");
			}

			if ( pciDevice.get() != IO_OBJECT_NULL )
			{
				device.deviceID = integerProperty(pciDevice.get(), "device-id").value_or(0);

				/* NOTE: The video memory of a discrete GPU, when its driver publishes it (in MiB). */
				if ( const auto VRAM = integerProperty(pciDevice.get(), "VRAM,totalMB"); VRAM.has_value() )
				{
					device.availableMemory = static_cast< size_t >(*VRAM) * 1024U * 1024U;
				}
			}

			device.vendorID = vendorID.value_or(pciDevice.get() == IO_OBJECT_NULL ? AppleVendorID : 0);
			device.vendorName = vendorNameOf(device.vendorID);

			/* NOTE: Apple Silicon and Intel GPUs share the system memory; an AMD / NVIDIA GPU of an Intel Mac has its own. */
			const auto integrated = device.vendorID == AppleVendorID || device.vendorID == IntelVendorID;

			device.deviceType = integrated ? GPUDeviceType::IntegratedGPU : GPUDeviceType::DiscreteGPU;
			device.dedicatedMemory = !integrated;
			device.bus = pciDevice.get() == IO_OBJECT_NULL ? "Built-in" : "PCI";

			/* NOTE: The kernel extension of the accelerator ("com.apple.AGXG18G") and its source version ("360.34.5"). */
			device.driverName = stringProperty(accelerator, "CFBundleIdentifier");
			device.driverVersion = stringProperty(accelerator, "IOSourceVersion");

			return device;
		}
	}

	bool
	SystemInfo::fetchOSInformation () noexcept
	{
		char OSVersion[32];
		size_t OSVersionLength = sizeof(OSVersion) - 1;
		int OSVersionName[] = {CTL_KERN, KERN_OSRELEASE};

		if ( sysctl(OSVersionName, 2, OSVersion, &OSVersionLength, nullptr, 0) == -1 )
		{
			return false;
		}

		// FIXME: Remove the unsafe sscanf() C method;
		uint32_t major, minor;

		if ( sscanf(OSVersion, "%u.%u", &major, &minor) != 2 )
		{
			return false;
		}

		if ( major >= 25 )
		{
			/* NOTE: macOS 26 (Tahoe) jumped from 15 to the year number while Darwin went from 24 to 25: Darwin 25 is
			 * macOS 26 (measured: Darwin 25.6 = macOS 26.6, which the old "- 9" called 16.6, 2026-10-05). */
			m_OSInformation.systemName = (std::stringstream{} << "Apple macOS " << (major + 1) << '.' << minor).str();
			m_OSInformation.systemVersion = (std::stringstream{} << major << '.' << minor).str();
		}
		else if ( major >= 20 )
		{
			// macOS 11 to 15
			m_OSInformation.systemName = (std::stringstream{} << "Apple macOS " << (major - 9) << '.' << minor).str();
			m_OSInformation.systemVersion = (std::stringstream{} << major << '.' << minor).str();
		}
		else
		{
			// macOS 10.1.1 and newer
			m_OSInformation.systemName = (std::stringstream{} << "Apple Mac OS X " << (major - 4) << '.' << minor).str();
			m_OSInformation.systemVersion = (std::stringstream{} << "10." << major << '.' << minor).str();
		}

		/* NOTE: Get the computer name. */
		{
			char hostname[256];

			if ( gethostname(hostname, sizeof(hostname)) == 0 )
			{
				m_OSInformation.computerName = hostname;
			}
		}

		/* NOTE: Get the machine UUID. */
		{
			io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMainPortDefault, "IOService:/");

			/* NOTE: The property can be missing (NULL: CFStringGetCStringPtr() and CFRelease() crashed on it), and
			 * CFStringGetCStringPtr() may legitimately return NULL: CFStringGetCString() copies instead. */
			const auto UUIDProperty = IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);

			if ( UUIDProperty != nullptr && CFGetTypeID(UUIDProperty) == CFStringGetTypeID() )
			{
				std::array< char, 128 > buffer{};

				if ( CFStringGetCString(static_cast< CFStringRef >(UUIDProperty), buffer.data(), static_cast< CFIndex >(buffer.size()), kCFStringEncodingUTF8) )
				{
					m_OSInformation.machineUUID.assign(buffer.data());
				}
				else
				{
					std::cerr << "Unable to get the machine UUID !" "\n";
				}
			}
			else
			{
				std::cerr << "Unable to get the machine UUID !" "\n";
			}

			if ( UUIDProperty != nullptr )
			{
				CFRelease(UUIDProperty);
			}

			if ( ioRegistryRoot != MACH_PORT_NULL )
			{
				IOObjectRelease(ioRegistryRoot);
			}
		}

		return true;
	}

	bool
	SystemInfo::fetchGPUInformation () noexcept
	{
		/* NOTE: IOKit's accelerator services (owner decision 2026-10-15): the operating system's view, read without
		 * loading any graphics driver (Metal would), so a broken driver still leaves the GPUs named. One service per GPU:
		 * Apple Silicon (AGXAccelerator), Intel Macs (an Intel, AMD or NVIDIA accelerator under its PCI device). */
		m_GPUDevices.clear();

		/* NOTE: IOServiceGetMatchingServices() consumes the matching dictionary, even on failure. */
		CFMutableDictionaryRef matching = IOServiceMatching("IOAccelerator");

		if ( matching == nullptr )
		{
			return false;
		}

		io_iterator_t iteratorHandle = IO_OBJECT_NULL;

		if ( IOServiceGetMatchingServices(kIOMainPortDefault, matching, &iteratorHandle) != KERN_SUCCESS )
		{
			return false;
		}

		const IOObject iterator{iteratorHandle};

		for ( size_t index = 0; index < MaximumAccelerators; ++index )
		{
			const IOObject accelerator{IOIteratorNext(iterator.get())};

			if ( accelerator.get() == IO_OBJECT_NULL )
			{
				break;
			}

			if ( m_GPUDevices.full() )
			{
				TraceWarning{ClassId} << "More graphics adapters than the list holds (" << m_GPUDevices.size() << "): the next ones are not listed !";

				break;
			}

			m_GPUDevices.push_back(describeAccelerator(accelerator.get()));
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
		/* NOTE: _SC_AVPHYS_PAGES does not exist on macOS — query the Mach VM statistics
		 * instead. "Available" memory counts the free pages plus the inactive ones
		 * (cached pages the kernel reclaims on demand), matching what Activity Monitor
		 * reports as available. */
		vm_statistics64_data_t vmStats{};
		mach_msg_type_number_t infoCount = HOST_VM_INFO64_COUNT;

		if ( host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast< host_info64_t >(&vmStats), &infoCount) != KERN_SUCCESS )
		{
			/* Can't access? Callers treat 0 as "probe unavailable". */
			return 0;
		}

		const auto pageSize = static_cast< size_t >(sysconf(_SC_PAGE_SIZE));

		return (static_cast< size_t >(vmStats.free_count) + static_cast< size_t >(vmStats.inactive_count)) * pageSize;
	}

	size_t
	SystemInfo::getPeakRSS () noexcept
	{
		rusage rusage{};
		getrusage(RUSAGE_SELF, &rusage);

		return static_cast< size_t >(rusage.ru_maxrss);
	}

	size_t
	SystemInfo::getCurrentRSS () noexcept
	{
		mach_task_basic_info info{};
		mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

		if ( task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast< task_info_t >(&info), &infoCount) != KERN_SUCCESS )
		{
			/* Can't access? */
			return 0;
		}

		return static_cast< size_t >(info.resident_size);
	}

	std::filesystem::path
	SystemInfo::getRealApplicationDir () noexcept
	{
		char exePath[PATH_MAX];
		uint32_t len = sizeof(exePath);

		if ( _NSGetExecutablePath(exePath, &len) != 0 )
		{
			exePath[0] = '\0'; // buffer too small (!)
		}
		else
		{
			/* Resolve the symbolic links, if possible. NOTE: realpath() allocates with malloc(): owned until std::free(),
			 * by RAII (a lambda deleter: std::free is not an addressable function in C++20). Its result is shorter
			 * than PATH_MAX, so the copy stays terminated. */
			const auto freeDeleter = [] (char * pointer) noexcept {
				std::free(pointer);
			};
			const std::unique_ptr< char, decltype(freeDeleter) > canonicalPath{realpath(exePath, nullptr), freeDeleter};

			if ( canonicalPath != nullptr )
			{
				std::strncpy(exePath, canonicalPath.get(), len);
			}
		}

		std::string realPath{exePath};

		const auto position = realPath.find_last_of('/');

		return
			position == std::string::npos ?
			realPath :
			realPath.substr(0, position);
	}

	std::string
	SystemInfo::platformCPUBrandString () noexcept
	{
		std::array< char, 256 > brand{};
		size_t length = brand.size() - 1;

		if ( sysctlbyname("machdep.cpu.brand_string", brand.data(), &length, nullptr, 0) != 0 )
		{
			return {};
		}

		return std::string{brand.data()};
	}
}
