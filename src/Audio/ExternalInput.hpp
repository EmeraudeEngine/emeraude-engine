/*
 * src/Audio/ExternalInput.hpp
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
#include <cstdint>
#include <fstream>
#include <thread>
#include <vector>
#include <filesystem>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/alc.h"

/* Local inclusions. */
#include "Libs/WaveFactory/Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Audio
	{
		class Manager;
	}

	class PrimaryServices;
}

namespace EmEn::Audio
{
	/**
	 * @brief Class that define a device to grab audio from outside the engine like a real microphone.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class ExternalInput final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AudioExternalInputService"};

			/**
			 * @brief Constructs the external audio input.
			 * @param primaryServices Reference to primary services for settings and filesystem access.
			 * @param audioManager Reference to the audio manager.
			 */
			ExternalInput (PrimaryServices & primaryServices, Manager & audioManager) noexcept;

			/**
			 * @brief Returns a list a available input audio devices.
			 * @return bool
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			availableInputDevices () const noexcept
			{
				return m_availableDevices;
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
			 * @brief Starts the recording in memory mode.
			 * @note Samples are accumulated in RAM. Use saveRecord() to write them to disk.
			 */
			void start () noexcept;

			/**
			 * @brief Starts the recording in streaming mode.
			 * @details Samples are written directly to a WAV file as they are captured.
			 * The file header is patched when stop() is called.
			 * @param outputPath The output WAV file path.
			 * @return true if the file was opened successfully.
			 */
			[[nodiscard]]
			bool start (const std::filesystem::path & outputPath) noexcept;

			/**
			 * @brief Stops the recording.
			 * @details In streaming mode, finalizes the WAV header and closes the file.
			 */
			void stop () noexcept;

			/**
			 * @brief Saves the in-memory recording to file.
			 * @note Only valid after a memory-mode recording (start() without path).
			 * @param filepath A reference to a filesystem path.
			 * @return bool
			 */
			[[nodiscard]]
			bool saveRecord (const std::filesystem::path & filepath) const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Queries the available basic audio device and save it.
			 * @return bool
			 */
			bool selectDevice () noexcept;

			/**
			 * @brief Process the capture task.
			 */
			void recordingTask () noexcept;

			/**
			 * @brief Writes a standard 44-byte WAV header.
			 * @param stream The output stream to write to.
			 * @param dataSize The size of the data chunk (0 for placeholder).
			 * @return bool
			 */
			bool writeWAVHeader (std::ostream & stream, uint32_t dataSize) const noexcept;

			/** @brief No audio capture device found to record sound. */
			static inline bool s_audioCaptureAvailable{false};

			PrimaryServices & m_primaryServices; ///< Primary services for settings and filesystem access.
			Manager & m_audioManager; ///< Audio manager owning this recorder.
			std::vector< std::string > m_availableDevices;
			std::string m_selectedDeviceName;
			ALCdevice * m_device{nullptr};
			Libs::WaveFactory::Channels m_channels{Libs::WaveFactory::Channels::Mono};
			Libs::WaveFactory::Frequency m_frequency{Libs::WaveFactory::Frequency::PCM48000Hz};
			/* Memory mode. */
			std::vector< ALshort > m_samples;
			/* Streaming mode. */
			std::filesystem::path m_outputPath{};
			std::ofstream m_outputFileStream{};
			uint32_t m_streamByteCount{0};
			/* Common. */
			std::thread m_process;
			bool m_showInformation{false};
			bool m_isRecording{false};
			bool m_streamingMode{false};
	};
}
