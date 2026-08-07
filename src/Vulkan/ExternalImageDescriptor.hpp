/*
 * src/Vulkan/ExternalImageDescriptor.hpp
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
#include <cstdint>
#include <array>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn::Vulkan
{
	/**
	 * @brief Platform-neutral descriptor of an externally-owned GPU image to import as a Vulkan image.
	 * @note This is the hand-off structure of the zero-copy CEF accelerated-paint path: the embedder
	 * (e.g. a CEF WebView's OnAcceleratedPaint callback) fills it from the platform shared-texture
	 * info, and the engine imports the memory through the matching Vulkan external-memory extension
	 * (VK_KHR_external_memory_win32 on Windows, VK_EXT_metal_objects/IOSurface on macOS —
	 * both implemented; DMA-BUF on Linux is a placeholder until its import path is built).
	 * @warning The external handle is only borrowed: its validity window is defined by the producer
	 * (for CEF it does NOT outlive the OnAcceleratedPaint callback). The import and the GPU copy must
	 * complete within that window, and the engine never closes the handle.
	 */
	struct EMEN_LEAN_API ExternalImageDescriptor
	{
		/** @brief Maximum number of DMA-BUF planes carried by the descriptor (Linux). */
		static constexpr size_t MaxPlanes{4};

		/** @brief The kind of platform handle carried by this descriptor. */
		enum class EMEN_API HandleType : uint8_t
		{
			None = 0,
			/** @brief Windows: DXGI shared HANDLE of a D3D11 texture (VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT). */
			Win32D3D11Texture = 1,
			/** @brief Linux: DMA-BUF plane file descriptors (not implemented yet). */
			DmaBuf = 2,
			/** @brief macOS: IOSurface pointer (imported through VK_EXT_metal_objects). */
			IOSurface = 3
		};

		/** @brief One DMA-BUF plane (Linux only). */
		struct EMEN_API Plane
		{
			int fd{-1};
			uint32_t stride{0};
			uint64_t offset{0};
			uint64_t size{0};
		};

		HandleType handleType{HandleType::None};
		uint32_t width{0};
		uint32_t height{0};
		/** @brief Vulkan format of the external image (the embedder maps the producer format, e.g. CEF BGRA_8888 → VK_FORMAT_B8G8R8A8_UNORM). */
		VkFormat format{VK_FORMAT_UNDEFINED};

		/** @brief Windows: the DXGI shared HANDLE (stored as void* to keep this header platform-neutral). */
		void * win32Handle{nullptr};

		/** @brief Linux: DRM format modifier of the DMA-BUF. */
		uint64_t drmModifier{0};
		/** @brief Linux: number of valid entries in planes. */
		uint32_t planeCount{0};
		/** @brief Linux: per-plane DMA-BUF descriptions. */
		std::array< Plane, MaxPlanes > planes{};

		/** @brief macOS: the IOSurface pointer. */
		void * ioSurface{nullptr};

		/**
		 * @brief Returns whether the descriptor carries a usable handle for its declared type.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			if ( width == 0 || height == 0 || format == VK_FORMAT_UNDEFINED )
			{
				return false;
			}

			switch ( handleType )
			{
				case HandleType::Win32D3D11Texture :
					return win32Handle != nullptr;

				case HandleType::DmaBuf :
					return planeCount > 0 && planes[0].fd >= 0;

				case HandleType::IOSurface :
					return ioSurface != nullptr;

				case HandleType::None :
				default :
					return false;
			}
		}
	};
}
