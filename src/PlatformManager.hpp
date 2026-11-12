/*
 * src/PlatformManager.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "SharedMutex.hpp" // IWYU pragma: keep (the thread-safety analysis needs the capability type of s_GLFWAccess, not a forward declaration)
#include "ThreadSafety.hpp"

/* Local inclusions for usages. */
#include "Vulkan/Loader.hpp"

namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn
{
	/**
	 * @brief The platform manager service class initialize check Vulkan and initialize GLFW with it.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class EMEN_LEAN_API PlatformManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PlatformManagerService"};

			/**
			 * @brief Constructs the platform manager.
			 * @param primaryServices A reference to primary services.
			 */
			explicit
			PlatformManager (PrimaryServices & primaryServices) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices}
			{

			}

			/**
			 * @brief Writes the Vulkan loader's environment ONCE, before any thread exists: the layers shipped with the
			 * application first in VK_ADD_LAYER_PATH, and on macOS the bundled MoltenVK driver (VK_DRIVER_FILES).
			 * @note The consumer's main() calls it before anything else (before CEF and before the Core): POSIX setenv()
			 * races any concurrent getenv(), and CEF's threads exist once CefInitialize() ran (owner decision 2026-10-11).
			 * PlatformManager::onInitialize() only reads the result and warns when no shipped layer directory is in the
			 * path: not called, or no 'vulkan/explicit_layer.d' beside the binary nor in the bundle (a tool outside a macOS
			 * bundle). An explicit VK_LAYER_PATH / VK_DRIVER_FILES from the user still wins.
			 * @return bool True when the shipped layer directory was put first.
			 */
			static bool prepareVulkanLoaderEnvironment () noexcept;

			/**
			 * @brief Returns the Vulkan loader the engine uses (the external dependencies' one, opened by explicit path).
			 * @return const Vulkan::Loader &
			 */
			[[nodiscard]]
			const Vulkan::Loader &
			vulkanLoader () const noexcept
			{
				return m_vulkanLoader;
			}

			/**
			 * @brief Returns whether GLFW detected a linux platform.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isLinux () noexcept;

			/**
			 * @brief Returns whether GLFW detected a macOS platform.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isMacOS () noexcept;

			/**
			 * @brief Returns whether GLFW detected a Windows platform.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isWindows () noexcept;

			/**
			 * @brief Returns whether GLFW detected a X11 graphic server.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isUsingX11 () noexcept;

			/**
			 * @brief Returns whether GLFW detected a Wayland graphic server.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isUsingWayland () noexcept;

			/**
			 * @brief Wakes up the thread blocked in GLFW's event wait (glfwPostEmptyEvent()), only while GLFW is
			 * initialized.
			 * @note Any thread (CEF asks for a main-loop cycle from its own threads, Core::scheduleMainLoopCycle()). Before
			 * glfwInit() — a startup that failed earlier — or after glfwTerminate(), nothing is posted: GLFW reported
			 * GLFW_NOT_INITIALIZED as an error for each call, and read its global state while the main thread initialized
			 * or terminated it (a data race). The initialization and the termination take the same lock exclusively.
			 */
			static void postEmptyEvent () noexcept;

		private:

			/**
			 * @brief Owns the GLFW library's initialization (Ave Robustus II P3: a native resource in an RAII holder):
			 * glfwTerminate() at its end. A failure after glfwInit() (no Vulkan support) used to leave GLFW initialized: a
			 * service that fails to start is never terminated.
			 */
			class GLFWLibrary final
			{
				public:

					GLFWLibrary () noexcept = default;

					GLFWLibrary (const GLFWLibrary & copy) noexcept = delete;

					GLFWLibrary (GLFWLibrary && copy) noexcept = delete;

					GLFWLibrary & operator= (const GLFWLibrary & copy) noexcept = delete;

					GLFWLibrary & operator= (GLFWLibrary && copy) noexcept = delete;

					/** @brief Terminates GLFW when this object initialized it. */
					~GLFWLibrary ();

					/**
					 * @brief Initializes GLFW (glfwInit()), once.
					 * @return bool
					 */
					[[nodiscard]]
					bool initialize () noexcept;

					/** @brief Terminates GLFW (glfwTerminate()) when this object initialized it. */
					void terminate () noexcept;

				private:

					bool m_initialized{false};
			};

			/** @brief Serializes GLFW's initialization and termination against postEmptyEvent() from other threads. */
			static Base::SharedMutex s_GLFWAccess;
			/** @brief Whether GLFW is initialized (GLFWLibrary::initialize() / terminate()). */
			static bool s_GLFWReady EMEN_GUARDED_BY(s_GLFWAccess);

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			PrimaryServices & m_primaryServices;
			/* NOTE: Opened before glfwInit(), closed after glfwTerminate(); this service is initialized before the Vulkan
			 * instance and terminated after it, so every Vulkan object is gone before the loader closes. */
			Vulkan::Loader m_vulkanLoader;
			/* NOTE: After the loader: destroyed (GLFW terminated) before the loader closes. */
			GLFWLibrary m_glfw;
			bool m_showInformation{false};
	};
}
