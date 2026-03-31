/*
 * src/Net/SerialPort.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <vector>

namespace EmEn::Net
{
	/**
	 * @brief Describes a serial port available on the system.
	 */
	struct SerialPortInfo
	{
		std::string path;            /* Device path (e.g., "/dev/ttyUSB0", "COM3"). */
		std::string manufacturer;    /* Manufacturer name (if available). */
		std::string serialNumber;    /* Serial number (if available). */
		std::string pnpId;           /* Plug-and-Play ID (if available). */
		std::string locationId;      /* Location identifier (if available). */
		uint16_t vendorId{0};        /* USB Vendor ID. */
		uint16_t productId{0};       /* USB Product ID. */
	};

	/**
	 * @brief Configuration for opening a serial port.
	 */
	struct SerialPortConfig
	{
		uint32_t baudRate{9600};
		uint8_t dataBits{8};         /* 5, 6, 7, or 8. */
		uint8_t stopBits{1};         /* 1 or 2. */
		char parity{'N'};            /* 'N' (none), 'E' (even), 'O' (odd). */
		bool rtscts{false};          /* Hardware flow control. */
		bool xon{false};             /* Software flow control (XON). */
		bool xoff{false};            /* Software flow control (XOFF). */
	};

	/**
	 * @brief Cross-platform serial port for reading/writing data.
	 * @note Uses termios on Linux/macOS, Win32 CreateFile/DCB on Windows.
	 */
	class SerialPort final
	{
		public:

			SerialPort () noexcept = default;

			/** @brief Destructor closes the port if still open. */
			~SerialPort () noexcept;

			/** @brief Non-copyable. */
			SerialPort (const SerialPort &) = delete;
			SerialPort & operator= (const SerialPort &) = delete;

			/** @brief Movable. */
			SerialPort (SerialPort && other) noexcept;
			SerialPort & operator= (SerialPort && other) noexcept;

			/**
			 * @brief Lists all available serial ports on the system.
			 * @return std::vector< SerialPortInfo >
			 */
			[[nodiscard]]
			static std::vector< SerialPortInfo > listPorts () noexcept;

			/**
			 * @brief Opens a serial port with the given configuration.
			 * @param path The device path.
			 * @param config The port configuration.
			 * @return bool True if the port was opened successfully.
			 */
			bool open (const std::string & path, const SerialPortConfig & config = {}) noexcept;

			/**
			 * @brief Closes the serial port.
			 * @return void
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the port is currently open.
			 * @return bool
			 */
			[[nodiscard]]
			bool isOpen () const noexcept;

			/**
			 * @brief Writes data to the serial port.
			 * @param data The data to write.
			 * @param length The number of bytes to write.
			 * @return int Number of bytes written, or -1 on error.
			 */
			int write (const void * data, size_t length) noexcept;

			/**
			 * @brief Writes a string to the serial port.
			 * @param data The string to write.
			 * @return int Number of bytes written, or -1 on error.
			 */
			int write (const std::string & data) noexcept;

			/**
			 * @brief Reads available data from the serial port.
			 * @param buffer The buffer to read into.
			 * @param maxLength Maximum number of bytes to read.
			 * @param timeoutMs Read timeout in milliseconds (0 = non-blocking).
			 * @return int Number of bytes read, 0 if no data available, or -1 on error.
			 */
			int read (void * buffer, size_t maxLength, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Reads available data as a string.
			 * @param maxLength Maximum number of bytes to read.
			 * @param timeoutMs Read timeout in milliseconds (0 = non-blocking).
			 * @return std::string The data read (may be empty if no data available).
			 */
			[[nodiscard]]
			std::string readString (size_t maxLength = 4096, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Returns the path of the currently opened port.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			path () const noexcept
			{
				return m_path;
			}

		private:

			std::string m_path;

#ifdef _WIN32
			void * m_handle{nullptr};  /* HANDLE (void*) to avoid including Windows.h. */
#else
			int m_fd{-1};
#endif
	};
}
