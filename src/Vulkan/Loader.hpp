/*
 * src/Vulkan/Loader.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <filesystem>

/* Third-party inclusions. */
#include "volk.h"

namespace EmEn::Vulkan
{
	/**
	 * @brief The Vulkan loader the engine uses: the one of the external dependencies' Vulkan SDK, opened by EXPLICIT
	 * path, never a loader found through the system search order (CEF ships its own next to the application, for its
	 * GPU process). Owner decision 2026-10-11.
	 * @note RAII: the destructor finalizes volk and closes the library. The owner (PlatformManager) keeps it open until
	 * after every Vulkan object, the instance and GLFW are gone.
	 * @note Single instance per process: volk's function pointers are process-wide.
	 */
	class EMEN_API Loader final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanLoader"};

			/** @brief Default constructor: nothing opened. */
			Loader () noexcept = default;

			Loader (const Loader & copy) noexcept = delete;

			Loader (Loader && copy) noexcept = delete;

			Loader & operator= (const Loader & copy) noexcept = delete;

			Loader & operator= (Loader && copy) noexcept = delete;

			/**
			 * @brief Destructs the loader: volk finalized, the library closed.
			 */
			~Loader ()
			{
				this->close();
			}

			/**
			 * @brief Opens the loader shipped with the application and initializes volk with it.
			 * @note Looked for in `<binary>/vulkan/` (Linux libvulkan.so.1, Windows vulkan-1.dll), and on macOS in the
			 * bundle's `Contents/Frameworks/libvulkan.1.dylib` first. No other location is tried: a missing loader is a
			 * packaging error, refused with a trace.
			 * @param binaryDirectory The directory of the running executable.
			 * @return bool
			 */
			[[nodiscard]]
			bool open (const std::filesystem::path & binaryDirectory) noexcept;

			/**
			 * @brief Finalizes volk and closes the library. Idempotent.
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the loader is open.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOpen () const noexcept
			{
				return m_getInstanceProcAddr != nullptr;
			}

			/**
			 * @brief Returns the loader's vkGetInstanceProcAddr (for GLFW and VMA).
			 * @return PFN_vkGetInstanceProcAddr Null when not open.
			 */
			[[nodiscard]]
			PFN_vkGetInstanceProcAddr
			getInstanceProcAddr () const noexcept
			{
				return m_getInstanceProcAddr;
			}

			/**
			 * @brief Returns the path of the opened loader.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			path () const noexcept
			{
				return m_path;
			}

		private:

			std::filesystem::path m_path;
			void * m_handle{nullptr};
			PFN_vkGetInstanceProcAddr m_getInstanceProcAddr{nullptr};
	};
}
