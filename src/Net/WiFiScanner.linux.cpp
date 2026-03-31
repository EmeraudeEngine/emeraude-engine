/*
 * src/Net/WiFiScanner.linux.cpp
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

		auto * pipe = popen(command, "r");

		if ( pipe == nullptr )
		{
			return result;
		}

		std::array< char, 256 > buffer{};

		while ( fgets(buffer.data(), static_cast< int >(buffer.size()), pipe) != nullptr )
		{
			result += buffer.data();
		}

		pclose(pipe);

		return result;
	}

	/**
	 * @brief Converts WiFi frequency (MHz) to channel number.
	 * @param frequency The frequency in MHz.
	 * @return int32_t The channel number.
	 */
	static int32_t
	frequencyToChannel (uint32_t frequency) noexcept
	{
		/* 2.4 GHz band. */
		if ( frequency >= 2412 && frequency <= 2484 )
		{
			if ( frequency == 2484 )
			{
				return 14;
			}

			return static_cast< int32_t >((frequency - 2412) / 5 + 1);
		}

		/* 5 GHz band. */
		if ( frequency >= 5170 && frequency <= 5825 )
		{
			return static_cast< int32_t >((frequency - 5000) / 5);
		}

		/* 6 GHz band. */
		if ( frequency >= 5955 && frequency <= 7115 )
		{
			return static_cast< int32_t >((frequency - 5950) / 5);
		}

		return 0;
	}

	/**
	 * @brief Parses a line from nmcli terse output into a Network struct.
	 * @note Expected format (LANG=C): SSID:BSSID(escaped):SIGNAL:FREQ:SECURITY:MODE
	 * BSSID colons are escaped as \: by nmcli, while field separator is unescaped :.
	 * @param line The nmcli output line.
	 * @return Network The parsed network.
	 */
	static Network
	parseLine (const std::string & line) noexcept
	{
		Network network;

		/* Replace escaped colons in BSSID with a placeholder. */
		std::string processed = line;
		const std::string escaped{"\\:"};
		const std::string placeholder{"\x01"};

		for ( auto pos = processed.find(escaped); pos != std::string::npos; pos = processed.find(escaped, pos + placeholder.size()) )
		{
			processed.replace(pos, escaped.size(), placeholder);
		}

		/* Split on unescaped colons. */
		std::vector< std::string > fields;
		std::istringstream stream(processed);
		std::string field;

		while ( std::getline(stream, field, ':') )
		{
			/* Restore colons from placeholder. */
			for ( auto pos = field.find(placeholder); pos != std::string::npos; pos = field.find(placeholder, pos + 1) )
			{
				field.replace(pos, placeholder.size(), ":");
			}

			fields.emplace_back(field);
		}

		/* Parse fields: SSID, BSSID, SIGNAL, FREQ, SECURITY, MODE */
		if ( fields.size() >= 6 )
		{
			network.ssid = fields[0];
			network.bssid = fields[1];
			network.quality = std::stoi(fields[2]);
			network.signalLevel = (network.quality / 2) - 100; /* Approximate dBm. */

			/* Parse frequency: "2412 MHz" → 2412 */
			const auto freqStr = fields[3];
			const auto spacePos = freqStr.find(' ');
			network.frequency = static_cast< uint32_t >(std::stoul(spacePos != std::string::npos ? freqStr.substr(0, spacePos) : freqStr));
			network.channel = frequencyToChannel(network.frequency);

			network.security = fields[4];
			network.mode = fields[5];

			/* Remove trailing newline from last field. */
			if ( !network.mode.empty() && network.mode.back() == '\n' )
			{
				network.mode.pop_back();
			}
		}

		return network;
	}

	std::vector< Network >
	scan () noexcept
	{
		std::vector< Network > networks;

		const auto output = executeCommand("LANG=C nmcli -t -f SSID,BSSID,SIGNAL,FREQ,SECURITY,MODE device wifi list 2>/dev/null");

		if ( output.empty() )
		{
			return networks;
		}

		std::istringstream stream(output);
		std::string line;

		while ( std::getline(stream, line) )
		{
			if ( line.empty() )
			{
				continue;
			}

			auto network = parseLine(line);

			if ( !network.ssid.empty() )
			{
				networks.emplace_back(std::move(network));
			}
		}

		return networks;
	}

	std::vector< Network >
	getCurrentConnections () noexcept
	{
		std::vector< Network > connections;

		const auto output = executeCommand("LANG=C nmcli -t -f ACTIVE,SSID,BSSID,SIGNAL,FREQ,SECURITY,MODE device wifi list 2>/dev/null");

		if ( output.empty() )
		{
			return connections;
		}

		std::istringstream stream(output);
		std::string line;

		while ( std::getline(stream, line) )
		{
			if ( line.empty() )
			{
				continue;
			}

			/* Check for "yes:" prefix (ACTIVE field). */
			if ( line.substr(0, 4) != "yes:" )
			{
				continue;
			}

			/* Remove the "yes:" prefix and parse the rest. */
			auto network = parseLine(line.substr(4));

			if ( !network.ssid.empty() )
			{
				connections.emplace_back(std::move(network));
			}
		}

		return connections;
	}
}
