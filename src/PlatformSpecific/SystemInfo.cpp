/*
 * src/PlatformSpecific/SystemInfo.cpp
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

#include "SystemInfo.hpp"

/* Project configuration files. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>
#include <sstream>
#include <cstddef>

/* Third-party inclusions. */
#ifdef CPUFEATURES_ENABLED
	#if IS_ARM_ARCH
		#include "cpu_features/cpuinfo_aarch64.h"
	#else
		#include "cpu_features/cpuinfo_x86.h"
	#endif
#endif
#ifdef HWLOC_ENABLED
	#include "hwloc.h"
#endif

namespace EmEn::PlatformSpecific
{
	SystemInfo::SystemInfo () noexcept
	{
		if ( this->fetchOSInformation() )
		{
			m_informationFound = true;
		}

		if ( this->fetchCPUInformation() )
		{
			m_informationFound = true;
		}
	}

	bool
	SystemInfo::fetchCPUInformation () noexcept
	{
#ifdef CPUFEATURES_ENABLED
	#if IS_ARM_ARCH
		const auto cpuInfo = cpu_features::GetAarch64Info();

        m_CPUInformation.vendorName = "Apple";
        m_CPUInformation.vendorID = cpuInfo.implementer;
        m_CPUInformation.deviceName = "Apple M?";
        m_CPUInformation.deviceID = cpuInfo.variant;
        m_CPUInformation.family = -1;
        m_CPUInformation.model = cpuInfo.part;
        m_CPUInformation.stepping = cpuInfo.revision;
	#else
		const auto cpuInfo = cpu_features::GetX86Info();

		m_CPUInformation.vendorName = cpuInfo.vendor;
		m_CPUInformation.vendorID = 0;
		m_CPUInformation.deviceName = cpuInfo.brand_string;
		m_CPUInformation.deviceID = 0;
		m_CPUInformation.family = cpuInfo.family;
		m_CPUInformation.model = cpuInfo.model;
		m_CPUInformation.stepping = cpuInfo.stepping;
	#endif
#endif

#ifdef HWLOC_ENABLED
		{
			hwloc_topology_t topology = nullptr;

			hwloc_topology_init(&topology);
			hwloc_topology_load(topology);

			/* Get the number of logical cores (PU) */
			{
				const auto depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);

				if ( depth != HWLOC_TYPE_DEPTH_UNKNOWN )
				{
					m_CPUInformation.logicalCores = hwloc_get_nbobjs_by_depth(topology, depth);
				}
			}

			/* Get the number of physical cores (CORE) */
			{
				const auto depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);

				if ( depth != HWLOC_TYPE_DEPTH_UNKNOWN )
				{
					m_CPUInformation.physicalCores = hwloc_get_nbobjs_by_depth(topology, depth);
				}
			}

			hwloc_topology_destroy(topology);
		}
#endif

		return m_CPUInformation.physicalCores > 0;
	}

	SystemMemory
	SystemInfo::getSystemMemoryInformation () noexcept
	{
		SystemMemory memory{};
		memory.total = SystemInfo::getTotalMemory();
		memory.free = SystemInfo::getFreeMemory();
		memory.used = memory.total - memory.free;
		memory.currentUsage = SystemInfo::getCurrentRSS();
		memory.peakUsage = SystemInfo::getPeakRSS();

		return memory;
	}

	std::ostream &
	operator<< (std::ostream & out, const SystemInfo & obj)
	{
		if ( !obj.informationFound() )
		{
			return out << "System information : Not found !" "\n";
		}

		constexpr auto MegaByteDivisor{1048576.0F};
		constexpr auto GigaByteDivisor{1073741824.0F};

		const auto memory = SystemInfo::getSystemMemoryInformation();

		out <<
			"System information :" "\n"

			"Operating System :" "\n"
			" - System name : " << obj.m_OSInformation.systemName << "\n"
			" - System version : " << obj.m_OSInformation.systemVersion << "\n"
			" - Computer name : " << obj.m_OSInformation.computerName << "\n"
			" - Machine UUID : " << obj.m_OSInformation.machineUUID << "\n\n"

			"CPU :" "\n"
			" - Vendor : " << obj.m_CPUInformation.vendorName << " (" << obj.m_CPUInformation.vendorID << ")" << "\n"
			" - Name : " << obj.m_CPUInformation.deviceName << "\n"
			" - Family : " << obj.m_CPUInformation.family << "\n"
			" - Model : " << obj.m_CPUInformation.model << "\n"
			" - Stepping : " << obj.m_CPUInformation.stepping << "\n"
			" - Physical cores : " << obj.m_CPUInformation.physicalCores << "\n"
			" - Logical cores : " << obj.m_CPUInformation.logicalCores << "\n"
			" - Efficiency cores : " << obj.m_CPUInformation.efficiencyCores << "\n"
			" - Performance cores : " << obj.m_CPUInformation.performanceCores << "\n\n"

			"System memory :" "\n"
			" - Total memory : " << static_cast< float >(memory.total) / GigaByteDivisor << " GiB" "\n"
			" - Free memory : " << static_cast< float >(memory.free) / GigaByteDivisor << " GiB" "\n"
			" - Used memory : " << static_cast< float >(memory.used) / GigaByteDivisor << " GiB" "\n"
			" - Current memory usage : " << static_cast< float >(memory.currentUsage) / MegaByteDivisor << " MiB" "\n"
			" - Peak memory usage : " << static_cast< float >(memory.peakUsage) / MegaByteDivisor << " MiB" "\n";

		for ( size_t index = 0; index < obj.m_GPUDevices.size(); ++index )
		{
			const auto & GPUDevice = obj.m_GPUDevices[index];

			out <<
				"GPU device #" << index << " :" "\n"
				" - Vendor : " << GPUDevice.vendorName << " (" << GPUDevice.vendorID << ")" << "\n"
				" - Name : " << GPUDevice.deviceName << "\n"
				" - Device type : " << to_string(GPUDevice.deviceType) << "\n"
				" - Manufacturer : " << GPUDevice.manufacturerName << " (" << GPUDevice.manufacturerID << ")" << "\n"
				" - Is dedicated memory : " << ( GPUDevice.dedicatedMemory ? "Yes" : "No" ) << "\n"
				" - Available memory : " << static_cast< float >(GPUDevice.availableMemory) / MegaByteDivisor << " GiB" << "\n"
				" - BUS : " << GPUDevice.bus << "\n\n";
		}

		return out;
	}

	std::string
	to_string (const SystemInfo & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
