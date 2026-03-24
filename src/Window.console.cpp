/*
 * src/Window.console.cpp
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

#include "Window.hpp"

namespace EmEn
{
	void
	Window::onRegisterToConsole () noexcept
	{
		this->bindCommand("resize", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: resize(width, height)");

				return false;
			}

			const auto width = arguments[0].asInteger();
			const auto height = arguments[1].asInteger();

			if ( width < 320 || height < 240 )
			{
				outputs.emplace_back(Severity::Error, "Minimum size is 320x240.");

				return false;
			}

			if ( this->resize(width, height) )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Window resized to " << width << "x" << height << ".");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Failed to resize window !");
			}

			return true;
		}, "Resizes the window. Usage: resize(width, height)");

		this->bindCommand("getState", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			const auto & state = this->state();

			std::stringstream json;
			json << "{";
			json << "\"windowWidth\":" << state.windowWidth << ",";
			json << "\"windowHeight\":" << state.windowHeight << ",";
			json << "\"windowXPosition\":" << state.windowXPosition << ",";
			json << "\"windowYPosition\":" << state.windowYPosition << ",";
			json << "\"framebufferWidth\":" << state.framebufferWidth << ",";
			json << "\"framebufferHeight\":" << state.framebufferHeight << ",";
			json << "\"contentXScale\":" << state.contentXScale << ",";
			json << "\"contentYScale\":" << state.contentYScale;
			json << "}";

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns the window state as JSON (size, position, framebuffer, scale).");
	}
}
