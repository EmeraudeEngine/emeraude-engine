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
#include "Libs/String.hpp"
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
			const auto query = arguments[0].asString();
			auto * container = m_resourceManager.container< MusicResource >();

			std::shared_ptr< MusicResource > soundtrack;

			/* 1. Exact resource name lookup — only if actually present in the store.
			 * Rationale: container->getResource() silently returns the "Default" fallback resource
			 * when the name is unknown; checking existence first lets us detect a real miss and fall
			 * through to the fuzzy path. */
			if ( container->isResourceExists(query) )
			{
				soundtrack = container->getResource(query);
			}

			/* 2. Fallback: case-insensitive substring match against the loaded playlist. */
			if ( soundtrack == nullptr )
			{
				soundtrack = this->findPlaylistTrack(query);
			}

			if ( soundtrack == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "No track matches '" << query << "' (exact resource name or substring in current playlist) !");

				return false;
			}

			this->play(soundtrack);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing '" << soundtrack->name() << "' ...");

			return true;
		}, "Play or resume a music. Argument: exact resource name OR case-insensitive substring of a playlist entry. Without argument: resumes or starts the playlist.");

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

		this->bindCommand("nowPlaying,np", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				outputs.emplace_back(Severity::Info, "No track is currently playing.");

				return true;
			}

			if ( m_playlist.empty() || m_musicIndex >= m_playlist.size() )
			{
				outputs.emplace_back(Severity::Warning, "Playing state is inconsistent (no playlist entry at current index) !");

				return false;
			}

			const auto & track = m_playlist[m_musicIndex];

			if ( track == nullptr )
			{
				outputs.emplace_back(Severity::Error, "Current playlist entry is a null pointer !");

				return false;
			}

			std::stringstream info;
			info << "=== Now Playing ===" "\n";
			info << "Title: " << track->title() << "\n";
			info << "Artist: " << track->artist() << "\n";
			info << "Position: " << this->currentPosition() << "s / " << this->currentDuration() << "s" "\n";
			info << "Index: " << (m_musicIndex + 1) << "/" << m_playlist.size();

			outputs.emplace_back(Severity::Info, info.str());

			return true;
		}, "Show the track currently playing (title, artist, position, playlist index). Alias: 'np'.");

		this->bindCommand("listPlaylists,lpl", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			auto * playlists = m_resourceManager.container< PlaylistResource >();
			const auto names = playlists->getResourceNames();

			if ( names.empty() )
			{
				outputs.emplace_back(Severity::Info, "No playlist manifest available in the MusicPlaylists store.");

				return true;
			}

			std::stringstream list;
			list << "=== Available Playlists (" << names.size() << ") ===" "\n";

			for ( const auto & name : names )
			{
				list << "  " << name << "\n";
			}

			outputs.emplace_back(Severity::Info, list.str());

			return true;
		}, "List available playlist manifests discovered in the MusicPlaylists store. Alias: 'lpl'.");

		this->bindCommand("loadPlaylist,lp", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return false;
			}

			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: loadPlaylist <name>");

				return false;
			}

			const auto query = arguments[0].asString();
			auto * playlists = m_resourceManager.container< PlaylistResource >();

			std::shared_ptr< PlaylistResource > manifest;

			/* 1. Exact match guarded by isResourceExists to avoid the Default fallback.
			 * asyncLoad=false forces the JSON to be parsed synchronously before we touch the playlist. */
			if ( playlists->isResourceExists(query) )
			{
				manifest = playlists->getResource(query, false);
			}

			/* 2. Fuzzy fallback: case-insensitive substring over available playlist names. */
			if ( manifest == nullptr )
			{
				const auto needle = String::toLower(query);

				for ( const auto & name : playlists->getResourceNames() )
				{
					if ( String::toLower(name).find(needle) != std::string::npos )
					{
						manifest = playlists->getResource(name, false);

						break;
					}
				}
			}

			if ( manifest == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "No playlist manifest matches '" << query << "' !");

				return false;
			}

			if ( !this->loadPlaylist(manifest) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to load playlist '" << manifest->name() << "' (empty or unresolved tracks) !");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Loaded playlist '" << manifest->name() << "' (" << manifest->trackCount() << " tracks).");

			return true;
		}, "Swap the current playlist for a manifest from the MusicPlaylists store. Argument: exact name OR substring. If music was playing, restarts from track 1 of the new playlist.");

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
