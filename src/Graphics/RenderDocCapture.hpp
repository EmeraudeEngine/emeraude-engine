/*
 * src/Graphics/RenderDocCapture.hpp
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

/* Emeraude Engine configuration. */
#include "emeraude_config.hpp"

#ifdef EMERAUDE_ENABLE_RENDERDOC

/* Third-party inclusions. */
#include "renderdoc_app.h"

/* STL inclusions. */
#include <cstdint>
#include <string>

namespace EmEn::Graphics
{
	/**
	 * @class RenderDocCapture
	 * @brief Wrapper for the RenderDoc in-application API.
	 * @details Provides programmatic GPU frame capture when the application is launched
	 * under RenderDoc. Designed to be owned by Vulkan::Instance which provides the
	 * native device handle (VkInstance) required for capture operations.
	 *
	 * Detection is performed via dlopen(RTLD_NOLOAD) on Linux/macOS or
	 * GetModuleHandleA on Windows — the library is never loaded by the engine itself.
	 *
	 * @note initialize() MUST be called before Vulkan instance creation.
	 * @note setDevice() MUST be called right after vkCreateInstance().
	 */
	class RenderDocCapture final
	{
		public:

			/**
			 * @brief Default constructor.
			 */
			RenderDocCapture () noexcept = default;

			/**
			 * @brief Detects the RenderDoc runtime library and retrieves the API.
			 * @details Must be called before Vulkan instance creation. Uses RTLD_NOLOAD
			 * so the library is only found if already injected by RenderDoc.
			 * @return true if RenderDoc was detected and the API loaded.
			 */
			[[nodiscard]]
			bool initialize () noexcept;

			/**
			 * @brief Registers the Vulkan instance handle for capture operations.
			 * @param nativeInstance The VkInstance handle (cast to void *).
			 */
			void setDevice (void * nativeInstance) noexcept;

			/**
			 * @brief Cleans up the API pointer and device reference.
			 */
			void shutdown () noexcept;

			/**
			 * @brief Returns whether the RenderDoc API was successfully loaded.
			 * @return true if RenderDoc is available.
			 */
			[[nodiscard]]
			bool isAvailable () const noexcept;

			/**
			 * @brief Begins a frame capture on the registered device.
			 * @details Captures all rendering commands until endCapture() is called.
			 */
			void startCapture () noexcept;

			/**
			 * @brief Ends the current frame capture on the registered device.
			 * @return true if the capture was saved successfully.
			 */
			[[nodiscard]]
			bool endCapture () noexcept;

			/**
			 * @brief Triggers capture of the next frame presented.
			 */
			void triggerCapture () noexcept;

			/**
			 * @brief Triggers capture of the next N frames presented.
			 * @param count Number of consecutive frames to capture.
			 */
			void triggerMultiFrameCapture (uint32_t count) noexcept;

			/**
			 * @brief Sets the file path template for capture output.
			 * @param path File path template (without extension).
			 */
			void setCaptureFilePath (const std::string & path) noexcept;

			/**
			 * @brief Sets the capture title visible in RenderDoc UI.
			 * @param title Title string.
			 */
			void setCaptureTitle (const std::string & title) noexcept;

			/**
			 * @brief Returns whether a frame capture is currently in progress.
			 * @return true if capturing.
			 */
			[[nodiscard]]
			bool isCapturing () const noexcept;

			/**
			 * @brief Launches the RenderDoc replay UI connected to this application.
			 * @return true if the UI was launched successfully.
			 */
			[[nodiscard]]
			bool launchReplayUI () noexcept;

		private:

			RENDERDOC_API_1_6_0 * m_api{nullptr};
			RENDERDOC_DevicePointer m_device{nullptr};
	};
}

#else /* !EMERAUDE_ENABLE_RENDERDOC */

/* STL inclusions. */
#include <cstdint>
#include <string>

namespace EmEn::Graphics
{
	/**
	 * @class RenderDocCapture
	 * @brief No-op stub when EMERAUDE_ENABLE_RENDERDOC is not defined.
	 * @details All methods are inline empty — the compiler eliminates them entirely,
	 * producing zero overhead in builds without RenderDoc support.
	 */
	class RenderDocCapture final
	{
		public:

			RenderDocCapture () noexcept = default;

			[[nodiscard]]
			bool initialize () noexcept { return false; }

			void setDevice (void *) noexcept {}
			void shutdown () noexcept {}

			[[nodiscard]]
			bool isAvailable () const noexcept { return false; }

			void startCapture () noexcept {}

			[[nodiscard]]
			bool endCapture () noexcept { return false; }

			void triggerCapture () noexcept {}
			void triggerMultiFrameCapture (uint32_t) noexcept {}
			void setCaptureFilePath (const std::string &) noexcept {}
			void setCaptureTitle (const std::string &) noexcept {}

			[[nodiscard]]
			bool isCapturing () const noexcept { return false; }

			[[nodiscard]]
			bool launchReplayUI () noexcept { return false; }
	};
}

#endif
