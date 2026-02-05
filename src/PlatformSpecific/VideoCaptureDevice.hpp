/*
 * src/PlatformSpecific/VideoCaptureDevice.hpp
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
#include <cstdint>
#include <string>
#include <vector>

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief Describes a video capture device available on the system.
	 */
	struct VideoCaptureDeviceInfo
	{
		std::string devicePath;
		std::string deviceName;
		uint32_t index{0};
	};

	/**
	 * @brief Cross-platform video capture device using native APIs.
	 * @note Linux: V4L2, macOS: AVFoundation (stub), Windows: Media Foundation (stub).
	 */
	class VideoCaptureDevice final
	{
		public:

			/**
			 * @brief Constructs a video capture device (closed state).
			 */
			VideoCaptureDevice () noexcept = default;

			/**
			 * @brief Destructs the video capture device, closing if open.
			 */
			~VideoCaptureDevice () noexcept;

			VideoCaptureDevice (const VideoCaptureDevice &) = delete;
			VideoCaptureDevice (VideoCaptureDevice &&) = delete;
			VideoCaptureDevice & operator= (const VideoCaptureDevice &) = delete;
			VideoCaptureDevice & operator= (VideoCaptureDevice &&) = delete;

			/**
			 * @brief Enumerates available video capture devices on the system.
			 * @return std::vector< VideoCaptureDeviceInfo >
			 */
			[[nodiscard]]
			static std::vector< VideoCaptureDeviceInfo > enumerateDevices () noexcept;

			/**
			 * @brief Opens a video capture device and starts streaming.
			 * @param devicePath The device path (e.g., "/dev/video0" on Linux).
			 * @param requestedWidth The requested capture width in pixels.
			 * @param requestedHeight The requested capture height in pixels.
			 * @return bool True if the device was successfully opened.
			 */
			[[nodiscard]]
			bool open (const std::string & devicePath, uint32_t requestedWidth, uint32_t requestedHeight) noexcept;

			/**
			 * @brief Closes the video capture device and releases resources.
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the device is currently open and streaming.
			 * @return bool
			 */
			[[nodiscard]]
			bool isOpen () const noexcept;

			/**
			 * @brief Captures a single frame from the device into RGBA format.
			 * @param rgbaOutput A writable reference to a vector that will receive the RGBA pixel data.
			 * @return bool True if a frame was successfully captured.
			 */
			[[nodiscard]]
			bool captureFrame (std::vector< uint8_t > & rgbaOutput) noexcept;

			/**
			 * @brief Returns the actual capture width.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t width () const noexcept;

			/**
			 * @brief Returns the actual capture height.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t height () const noexcept;

		private:

			/**
			 * @brief Converts YUYV (4:2:2) pixel data to RGBA using BT.601 coefficients.
			 * @param yuyvData Pointer to the YUYV source data.
			 * @param yuyvSize Size of the YUYV data in bytes.
			 * @param rgbaOutput A writable reference to the RGBA output vector.
			 * @param width The image width in pixels.
			 * @param height The image height in pixels.
			 */
			static void convertYUYVtoRGBA (const uint8_t * yuyvData, size_t yuyvSize, std::vector< uint8_t > & rgbaOutput, uint32_t width, uint32_t height) noexcept;

			uint32_t m_width{0};
			uint32_t m_height{0};
			bool m_isOpen{false};

#if IS_LINUX
			int m_fd{-1};
			void * m_buffer{nullptr};
			size_t m_bufferLength{0};
#endif

#if IS_MACOS
			void * m_platformHandle{nullptr};
#endif

#if IS_WINDOWS
			void * m_platformHandle{nullptr};
#endif
	};
}
