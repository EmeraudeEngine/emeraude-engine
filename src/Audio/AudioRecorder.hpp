/*
 * src/Audio/AudioRecorder.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <thread>
#include <vector>
#include <filesystem>

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/alc.h"

/* Local inclusions. */
#include "Libs/WaveFactory/Types.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief Class that define a device to grab audio from outside the engine like a real microphone.
	 */
	class AudioRecorder final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AudioRecorder"};

			/**
			 * @brief Constructs an audio recorder.
			 */
			AudioRecorder () noexcept = default;

			/**
			 * @brief Destructs the audio recorder.
			 */
			~AudioRecorder ()
			{
				if ( m_process.joinable() )
				{
					m_process.join();
				}
			}

			/**
			 * @brief Set an input device to enable recording.
			 * @param device A pointer to an input device.
			 * @param channels The recording channel number.
			 * @param frequency The recording frequency.
			 * @return void
			 */
			void
			configure (ALCdevice * device, Libs::WaveFactory::Channels channels, Libs::WaveFactory::Frequency frequency) noexcept
			{
				if ( device == nullptr )
				{
					this->stop();
				}

				m_device = device;
				m_channels = channels;
				m_frequency = frequency;
			}

			/**
			 * @brief Returns whether the capture is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRecording () const noexcept
			{
				return m_isRecording;
			}

			/**
			 * @brief Starts the recording.
			 */
			void start () noexcept;

			/**
			 * @brief Stops the recording.
			 */
			void stop () noexcept;

			/**
			 * @brief Saves the recording to file.
			 * @param filepath A reference to a filesystem path.
			 * @return bool
			 */
			[[nodiscard]]
			bool saveRecord (const std::filesystem::path & filepath) const noexcept;

		private:

			/**
			 * @brief Process the capture task.
			 */
			void recordingTask () noexcept;

			ALCdevice * m_device{nullptr};
			Libs::WaveFactory::Channels m_channels{Libs::WaveFactory::Channels::Invalid};
			Libs::WaveFactory::Frequency m_frequency{Libs::WaveFactory::Frequency::Invalid};
			std::vector< ALshort > m_samples;
			std::thread m_process;
			bool m_isRecording{false};
	};
}
