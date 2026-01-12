/*
 * src/Scenes/AVConsole/Manager.console.cpp
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

#include "Manager.hpp"

namespace EmEn::Scenes::AVConsole
{
	using namespace Libs;

	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("listDevices", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			auto deviceType{DeviceType::Both};

			if ( !arguments.empty() )
			{
				const auto argument = arguments[0].asString();

				if ( argument == "video" )
				{
					deviceType = DeviceType::Video;
				}
				else if ( argument == "audio" )
				{
					deviceType = DeviceType::Audio;
				}
				else if ( argument == "both" )
				{
					deviceType = DeviceType::Both;
				}
			}

			outputs.emplace_back(Severity::Info, this->getDeviceList(deviceType));

			return 0;
		}, "Get a list of input/output audio/video devices.");

		this->bindCommand("registerRoute", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() != 3 )
			{
				outputs.emplace_back(Severity::Error, "This method need 3 parameters.");

				return 1;
			}

			const auto type = arguments[0].asString();
			const auto source = arguments[1].asString();
			const auto target = arguments[2].asString();

			if ( type == "video" )
			{
				if ( !this->connectVideoDevices(source, target) )
				{
					outputs.emplace_back(Severity::Error, "Unable to connect the video device.");

					return 3;
				}
			}
			else if ( type == "audio" )
			{
				if ( !this->connectAudioDevices(source, target) )
				{
					outputs.emplace_back(Severity::Error, "Unable to connect the audio device.");

					return 3;
				}
			}
			else
			{
				outputs.emplace_back(Severity::Error, "First parameter must be 'video' or 'audio'.");

				return 2;
			}

			return 0;
		}, "Register a route from input device to output device.");
	}
}
