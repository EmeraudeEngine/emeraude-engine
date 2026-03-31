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
#include <array>
#include <cstdio>
#include <sstream>
#include <algorithm>

namespace EmEn::Net::WiFiScanner
{
	/**
	 * @brief Executes a shell command and returns its stdout.
	 * @param command The command to execute.
	 * @return std::string The captured output.
	 */
	static std::string
	executeCommand (const char * command) noexcept
	{
		std::string result;

		auto * pipe = _popen(command, "r");

		if ( pipe == nullptr )
		{
			return result;
		}

		std::array< char, 256 > buffer{};

		while ( fgets(buffer.data(), static_cast< int >(buffer.size()), pipe) != nullptr )
		{
			result += buffer.data();
		}

		_pclose(pipe);

		return result;
	}

	/**
	 * @brief Trims leading and trailing whitespace from a string.
	 * @param str The string to trim.
	 * @return std::string The trimmed string.
	 */
	static std::string
	trim (const std::string & str) noexcept
	{
		const auto start = str.find_first_not_of(" \t\r\n");

		if ( start == std::string::npos )
		{
			return "";
		}

		const auto end = str.find_last_not_of(" \t\r\n");

		return str.substr(start, end - start + 1);
	}

	/**
	 * @brief Extracts the value after a label in a netsh output line.
	 * @param line The line to parse (e.g., "    SSID 1 : MyNetwork").
	 * @param label The label to find (e.g., "SSID").
	 * @return std::string The extracted value, or empty if not found.
	 */
	static std::string
	extractValue (const std::string & line, const std::string & label) noexcept
	{
		const auto pos = line.find(label);

		if ( pos == std::string::npos )
		{
			return "";
		}

		const auto colonPos = line.find(':', pos + label.size());

		if ( colonPos == std::string::npos )
		{
			return "";
		}

		return trim(line.substr(colonPos + 1));
	}

	std::vector< Network >
	scan () noexcept
	{
		std::vector< Network > networks;

		const auto output = executeCommand("netsh wlan show networks mode=Bssid");

		if ( output.empty() )
		{
			return networks;
		}

		std::istringstream stream(output);
		std::string line;
		Network current;
		bool inNetwork = false;

		while ( std::getline(stream, line) )
		{
			if ( line.find("SSID") != std::string::npos && line.find("BSSID") == std::string::npos )
			{
				/* Start of a new network entry. */
				if ( inNetwork && !current.bssid.empty() )
				{
					networks.emplace_back(std::move(current));
					current = Network{};
				}

				current.ssid = extractValue(line, "SSID");
				inNetwork = true;
			}
			else if ( line.find("Network type") != std::string::npos || line.find("Type") != std::string::npos )
			{
				current.mode = extractValue(line, ":");
			}
			else if ( line.find("Authentication") != std::string::npos )
			{
				current.security = extractValue(line, "Authentication");
			}
			else if ( line.find("BSSID") != std::string::npos )
			{
				/* If we already have a BSSID, this is a second AP for the same SSID. */
				if ( !current.bssid.empty() )
				{
					networks.emplace_back(current);

					const auto savedSsid = current.ssid;
					const auto savedSecurity = current.security;
					const auto savedMode = current.mode;

					current = Network{};
					current.ssid = savedSsid;
					current.security = savedSecurity;
					current.mode = savedMode;
				}

				current.bssid = extractValue(line, "BSSID");
			}
			else if ( line.find("Signal") != std::string::npos )
			{
				auto signalStr = extractValue(line, "Signal");

				/* Remove the % sign. */
				if ( !signalStr.empty() && signalStr.back() == '%' )
				{
					signalStr.pop_back();
				}

				if ( !signalStr.empty() )
				{
					current.quality = std::stoi(signalStr);
					current.signalLevel = (current.quality / 2) - 100;
				}
			}
			else if ( line.find("Channel") != std::string::npos && line.find("Radio") == std::string::npos )
			{
				const auto channelStr = extractValue(line, "Channel");

				if ( !channelStr.empty() )
				{
					current.channel = std::stoi(channelStr);
				}
			}
		}

		/* Don't forget the last entry. */
		if ( inNetwork && !current.bssid.empty() )
		{
			networks.emplace_back(std::move(current));
		}

		return networks;
	}

	std::vector< Network >
	getCurrentConnections () noexcept
	{
		std::vector< Network > connections;

		const auto output = executeCommand("netsh wlan show interfaces");

		if ( output.empty() )
		{
			return connections;
		}

		std::istringstream stream(output);
		std::string line;
		Network current;
		bool hasConnection = false;

		while ( std::getline(stream, line) )
		{
			if ( line.find("SSID") != std::string::npos && line.find("BSSID") == std::string::npos )
			{
				current.ssid = extractValue(line, "SSID");
				hasConnection = true;
			}
			else if ( line.find("BSSID") != std::string::npos )
			{
				current.bssid = extractValue(line, "BSSID");
			}
			else if ( line.find("Signal") != std::string::npos )
			{
				auto signalStr = extractValue(line, "Signal");

				if ( !signalStr.empty() && signalStr.back() == '%' )
				{
					signalStr.pop_back();
				}

				if ( !signalStr.empty() )
				{
					current.quality = std::stoi(signalStr);
					current.signalLevel = (current.quality / 2) - 100;
				}
			}
			else if ( line.find("Channel") != std::string::npos && line.find("Radio") == std::string::npos )
			{
				const auto channelStr = extractValue(line, "Channel");

				if ( !channelStr.empty() )
				{
					current.channel = std::stoi(channelStr);
				}
			}
			else if ( line.find("Authentication") != std::string::npos )
			{
				current.security = extractValue(line, "Authentication");
			}
		}

		if ( hasConnection && !current.ssid.empty() )
		{
			connections.emplace_back(std::move(current));
		}

		return connections;
	}
}
