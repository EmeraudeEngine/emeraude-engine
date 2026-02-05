/*
 * src/Graphics/ExternalInput.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"
#include "PlatformSpecific/VideoCaptureDevice.hpp"

/* Forward declarations. */
namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Class that define a device to grab video from outside the engine like a webcam.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class ExternalInput final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GraphicsExternalInputService"};

			/**
			 * @brief Constructs the external video input.
			 * @param primaryServices A reference to the primary services.
			 */
			explicit
			ExternalInput (PrimaryServices & primaryServices) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices}
			{

			}

			/**
			 * @brief Enumerates available video capture devices on the system.
			 * @return std::vector< PlatformSpecific::VideoCaptureDeviceInfo >
			 */
			[[nodiscard]]
			static std::vector< PlatformSpecific::VideoCaptureDeviceInfo > enumerateDevices () noexcept;

			/**
			 * @brief Opens a video capture device.
			 * @param devicePath The device path (e.g., "/dev/video0" on Linux).
			 * @param width The requested capture width. Default 0 means use settings value.
			 * @param height The requested capture height. Default 0 means use settings value.
			 * @return bool True if the device was successfully opened.
			 */
			[[nodiscard]]
			bool openDevice (const std::string & devicePath, uint32_t width = 0, uint32_t height = 0) noexcept;

			/**
			 * @brief Closes the currently open video capture device.
			 */
			void closeDevice () noexcept;

			/**
			 * @brief Returns whether a video capture device is currently open.
			 * @return bool
			 */
			[[nodiscard]]
			bool isDeviceOpen () const noexcept;

			/**
			 * @brief Captures a single frame and stores it in the internal frame buffer.
			 * @return bool True if the frame was successfully captured.
			 */
			[[nodiscard]]
			bool captureFrame () noexcept;

			/**
			 * @brief Returns a const reference to all captured frames.
			 * @return const std::vector< Libs::PixelFactory::Pixmap< uint8_t > > &
			 */
			[[nodiscard]]
			const std::vector< Libs::PixelFactory::Pixmap< uint8_t > > & capturedFrames () const noexcept;

			/**
			 * @brief Returns a pointer to the last captured frame, or nullptr if none.
			 * @return const Libs::PixelFactory::Pixmap< uint8_t > *
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Pixmap< uint8_t > * lastFrame () const noexcept;

			/**
			 * @brief Clears all captured frames from memory.
			 */
			void clearFrames () noexcept;

			/**
			 * @brief Returns the number of captured frames stored.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t frameCount () const noexcept;

			/**
			 * @brief Returns the configured default device index from settings.
			 * @return int32_t -1 means auto (first available).
			 */
			[[nodiscard]]
			int32_t defaultDeviceIndex () const noexcept;

			/**
			 * @brief Returns the configured capture width from settings.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t captureWidth () const noexcept;

			/**
			 * @brief Returns the configured capture height from settings.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t captureHeight () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			PrimaryServices & m_primaryServices;
			int32_t m_defaultDeviceIndex{-1};
			uint32_t m_captureWidth{640};
			uint32_t m_captureHeight{480};
			PlatformSpecific::VideoCaptureDevice m_captureDevice;
			std::vector< uint8_t > m_rgbaBuffer;
			std::vector< Libs::PixelFactory::Pixmap< uint8_t > > m_capturedFrames;
	};
}
