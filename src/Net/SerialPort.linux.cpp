/*
 * src/Net/SerialPort.linux.cpp
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

#include "SerialPort.hpp"

/* STL inclusions. */
#include <algorithm>
#include <charconv>
#include <optional>
#include <system_error>
#include <filesystem>
#include <fstream>

/* Third-party inclusions. */
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>

/* NOTE: an arbitrary baud rate needs the kernel's termios2 (TCSETS2 + BOTHER). <asm/termbits.h>,
 * which declares it, collides with <termios.h>, so the layout is mirrored here under our own name.
 * It matches asm-generic/termbits.h and has been stable for the whole 2.6/3.x/4.x/5.x/6.x line. */
#ifndef BOTHER
	#define BOTHER 0010000
#endif

namespace
{
	struct KernelTermios2
	{
		tcflag_t c_iflag;
		tcflag_t c_oflag;
		tcflag_t c_cflag;
		tcflag_t c_lflag;
		cc_t c_line;
		cc_t c_cc[19];
		speed_t c_ispeed;
		speed_t c_ospeed;
	};

	/* The system's TCGETS2 / TCSETS2 macros encode sizeof(struct termios2), a type we cannot
	 * include: the same request numbers are rebuilt from the mirrored layout, which has the
	 * same size by construction. */
	constexpr unsigned long KernelTCGETS2{_IOR('T', 0x2A, KernelTermios2)};
	constexpr unsigned long KernelTCSETS2{_IOW('T', 0x2B, KernelTermios2)};
}
#include <unistd.h>

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
	 * Port Enumeration
	 * ======================================================================= */

	/**
	 * @brief Reads the content of a sysfs attribute file.
	 * @param path The sysfs attribute path.
	 * @return std::string The attribute value (trimmed), or empty.
	 */
	static std::string
	readSysfsAttribute (const std::filesystem::path & path) noexcept
	{
		std::ifstream file(path);

		if ( !file.is_open() )
		{
			return "";
		}

		std::string value;
		std::getline(file, value);

		/* Trim trailing whitespace/newline. */
		while ( !value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ') )
		{
			value.pop_back();
		}

		return value;
	}

	/**
	 * @brief Reads a hexadecimal sysfs attribute as a uint16_t.
	 * @param path The sysfs attribute path.
	 * @return uint16_t The parsed value, or 0.
	 */
	static uint16_t
	readSysfsHex (const std::filesystem::path & path) noexcept
	{
		const auto str = readSysfsAttribute(path);

		if ( str.empty() )
		{
			return 0;
		}

		/* ⚠️ std::stoul throws on a non-hex or oversized value, and this function is noexcept
		 * inside a -fno-exceptions build: a driver exposing a malformed idVendor would have
		 * terminated the process. from_chars reports it as a value. */
		uint32_t value = 0;

		const auto * begin = str.data();
		const auto * end = str.data() + str.size();

		if ( std::from_chars(begin, end, value, 16).ec != std::errc{} || value > 0xFFFFU )
		{
			return 0;
		}

		return static_cast< uint16_t >(value);
	}

	std::vector< SerialPortInfo >
	SerialPort::listPorts () noexcept
	{
		std::vector< SerialPortInfo > ports;

		const std::filesystem::path sysClassTty{"/sys/class/tty"};

		/* ⚠️ Every std::filesystem call here takes its error_code overload. The throwing ones
		 * terminate the process in a noexcept function, and this whole walk races with the user:
		 * a USB adapter unplugged between the iteration and canonical() leaves a dangling symlink
		 * that would have killed the application. */
		std::error_code error;

		if ( !std::filesystem::exists(sysClassTty, error) || error )
		{
			return ports;
		}

		std::filesystem::directory_iterator iterator{sysClassTty, error};

		if ( error )
		{
			return ports;
		}

		for ( const auto & entry : iterator )
		{
			const auto devicePath = entry.path() / "device";

			/* Only include entries that have a device symlink (filters out virtual ttys). */
			if ( !std::filesystem::exists(devicePath, error) || error )
			{
				error.clear();

				continue;
			}

			/* Check if it's a USB serial device by looking for a parent with idVendor. */
			auto subsystem = readSysfsAttribute(devicePath / "subsystem");

			/* Resolve the subsystem symlink to get the subsystem name. */
			if ( std::filesystem::is_symlink(devicePath / "subsystem", error) && !error )
			{
				const auto target = std::filesystem::read_symlink(devicePath / "subsystem", error);

				if ( !error )
				{
					subsystem = target.filename().string();
				}
			}

			error.clear();

			SerialPortInfo info;
			info.path = "/dev/" + entry.path().filename().string();

			/* Walk up the device tree to find USB info. */
			auto usbDevicePath = std::filesystem::canonical(devicePath, error);

			if ( error )
			{
				/* The device vanished mid-walk: keep the port, drop the USB details. */
				error.clear();

				ports.emplace_back(std::move(info));

				continue;
			}

			for ( int depth = 0; depth < 5; depth++ )
			{
				if ( std::filesystem::exists(usbDevicePath / "idVendor", error) && !error )
				{
					info.vendorId = readSysfsHex(usbDevicePath / "idVendor");
					info.productId = readSysfsHex(usbDevicePath / "idProduct");
					info.manufacturer = readSysfsAttribute(usbDevicePath / "manufacturer");
					info.serialNumber = readSysfsAttribute(usbDevicePath / "serial");

					break;
				}

				usbDevicePath = usbDevicePath.parent_path();

				if ( usbDevicePath == "/" )
				{
					break;
				}
			}

			ports.emplace_back(std::move(info));
		}

		/* Sort by path for consistent ordering. */
		std::sort(ports.begin(), ports.end(), [] (const auto & a, const auto & b) {
			return a.path < b.path;
		});

		return ports;
	}

	/* =========================================================================
	 * Port Baud Rate Mapping
	 * ======================================================================= */

	/**
	 * @brief Converts a numeric baud rate to a POSIX termios speed constant.
	 * @param baudRate The baud rate value.
	 * @return speed_t The termios constant, or B9600 as fallback.
	 */
	/**
	 * @brief Converts a numeric baud rate to a POSIX termios speed constant.
	 * @note ⚠️ Returns nothing for a rate POSIX has no constant for — 250000, the default of
	 * Marlin-based 3D printers, is one of them. Falling back to B9600 and reporting success made
	 * the port open at the wrong speed with every reply unreadable and no diagnostic; such a rate
	 * goes through the BOTHER path below instead.
	 * @param baudRate The baud rate value.
	 * @return std::optional< speed_t >
	 */
	static std::optional< speed_t >
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
			case 460800 : return B460800;
			case 500000 : return B500000;
			case 576000 : return B576000;
			case 921600 : return B921600;
			case 1000000 : return B1000000;
			case 1152000 : return B1152000;
			case 1500000 : return B1500000;
			case 2000000 : return B2000000;
			case 2500000 : return B2500000;
			case 3000000 : return B3000000;
			case 3500000 : return B3500000;
			case 4000000 : return B4000000;

			default :
				return std::nullopt;
		}
	}

	/**
	 * @brief Applies an arbitrary baud rate the termios constants cannot express (Linux only).
	 * @note TCSETS2 + BOTHER take the rate as a number in c_ispeed / c_ospeed. This is how
	 * 250000 (Marlin), 76800 and the odd rates of some USB adapters are reached.
	 * @param fd The open descriptor.
	 * @param baudRate The wanted rate.
	 * @return bool
	 */
	static bool
	applyCustomBaudRate (int fd, uint32_t baudRate) noexcept
	{
		KernelTermios2 tty2{};

		if ( ::ioctl(fd, KernelTCGETS2, &tty2) != 0 )
		{
			return false;
		}

		tty2.c_cflag &= ~CBAUD;
		tty2.c_cflag |= BOTHER;
		tty2.c_ispeed = baudRate;
		tty2.c_ospeed = baudRate;

		return ::ioctl(fd, KernelTCSETS2, &tty2) == 0;
	}

	/* =========================================================================
	 * Open / Close / State
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

		/* Configure the port with termios. */
		struct termios tty{};

		if ( tcgetattr(m_fd, &tty) != 0 )
		{
			this->close();

			return false;
		}

		/* Set baud rate. A rate with no termios constant is applied after tcsetattr(), through
		 * the BOTHER path; the standard constant is used meanwhile. */
		const auto baudConstant = toBaudConstant(config.baudRate);

		cfsetispeed(&tty, baudConstant.value_or(B9600));
		cfsetospeed(&tty, baudConstant.value_or(B9600));

		/* Data bits. */
		tty.c_cflag &= ~CSIZE;

		switch ( config.dataBits )
		{
			case 5 : tty.c_cflag |= CS5; break;
			case 6 : tty.c_cflag |= CS6; break;
			case 7 : tty.c_cflag |= CS7; break;
			default : tty.c_cflag |= CS8; break;
		}

		/* Stop bits. */
		if ( config.stopBits == 2 )
		{
			tty.c_cflag |= CSTOPB;
		}
		else
		{
			tty.c_cflag &= ~CSTOPB;
		}

		/* Parity. */
		switch ( config.parity )
		{
			case 'E' :
			case 'e' :
				tty.c_cflag |= PARENB;
				tty.c_cflag &= ~PARODD;
				break;

			case 'O' :
			case 'o' :
				tty.c_cflag |= PARENB;
				tty.c_cflag |= PARODD;
				break;

			default :
				tty.c_cflag &= ~PARENB;
				break;
		}

		/* Hardware flow control. */
		if ( config.rtscts )
		{
			tty.c_cflag |= CRTSCTS;
		}
		else
		{
			tty.c_cflag &= ~CRTSCTS;
		}

		/* Software flow control. */
		if ( config.xon || config.xoff )
		{
			tty.c_iflag |= (IXON | IXOFF | IXANY);
		}
		else
		{
			tty.c_iflag &= ~(IXON | IXOFF | IXANY);
		}

		/* Raw mode (no canonical processing, no echo, no signals). */
		tty.c_cflag |= (CLOCAL | CREAD);
		tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
		tty.c_oflag &= ~OPOST;

		/* Non-blocking read: return immediately with available data. */
		tty.c_cc[VMIN] = 0;
		tty.c_cc[VTIME] = 0;

		if ( tcsetattr(m_fd, TCSANOW, &tty) != 0 )
		{
			this->close();

			return false;
		}

		if ( !baudConstant.has_value() && !applyCustomBaudRate(m_fd, config.baudRate) )
		{
			/* The adapter cannot do that rate: say so instead of running at 9600 silently. */
			this->close();

			return false;
		}

		/* Claim the port: a second process opening the same adapter mid-print is a corrupted
		 * stream nobody can diagnose. Best effort — not every driver implements it. */
		static_cast< void >(::ioctl(m_fd, TIOCEXCL));

		/* Flush any pending data. */
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
	 * Read / Write
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
				return result; /* 0 = timeout, -1 = error. */
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
