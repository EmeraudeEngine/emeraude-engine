/*
 * src/PlatformSpecific/Desktop/Dialog/Abstract.hpp
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
#include <vector>
#include <string>

/* Forward declarations. */
namespace EmEn
{
	class Arguments;
	class Window;
}

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	/**
	 * @brief The base class for all dialogs.
	 */
	class Abstract
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractBox &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractBox &
			 */
			Abstract & operator= (Abstract && copy) noexcept = delete;

			/**
			 * @brief Destructs the abstract dialog box.
			 */
			virtual ~Abstract () = default;

			/**
			 * @brief Returns the box title.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			title () const noexcept
			{
				return m_title;
			}

			/**
			 * @brief Executes the dialog, if something goes wrong, the method will return false.
			 * @param window A pointer to the window to set parenting (Optional).
			 * @return bool
			 */
			virtual bool execute (Window * window) noexcept = 0;

		protected:

			/** 
			 * @brief Constructs an abstract dialog.
			 * @param title A string to name the dialog [std::move].
			 */
			explicit Abstract (std::string title) noexcept;

#ifdef IS_LINUX
			/**
			 * @brief Helpers for "portable-file-dialogs" and generate filter structure.
			 * @param extensionFilters A reference to vector.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			static std::vector< std::string > convertFilterStructureForPFD (const std::vector< std::pair< std::string, std::vector< std::string > > > & extensionFilters) noexcept;
#endif

		private:

			std::string m_title;
	};
}
