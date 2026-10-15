/*
 * tools/stress/TrackMixerStress.cpp
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

/* STL inclusions. */
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

/* Local inclusions. */
#include "Audio/TrackMixer.hpp"
#include "Core.hpp"
#include "PlatformManager.hpp"
#include "Thread.hpp"

/*
 * The ThreadSanitizer proof of Audio::TrackMixer (decision D12, Ave Robustus II; built with EMERAUDE_ENABLE_TSAN only).
 * A window-less, CEF-less engine plays a playlist while four threads drive every public method of the mixer at random
 * moments, the logic thread reads its state each cycle, and the mixer's own threads run (the event thread: the next
 * track, the cross-fade; OpenAL Soft's event thread: a track that ended; a loading thread: a MIDI track rendered).
 * The verdict is TSan's: run it with TSAN_OPTIONS=halt_on_error=1 (procedure: projet-alpha docs/unit-testing.md).
 */
namespace
{
	using namespace EmEn;

	/* How long the drivers run, and the pause between two calls of one driver. */
	constexpr std::chrono::seconds RunDuration{30};
	constexpr std::chrono::milliseconds CallPeriod{4};
	constexpr size_t DriverCount{4};

	class TrackMixerStress final : public Core
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TrackMixerStress"};

			TrackMixerStress (int argc, char * * argv) noexcept
				: Core{argc, argv, "EmeraudeTrackMixerStress", {1, 0, 0}, "EmeraudeEngine", "emeraude-engine.org"}
			{

			}

			TrackMixerStress (const TrackMixerStress & copy) noexcept = delete;
			TrackMixerStress (TrackMixerStress && copy) noexcept = delete;
			TrackMixerStress & operator= (const TrackMixerStress & copy) noexcept = delete;
			TrackMixerStress & operator= (TrackMixerStress && copy) noexcept = delete;

			~TrackMixerStress () override
			{
				this->stopDrivers();
			}

			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return Base::Hash::FNV1a(ClassId);
			}

			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == Base::Hash::FNV1a(ClassId);
			}

			/** @brief Whether the run drove the mixer (a mixer refused at start is a failed run). */
			[[nodiscard]]
			bool
			drove () const noexcept
			{
				return m_calls.load() > 0;
			}

		private:

			bool
			onCoreStarted (const Arguments & /*arguments*/, Settings & /*settings*/) noexcept override
			{
				auto & mixer = this->audioManager().trackMixer();

				if ( !mixer.usable() )
				{
					TraceError{ClassId} << "The track mixer is unavailable (no audio device ?) !";

					return false;
				}

				const auto names = mixer.availablePlaylistNames();

				if ( names.empty() )
				{
					TraceError{ClassId} << "No playlist manifest: pass --add-data-directory=<a data directory with MusicPlaylists> !";

					return false;
				}

				m_playlist = this->resourceManager().container< Audio::PlaylistResource >()->getResource(names.front(), false);

				if ( m_playlist == nullptr || !mixer.loadPlaylist(m_playlist) )
				{
					TraceError{ClassId} << "Unable to load the playlist '" << names.front() << "' !";

					return false;
				}

				mixer.enableCrossFader(true);
				static_cast< void >(mixer.play());

				m_start = std::chrono::steady_clock::now();

				for ( size_t index = 0; index < DriverCount; ++index )
				{
					if ( !m_drivers[index].start([this, index, token = m_stop.get_token()] { this->drive(index, token); }) )
					{
						TraceError{ClassId} << "Unable to start the driver thread #" << index << " !";

						return false;
					}
				}

				TraceInfo{ClassId} << "Playlist '" << names.front() << "' loaded, " << DriverCount << " drivers for " << RunDuration.count() << " s.";

				return true;
			}

			void
			onCoreMainLoopCycle () noexcept override
			{
				if ( !m_stopping && std::chrono::steady_clock::now() - m_start >= RunDuration )
				{
					m_stopping = true;

					this->stop();
				}
			}

			void
			onCoreProcessLogics (size_t /*engineCycle*/) noexcept override
			{
				/* NOTE: The logic thread is a fifth reader. */
				const auto status = this->audioManager().trackMixer().status();

				m_logicReads.fetch_add(status.playlist.size() > 0 ? 1U : 0U);
			}

			bool
			onBeforeCoreStop () noexcept override
			{
				/* NOTE: The services are still up: the drivers stop and join before the mixer goes. */
				this->stopDrivers();

				TraceInfo{ClassId} << m_calls.load() << " calls by the drivers, " << m_logicReads.load() << " logic-thread reads.";

				return true;
			}

			void
			stopDrivers () noexcept
			{
				m_stop.request_stop();

				for ( auto & driver : m_drivers )
				{
					if ( driver.joinable() )
					{
						driver.join();
					}
				}
			}

			/** @brief One driver: a fixed sequence of calls per index (reproducible), every CallPeriod. */
			void
			drive (size_t driverIndex, const std::stop_token & token) noexcept
			{
				auto & mixer = this->audioManager().trackMixer();
				uint32_t step = static_cast< uint32_t >(driverIndex) * 7U;

				while ( !token.stop_requested() )
				{
					switch ( step % 16U )
					{
						case 0 : static_cast< void >(mixer.next()); break;
						case 1 : static_cast< void >(mixer.previous()); break;
						case 2 : mixer.pause(); break;
						case 3 : mixer.resume(); break;
						case 4 : static_cast< void >(mixer.play()); break;
						case 5 : mixer.setVolume(static_cast< float >(step % 10U) / 10.0F); break;
						case 6 : mixer.enableCrossFader((step & 1U) == 0U); break;
						case 7 : mixer.enableShuffle((step & 2U) == 0U); break;
						case 8 : static_cast< void >(mixer.status()); break;
						case 9 : static_cast< void >(mixer.playlist()); break;
						case 10 : mixer.seek(mixer.currentPosition() + 1.0F); break;
						case 11 : static_cast< void >(mixer.currentDuration()); break;
						case 12 : static_cast< void >(mixer.playIndex(step % 3U)); break;
						case 13 : static_cast< void >(mixer.findPlaylistTrack("a")); break;
						case 14 : mixer.setPlayMode((step & 4U) == 0U ? Audio::PlayMode::Loop : Audio::PlayMode::Once); break;
						/* NOTE: Rare on purpose: a swap stops the music. */
						default :
							if ( step % 64U == 15U )
							{
								static_cast< void >(mixer.loadPlaylist(m_playlist));
							}
							else
							{
								mixer.stop();
							}
							break;
					}

					m_calls.fetch_add(1U);
					step += 5U;

					std::this_thread::sleep_for(CallPeriod);
				}
			}

			std::shared_ptr< Audio::PlaylistResource > m_playlist;
			std::chrono::steady_clock::time_point m_start{};
			std::atomic< uint64_t > m_calls{0};
			std::atomic< uint64_t > m_logicReads{0};
			std::stop_source m_stop;
			bool m_stopping{false};
			/* NOTE: LAST (Ave Robustus II, member order): the drivers use every member above. */
			std::array< Base::Thread, DriverCount > m_drivers{};
	};
}

int
main (int argc, char * * argv)
{
	/* NOTE: Before any thread, as every consumer's main() (PlatformManager::prepareVulkanLoaderEnvironment()). */
	static_cast< void >(EmEn::PlatformManager::prepareVulkanLoaderEnvironment());

	TrackMixerStress stress{argc, argv};

	const auto exitCode = stress.run();

	if ( !stress.drove() )
	{
		std::cerr << "[TrackMixerStress] The mixer was never driven: the run proves nothing !" "\n";

		return 1;
	}

	return exitCode;
}
