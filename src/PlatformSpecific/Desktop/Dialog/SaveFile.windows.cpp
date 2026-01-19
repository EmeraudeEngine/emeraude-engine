/*
 * src/PlatformSpecific/Desktop/Dialog/SaveFile.windows.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "SaveFile.hpp"

/* STL inclusions. */
#include <filesystem>
#include <map>
#include <vector>

/* Third-party inclusions. */
#include <shobjidl.h>

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "Window.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	SaveFile::execute (Window * window) noexcept
	{
		IFileOpenDialog * dialogHandle = nullptr;

		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast< void * * >(&dialogHandle));
		if ( FAILED(hr) )
		{
			Tracer::error(ClassId, "Unable to create the dialog instance !");

			return false;
		}

		/* NOTE: Create an automatic dialog release. */
		std::unique_ptr< IFileOpenDialog, void (*)(IFileOpenDialog *) > autoRelease(dialogHandle, [] (IFileOpenDialog * p) {
			p->Release();
		});

		/* NOTE: Set the dialog title. */
		const auto title = convertUTF8ToWide(this->title());

		hr = dialogHandle->SetTitle(title.data());
		if ( FAILED(hr) )
		{
			Tracer::error(ClassId, "Unable to set the title of the dialog instance !");

			return false;
		}

		/* NOTE: Set the dialog file extension filter. */
		std::map< std::wstring, std::wstring > dataHolder;

		if ( m_extensionFilters.empty() )
		{
			COMDLG_FILTERSPEC save_filter[1];
			save_filter[0].pszName = L"All files";
			save_filter[0].pszSpec = L"*.*";

			hr = dialogHandle->SetFileTypes(1, save_filter);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to set file types to the dialog instance !");

				return false;
			}

			hr = dialogHandle->SetFileTypeIndex(1);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to set file type index to the dialog instance !");

				return false;
			}
		}
		else
		{
			const auto fileTypes = createExtensionFilter(m_extensionFilters, dataHolder);

			hr = dialogHandle->SetFileTypes(static_cast< uint32_t >(fileTypes.size()), fileTypes.data());
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to set file types to the dialog instance !");

				return false;
			}

			hr = dialogHandle->SetFileTypeIndex(1);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to set file type index to the dialog instance !");

				return false;
			}
		}

		/* NOTE: Set a default file name. */
		const auto defaultFilename = convertUTF8ToWide(m_defaultFilename);

		if ( !defaultFilename.empty() )
		{
			hr = dialogHandle->SetFileName(defaultFilename.data());

			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to set the default filename to the dialog instance !");

				return false;
			}
		}

		/* NOTE: Open the dialog box. */
		hr = dialogHandle->Show(window != nullptr ? window->getWin32Window() : nullptr);
		if ( hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) ) // No item was selected.
		{
			return true;
		}
		else if ( FAILED(hr) )
		{
			Tracer::error(ClassId, "Unable to show the dialog instance !");

			return false;
		}

		/* Get the filepath selected. */
		{
			IShellItem * item = nullptr;

			hr = dialogHandle->GetResult(&item);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to get the item selected !");

				return false;
			}

			{
				PWSTR filepath = nullptr;

				hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filepath);
				if ( FAILED(hr) )
				{
					Tracer::error(ClassId, "Unable to get the filepath from the item selected !");

					return false;
				}

				m_filepath = filepath;

				CoTaskMemFree(filepath);
			}

			item->Release();
		}

		return true;
	}
}
