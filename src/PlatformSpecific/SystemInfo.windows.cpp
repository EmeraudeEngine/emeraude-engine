/*
 * src/PlatformSpecific/SystemInfo.windows.cpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_WINDOWS

/* STL inclusions. */
#include <iostream>
#include <string>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <psapi.h>
#define _WIN32_DCOM // NOLINT
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

/* Local inclusions. */
#include "Helpers.hpp"

namespace EmEn::PlatformSpecific
{
	bool
	SystemInfo::fetchOSInformation () noexcept
	{
		try
		{
			const std::wstring systemName = getStringValueFromHKLM(L"SOFTWARE\\MICROSOFT\\windows nt\\currentversion", L"ProductName");
			const std::wstring systemVersion = getStringValueFromHKLM(L"SOFTWARE\\MICROSOFT\\windows nt\\currentversion", L"CurrentBuildNumber");

			m_OSInformation.systemName = convertWideToUTF8(systemName);
			m_OSInformation.systemVersion = convertWideToUTF8(systemVersion);
		}
		catch ( std::exception & e )
		{
			std::cerr << e.what();

			return false;
		}

		// TODO: Not required for now
		m_OSInformation.computerName = "MyComputer";

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
		std::string realPath(1024, '\0');

		GetModuleFileNameA(
			nullptr,
			realPath.data(),
			static_cast< DWORD >(realPath.size())
		);

		const auto position = realPath.find_last_of('\\');

		return
			position == std::string::npos ?
			realPath :
			realPath.substr(0, position);
	}
}

#endif
