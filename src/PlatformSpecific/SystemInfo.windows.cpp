/*
 * src/PlatformSpecific/SystemInfo.windows.cpp
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
#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WIN32_DCOM // NOLINT
#include <Windows.h>
#include <comdef.h>
#include <dxgi.h>
#include <psapi.h>
#include <Wbemidl.h>
#include <wrl/client.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "dxgi.lib")

/* Local inclusions. */
#include "Helpers.hpp"
#include "Vulkan/Types.hpp"

namespace
{
	/** @brief Microsoft's PCI vendor ID: the Basic Render Driver (WARP) and other software adapters. */
	constexpr UINT MicrosoftVendorID{0x1414};

	/**
	 * @brief The dedicated video memory at or below which a hardware adapter is called integrated.
	 * @note An integrated GPU's dedicated memory is a small firmware carve-out of the system RAM (Intel: 128 MiB on the
	 * RaptorLake-S iGPU, measured 2026-10-15; AMD APUs: 512 MiB by default), a discrete GPU has its own gigabytes (the
	 * smallest current ones have 2 GiB). An AMD APU whose firmware carve-out was raised past 512 MiB reads as discrete:
	 * DXGI 1.1 tells no "integrated" flag, and this list is informative (the Vulkan device type decides the device).
	 */
	constexpr SIZE_T IntegratedDedicatedMemoryLimit{SIZE_T{512} * 1024 * 1024};

	/**
	 * @brief Appends a value as fixed-width upper-case hexadecimal digits.
	 * @param output A reference to the output string.
	 * @param value The value.
	 * @param digits The number of digits (the value is masked to them).
	 */
	void
	appendHex (std::string & output, uint64_t value, int digits) noexcept
	{
		constexpr std::string_view Digits{"0123456789ABCDEF"};

		for ( int shift = (digits - 1) * 4; shift >= 0; shift -= 4 )
		{
			output.push_back(Digits[static_cast< size_t >((value >> shift) & 0xFU)]);
		}
	}

	/**
	 * @brief Formats a user-mode driver version as a.b.c.d (four 16-bit fields, IDXGIAdapter::CheckInterfaceSupport()).
	 * @param version The packed version.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	formatDriverVersion (uint64_t version) noexcept
	{
		std::string output;

		for ( int field = 3; field >= 0; --field )
		{
			output += std::to_string((version >> (field * 16)) & 0xFFFFU);

			if ( field > 0 )
			{
				output.push_back('.');
			}
		}

		return output;
	}
}

namespace EmEn::PlatformSpecific
{
	bool
	SystemInfo::fetchOSInformation () noexcept
	{
		{
			/* NOTE: The registry helper reports a failure as an empty optional (and prints the Windows
			 * error code itself) — no exceptions in the engine. */
			const auto systemName = getStringValueFromHKLM(L"SOFTWARE\\MICROSOFT\\windows nt\\currentversion", L"ProductName");
			const auto systemVersion = getStringValueFromHKLM(L"SOFTWARE\\MICROSOFT\\windows nt\\currentversion", L"CurrentBuildNumber");

			if ( !systemName.has_value() || !systemVersion.has_value() )
			{
				std::cerr << "Unable to read the OS product name/build number from the registry !" << std::endl;

				return false;
			}

			m_OSInformation.systemName = convertWideToUTF8(systemName.value());
			m_OSInformation.systemVersion = convertWideToUTF8(systemVersion.value());

			/* NOTE: Windows 11 kept "Windows 10 ..." as the registry ProductName (measured on build 26300, 2026-10-05):
			 * the build number decides, Windows 11 starts at 22000. */
			constexpr unsigned long FirstWindows11Build{22000};
			constexpr std::string_view Windows10Prefix{"Windows 10"};

			if ( m_OSInformation.systemName.starts_with(Windows10Prefix) )
			{
				unsigned long buildNumber = 0;
				const auto & build = m_OSInformation.systemVersion;

				if ( const auto [pointer, errorCode] = std::from_chars(build.data(), build.data() + build.size(), buildNumber); errorCode == std::errc{} && buildNumber >= FirstWindows11Build )
				{
					m_OSInformation.systemName.replace(0, Windows10Prefix.size(), "Windows 11");
				}
			}
		}

		/* NOTE: Get the DNS host name (matches Node.js os.hostname() on Windows). */
		{
			DWORD size = 0;

			/* First call retrieves the required buffer size (including the trailing null). */
			GetComputerNameExW(ComputerNameDnsHostname, nullptr, &size);

			if ( size == 0 )
			{
				std::cerr << "GetComputerNameExW() size probe failed : 0x" << std::hex << GetLastError() << std::endl;

				m_OSInformation.computerName = "UnknownComputer";
			}
			else
			{
				std::wstring buffer(size, L'\0');

				if ( GetComputerNameExW(ComputerNameDnsHostname, buffer.data(), &size) )
				{
					buffer.resize(size); // Strip the trailing null.

					m_OSInformation.computerName = convertWideToUTF8(buffer);
				}
				else
				{
					std::cerr << "GetComputerNameExW() failed : 0x" << std::hex << GetLastError() << std::endl;

					m_OSInformation.computerName = "UnknownComputer";
				}
			}
		}

		/* NOTE: Get the system unique identifier. */
		{
			// Call: "wmic CsProduct Get UUID"
			// System: 070B744C-3067-11B2-A85C-F9984AD22366
			// Requested: 070B744C-3067-11B2-A85C-F9984AD22366

			/* NOTE: Init COM */
			HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

			if ( FAILED(hr) )
			{
				std::cerr << "CoInitializeEx() : 0x" << std::hex << hr << std::endl;

				return false;
			}

			/* NOTE: Check for COM security */
			hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

			if ( FAILED(hr) && hr != RPC_E_TOO_LATE )
			{
				std::cerr << "CoInitializeSecurity() : 0x" << std::hex << hr << std::endl;

				CoUninitialize();

				return false;
			}

			/* NOTE: WMI connexion. */
			IWbemLocator * pLoc = nullptr;
			hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast< LPVOID * >(&pLoc));

			if ( FAILED(hr) )
			{
				std::cerr << "CoCreateInstance() : 0x" << std::hex << hr << std::endl;

				CoUninitialize();

				return false;
			}

			IWbemServices * pSvc = nullptr;

			/* NOTE: Connect to root\cimv2 space */
			hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &pSvc);

			if ( FAILED(hr) )
			{
				std::cerr << "Unable to connect to WMI (ROOT\\CIMV2) : 0x" << std::hex << hr << std::endl;

				pLoc->Release();

				CoUninitialize();

				return false;
			}

			/* NOTE: Execute WQL request */
			hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

			if ( FAILED(hr) )
			{
				std::cerr << "CoSetProxyBlanket() : 0x" << std::hex << hr << std::endl;

				pSvc->Release();
				pLoc->Release();

				CoUninitialize();

				return false;
			}

			/* NOTE: Use WQL (WMI Query Language) to get UUID. */
			IEnumWbemClassObject* pEnumerator = nullptr;

			hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT UUID FROM Win32_ComputerSystemProduct"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);

			if ( FAILED(hr) )
			{
				std::cerr << "Unable to execute the query : 0x" << std::hex << hr << std::endl;

				pSvc->Release();
				pLoc->Release();

				CoUninitialize();

				return false;
			}

			IWbemClassObject * pclsObj = nullptr;
			ULONG uReturn = 0;

			if ( pEnumerator )
			{
				hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

				if ( SUCCEEDED(hr) && uReturn != 0 && pclsObj != nullptr )
				{
					VARIANT vtProp;
					VariantInit(&vtProp);

					hr = pclsObj->Get(L"UUID", 0, &vtProp, nullptr, nullptr);

					if ( SUCCEEDED(hr) && vtProp.vt == VT_BSTR )
					{
						m_OSInformation.machineUUID = convertWideToANSI(vtProp.bstrVal);
					}
					else
					{
						std::cerr << "Unable to get machine UUID !" << std::endl;
					}

					VariantClear(&vtProp);

					pclsObj->Release();
				}
				else if ( FAILED(hr) )
				{
					std::cerr << "Error : 0x" << std::hex << hr << std::endl;
				}
				else
				{
					std::cerr << "No Win32_ComputerSystemProduct object found !" << std::endl;
				}
			}

			if ( pEnumerator )
			{
				pEnumerator->Release();
			}

			if ( pSvc )
			{
				pSvc->Release();
			}

			if ( pLoc )
			{
				pLoc->Release();
			}

			CoUninitialize();
		}

		return true;
	}

	bool
	SystemInfo::fetchGPUInformation () noexcept
	{
		/* NOTE: DXGI lists the adapters the OS drives, with or without a Vulkan driver: what the user is told when the
		 * Vulkan startup refuses. Doc: https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory1-enumadapters1 */
		Microsoft::WRL::ComPtr< IDXGIFactory1 > factory;

		if ( const auto result = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())); FAILED(result) )
		{
			std::cerr << "CreateDXGIFactory1() failed : 0x" << std::hex << static_cast< unsigned long >(result) << std::dec << std::endl;

			return false;
		}

		/* NOTE: A bound on the walk (Power of Ten rule 2): a machine has a handful of adapters. */
		constexpr UINT MaxAdapters{64};

		for ( UINT index = 0; index < MaxAdapters; ++index )
		{
			Microsoft::WRL::ComPtr< IDXGIAdapter1 > adapter;

			const auto enumeration = factory->EnumAdapters1(index, adapter.GetAddressOf());

			if ( enumeration == DXGI_ERROR_NOT_FOUND )
			{
				return true;
			}

			if ( FAILED(enumeration) )
			{
				std::cerr << "IDXGIFactory1::EnumAdapters1(" << index << ") failed : 0x" << std::hex << static_cast< unsigned long >(enumeration) << std::dec << std::endl;

				return false;
			}

			DXGI_ADAPTER_DESC1 description{};

			if ( const auto result = adapter->GetDesc1(&description); FAILED(result) )
			{
				std::cerr << "IDXGIAdapter1::GetDesc1() failed for adapter #" << index << " : 0x" << std::hex << static_cast< unsigned long >(result) << std::dec << std::endl;

				continue;
			}

			if ( m_GPUDevices.full() )
			{
				std::cerr << "More than " << m_GPUDevices.capacity() << " graphics adapters: adapter #" << index << " and the next ones are not listed." << std::endl;

				return true;
			}

			GPUDevice device;

			/* NOTE: Description is a fixed WCHAR[128], NUL-terminated by DXGI; the view stops at the array end anyway. */
			const std::wstring_view name{description.Description, std::size(description.Description)};

			device.deviceName = convertWideToUTF8(std::wstring{name.substr(0, name.find(L'\0'))});
			device.vendorID = description.VendorId;
			device.deviceID = description.DeviceId;
			/* The subsystem vendor (high 16 bits of SubSysId) is the board maker (ASUS, MSI, a laptop brand...). */
			device.manufacturerID = description.SubSysId >> 16;
			device.availableMemory = description.DedicatedVideoMemory;
			device.dedicatedMemory = description.DedicatedVideoMemory > 0;
			device.software = (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 || description.VendorId == MicrosoftVendorID;

			if ( device.software )
			{
				device.vendorName = "Microsoft";
				device.deviceType = GPUDeviceType::CPU;
			}
			else
			{
				device.vendorName = Vulkan::to_cstring(Vulkan::toVendor(description.VendorId));
				device.deviceType = description.DedicatedVideoMemory <= IntegratedDedicatedMemoryLimit ? GPUDeviceType::IntegratedGPU : GPUDeviceType::DiscreteGPU;
			}

			/* NOTE: The LUID is what Vulkan reports as VkPhysicalDeviceIDProperties::deviceLUID: the key that matches both lists. */
			device.bus = "LUID 0x";
			appendHex(device.bus, static_cast< uint32_t >(description.AdapterLuid.HighPart), 8);
			appendHex(device.bus, description.AdapterLuid.LowPart, 8);

			/* NOTE: The user-mode driver version (the Windows driver version, e.g. 32.0.15.9658 for NVIDIA 596.58);
			 * IDXGIDevice is the only interface this query accepts. */
			device.driverName = device.vendorName;

			if ( LARGE_INTEGER userModeVersion{}; SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &userModeVersion)) )
			{
				device.driverVersion = formatDriverVersion(static_cast< uint64_t >(userModeVersion.QuadPart));
			}

			m_GPUDevices.emplace_back(std::move(device));
		}

		std::cerr << "More than " << MaxAdapters << " graphics adapters enumerated: the walk stopped." << std::endl;

		return true;
	}

	size_t
	SystemInfo::getTotalMemory () noexcept
	{
		/* Doc: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-globalmemorystatusex */
		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);
		GlobalMemoryStatusEx(&status);

		return status.ullTotalPhys;
	}

	size_t
	SystemInfo::getFreeMemory () noexcept
	{
		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);
		GlobalMemoryStatusEx(&status);

		return status.ullAvailPhys;
	}

	size_t
	SystemInfo::getPeakRSS () noexcept
	{
		PROCESS_MEMORY_COUNTERS info{};

		GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));

		return info.PeakWorkingSetSize;
	}

	size_t
	SystemInfo::getCurrentRSS () noexcept
	{
		PROCESS_MEMORY_COUNTERS info{};

		GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));

		return info.WorkingSetSize;
	}

	std::filesystem::path
	SystemInfo::getRealApplicationDir () noexcept
	{
		/* NOTE: The wide API in a growing buffer. GetModuleFileNameA turned every character outside the ANSI code page
		 * into '?' (an install path with such a character: the data not found), and a path past the 1024-byte buffer
		 * was truncated unchecked. 32768 is the longest Windows path. */
		std::wstring realPath(MAX_PATH, L'\0');

		while ( true )
		{
			const auto length = GetModuleFileNameW(nullptr, realPath.data(), static_cast< DWORD >(realPath.size()));

			if ( length == 0 )
			{
				return {};
			}

			if ( length < realPath.size() )
			{
				realPath.resize(length);

				break;
			}

			if ( realPath.size() >= 32768 )
			{
				return {};
			}

			realPath.resize(realPath.size() * 2);
		}

		return std::filesystem::path{realPath}.parent_path();
	}

	std::string
	SystemInfo::platformCPUBrandString () noexcept
	{
		/* NOTE: cpu_features reads the brand string (CPUID) on this platform. */
		return {};
	}
}
