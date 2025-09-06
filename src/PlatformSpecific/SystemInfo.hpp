/*
 * src/PlatformSpecific/SystemInfo.hpp
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
#include <ostream>
#include <sstream>
#include <vector>
#include <filesystem>

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief The system info class. This will gather system information.
	 */
	class SystemInfo final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SystemInfo"};

			/** 
			 * @brief Constructs a system info structure.
			 */
			SystemInfo () noexcept
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

			/**
			 * @brief Declares a GPU device.
			 * @param device A reference to a device struct.
			 * @return void
			 */
			void
			declareGPUDevice (const GPUDevice & device) noexcept
			{
				m_GPUDevices.emplace_back(device);
			}

			/**
			 * @brief Sets a list of devices.
			 * @note This will replace the existing devices.
			 * @param devices A reference to a GPU device list.
			 * @return void
			 */
			void
			setGPUDevices (const std::vector< GPUDevice > & devices) noexcept
			{
				m_GPUDevices = devices;
			}

			/**
			 * @brief Returns whether the current user information has been reached.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			informationFound () const noexcept
			{
				return m_informationFound;
			}

			/**
			 * @brief Returns the operating system information structure.
			 * @return OperatingSystem &
			 */
			[[nodiscard]]
			const OperatingSystem &
			getOSInformation () const noexcept
			{
				return m_OSInformation;
			}

			/**
			 * @brief Returns the CPU information structure.
			 * @return const CPU &
			 */
			[[nodiscard]]
			const CPU &
			getCPUInformation () const noexcept
			{
				return m_CPUInformation;
			}

			/**
			 * @brief Returns a list of available GPU devices.
			 * @return const std::vector< GPUDevice > &
			 */
			[[nodiscard]]
			const std::vector< GPUDevice > &
			getGPUDeviceInformation () const noexcept
			{
				return m_GPUDevices;
			}

			/**
			 * @brief Returns the system memory information structure.
			 * @return SystemMemory
			 */
			[[nodiscard]]
			static SystemMemory getSystemMemoryInformation () noexcept;

			/**
			 * @brief Returns the real path where the application is.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			static std::filesystem::path getRealApplicationDir () noexcept;

		private:

			/**
			 * @brief Fetches OS information on the system.
			 * @return bool
			 */
			bool fetchOSInformation () noexcept;

			/**
			 * @brief Fetches CPU information on the system.
			 * @return bool
			 */
			bool fetchCPUInformation () noexcept;

			/**
			 * @brief Returns the total memory available on the system.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getTotalMemory () noexcept;

			/**
			 * @brief Returns the free amount of memory on the system.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getFreeMemory () noexcept;

			/**
			 * @brief Returns the peak (maximum so far) resident set size (physical memory use) measured in bytes, or zero if the value cannot be determined on this OS.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getPeakRSS () noexcept;

			/**
			 * @brief Returns the current resident set size (physical memory use) measured in bytes, or zero if the value cannot be determined on this OS.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getCurrentRSS () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const SystemInfo & obj);

			OperatingSystem m_OSInformation{};
			CPU m_CPUInformation{};
			std::vector< GPUDevice > m_GPUDevices;
			bool m_informationFound{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const SystemInfo & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
