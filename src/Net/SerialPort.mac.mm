/*
 * src/Net/SerialPort.mac.mm
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

#include "SerialPort.hpp"

/* STL inclusions. */
#include <algorithm>

/* POSIX inclusions (shared with Linux for I/O). */
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

/* macOS IOKit for port enumeration. */
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/serial/IOSerialKeys.h>
#import <IOKit/usb/IOUSBLib.h>

namespace EmEn::Net
{
	/* =========================================================================
	 * Lifecycle
	 * ======================================================================= */

	SerialPort::~SerialPort () noexcept
	{
		this->close();
	}

	SerialPort::SerialPort (SerialPort && other) noexcept
		: m_path(std::move(other.m_path)),
		m_fd(other.m_fd)
	{
		other.m_fd = -1;
	}

	SerialPort &
	SerialPort::operator= (SerialPort && other) noexcept
	{
		if ( this != &other )
		{
			this->close();

			m_path = std::move(other.m_path);
			m_fd = other.m_fd;
			other.m_fd = -1;
		}

		return *this;
	}

	/* =========================================================================
	 * Port Enumeration (IOKit)
	 * ======================================================================= */

	/**
	 * @brief Reads a string property from an IOKit registry entry.
	 * @param service The IOKit service.
	 * @param key The property key.
	 * @return std::string The value, or empty.
	 */
	static std::string
	getIOKitStringProperty (io_object_t service, CFStringRef key) noexcept
	{
		auto ref = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);

		if ( ref == nullptr )
		{
			return "";
		}

		std::string result;

		if ( CFGetTypeID(ref) == CFStringGetTypeID() )
		{
			char buffer[256];

			if ( CFStringGetCString(static_cast< CFStringRef >(ref), buffer, sizeof(buffer), kCFStringEncodingUTF8) )
			{
				result = buffer;
			}
		}

		CFRelease(ref);

		return result;
	}

	/**
	 * @brief Reads an integer property from an IOKit registry entry.
	 * @param service The IOKit service.
	 * @param key The property key.
	 * @return uint16_t The value, or 0.
	 */
	static uint16_t
	getIOKitIntProperty (io_object_t service, CFStringRef key) noexcept
	{
		auto ref = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);

		if ( ref == nullptr )
		{
			return 0;
		}

		uint16_t result = 0;

		if ( CFGetTypeID(ref) == CFNumberGetTypeID() )
		{
			int value = 0;
			CFNumberGetValue(static_cast< CFNumberRef >(ref), kCFNumberIntType, &value);
			result = static_cast< uint16_t >(value);
		}

		CFRelease(ref);

		return result;
	}

	/**
	 * @brief Walks up the IOKit registry tree to find USB parent properties.
	 * @param service The starting IOKit service.
	 * @param info The SerialPortInfo to populate.
	 */
	static void
	findUSBParentProperties (io_object_t service, SerialPortInfo & info) noexcept
	{
		io_object_t parent = 0;
		auto current = service;

		IOObjectRetain(current);

		for ( int depth = 0; depth < 10; depth++ )
		{
			auto vid = getIOKitIntProperty(current, CFSTR("idVendor"));

			if ( vid != 0 )
			{
				info.vendorId = vid;
				info.productId = getIOKitIntProperty(current, CFSTR("idProduct"));
				info.manufacturer = getIOKitStringProperty(current, CFSTR("USB Vendor Name"));
				info.serialNumber = getIOKitStringProperty(current, CFSTR("USB Serial Number"));

				IOObjectRelease(current);

				return;
			}

			if ( IORegistryEntryGetParentEntry(current, kIOServicePlane, &parent) != KERN_SUCCESS )
			{
				IOObjectRelease(current);

				return;
			}

			IOObjectRelease(current);
			current = parent;
		}

		IOObjectRelease(current);
	}

	std::vector< SerialPortInfo >
	SerialPort::listPorts () noexcept
	{
		std::vector< SerialPortInfo > ports;

		@autoreleasepool
		{
			auto matching = IOServiceMatching(kIOSerialBSDServiceValue);

			if ( matching == nullptr )
			{
				return ports;
			}

			CFDictionarySetValue(matching, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

			io_iterator_t iterator = 0;

			if ( IOServiceGetMatchingServices(kIOMainPortDefault, matching, &iterator) != KERN_SUCCESS )
			{
				return ports;
			}

			io_object_t service = 0;

			while ( (service = IOIteratorNext(iterator)) != 0 )
			{
				SerialPortInfo info;

				info.path = getIOKitStringProperty(service, CFSTR(kIOCalloutDeviceKey));

				if ( info.path.empty() )
				{
					info.path = getIOKitStringProperty(service, CFSTR(kIODialinDeviceKey));
				}

				if ( !info.path.empty() )
				{
					findUSBParentProperties(service, info);

					ports.emplace_back(std::move(info));
				}

				IOObjectRelease(service);
			}

			IOObjectRelease(iterator);
		}

		std::sort(ports.begin(), ports.end(), [] (const auto & a, const auto & b) {
			return a.path < b.path;
		});

		return ports;
	}

	/* =========================================================================
	 * Baud Rate Mapping (POSIX - shared with Linux)
	 * ======================================================================= */

	static speed_t
	toBaudConstant (uint32_t baudRate) noexcept
	{
		switch ( baudRate )
		{
			case 50 : return B50;
			case 75 : return B75;
			case 110 : return B110;
			case 134 : return B134;
			case 150 : return B150;
			case 200 : return B200;
			case 300 : return B300;
			case 600 : return B600;
			case 1200 : return B1200;
			case 1800 : return B1800;
			case 2400 : return B2400;
			case 4800 : return B4800;
			case 9600 : return B9600;
			case 19200 : return B19200;
			case 38400 : return B38400;
			case 57600 : return B57600;
			case 115200 : return B115200;
			case 230400 : return B230400;

			default :
				return B9600;
		}
	}

	/* =========================================================================
	 * Open / Close / State (POSIX - shared with Linux)
	 * ======================================================================= */

	bool
	SerialPort::open (const std::string & path, const SerialPortConfig & config) noexcept
	{
		if ( this->isOpen() )
		{
			this->close();
		}

		m_fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

		if ( m_fd < 0 )
		{
			return false;
		}

		struct termios tty{};

		if ( tcgetattr(m_fd, &tty) != 0 )
		{
			this->close();

			return false;
		}

		const auto baudConstant = toBaudConstant(config.baudRate);
		cfsetispeed(&tty, baudConstant);
		cfsetospeed(&tty, baudConstant);

		tty.c_cflag &= ~CSIZE;

		switch ( config.dataBits )
		{
			case 5 : tty.c_cflag |= CS5; break;
			case 6 : tty.c_cflag |= CS6; break;
			case 7 : tty.c_cflag |= CS7; break;
			default : tty.c_cflag |= CS8; break;
		}

		if ( config.stopBits == 2 )
		{
			tty.c_cflag |= CSTOPB;
		}
		else
		{
			tty.c_cflag &= ~CSTOPB;
		}

		switch ( config.parity )
		{
			case 'E' : case 'e' :
				tty.c_cflag |= PARENB;
				tty.c_cflag &= ~PARODD;
				break;

			case 'O' : case 'o' :
				tty.c_cflag |= PARENB;
				tty.c_cflag |= PARODD;
				break;

			default :
				tty.c_cflag &= ~PARENB;
				break;
		}

		if ( config.rtscts )
		{
			tty.c_cflag |= CRTSCTS;
		}
		else
		{
			tty.c_cflag &= ~CRTSCTS;
		}

		if ( config.xon || config.xoff )
		{
			tty.c_iflag |= (IXON | IXOFF | IXANY);
		}
		else
		{
			tty.c_iflag &= ~(IXON | IXOFF | IXANY);
		}

		tty.c_cflag |= (CLOCAL | CREAD);
		tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
		tty.c_oflag &= ~OPOST;

		tty.c_cc[VMIN] = 0;
		tty.c_cc[VTIME] = 0;

		if ( tcsetattr(m_fd, TCSANOW, &tty) != 0 )
		{
			this->close();

			return false;
		}

		tcflush(m_fd, TCIOFLUSH);

		m_path = path;

		return true;
	}

	void
	SerialPort::close () noexcept
	{
		if ( m_fd >= 0 )
		{
			::close(m_fd);
			m_fd = -1;
			m_path.clear();
		}
	}

	bool
	SerialPort::isOpen () const noexcept
	{
		return m_fd >= 0;
	}

	/* =========================================================================
	 * Read / Write (POSIX - shared with Linux)
	 * ======================================================================= */

	int
	SerialPort::write (const void * data, size_t length) noexcept
	{
		if ( !this->isOpen() || data == nullptr || length == 0 )
		{
			return -1;
		}

		return static_cast< int >(::write(m_fd, data, length));
	}

	int
	SerialPort::write (const std::string & data) noexcept
	{
		return this->write(data.data(), data.size());
	}

	int
	SerialPort::read (void * buffer, size_t maxLength, uint32_t timeoutMs) noexcept
	{
		if ( !this->isOpen() || buffer == nullptr || maxLength == 0 )
		{
			return -1;
		}

		if ( timeoutMs > 0 )
		{
			fd_set readFds;
			FD_ZERO(&readFds);
			FD_SET(m_fd, &readFds);

			struct timeval tv{};
			tv.tv_sec = static_cast< long >(timeoutMs / 1000);
			tv.tv_usec = static_cast< long >((timeoutMs % 1000) * 1000);

			const auto result = select(m_fd + 1, &readFds, nullptr, nullptr, &tv);

			if ( result <= 0 )
			{
				return result;
			}
		}

		return static_cast< int >(::read(m_fd, buffer, maxLength));
	}

	std::string
	SerialPort::readString (size_t maxLength, uint32_t timeoutMs) noexcept
	{
		std::string result;
		result.resize(maxLength);

		const auto bytesRead = this->read(result.data(), maxLength, timeoutMs);

		if ( bytesRead <= 0 )
		{
			return "";
		}

		result.resize(static_cast< size_t >(bytesRead));

		return result;
	}
}
