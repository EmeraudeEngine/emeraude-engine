/*
 * src/PlatformSpecific/Desktop/Dialog/OpenFile.hpp
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
#include <utility>
#include <array>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	/**
	 * @brief User dialog to select multiple files.
	 * @extends EmEn::PlatformSpecific::Desktop::Dialog::Abstract
	 */
	class OpenFile final : public Abstract
	{
		public:

			static constexpr auto ClassId{"OpenFile"};

			/** 
			 * @brief Constructs a file box.
			 * @param title A reference to a string.
			 * @param selectFolder Select folders instead of files. Default false.
			 * @param multiSelect Allow multiple selection. Default false.
			 */
			explicit OpenFile (const std::string & title, bool selectFolder = false, bool multiSelect = false);

			/** @copydoc EmEn::PlatformSpecific::Desktop::Dialog::Abstract::execute() */
			bool execute (Window * window) noexcept override;

			/**
			 * @brief Sets file extension filters.
			 * @param filters A reference to a vector.
			 * @return void
			 */
			void
			setExtensionFilters (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters)
			{
				m_extensionFilters = filters;
			}

			/**
			 * @brief Sets file extension filters.
			 * @param filterName A reference to a string.
			 * @param extensions A reference to a vector.
			 * @return void
			 */
			void
			setExtensionFilters (const std::string & filterName, const std::vector< std::string > & extensions)
			{
				m_extensionFilters.emplace_back(filterName, extensions);
			}

			/**
			 * @brief Returns the file extension filters.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::pair< std::string, std::vector< std::string > > > &
			extensionFilters () const
			{
				return m_extensionFilters;
			}

			/**
			 * @brief Returns whether the user has canceled the dialog.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasBeenCanceled () const noexcept
			{
				return m_flags[Canceled];
			}

			/**
			 * @brief Returns the selected files.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			filepaths () const
			{
				return m_filepaths;
			}

		private:

			/* Flag names. */
			static constexpr auto Canceled{0UL};
			static constexpr auto SelectFolder{1UL};
			static constexpr auto MultiSelect{2UL};

			std::vector< std::pair< std::string, std::vector< std::string > > > m_extensionFilters;
			std::vector< std::string > m_filepaths;
			std::array< bool, 8 > m_flags{
				false/*Canceled*/,
				false/*SelectFolder*/,
				false/*MultiSelect*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
