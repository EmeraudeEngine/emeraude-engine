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

/* STL inclusions. */
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "IO/IO.hpp"

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

		this->bindCommand("togglePhysicalSimulation", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* Debug affordance: isolates a rendering or logic defect from a physics one without
			 * a rebuild. Not a pause — entities keep moving, only the collision resolution stops. */
			const auto state = !this->physicalSimulationEnabled();

			this->enablePhysicalSimulation(state);

			outputs.emplace_back(Severity::Success, state ? "Physical simulation ENABLED." : "Physical simulation DISABLED (entities move, nothing collides).");

			return true;
		}, "Toggles the physical simulation (collisions, boundaries, ground response) of the active scene.");

		this->bindCommand("openFiles", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			/* NOTE: Exercises the whole dropped-files pipeline without a real drag and drop,
			 * which cannot be produced from the remote console. */
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: openFiles(filepath[, filepath, ...])");

				return false;
			}

			std::vector< std::filesystem::path > filepaths;
			filepaths.reserve(arguments.size());

			for ( const auto & argument : arguments )
			{
				auto filepath = IO::u8path(argument.asString());

				if ( !IO::fileExists(filepath) )
				{
					outputs.emplace_back(Severity::Warning, "The file '" + argument.asString() + "' doesn't exists! Skipping ...");

					continue;
				}

				filepaths.emplace_back(std::move(filepath));
			}

			if ( filepaths.empty() )
			{
				outputs.emplace_back(Severity::Error, "No usable file to open !");

				return false;
			}

			const auto fileCount = filepaths.size();

			this->openFiles(filepaths);

			outputs.emplace_back(Severity::Success, std::to_string(fileCount) + " file(s) submitted to the opening pipeline.");

			return true;
		}, "Opens files as if they were dropped onto the window. Usage: openFiles(filepath[, filepath, ...])");

		this->bindCommand("exit,quit,shutdown", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, "Shutdown procedure called from console ...");

			this->stop();

			return 0;
		}, "Quit the application.");
	}
}
