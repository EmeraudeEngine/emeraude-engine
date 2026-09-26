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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Window.hpp"

namespace EmEn
{
	void
	Window::onRegisterToConsole () noexcept
	{
		this->bindCommand("resize", "Resizes the window (320x240 minimum).",
			{
				{"width", "New window width, in screen coordinates."},
				{"height", "New window height, in screen coordinates."}
			},
			[this] (int32_t width, int32_t height) {
				if ( width < 320 || height < 240 )
				{
					return Console::CommandResult::error("Minimum size is 320x240.");
				}

				if ( !this->resize(width, height) )
				{
					return Console::CommandResult::error("Failed to resize window !");
				}

				return Console::CommandResult::success("Window resized to " + std::to_string(width) + "x" + std::to_string(height) + ".");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("getState", "Returns the window state as JSON (size, position, framebuffer, scale).", [this] () {
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

			return Console::CommandResult::json(json.str());
		}, Console::CommandHint::ReadOnly);
	}
}
