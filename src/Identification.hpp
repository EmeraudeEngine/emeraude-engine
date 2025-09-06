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
#include <sstream>
#include <string>
#include <ranges>

/* Local inclusions for usages. */
#include "Libs/Version.hpp"
#include "Libs/String.hpp"

namespace EmEn
{
	/**
	 * @brief Describe information about an application.
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
			Identification (const char * name, const Libs::Version & version, const char * organization, const char * domain) noexcept
				: m_applicationName{name},
				m_applicationVersion{version},
				m_applicationOrganization{organization},
				m_applicationDomain{domain}
			{
				/* NOTE: Engine identification string. */
				{
					std::stringstream stream;

					stream << LibraryName << " (" << LibraryVersion << "; " << LibraryPlatform << "; " << LibraryCompilationDate << ") LGPLv3 - " << LibraryAuthorName;

					m_engineId = stream.str();
				}

				/* NOTE: Application identification string. */
				{
					std::stringstream stream;

					stream << m_applicationName << " (" << m_applicationVersion << ") - " << m_applicationOrganization;

					m_applicationId = stream.str();
				}

				/* NOTE: Application reverse id. */
				{
					std::stringstream stream;

					for ( const auto & chunk : std::ranges::reverse_view(Libs::String::explode(m_applicationDomain, '.')) )
					{
						stream << chunk << '.';
					}

					stream << m_applicationName;

					m_applicationReverseId = Libs::String::toLower(stream.str());
				}
			}

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

			const std::string m_applicationName;
			const Libs::Version m_applicationVersion;
			const std::string m_applicationOrganization;
			const std::string m_applicationDomain;
			std::string m_engineId;
			std::string m_applicationId;
			std::string m_applicationReverseId;
	};
}
