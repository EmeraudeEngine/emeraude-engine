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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Manager.hpp"

namespace EmEn::Scenes::AVConsole
{
	using namespace Base;

	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("listDevices", "Get a list of input/output audio/video devices.",
			{
				{"type", "Which devices to list: 'video', 'audio' or 'both'.", "both"}
			},
			[this] (const std::string & type) {
				auto deviceType{DeviceType::Both};

				if ( type == "video" )
				{
					deviceType = DeviceType::Video;
				}
				else if ( type == "audio" )
				{
					deviceType = DeviceType::Audio;
				}
				else if ( type != "both" )
				{
					return Console::CommandResult::error("listDevices(): type must be 'video', 'audio' or 'both', got '" + type + "'.");
				}

				return Console::CommandResult::info(this->getDeviceList(deviceType));
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("registerRoute", "Register a route from input device to output device.",
			{
				{"type", "The route kind: 'video' or 'audio'."},
				{"source", "The name of the input device (listDevices() lists them)."},
				{"target", "The name of the output device (listDevices() lists them)."}
			},
			[this] (const std::string & type, const std::string & source, const std::string & target) {
				if ( type == "video" )
				{
					if ( !this->connectVideoDevices(source, target) )
					{
						return Console::CommandResult::error("Unable to connect the video device.");
					}
				}
				else if ( type == "audio" )
				{
					if ( !this->connectAudioDevices(source, target) )
					{
						return Console::CommandResult::error("Unable to connect the audio device.");
					}
				}
				else
				{
					return Console::CommandResult::error("First parameter must be 'video' or 'audio'.");
				}

				return Console::CommandResult::success("Route '" + source + "' -> '" + target + "' (" + type + ") registered.");
			}, Console::CommandHint::Idempotent);
	}
}
