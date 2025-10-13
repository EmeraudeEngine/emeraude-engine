/*
 * src/PlatformManager.hpp
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

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Forward declarations. */
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
	class PlatformManager final : public ServiceInterface
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

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			PrimaryServices & m_primaryServices;
			bool m_showInformation{false};
	};
}
