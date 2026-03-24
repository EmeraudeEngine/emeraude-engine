/*
 * src/Audio/TrackMixer.console.cpp
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

#include "TrackMixer.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	void
	TrackMixer::onRegisterToConsole () noexcept
	{
		this->bindCommand("play", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			/* No argument: resume current track or start playlist. */
			if ( arguments.empty() )
			{
				if ( m_userState == UserState::Paused )
				{
					this->resume();

					outputs.emplace_back(Severity::Success, "Resumed.");

					return true;
				}

				/* Nothing playing: start the playlist if available. */
				if ( !m_playlist.empty() )
				{
					if ( this->playIndex(m_musicIndex) )
					{
						outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing track " << (m_musicIndex + 1) << "/" << m_playlist.size() << ".");

						return true;
					}
				}

				outputs.emplace_back(Severity::Warning, "Nothing to play !");

				return false;
			}

			/* Search the song by name. */
			const auto soundTrackName = arguments[0].asString();
			const auto soundtrack = m_resourceManager.container< MusicResource >()->getResource(soundTrackName);

			if ( soundtrack == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Soundtrack '" << soundTrackName << "' doesn't exist !");

				return false;
			}

			this->play(soundtrack);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing '" << soundTrackName << "' ...");

			return true;
		}, "Play or resume a music. Without argument: resumes or starts the playlist.");

		this->bindCommand("pause", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				outputs.emplace_back(Severity::Warning, "There is no track playing !");

				return false;
			}

			this->pause();

			outputs.emplace_back(Severity::Success, "Paused.");

			return true;
		}, "Pause music playback.");

		this->bindCommand("stop", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				outputs.emplace_back(Severity::Warning, "There is no track playing !");

				return false;
			}

			this->stop();

			outputs.emplace_back(Severity::Success, "Stopped.");

			return true;
		}, "Stop music.");

		this->bindCommand("volume,vol", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Current volume: " << (m_gain * 100.0F) << "%");

				return true;
			}

			const auto newVolume = arguments[0].asFloat();

			if ( newVolume < 0.0F || newVolume > 100.0F )
			{
				outputs.emplace_back(Severity::Error, "Volume must be between 0 and 100.");

				return false;
			}

			this->setVolume(newVolume / 100.0F);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Volume set to " << newVolume << "%");

			return true;
		}, "Get or set volume (0-100).");

		this->bindCommand("next", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( m_playlist.empty() )
			{
				outputs.emplace_back(Severity::Warning, "Playlist is empty !");

				return false;
			}

			if ( this->next() )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing next track (" << (m_musicIndex + 1) << "/" << m_playlist.size() << ")");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Unable to play next track !");
			}

			return true;
		}, "Play next track in playlist.");

		this->bindCommand("previous,prev", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( m_playlist.empty() )
			{
				outputs.emplace_back(Severity::Warning, "Playlist is empty !");

				return false;
			}

			if ( this->previous() )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing previous track (" << (m_musicIndex + 1) << "/" << m_playlist.size() << ")");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Unable to play previous track !");
			}

			return true;
		}, "Play previous track in playlist.");

		this->bindCommand("shuffle", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Shuffle mode: " << (m_shuffleEnabled ? "ON" : "OFF"));

				return true;
			}

			const auto state = arguments[0].asString();

			if ( state == "on" || state == "1" || state == "true" )
			{
				this->enableShuffle(true);

				outputs.emplace_back(Severity::Success, "Shuffle mode enabled.");
			}
			else if ( state == "off" || state == "0" || state == "false" )
			{
				this->enableShuffle(false);

				outputs.emplace_back(Severity::Success, "Shuffle mode disabled.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Invalid argument. Use 'on' or 'off'.");

				return false;
			}

			return true;
		}, "Get or set shuffle mode (on/off).");

		this->bindCommand("loop", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Loop mode: " << (m_playMode == PlayMode::Loop ? "ON" : "OFF"));

				return true;
			}

			const auto state = arguments[0].asString();

			if ( state == "on" || state == "1" || state == "true" )
			{
				this->setPlayMode(PlayMode::Loop);

				outputs.emplace_back(Severity::Success, "Loop mode enabled.");
			}
			else if ( state == "off" || state == "0" || state == "false" )
			{
				this->setPlayMode(PlayMode::Once);

				outputs.emplace_back(Severity::Success, "Loop mode disabled.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Invalid argument. Use 'on' or 'off'.");

				return false;
			}

			return true;
		}, "Get or set loop mode (on/off).");

		this->bindCommand("crossfade", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Crossfade: " << (m_crossFaderEnabled ? "ON" : "OFF"));

				return true;
			}

			const auto state = arguments[0].asString();

			if ( state == "on" || state == "1" || state == "true" )
			{
				this->enableCrossFader(true);

				outputs.emplace_back(Severity::Success, "Crossfade enabled.");
			}
			else if ( state == "off" || state == "0" || state == "false" )
			{
				this->enableCrossFader(false);

				outputs.emplace_back(Severity::Success, "Crossfade disabled.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Invalid argument. Use 'on' or 'off'.");

				return false;
			}

			return true;
		}, "Get or set crossfade transition (on/off).");

		this->bindCommand("seek", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				outputs.emplace_back(Severity::Warning, "No track is currently playing !");

				return false;
			}

			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "Current position: " << this->currentPosition() << "s / " << this->currentDuration() << "s");

				return true;
			}

			const auto position = arguments[0].asFloat();
			const auto duration = this->currentDuration();

			if ( position < 0.0F || position > duration )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Position must be between 0 and " << duration << " seconds.");

				return false;
			}

			this->seek(position);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Seeked to " << position << "s");

			return true;
		}, "Seek to position in seconds.");

		this->bindCommand("status", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			std::stringstream status;
			status << "=== Track Mixer Status ===" "\n";

			/* User state. */
			switch ( m_userState )
			{
				case UserState::Stopped :
					status << "State: Stopped" "\n";
					break;

				case UserState::Playing :
					status << "State: Playing" "\n";
					break;

				case UserState::Paused :
					status << "State: Paused" "\n";
					break;
			}

			/* Volume. */
			status << "Volume: " << (m_gain * 100.0F) << "%" "\n";

			/* Modes. */
			status << "Loop: " << (m_playMode == PlayMode::Loop ? "ON" : "OFF") << "\n";
			status << "Shuffle: " << (m_shuffleEnabled ? "ON" : "OFF") << "\n";
			status << "Crossfade: " << (m_crossFaderEnabled ? "ON" : "OFF") << "\n";

			/* Playlist info. */
			status << "Playlist: " << m_playlist.size() << " track(s)" "\n";

			if ( !m_playlist.empty() )
			{
				status << "Current track: " << (m_musicIndex + 1) << "/" << m_playlist.size() << "\n";
			}

			/* Current playback info. */
			if ( m_playingTrack != PlayingTrack::None )
			{
				status << "Position: " << this->currentPosition() << "s / " << this->currentDuration() << "s" "\n";
			}

			outputs.emplace_back(Severity::Info, status.str());

			return true;
		}, "Show current track mixer status.");

		this->bindCommand("playlist,pl", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( arguments.empty() )
			{
				if ( m_playlist.empty() )
				{
					outputs.emplace_back(Severity::Info, "Playlist is empty.");

					return true;
				}

				std::stringstream list;
				list << "=== Playlist (" << m_playlist.size() << " track(s)) ===" "\n";

				size_t index = 0;

				for ( const auto & track : m_playlist )
				{
					const auto marker = (index == m_musicIndex) ? " > " : "   ";

					list << marker << (index + 1) << ". " << track->name() << "\n";

					index++;
				}

				outputs.emplace_back(Severity::Info, list.str());

				return true;
			}

			const auto subCommand = arguments[0].asString();

			if ( subCommand == "clear" )
			{
				this->clearPlaylist();

				outputs.emplace_back(Severity::Success, "Playlist cleared.");

				return true;
			}

			if ( subCommand == "add" )
			{
				if ( arguments.size() < 2 )
				{
					outputs.emplace_back(Severity::Error, "Usage: playlist add <track_name>");

					return false;
				}

				const auto trackName = arguments[1].asString();
				const auto track = m_resourceManager.container< MusicResource >()->getResource(trackName);

				if ( track == nullptr )
				{
					outputs.emplace_back(Severity::Error, std::stringstream{} << "Track '" << trackName << "' not found !");

					return false;
				}

				this->addToPlaylist(track);

				outputs.emplace_back(Severity::Success, std::stringstream{} << "Added '" << trackName << "' to playlist.");

				return true;
			}

			if ( subCommand == "play" )
			{
				if ( arguments.size() < 2 )
				{
					outputs.emplace_back(Severity::Error, "Usage: playlist play <index>");

					return false;
				}

				const auto index = static_cast< size_t >(arguments[1].asInteger());

				if ( index < 1 || index > m_playlist.size() )
				{
					outputs.emplace_back(Severity::Error, std::stringstream{} << "Invalid index. Must be between 1 and " << m_playlist.size() << ".");

					return false;
				}

				if ( this->playIndex(index - 1) )
				{
					outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing track " << index << ".");
				}
				else
				{
					outputs.emplace_back(Severity::Error, "Unable to play track !");
				}

				return true;
			}

			outputs.emplace_back(Severity::Error, "Unknown subcommand. Use: clear, add, play");

			return false;
		}, "Manage playlist. Subcommands: clear, add <track>, play <index>");
	}
}
