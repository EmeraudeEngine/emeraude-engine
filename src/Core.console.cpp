/*
 * src/Core.console.cpp
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

#include "Core.hpp"

namespace EmEn
{
	using namespace Base;

	void
	Core::onRegisterToConsole () noexcept
	{
		this->bindCommand("toggleRecording", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* RushMaker toggle (same as Shift+Ctrl+F12), for AI-driven capture sessions. */
			if ( m_graphicsRenderer.recorder().isRecording() )
			{
				this->stopAudioVideoRecording();

				outputs.emplace_back(Severity::Success, "Recording stopped (encoding finishes in background).");

				return true;
			}

			if ( !this->startAudioVideoRecording() )
			{
				outputs.emplace_back(Severity::Error, "Unable to start the recording !");

				return false;
			}

			outputs.emplace_back(Severity::Success, "Recording started.");

			return true;
		}, "Toggles the RushMaker audio/video recording (same as Shift+Ctrl+F12).");

		this->bindCommand("exit,quit,shutdown", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, "Shutdown procedure called from console ...");

			this->stop();

			return 0;
		}, "Quit the application.");
	}
}
