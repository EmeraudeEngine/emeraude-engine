/*
 * src/PlatformSpecific/Desktop/Dialog/OpenFile.linux.cpp
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

#include "OpenFile.hpp"

#if IS_LINUX

/* Third-party inclusions. */
#include "portable-file-dialogs.h"

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	OpenFile::execute (Window * /*window*/) noexcept
	{
		/* NOTE: This mode selects a folder. */
		if ( m_flags[SelectFolder] )
		{
			pfd::select_folder dialog{this->title()};

			const auto filepath = dialog.result();

			if ( filepath.empty() )
			{
				m_flags[Canceled] = true;
			}
			else
			{
				m_filepaths.emplace_back(filepath);
			}

			return true;
		}

		pfd::open_file dialog{
			this->title(),
			{},
			OpenFile::convertFilterStructureForPFD(m_extensionFilters),
			m_flags[MultiSelect] ? pfd::opt::multiselect : pfd::opt::none
		};

		const auto filepaths = dialog.result();

		if ( filepaths.empty() )
		{
			m_flags[Canceled] = true;
		}
		else
		{
			for ( auto & filepath : filepaths )
			{
				m_filepaths.emplace_back(filepath);
			}
		}

		return true;
	}
}

#endif
