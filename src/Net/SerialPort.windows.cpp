/*
 * src/Net/SerialPort.windows.cpp
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

/* Windows inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>

#pragma comment(lib, "SetupAPI.lib")

namespace EmEn::Net
{
	static constexpr auto InvalidHandle = INVALID_HANDLE_VALUE;

	/* =========================================================================
	 * Lifecycle
	 * ======================================================================= */

	SerialPort::~SerialPort () noexcept
	{
		this->close();
	}

	SerialPort::SerialPort (SerialPort && other) noexcept
		: m_path(std::move(other.m_path)),
		m_handle(other.m_handle)
	{
		other.m_handle = nullptr;
	}

	SerialPort &
	SerialPort::operator= (SerialPort && other) noexcept
	{
		if ( this != &other )
		{
			this->close();

			m_path = std::move(other.m_path);
			m_handle = other.m_handle;
			other.m_handle = nullptr;
		}

		return *this;
	}

	/* =========================================================================
	 * Port Enumeration
	 * ======================================================================= */

	std::vector< SerialPortInfo >
	SerialPort::listPorts () noexcept
	{
		std::vector< SerialPortInfo > ports;

		auto * devInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);

		if ( devInfo == INVALID_HANDLE_VALUE )
		{
			return ports;
		}

		SP_DEVINFO_DATA devInfoData{};
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		for ( DWORD index = 0; SetupDiEnumDeviceInfo(devInfo, index, &devInfoData); index++ )
		{
			/* Get the port name (COM1, COM3, etc.) from the registry. */
			HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

			if ( hKey == INVALID_HANDLE_VALUE )
			{
				continue;
			}

			char portName[256]{};
			DWORD portNameSize = sizeof(portName);
			DWORD type = 0;

			if ( RegQueryValueExA(hKey, "PortName", nullptr, &type, reinterpret_cast< LPBYTE >(portName), &portNameSize) != ERROR_SUCCESS )
			{
				RegCloseKey(hKey);

				continue;
			}

			RegCloseKey(hKey);

			/* Skip non-COM ports (e.g., LPT). */
			if ( std::string(portName).find("COM") == std::string::npos )
			{
				continue;
			}

			SerialPortInfo info;
			info.path = portName;

			/* Get the friendly name for manufacturer info. */
			char friendlyName[256]{};
			DWORD friendlyNameSize = sizeof(friendlyName);

			if ( SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData, SPDRP_FRIENDLYNAME,
				nullptr, reinterpret_cast< PBYTE >(friendlyName), friendlyNameSize, nullptr) )
			{
				info.manufacturer = friendlyName;
			}

			/* Get hardware IDs for VID/PID. */
			char hardwareId[512]{};
			DWORD hardwareIdSize = sizeof(hardwareId);

			if ( SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData, SPDRP_HARDWAREID,
				nullptr, reinterpret_cast< PBYTE >(hardwareId), hardwareIdSize, nullptr) )
			{
				const std::string hwId(hardwareId);

				/* Parse VID and PID from strings like "USB\VID_2341&PID_0043". */
				if ( auto vidPos = hwId.find("VID_"); vidPos != std::string::npos )
				{
					info.vendorId = static_cast< uint16_t >(std::stoul(hwId.substr(vidPos + 4, 4), nullptr, 16));
				}

				if ( auto pidPos = hwId.find("PID_"); pidPos != std::string::npos )
				{
					info.productId = static_cast< uint16_t >(std::stoul(hwId.substr(pidPos + 4, 4), nullptr, 16));
				}
			}

			ports.emplace_back(std::move(info));
		}

		SetupDiDestroyDeviceInfoList(devInfo);

		std::sort(ports.begin(), ports.end(), [] (const auto & a, const auto & b) {
			return a.path < b.path;
		});

		return ports;
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

		/* Windows requires \\.\COMxx prefix for port numbers >= 10. */
		const auto devicePath = (path.find("\\\\.\\") == 0) ? path : "\\\\.\\" + path;

		m_handle = CreateFileA(
			devicePath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr
		);

		if ( m_handle == InvalidHandle )
		{
			m_handle = nullptr;

			return false;
		}

		/* Configure DCB. */
		DCB dcb{};
		dcb.DCBlength = sizeof(DCB);

		if ( !GetCommState(m_handle, &dcb) )
		{
			this->close();

			return false;
		}

		dcb.BaudRate = config.baudRate;
		dcb.ByteSize = config.dataBits;
		dcb.StopBits = (config.stopBits == 2) ? TWOSTOPBITS : ONESTOPBIT;

		switch ( config.parity )
		{
			case 'E' : case 'e' : dcb.Parity = EVENPARITY; break;
			case 'O' : case 'o' : dcb.Parity = ODDPARITY; break;
			default : dcb.Parity = NOPARITY; break;
		}

		dcb.fRtsControl = config.rtscts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fOutxCtsFlow = config.rtscts ? TRUE : FALSE;
		dcb.fInX = config.xon ? TRUE : FALSE;
		dcb.fOutX = config.xoff ? TRUE : FALSE;
		dcb.fBinary = TRUE;

		if ( !SetCommState(m_handle, &dcb) )
		{
			this->close();

			return false;
		}

		/* Set timeouts for non-blocking reads. */
		COMMTIMEOUTS timeouts{};
		timeouts.ReadIntervalTimeout = MAXDWORD;
		timeouts.ReadTotalTimeoutMultiplier = 0;
		timeouts.ReadTotalTimeoutConstant = 0;
		timeouts.WriteTotalTimeoutMultiplier = 0;
		timeouts.WriteTotalTimeoutConstant = 0;

		SetCommTimeouts(m_handle, &timeouts);

		/* Flush buffers. */
		PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

		m_path = path;

		return true;
	}

	void
	SerialPort::close () noexcept
	{
		if ( m_handle != nullptr )
		{
			CloseHandle(m_handle);
			m_handle = nullptr;
			m_path.clear();
		}
	}

	bool
	SerialPort::isOpen () const noexcept
	{
		return m_handle != nullptr;
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

		DWORD bytesWritten = 0;

		if ( !WriteFile(m_handle, data, static_cast< DWORD >(length), &bytesWritten, nullptr) )
		{
			return -1;
		}

		return static_cast< int >(bytesWritten);
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

		/* Update timeouts if a blocking read was requested. */
		if ( timeoutMs > 0 )
		{
			COMMTIMEOUTS timeouts{};
			timeouts.ReadIntervalTimeout = 0;
			timeouts.ReadTotalTimeoutMultiplier = 0;
			timeouts.ReadTotalTimeoutConstant = timeoutMs;
			timeouts.WriteTotalTimeoutMultiplier = 0;
			timeouts.WriteTotalTimeoutConstant = 0;

			SetCommTimeouts(m_handle, &timeouts);
		}

		DWORD bytesRead = 0;

		if ( !ReadFile(m_handle, buffer, static_cast< DWORD >(maxLength), &bytesRead, nullptr) )
		{
			return -1;
		}

		/* Restore non-blocking timeouts. */
		if ( timeoutMs > 0 )
		{
			COMMTIMEOUTS timeouts{};
			timeouts.ReadIntervalTimeout = MAXDWORD;
			timeouts.ReadTotalTimeoutMultiplier = 0;
			timeouts.ReadTotalTimeoutConstant = 0;
			timeouts.WriteTotalTimeoutMultiplier = 0;
			timeouts.WriteTotalTimeoutConstant = 0;

			SetCommTimeouts(m_handle, &timeouts);
		}

		return static_cast< int >(bytesRead);
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
