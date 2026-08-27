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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "WiFiScanner.hpp"

/* STL inclusions. */
#include <array>
#include <cstdio>
#include <sstream>
#include <optional>
#include <charconv>

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

			return static_cast< int32_t >(((frequency - 2412) / 5) + 1);
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
	/**
	 * @brief Parses a decimal number out of untrusted text, without ever throwing.
	 * @tparam number_t The wanted integral type.
	 * @param text The text to read.
	 * @return std::optional< number_t > Empty when the text is not a plain number.
	 */
	template< typename number_t >
	static std::optional< number_t >
	parseNumber (const std::string & text) noexcept
	{
		number_t value{0};

		const auto * begin = text.data();
		const auto * end = text.data() + text.size();

		if ( std::from_chars(begin, end, value).ec != std::errc{} )
		{
			return std::nullopt;
		}

		return value;
	}

	static Network
	parseLine (const std::string & line) noexcept
	{
		Network network;

		/* ⚠️ nmcli's terse output escapes BOTH ':' and '\\' with a backslash. Scanning for "\\:"
		 * alone mis-reads an SSID that ends with a backslash ("Free\\" becomes "Free\\:", whose
		 * trailing "\\:" was eaten as an escape): every field then shifted left and a text field
		 * reached std::stoi. The unescaping is done in one left-to-right pass instead, which is the
		 * only way to honour "\\\\" before "\\:". */
		std::vector< std::string > fields;
		std::string field;

		for ( size_t index = 0; index < line.size(); ++index )
		{
			const auto character = line[index];

			if ( character == '\\' && index + 1 < line.size() )
			{
				/* The escaped character is taken verbatim, whatever it is. */
				field += line[index + 1];
				++index;

				continue;
			}

			if ( character == ':' )
			{
				fields.emplace_back(field);
				field.clear();

				continue;
			}

			field += character;
		}

		fields.emplace_back(field);

		/* Parse fields: SSID, BSSID, SIGNAL, FREQ, SECURITY, MODE */
		if ( fields.size() >= 6 )
		{
			network.ssid = fields[0];
			network.bssid = fields[1];
			/* ⚠️ std::stoi / std::stoul throw on text, and this function is noexcept in a
			 * -fno-exceptions build: a neighbour's SSID was enough to terminate the process.
			 * from_chars reports it as a value, and a malformed line is simply dropped. */
			network.quality = parseNumber< int32_t >(fields[2]).value_or(0);
			network.signalLevel = (network.quality / 2) - 100; /* Approximate dBm. */

			/* Parse frequency: "2412 MHz" → 2412 */
			const auto freqStr = fields[3];
			const auto spacePos = freqStr.find(' ');
			network.frequency = parseNumber< uint32_t >(spacePos != std::string::npos ? freqStr.substr(0, spacePos) : freqStr).value_or(0);
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
