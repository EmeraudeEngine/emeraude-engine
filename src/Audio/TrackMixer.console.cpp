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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "TrackMixer.hpp"

/* STL inclusions. */
#include <optional>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "String.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Audio
{
	using namespace Base;

	void
	TrackMixer::onRegisterToConsole () noexcept
	{
		/* A refusal that keeps the Warning severity the console has always printed for it. */
		const auto refuse = [] (const char * message) {
			return Console::CommandResult::fromOutputs({Console::Output{Severity::Warning, message}}, false);
		};

		/* The on/off vocabulary of the mode switches: "on"/"1"/"true" and "off"/"0"/"false". */
		const auto parseSwitch = [] (const std::string & state) -> std::optional< bool > {
			if ( state == "on" || state == "1" || state == "true" )
			{
				return true;
			}

			if ( state == "off" || state == "0" || state == "false" )
			{
				return false;
			}

			return std::nullopt;
		};

		this->bindCommand("play", "Play or resume a music. Without argument: resumes or starts the playlist.",
			{
				{"track", "Exact music resource name OR case-insensitive substring of a playlist entry; omit it to resume or start the playlist."}
			},
			[this, refuse] (const std::optional< std::string > & query) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				/* No argument: resume current track or start playlist. */
				if ( !query.has_value() )
				{
					if ( m_userState == UserState::Paused )
					{
						this->resume();

						return Console::CommandResult::success("Resumed.");
					}

					/* Nothing playing: start the playlist if available. */
					if ( !m_playlist.empty() )
					{
						if ( this->playIndex(m_musicIndex) )
						{
							std::stringstream message;
							message << "Playing track " << (m_musicIndex + 1) << "/" << m_playlist.size() << ".";

							return Console::CommandResult::success(message.str());
						}
					}

					return refuse("Nothing to play !");
				}

				/* Search the song by name. */
				auto * container = m_resourceManager.container< MusicResource >();

				std::shared_ptr< MusicResource > soundtrack;

				/* 1. Exact resource name lookup — only if actually present in the store.
				 * Rationale: container->getResource() silently returns the "Default" fallback resource
				 * when the name is unknown; checking existence first lets us detect a real miss and fall
				 * through to the fuzzy path. */
				if ( container->isResourceExists(*query) )
				{
					soundtrack = container->getResource(*query);
				}

				/* 2. Fallback: case-insensitive substring match against the loaded playlist. */
				if ( soundtrack == nullptr )
				{
					soundtrack = this->findPlaylistTrack(*query);
				}

				if ( soundtrack == nullptr )
				{
					return Console::CommandResult::error("No track matches '" + *query + "' (exact resource name or substring in current playlist) !");
				}

				this->play(soundtrack);

				return Console::CommandResult::success("Playing '" + soundtrack->name() + "' ...");
			});

		this->bindCommand("pause", "Pause music playback.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				return refuse("There is no track playing !");
			}

			this->pause();

			return Console::CommandResult::success("Paused.");
		});

		this->bindCommand("stop", "Stop music.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				return refuse("There is no track playing !");
			}

			this->stop();

			return Console::CommandResult::success("Stopped.");
		});

		this->bindCommand("volume,vol", "Get or set the music volume.",
			{
				{"volume", "The new volume, in percent (0 to 100); omit it to read the current one."}
			},
			[this, refuse] (std::optional< float > newVolume) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				if ( !newVolume.has_value() )
				{
					std::stringstream message;
					message << "Current volume: " << (m_gain * 100.0F) << "%";

					return Console::CommandResult::info(message.str());
				}

				if ( *newVolume < 0.0F || *newVolume > 100.0F )
				{
					return Console::CommandResult::error("Volume must be between 0 and 100.");
				}

				this->setVolume(*newVolume / 100.0F);

				std::stringstream message;
				message << "Volume set to " << *newVolume << "%";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("next", "Play next track in playlist.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( m_playlist.empty() )
			{
				return refuse("Playlist is empty !");
			}

			if ( !this->next() )
			{
				return Console::CommandResult::error("Unable to play next track !");
			}

			std::stringstream message;
			message << "Playing next track (" << (m_musicIndex + 1) << "/" << m_playlist.size() << ")";

			return Console::CommandResult::success(message.str());
		});

		this->bindCommand("previous,prev", "Play previous track in playlist.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( m_playlist.empty() )
			{
				return refuse("Playlist is empty !");
			}

			if ( !this->previous() )
			{
				return Console::CommandResult::error("Unable to play previous track !");
			}

			std::stringstream message;
			message << "Playing previous track (" << (m_musicIndex + 1) << "/" << m_playlist.size() << ")";

			return Console::CommandResult::success(message.str());
		});

		this->bindCommand("shuffle", "Get or set the shuffle mode.",
			{
				{"state", "'on' (or 1, true) / 'off' (or 0, false); omit it to read the current mode."}
			},
			[this, refuse, parseSwitch] (const std::optional< std::string > & state) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				if ( !state.has_value() )
				{
					return Console::CommandResult::info(std::string{"Shuffle mode: "} + (m_shuffleEnabled ? "ON" : "OFF"));
				}

				const auto enabled = parseSwitch(*state);

				if ( !enabled.has_value() )
				{
					return Console::CommandResult::error("Invalid argument. Use 'on' or 'off'.");
				}

				this->enableShuffle(*enabled);

				return Console::CommandResult::success(*enabled ? "Shuffle mode enabled." : "Shuffle mode disabled.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("loop", "Get or set the loop mode.",
			{
				{"state", "'on' (or 1, true) / 'off' (or 0, false); omit it to read the current mode."}
			},
			[this, refuse, parseSwitch] (const std::optional< std::string > & state) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				if ( !state.has_value() )
				{
					return Console::CommandResult::info(std::string{"Loop mode: "} + (m_playMode == PlayMode::Loop ? "ON" : "OFF"));
				}

				const auto enabled = parseSwitch(*state);

				if ( !enabled.has_value() )
				{
					return Console::CommandResult::error("Invalid argument. Use 'on' or 'off'.");
				}

				this->setPlayMode(*enabled ? PlayMode::Loop : PlayMode::Once);

				return Console::CommandResult::success(*enabled ? "Loop mode enabled." : "Loop mode disabled.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("crossfade", "Get or set the crossfade transition.",
			{
				{"state", "'on' (or 1, true) / 'off' (or 0, false); omit it to read the current setting."}
			},
			[this, refuse, parseSwitch] (const std::optional< std::string > & state) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				if ( !state.has_value() )
				{
					return Console::CommandResult::info(std::string{"Crossfade: "} + (m_crossFaderEnabled ? "ON" : "OFF"));
				}

				const auto enabled = parseSwitch(*state);

				if ( !enabled.has_value() )
				{
					return Console::CommandResult::error("Invalid argument. Use 'on' or 'off'.");
				}

				this->enableCrossFader(*enabled);

				return Console::CommandResult::success(*enabled ? "Crossfade enabled." : "Crossfade disabled.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("seek", "Get or set the playback position of the current track.",
			{
				{"position", "The new position, in seconds from the start of the track; omit it to read the current one."}
			},
			[this, refuse] (std::optional< float > position) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				if ( m_playingTrack == PlayingTrack::None )
				{
					return refuse("No track is currently playing !");
				}

				if ( !position.has_value() )
				{
					std::stringstream message;
					message << "Current position: " << this->currentPosition() << "s / " << this->currentDuration() << "s";

					return Console::CommandResult::info(message.str());
				}

				const auto duration = this->currentDuration();

				if ( *position < 0.0F || *position > duration )
				{
					std::stringstream message;
					message << "Position must be between 0 and " << duration << " seconds.";

					return Console::CommandResult::error(message.str());
				}

				this->seek(*position);

				std::stringstream message;
				message << "Seeked to " << *position << "s";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("status", "Show current track mixer status.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
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

			return Console::CommandResult::info(status.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("nowPlaying,np", "Show the track currently playing (title, artist, position, playlist index).", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( m_playingTrack == PlayingTrack::None )
			{
				return Console::CommandResult::info("No track is currently playing.");
			}

			if ( m_playlist.empty() || m_musicIndex >= m_playlist.size() )
			{
				return refuse("Playing state is inconsistent (no playlist entry at current index) !");
			}

			const auto & track = m_playlist[m_musicIndex];

			if ( track == nullptr )
			{
				return Console::CommandResult::error("Current playlist entry is a null pointer !");
			}

			std::stringstream info;
			info << "=== Now Playing ===" "\n";
			info << "Title: " << track->title() << "\n";
			info << "Artist: " << track->artist() << "\n";
			info << "Position: " << this->currentPosition() << "s / " << this->currentDuration() << "s" "\n";
			info << "Index: " << (m_musicIndex + 1) << "/" << m_playlist.size();

			return Console::CommandResult::info(info.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("currentPlaylist,cpl", "Show the manifest name currently backing the playlist. Returns 'no manifest' if the playlist was modified ad-hoc via addToPlaylist.", [this] () {
			const auto & manifest = this->loadedPlaylist();

			std::stringstream message;

			if ( manifest == nullptr )
			{
				message << "No playlist manifest loaded (ad-hoc or empty playlist). Tracks: " << m_playlist.size() << ".";
			}
			else
			{
				message << "Current playlist: '" << manifest->name() << "' (" << m_playlist.size() << " tracks).";
			}

			return Console::CommandResult::info(message.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("listPlaylists,lpl", "List available playlists (store-backed manifests + runtime-created).", [this] () {
			const auto names = this->availablePlaylistNames();

			if ( names.empty() )
			{
				return Console::CommandResult::info("No playlist manifest available.");
			}

			std::stringstream list;
			list << "=== Available Playlists (" << names.size() << ") ===" "\n";

			for ( const auto & name : names )
			{
				list << "  " << name << "\n";
			}

			return Console::CommandResult::info(list.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("loadPlaylist,lp", "Swap the current playlist for a manifest from the MusicPlaylists store. If music was playing, restarts from track 1 of the new playlist.",
			{
				{"name", "Exact playlist manifest name OR case-insensitive substring of one (listPlaylists() lists them)."}
			},
			[this, refuse] (const std::string & query) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

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
					return Console::CommandResult::error("No playlist manifest matches '" + query + "' !");
				}

				if ( !this->loadPlaylist(manifest) )
				{
					return Console::CommandResult::error("Failed to load playlist '" + manifest->name() + "' (empty or unresolved tracks) !");
				}

				std::stringstream message;
				message << "Loaded playlist '" << manifest->name() << "' (" << manifest->trackCount() << " tracks).";

				return Console::CommandResult::success(message.str());
			});

		this->bindCommand("playlist,pl", "Lists the playlist (the current entry is marked).", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( m_playlist.empty() )
			{
				return Console::CommandResult::info("Playlist is empty.");
			}

			std::stringstream list;
			list << "=== Playlist (" << m_playlist.size() << " track(s)) ===" "\n";

			size_t index = 0;

			for ( const auto & track : m_playlist )
			{
				const auto * const marker = (index == m_musicIndex) ? " > " : "   ";

				list << marker << (index + 1) << ". " << track->name() << "\n";

				index++;
			}

			return Console::CommandResult::info(list.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("playlistClear", "Empties the playlist.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			this->clearPlaylist();

			return Console::CommandResult::success("Playlist cleared.");
		}, Console::CommandHint::Destructive | Console::CommandHint::Idempotent);

		this->bindCommand("playlistAdd", "Appends a music to the playlist.",
			{
				{"track", "The music resource name."}
			},
			[this, refuse] (const std::string & trackName) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				const auto track = m_resourceManager.container< MusicResource >()->getResource(trackName);

				if ( track == nullptr )
				{
					return Console::CommandResult::error("Track '" + trackName + "' not found !");
				}

				this->addToPlaylist(track);

				return Console::CommandResult::success("Added '" + trackName + "' to playlist.");
			});

		this->bindCommand("playlistPlay", "Plays one entry of the playlist.",
			{
				{"index", "The 1-based playlist index (playlist() lists them)."}
			},
			[this, refuse] (int32_t index) {
				if ( !this->usable() )
				{
					return refuse("The track mixer is unavailable !");
				}

				if ( index < 1 || static_cast< size_t >(index) > m_playlist.size() )
				{
					return Console::CommandResult::error("Invalid index. Must be between 1 and " + std::to_string(m_playlist.size()) + ".");
				}

				if ( !this->playIndex(static_cast< size_t >(index) - 1) )
				{
					return Console::CommandResult::error("Unable to play track !");
				}

				return Console::CommandResult::success("Playing track " + std::to_string(index) + ".");
			});
	}
}
