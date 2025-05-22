/*
 * src/Identification.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions for usages. */
#include "Libs/Version.hpp"

namespace EmEn
{
	/**
	 * @brief This class holds information about the application.
	 */
	class Identification final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Identification"};

			static constexpr auto LibraryAuthorName{"\"LondNoir\" <londnoir@gmail.com>"};
			static constexpr auto LibraryName{EngineName};
			static constexpr auto LibraryVersion{Libs::Version(VersionMajor, VersionMinor, VersionPatch)};
			static constexpr auto LibraryPlatform{PlatformTargeted};
			static constexpr auto LibraryCompilationDate{__DATE__};

			/** 
			 * @brief Constructs an application identification structure.
			 * @param name The name of the application using the engine.
			 * @param version A reference to a version of the application.
			 * @param organization The name of the application organization.
			 * @param domain The domain of the application.
			 */
			Identification (const char * name, const Libs::Version & version, const char * organization, const char * domain) noexcept;

			/**
			 * @brief Returns the application name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			applicationName () const noexcept
			{
				return m_applicationName;
			}

			/**
			 * @brief Returns the application version.
			 * @return const Libs::Version &
			 */
			[[nodiscard]]
			const Libs::Version &
			applicationVersion () const noexcept
			{
				return m_applicationVersion;
			}

			/**
			 * @brief Returns the application organization name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			applicationOrganization () const noexcept
			{
				return m_applicationOrganization;
			}

			/**
			 * @brief Returns the application domain.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			applicationDomain () const noexcept
			{
				return m_applicationDomain;
			}

			/**
			 * @brief Returns the engine identification.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			engineId () const noexcept
			{
				return m_engineId;
			}

			/**
			 * @brief Returns the full application identification.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			applicationId () const noexcept
			{
				return m_applicationId;
			}

			/**
			 * @brief Returns the application reverse id.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			applicationReverseId () const noexcept
			{
				return m_applicationReverseId;
			}

		private:

			std::string m_applicationName;
			Libs::Version m_applicationVersion;
			std::string m_applicationOrganization;
			std::string m_applicationDomain;
			std::string m_engineId;
			std::string m_applicationId;
			std::string m_applicationReverseId;
	};
}
