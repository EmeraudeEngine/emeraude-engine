/*
 * src/Net/WiFiScanner.windows.cpp
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

#include "WiFiScanner.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstdio>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <wlanapi.h>

#pragma comment(lib, "wlanapi.lib")

namespace EmEn::Net::WiFiScanner
{
	/**
	 * @brief RAII wrapper for WLAN API client handle.
	 */
	class WlanHandle final
	{
	public:

		WlanHandle () noexcept
		{
			DWORD negotiatedVersion = 0;

			if ( WlanOpenHandle(2, nullptr, &negotiatedVersion, &m_handle) != ERROR_SUCCESS )
			{
				m_handle = nullptr;
			}
		}

		~WlanHandle () noexcept
		{
			if ( m_handle != nullptr )
			{
				WlanCloseHandle(m_handle, nullptr);
			}
		}

		WlanHandle (const WlanHandle &) = delete;
		WlanHandle & operator= (const WlanHandle &) = delete;

		[[nodiscard]] HANDLE get () const noexcept { return m_handle; }
		[[nodiscard]] bool valid () const noexcept { return m_handle != nullptr; }

	private:

		HANDLE m_handle{nullptr};
	};

	/**
	 * @brief Converts a DOT11_AUTH_ALGORITHM to a human-readable security string.
	 * @param auth The authentication algorithm.
	 * @return std::string The security type label.
	 */
	static std::string
	authToString (DOT11_AUTH_ALGORITHM auth) noexcept
	{
		switch ( auth )
		{
			case DOT11_AUTH_ALGO_80211_OPEN : return "Open";
			case DOT11_AUTH_ALGO_80211_SHARED_KEY : return "WEP";
			case DOT11_AUTH_ALGO_WPA : return "WPA";
			case DOT11_AUTH_ALGO_WPA_PSK : return "WPA-PSK";
			case DOT11_AUTH_ALGO_WPA_NONE : return "WPA-None";
			case DOT11_AUTH_ALGO_RSNA : return "WPA2";
			case DOT11_AUTH_ALGO_RSNA_PSK : return "WPA2-PSK";
			case DOT11_AUTH_ALGO_WPA3 : return "WPA3";
			case DOT11_AUTH_ALGO_WPA3_SAE : return "WPA3-SAE";
			case DOT11_AUTH_ALGO_OWE : return "OWE";
			default : return "Unknown";
		}
	}

	/**
	 * @brief Converts a DOT11_BSS_TYPE to a human-readable mode string.
	 * @param bssType The BSS type.
	 * @return std::string The mode label.
	 */
	static std::string
	bssTypeToString (DOT11_BSS_TYPE bssType) noexcept
	{
		switch ( bssType )
		{
			case dot11_BSS_type_infrastructure : return "Infrastructure";
			case dot11_BSS_type_independent : return "Ad-Hoc";
			default : return "Unknown";
		}
	}

	/**
	 * @brief Formats a 6-byte MAC address as a colon-separated string.
	 * @param mac Pointer to 6 bytes.
	 * @return std::string Formatted BSSID (e.g., "aa:bb:cc:dd:ee:ff").
	 */
	static std::string
	formatBssid (const uint8_t * mac) noexcept
	{
		char buf[18]{};

		std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		return buf;
	}

	/**
	 * @brief Derives WiFi frequency (MHz) from a channel number.
	 * @param channel The channel number.
	 * @return uint32_t The frequency in MHz, or 0 if unknown.
	 */
	static uint32_t
	channelToFrequency (int32_t channel) noexcept
	{
		if ( channel >= 1 && channel <= 13 )
		{
			return static_cast< uint32_t >(2407 + channel * 5);
		}

		if ( channel == 14 )
		{
			return 2484;
		}

		if ( channel >= 32 )
		{
			return static_cast< uint32_t >(5000 + channel * 5);
		}

		return 0;
	}

	/**
	 * @brief Finds the first WLAN interface GUID.
	 * @param handle The WLAN client handle.
	 * @param guid [out] The interface GUID.
	 * @return bool True if an interface was found.
	 */
	static bool
	getFirstInterfaceGuid (HANDLE handle, GUID & guid) noexcept
	{
		PWLAN_INTERFACE_INFO_LIST interfaceList = nullptr;

		if ( WlanEnumInterfaces(handle, nullptr, &interfaceList) != ERROR_SUCCESS || interfaceList == nullptr )
		{
			return false;
		}

		if ( interfaceList->dwNumberOfItems == 0 )
		{
			WlanFreeMemory(interfaceList);

			return false;
		}

		guid = interfaceList->InterfaceInfo[0].InterfaceGuid;

		WlanFreeMemory(interfaceList);

		return true;
	}

	std::vector< Network >
	scan () noexcept
	{
		std::vector< Network > networks;

		WlanHandle wlan;

		if ( !wlan.valid() )
		{
			return networks;
		}

		GUID interfaceGuid{};

		if ( !getFirstInterfaceGuid(wlan.get(), interfaceGuid) )
		{
			return networks;
		}

		/* Request a fresh scan (best-effort, results come from cache if scan is still in progress). */
		WlanScan(wlan.get(), &interfaceGuid, nullptr, nullptr, nullptr);

		/* Retrieve BSS list (detailed per-AP info including channel and BSSID). */
		PWLAN_BSS_LIST bssList = nullptr;

		if ( WlanGetNetworkBssList(wlan.get(), &interfaceGuid, nullptr, dot11_BSS_type_any, FALSE, nullptr, &bssList) != ERROR_SUCCESS || bssList == nullptr )
		{
			return networks;
		}

		/* Also get the available network list for security/auth info per SSID. */
		PWLAN_AVAILABLE_NETWORK_LIST networkList = nullptr;

		WlanGetAvailableNetworkList(wlan.get(), &interfaceGuid, 0, nullptr, &networkList);

		for ( DWORD i = 0; i < bssList->dwNumberOfItems; i++ )
		{
			const auto & bssEntry = bssList->wlanBssEntries[i];

			Network net;

			/* SSID. */
			if ( bssEntry.dot11Ssid.uSSIDLength > 0 )
			{
				net.ssid.assign(reinterpret_cast< const char * >(bssEntry.dot11Ssid.ucSSID), bssEntry.dot11Ssid.uSSIDLength);
			}

			/* BSSID. */
			net.bssid = formatBssid(bssEntry.dot11Bssid);

			/* Signal strength. */
			net.quality = static_cast< int32_t >(bssEntry.uLinkQuality);
			net.signalLevel = static_cast< int32_t >(bssEntry.lRssi);

			/* Frequency: the WLAN API provides it directly in kHz via ulChCenterFrequency. */
			net.frequency = static_cast< uint32_t >(bssEntry.ulChCenterFrequency / 1000);

			/* Channel: derive from frequency. */
			if ( net.frequency >= 2412 && net.frequency <= 2484 )
			{
				net.channel = (net.frequency == 2484) ? 14 : static_cast< int32_t >((net.frequency - 2412) / 5 + 1);
			}
			else if ( net.frequency >= 5170 && net.frequency <= 5825 )
			{
				net.channel = static_cast< int32_t >((net.frequency - 5000) / 5);
			}
			else if ( net.frequency >= 5955 && net.frequency <= 7115 )
			{
				net.channel = static_cast< int32_t >((net.frequency - 5950) / 5);
			}

			/* BSS type → mode. */
			net.mode = bssTypeToString(bssEntry.dot11BssType);

			/* Look up security from the available network list (matched by SSID). */
			if ( networkList != nullptr )
			{
				for ( DWORD j = 0; j < networkList->dwNumberOfItems; j++ )
				{
					const auto & avail = networkList->Network[j];

					if ( avail.dot11Ssid.uSSIDLength == bssEntry.dot11Ssid.uSSIDLength &&
						std::memcmp(avail.dot11Ssid.ucSSID, bssEntry.dot11Ssid.ucSSID, avail.dot11Ssid.uSSIDLength) == 0 )
					{
						net.security = authToString(avail.dot11DefaultAuthAlgorithm);

						break;
					}
				}
			}

			networks.emplace_back(std::move(net));
		}

		if ( networkList != nullptr )
		{
			WlanFreeMemory(networkList);
		}

		WlanFreeMemory(bssList);

		std::sort(networks.begin(), networks.end(), [] (const auto & a, const auto & b) {
			return a.ssid < b.ssid;
		});

		return networks;
	}

	std::vector< Network >
	getCurrentConnections () noexcept
	{
		std::vector< Network > connections;

		WlanHandle wlan;

		if ( !wlan.valid() )
		{
			return connections;
		}

		PWLAN_INTERFACE_INFO_LIST interfaceList = nullptr;

		if ( WlanEnumInterfaces(wlan.get(), nullptr, &interfaceList) != ERROR_SUCCESS || interfaceList == nullptr )
		{
			return connections;
		}

		for ( DWORD i = 0; i < interfaceList->dwNumberOfItems; i++ )
		{
			const auto & info = interfaceList->InterfaceInfo[i];

			if ( info.isState != wlan_interface_state_connected )
			{
				continue;
			}

			/* Query connection attributes for the connected network. */
			DWORD dataSize = 0;
			PWLAN_CONNECTION_ATTRIBUTES connAttr = nullptr;

			if ( WlanQueryInterface(wlan.get(), &info.InterfaceGuid,
				wlan_intf_opcode_current_connection, nullptr, &dataSize,
				reinterpret_cast< PVOID * >(&connAttr), nullptr) != ERROR_SUCCESS || connAttr == nullptr )
			{
				continue;
			}

			Network net;

			const auto & assoc = connAttr->wlanAssociationAttributes;

			/* SSID. */
			if ( assoc.dot11Ssid.uSSIDLength > 0 )
			{
				net.ssid.assign(reinterpret_cast< const char * >(assoc.dot11Ssid.ucSSID), assoc.dot11Ssid.uSSIDLength);
			}

			/* BSSID. */
			net.bssid = formatBssid(assoc.dot11Bssid);

			/* Signal quality. */
			net.quality = static_cast< int32_t >(assoc.wlanSignalQuality);
			net.signalLevel = (net.quality / 2) - 100;

			/* Security. */
			net.security = authToString(connAttr->wlanSecurityAttributes.dot11AuthAlgorithm);

			/* Get channel/frequency from BSS list for the connected BSSID. */
			PWLAN_BSS_LIST bssList = nullptr;

			auto ssidCopy = assoc.dot11Ssid;

			if ( WlanGetNetworkBssList(wlan.get(), &info.InterfaceGuid,
				&ssidCopy, dot11_BSS_type_infrastructure, TRUE, nullptr, &bssList) == ERROR_SUCCESS && bssList != nullptr )
			{
				for ( DWORD j = 0; j < bssList->dwNumberOfItems; j++ )
				{
					const auto & bss = bssList->wlanBssEntries[j];

					if ( std::memcmp(bss.dot11Bssid, assoc.dot11Bssid, 6) == 0 )
					{
						net.frequency = static_cast< uint32_t >(bss.ulChCenterFrequency / 1000);
						net.signalLevel = static_cast< int32_t >(bss.lRssi);

						if ( net.frequency >= 2412 && net.frequency <= 2484 )
						{
							net.channel = (net.frequency == 2484) ? 14 : static_cast< int32_t >((net.frequency - 2412) / 5 + 1);
						}
						else if ( net.frequency >= 5170 && net.frequency <= 5825 )
						{
							net.channel = static_cast< int32_t >((net.frequency - 5000) / 5);
						}
						else if ( net.frequency >= 5955 && net.frequency <= 7115 )
						{
							net.channel = static_cast< int32_t >((net.frequency - 5950) / 5);
						}

						break;
					}
				}

				WlanFreeMemory(bssList);
			}
			else
			{
				/* Fallback: derive frequency from channel if BSS lookup failed. */
				net.frequency = channelToFrequency(net.channel);
			}

			net.mode = "Infrastructure";

			connections.emplace_back(std::move(net));

			WlanFreeMemory(connAttr);
		}

		WlanFreeMemory(interfaceList);

		return connections;
	}
}
