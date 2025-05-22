/*
 * src/Audio/ExternalInput.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <filesystem>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/alc.h"

/* Local inclusions for usages. */
#include "Libs/WaveFactory/Types.hpp"

/* Forward declarations. */
namespace EmEn
{
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
			 * @param primaryServices A reference to primary services.
			 */
			explicit
			ExternalInput (PrimaryServices & primaryServices) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices}
			{

			}

			/**
			 * @brief Destructs the external audio input.
			 */
			~ExternalInput () override
			{
				if ( m_process.joinable() )
				{
					m_process.join();
				}
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_device != nullptr;
			}

			/**
			 * @brief Returns whether the capture is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRecording () const noexcept
			{
				return m_flags[IsRecording];
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

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Process the capture task.
			 */
			void process () noexcept;

			/**
			 * @brief Queries devices information.
			 * @return bool.
			 */
			bool queryDevices () noexcept;

			/* Flag names */
			static constexpr auto ShowInformation{0UL};
			static constexpr auto IsRecording{1UL};

			PrimaryServices & m_primaryServices;
			ALCdevice * m_device{nullptr};
			std::vector< std::string > m_availableCaptureDevices;
			std::vector< ALshort > m_samples;
			std::thread m_process;
			Libs::WaveFactory::Frequency m_frequency = Libs::WaveFactory::Frequency::PCM44100Hz;
			std::array< bool, 8 > m_flags{
				false/*ShowInformation*/,
				false/*IsRecording*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
