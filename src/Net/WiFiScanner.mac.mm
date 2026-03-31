/*
 * src/Net/WiFiScanner.mac.mm
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

/* macOS framework. */
#import <CoreWLAN/CoreWLAN.h>

namespace EmEn::Net::WiFiScanner
{
	/**
	 * @brief Converts a CWSecurity enum to a human-readable string.
	 * @param security The CWSecurity value.
	 * @return std::string
	 */
	static std::string
	securityToString (CWSecurity security) noexcept
	{
		switch ( security )
		{
			case kCWSecurityNone :
				return "Open";

			case kCWSecurityWEP :
				return "WEP";

			case kCWSecurityWPAPersonal :
				return "WPA-PSK";

			case kCWSecurityWPAEnterprise :
				return "WPA-EAP";

			case kCWSecurityWPA2Personal :
				return "WPA2-PSK";

			case kCWSecurityWPA2Enterprise :
				return "WPA2-EAP";

			case kCWSecurityWPA3Personal :
				return "WPA3-SAE";

			case kCWSecurityWPA3Enterprise :
				return "WPA3-EAP";

			default :
				return "Unknown";
		}
	}

	/**
	 * @brief Converts a CWNetwork object to a Network struct.
	 * @param cwNetwork The CoreWLAN network object.
	 * @return Network
	 */
	static Network
	fromCWNetwork (CWNetwork * cwNetwork) noexcept
	{
		Network network;

		if ( cwNetwork.ssid != nil )
		{
			network.ssid = [cwNetwork.ssid UTF8String];
		}

		if ( cwNetwork.bssid != nil )
		{
			network.bssid = [cwNetwork.bssid UTF8String];
		}

		network.signalLevel = static_cast< int32_t >(cwNetwork.rssiValue);
		network.quality = std::max(0, std::min(100, static_cast< int >((network.signalLevel + 100) * 2)));

		if ( cwNetwork.wlanChannel != nil )
		{
			network.channel = static_cast< int32_t >(cwNetwork.wlanChannel.channelNumber);

			/* Derive frequency from channel. */
			if ( network.channel >= 1 && network.channel <= 14 )
			{
				network.frequency = (network.channel == 14) ? 2484 : static_cast< uint32_t >(2407 + network.channel * 5);
			}
			else if ( network.channel >= 36 )
			{
				network.frequency = static_cast< uint32_t >(5000 + network.channel * 5);
			}
		}

		network.security = securityToString(cwNetwork.security);
		network.mode = "Infra";

		return network;
	}

	std::vector< Network >
	scan () noexcept
	{
		std::vector< Network > networks;

		@autoreleasepool
		{
			CWInterface * interface = [[CWWiFiClient sharedWiFiClient] interface];

			if ( interface == nil )
			{
				return networks;
			}

			NSError * error = nil;
			NSSet< CWNetwork * > * scanResults = [interface scanForNetworksWithName:nil error:&error];

			if ( error != nil || scanResults == nil )
			{
				return networks;
			}

			for ( CWNetwork * cwNetwork in scanResults )
			{
				networks.emplace_back(fromCWNetwork(cwNetwork));
			}
		}

		return networks;
	}

	std::vector< Network >
	getCurrentConnections () noexcept
	{
		std::vector< Network > connections;

		@autoreleasepool
		{
			CWInterface * interface = [[CWWiFiClient sharedWiFiClient] interface];

			if ( interface == nil || interface.ssid == nil )
			{
				return connections;
			}

			Network network;
			network.ssid = [interface.ssid UTF8String];

			if ( interface.bssid != nil )
			{
				network.bssid = [interface.bssid UTF8String];
			}

			network.signalLevel = static_cast< int32_t >(interface.rssiValue);
			network.quality = std::max(0, std::min(100, static_cast< int >((network.signalLevel + 100) * 2)));

			if ( interface.wlanChannel != nil )
			{
				network.channel = static_cast< int32_t >(interface.wlanChannel.channelNumber);

				if ( network.channel >= 1 && network.channel <= 14 )
				{
					network.frequency = (network.channel == 14) ? 2484 : static_cast< uint32_t >(2407 + network.channel * 5);
				}
				else if ( network.channel >= 36 )
				{
					network.frequency = static_cast< uint32_t >(5000 + network.channel * 5);
				}
			}

			network.security = securityToString(interface.security);
			network.mode = "Infra";

			connections.emplace_back(std::move(network));
		}

		return connections;
	}
}
