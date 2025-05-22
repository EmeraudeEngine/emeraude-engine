/*
 * src/User.hpp
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
#include <array>
#include <string>

/* Local inclusion for inheritances */
#include "ServiceInterface.hpp"

/* Forward declarations. */
namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn
{
	/**
	 * @brief The user service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class User final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"UserService"};

			/**
			 * @brief Constructs a user.
			 * @param primaryServices A reference to primary services.
			 */
			explicit
			User (PrimaryServices & primaryServices) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices}
			{

			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
			}

			/**
			 * @brief Sets a user identification.
			 * @param identifier An integer like ID from external platform.
			 * @return void
			 */
			void
			setUserID (uint64_t identifier) noexcept
			{
				m_userID = identifier;
			}

			/**
			 * @brief Sets the username.
			 * @param username A reference to a string.
			 * @return void
			 */
			void
			setUsername (const std::string & username) noexcept
			{
				m_username = username;
			}

			/**
			 * @brief Returns the third-party user id.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			userID () const noexcept
			{
				return m_userID;
			}

			/**
			 * @brief Returns the username.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			username () const noexcept
			{
				return m_username;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};

			PrimaryServices & m_primaryServices;
			uint64_t m_userID{0};
			std::string m_username{"John.Doe"};
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
