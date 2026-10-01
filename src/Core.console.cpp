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
#include "FastJSON.hpp"
#include "IO/IO.hpp"

namespace EmEn
{
	using namespace Base;

	void
	Core::onRegisterToConsole () noexcept
	{
		this->bindCommand("remoteConsoleStatus", "Returns the remote console state as JSON (running, endpoint), and the MCP endpoint (null when the MCP server is off).", [this] () {
			Json::Value state{Json::objectValue};
			state["running"] = m_consoleController.isRemoteListenerRunning();

			if ( m_consoleController.isRemoteListenerRunning() )
			{
				state["endpoint"] = m_consoleController.remoteListenerEndpoint();
			}

			const auto MCPEndpoint = m_consoleController.MCPEndpoint();
			state["mcp"] = MCPEndpoint.empty() ? Json::Value{Json::nullValue} : Json::Value{MCPEndpoint};

			return Console::CommandResult::json(FastJSON::stringify(state));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("restartRemoteConsole", "Moves the remote console to another endpoint on the next cycle (this connection is closed).",
			{
				{"endpoint", "A port (1-65535), address:port or [ipv6]:port."}
			},
			[this] (const std::string & endpoint) {
				std::string address;
				uint16_t port = 0;

				if ( !Console::Controller::parseEndpoint(endpoint, m_consoleController.defaultRemoteListenerAddress(), address, port) )
				{
					return Console::CommandResult::error("Invalid endpoint. Expected a port (1-65535), address:port or [ipv6]:port.");
				}

				m_consoleController.requestRemoteListenerRestart(address, port);

				return Console::CommandResult::success("The remote console will move to " + address + ':' + std::to_string(port) + " on the next cycle; this connection closes. Reconnect there.");
			});

		this->bindCommand("toggleRecording", "Toggles the RushMaker audio/video recording (same as Shift+Ctrl+F12).", [this] () {
			/* RushMaker toggle (same as Shift+Ctrl+F12), for AI-driven capture sessions. */
			if ( this->rushRecording() )
			{
				this->stopAudioVideoRecording();

				return Console::CommandResult::success("Recording stopped (encoding finishes in background).");
			}

			if ( !this->startAudioVideoRecording() )
			{
				return Console::CommandResult::error("Unable to start the recording !");
			}

			return Console::CommandResult::success("Recording started.");
		});

		this->bindCommand("togglePhysicalSimulation", "Toggles the physical simulation (collisions, boundaries, ground response) of the active scene.", [this] () {
			/* Debug affordance: isolates a rendering or logic defect from a physics one without
			 * a rebuild. Not a pause — entities keep moving, only the collision resolution stops. */
			const auto state = !this->physicalSimulationEnabled();

			this->enablePhysicalSimulation(state);

			return Console::CommandResult::success(state ? "Physical simulation ENABLED." : "Physical simulation DISABLED (entities move, nothing collides).");
		});

		this->bindCommand("openFiles", "Opens files as if they were dropped onto the window (viewers, audio, JSON stores or scene). A running demo scene is never disturbed.",
			{
				{"filepaths", "Absolute paths of the files to open; quote a path that contains a comma or a parenthesis."}
			},
			[this] (const std::vector< std::string > & paths) {
				/* NOTE: Exercises the whole dropped-files pipeline without a real drag and drop,
				 * which cannot be produced from the remote console. */
				if ( paths.empty() )
				{
					return Console::CommandResult::error("openFiles(): give at least one file path.");
				}

				/* NOTE: Bounded (owner ruling 2026-10-01): every file loads synchronously on the main thread. */
				constexpr size_t MaxFileCount{64};

				if ( paths.size() > MaxFileCount )
				{
					return Console::CommandResult::error("openFiles(): at most " + std::to_string(MaxFileCount) + " files per call.");
				}

				Console::Outputs outputs;
				std::vector< std::filesystem::path > filepaths;
				filepaths.reserve(paths.size());

				for ( const auto & path : paths )
				{
					auto filepath = IO::u8path(path);

					/* NOTE: Absolute paths only, as the help says (owner ruling 2026-10-01): a relative one resolved against
					 * the working directory of the process. */
					if ( !filepath.is_absolute() )
					{
						outputs.emplace_back(Severity::Warning, "The path '" + path + "' is not absolute! Skipping ...");

						continue;
					}

					if ( !IO::fileExists(filepath) )
					{
						outputs.emplace_back(Severity::Warning, "The file '" + path + "' doesn't exists! Skipping ...");

						continue;
					}

					filepaths.emplace_back(std::move(filepath));
				}

				if ( filepaths.empty() )
				{
					outputs.emplace_back(Severity::Error, "No usable file to open !");

					return Console::CommandResult::fromOutputs(std::move(outputs), false);
				}

				const auto fileCount = filepaths.size();

				/* NOTE: The reply said "submitted" with a success even when no file could be opened. */
				const auto unopenedCount = this->openFiles(filepaths);

				if ( unopenedCount == fileCount )
				{
					outputs.emplace_back(Severity::Error, "None of the " + std::to_string(fileCount) + " file(s) could be opened (refused, or no behavior for them).");

					return Console::CommandResult::fromOutputs(std::move(outputs), false);
				}

				outputs.emplace_back(Severity::Success, std::to_string(fileCount - unopenedCount) + " of " + std::to_string(fileCount) + " file(s) taken by the opening pipeline.");

				return Console::CommandResult::fromOutputs(std::move(outputs), true);
			});

		this->bindCommand("cycleAnimation", "Walks the animations of the model shown by the viewer: OFF -> clip 1 -> ... -> OFF (same as the space bar).", [this] () {
			/* NOTE: The remote counterpart of the space bar, calling the very same code. It predates the
			 * console key injection reaching Core-level bindings, and stays the deterministic way in. */
			if ( !this->cycleViewerAnimation() )
			{
				return Console::CommandResult::error("Nothing to cycle: the model viewer must be the active scene and its asset must carry animations.");
			}

			return Console::CommandResult::success("Animation cycled.");
		});

		this->bindCommand("resetAnimation", "Forces the model shown by the viewer back to its rest pose, in one call and without drawing a notification. Use it before an automated capture.", [this] () {
			/* NOTE: The deterministic counterpart of cycleAnimation(). Reaching the rest pose with
			 * the cycle alone takes as many calls as the asset has clips, and the caller cannot know
			 * how many without tracking the state itself — so an automated capture had no way to
			 * guarantee a known pose. It draws NO notification, on purpose: the toast is what would
			 * end up burnt into the screenshot. */
			if ( !this->resetViewerAnimation() )
			{
				return Console::CommandResult::error("The model viewer is not the active scene.");
			}

			return Console::CommandResult::success("Animation reset to the rest pose.");
		}, Console::CommandHint::Idempotent);

		this->bindCommand("exit,quit,shutdown", "Quits the application (settings are saved).", [this] () {
			this->stop();

			return Console::CommandResult::info("Shutdown procedure called from console ...");
		}, Console::CommandHint::Destructive);
	}
}
