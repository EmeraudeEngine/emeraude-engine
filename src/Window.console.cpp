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

/* STL inclusions. */
#include <cmath>
#include <utility>

/* Local inclusions. */
#include "FastJSON.hpp"

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

				/* NOTE: Refused past the device and monitor limits (owner ruling 2026-10-01): the framebuffer of a too large
				 * window exhausted the GPU memory and lost the device. */
				if ( const auto maximum = this->maximumWindowSize(); std::cmp_greater(width, maximum[0]) || std::cmp_greater(height, maximum[1]) )
				{
					return Console::CommandResult::error("The size " + std::to_string(width) + "x" + std::to_string(height) + " is past the limits: at most " + std::to_string(maximum[0]) + "x" + std::to_string(maximum[1]) + ".");
				}

				if ( !this->resize(width, height) )
				{
					return Console::CommandResult::error("Failed to resize window !");
				}

				/* NOTE: The REQUESTED size: the window system applies it asynchronously and may clamp it (to the desktop on
				 * Windows). getState() reads the size applied. */
				return Console::CommandResult::success("Window resize to " + std::to_string(width) + "x" + std::to_string(height) + " requested (getState() gives the size applied).");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("getState", "Returns the window state as JSON (size, position, framebuffer, scale).", [this] () {
			const auto & state = this->state();

			/* NOTE: A JSON document, not a stream: a stream wrote a non-finite scale as "nan" and followed the global
			 * locale (a decimal comma), both invalid JSON. */
			Json::Value json{Json::objectValue};
			json["windowWidth"] = state.windowWidth;
			json["windowHeight"] = state.windowHeight;
			json["windowXPosition"] = state.windowXPosition;
			json["windowYPosition"] = state.windowYPosition;
			json["framebufferWidth"] = state.framebufferWidth;
			json["framebufferHeight"] = state.framebufferHeight;
			json["contentXScale"] = static_cast< double >(std::isfinite(state.contentXScale) ? state.contentXScale : 0.0F);
			json["contentYScale"] = static_cast< double >(std::isfinite(state.contentYScale) ? state.contentYScale : 0.0F);

			return Console::CommandResult::json(Base::FastJSON::stringify(json));
		}, Console::CommandHint::ReadOnly);
	}
}
