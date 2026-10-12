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
#include <utility>

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
					const auto current = this->status();

					if ( current.userState == UserState::Paused )
					{
						this->resume();

						return Console::CommandResult::success("Resumed.");
					}

					/* Nothing playing: start the playlist if available (from its start after a track played outside it). */
					if ( !current.playlist.empty() )
					{
						const auto index = current.trackIndex < current.playlist.size() ? current.trackIndex : 0;

						if ( this->playIndex(index) )
						{
							std::stringstream message;
							message << "Playing track " << (index + 1) << "/" << current.playlist.size() << ".";

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

			if ( !this->isPlaying() )
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

			if ( !this->isPlaying() )
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
					message << "Current volume: " << (this->volume() * 100.0F) << "%";

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

			if ( this->playlistSize() == 0 )
			{
				return refuse("Playlist is empty !");
			}

			if ( !this->next() )
			{
				return Console::CommandResult::error("Unable to play next track !");
			}

			const auto current = this->status();

			std::stringstream message;
			message << "Playing next track (" << (current.trackIndex + 1) << "/" << current.playlist.size() << ")";

			return Console::CommandResult::success(message.str());
		});

		this->bindCommand("previous,prev", "Play previous track in playlist.", [this, refuse] () {
			if ( !this->usable() )
			{
				return refuse("The track mixer is unavailable !");
			}

			if ( this->playlistSize() == 0 )
			{
				return refuse("Playlist is empty !");
			}

			if ( !this->previous() )
			{
				return Console::CommandResult::error("Unable to play previous track !");
			}

			const auto current = this->status();

			std::stringstream message;
			message << "Playing previous track (" << (current.trackIndex + 1) << "/" << current.playlist.size() << ")";

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
					return Console::CommandResult::info(std::string{"Shuffle mode: "} + (this->isShuffleEnabled() ? "ON" : "OFF"));
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
					return Console::CommandResult::info(std::string{"Loop mode: "} + (this->playMode() == PlayMode::Loop ? "ON" : "OFF"));
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
					return Console::CommandResult::info(std::string{"Crossfade: "} + (this->isCrossFaderEnabled() ? "ON" : "OFF"));
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

				if ( !this->isPlaying() )
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

			const auto current = this->status();

			std::stringstream status;
			status << "=== Track Mixer Status ===" "\n";

			/* User state. */
			switch ( current.userState )
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
			status << "Volume: " << (current.gain * 100.0F) << "%" "\n";

			/* Modes. */
			status << "Loop: " << (current.playMode == PlayMode::Loop ? "ON" : "OFF") << "\n";
			status << "Shuffle: " << (current.shuffleEnabled ? "ON" : "OFF") << "\n";
			status << "Crossfade: " << (current.crossFaderEnabled ? "ON" : "OFF") << "\n";

			/* Playlist info. */
			status << "Playlist: " << current.playlist.size() << " track(s)" "\n";

			if ( current.trackIndex < current.playlist.size() )
			{
				status << "Current track: " << (current.trackIndex + 1) << "/" << current.playlist.size() << "\n";
			}

			/* Current playback info. */
			if ( current.hasTrack )
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

			const auto current = this->status();

			if ( !current.hasTrack )
			{
				return Console::CommandResult::info("No track is currently playing.");
			}

			if ( current.trackIndex >= current.playlist.size() )
			{
				return refuse("Playing state is inconsistent (no playlist entry at current index) !");
			}

			const auto & track = current.currentTrack;

			if ( track == nullptr )
			{
				return Console::CommandResult::error("Current playlist entry is a null pointer !");
			}

			std::stringstream info;
			info << "=== Now Playing ===" "\n";
			info << "Title: " << track->title() << "\n";
			info << "Artist: " << track->artist() << "\n";
			info << "Position: " << this->currentPosition() << "s / " << this->currentDuration() << "s" "\n";
			info << "Index: " << (current.trackIndex + 1) << "/" << current.playlist.size();

			return Console::CommandResult::info(info.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("currentPlaylist,cpl", "Show the manifest name currently backing the playlist. Returns 'no manifest' if the playlist was modified ad-hoc via addToPlaylist.", [this] () {
			const auto current = this->status();

			std::stringstream message;

			if ( current.loadedPlaylist == nullptr )
			{
				message << "No playlist manifest loaded (ad-hoc or empty playlist). Tracks: " << current.playlist.size() << ".";
			}
			else
			{
				message << "Current playlist: '" << current.loadedPlaylist->name() << "' (" << current.playlist.size() << " tracks).";
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

			const auto current = this->status();

			if ( current.playlist.empty() )
			{
				return Console::CommandResult::info("Playlist is empty.");
			}

			std::stringstream list;
			list << "=== Playlist (" << current.playlist.size() << " track(s)) ===" "\n";

			size_t index = 0;

			for ( const auto & track : current.playlist )
			{
				const auto * const marker = (index == current.trackIndex) ? " > " : "   ";

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

				auto * musics = m_resourceManager.container< MusicResource >();

				/* NOTE: getResource() answers the "Default" resource for an unknown name, never nullptr: the existence is
				 * checked first (the refusal below was unreachable). */
				if ( !musics->isResourceExists(trackName) )
				{
					return Console::CommandResult::error("Track '" + trackName + "' not found !");
				}

				const auto track = musics->getResource(trackName);

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

				const auto count = this->playlistSize();

				if ( index < 1 || std::cmp_greater(index, count) )
				{
					return Console::CommandResult::error("Invalid index. Must be between 1 and " + std::to_string(count) + ".");
				}

				if ( !this->playIndex(static_cast< size_t >(index) - 1) )
				{
					return Console::CommandResult::error("Unable to play track !");
				}

				return Console::CommandResult::success("Playing track " + std::to_string(index) + ".");
			});
	}
}
