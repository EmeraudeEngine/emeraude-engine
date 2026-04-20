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

/* NOTE: Must be defined before any header that may transitively include <windows.h>
 * (e.g. <filesystem> on MSVC). Prevents the min/max macros from polluting template
 * code pulled in later via project headers (PrimaryServices -> ThreadPool). */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "SaveFile.hpp"

/* STL inclusions. */
#include <algorithm>
#include <filesystem>
#include <map>
#include <vector>

/* Third-party inclusions. */
#include <shobjidl.h>
#include <commdlg.h>

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Window.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	namespace
	{
		/**
		 * @brief Builds a Win32 legacy filter string (double-null terminated) from the extension filter list.
		 * The returned buffer must stay alive while OPENFILENAMEW references it.
		 */
		std::vector< wchar_t >
		buildLegacyFilter (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters) noexcept
		{
			std::vector< wchar_t > buffer;

			const auto append = [&buffer] (const std::wstring & str) {
				buffer.insert(buffer.end(), str.begin(), str.end());
				buffer.push_back(L'\0');
			};

			if ( filters.empty() )
			{
				append(L"All files");
				append(L"*.*");
			}
			else
			{
				for ( const auto & filter : filters )
				{
					append(convertUTF8ToWide(filter.first));

					std::wstring spec;

					for ( const auto & extension : filter.second )
					{
						if ( !spec.empty() )
						{
							spec.push_back(L';');
						}

						const auto wideExt = convertUTF8ToWide(extension);

						if ( wideExt.starts_with(L"*.") )
						{
							spec.append(wideExt);
						}
						else if ( wideExt.starts_with(L".") )
						{
							spec.append(L"*").append(wideExt);
						}
						else
						{
							spec.append(L"*.").append(wideExt);
						}
					}

					append(spec);
				}
			}

			/* Extra null to form a double-null terminator. */
			buffer.push_back(L'\0');

			return buffer;
		}

		/**
		 * @brief Extracts the first extension (without leading dot or wildcard) of the first filter, if any.
		 */
		std::wstring
		extractDefaultExtension (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters) noexcept
		{
			if ( filters.empty() || filters.front().second.empty() )
			{
				return {};
			}

			auto extension = convertUTF8ToWide(filters.front().second.front());

			if ( extension.starts_with(L"*.") )
			{
				extension.erase(0, 2);
			}
			else if ( extension.starts_with(L".") )
			{
				extension.erase(0, 1);
			}

			return extension;
		}

		bool
		executeLegacySaveFile (const std::string & title, HWND parentWindow, const std::filesystem::path & defaultDirectory, const std::string & defaultFilename, const std::vector< std::pair< std::string, std::vector< std::string > > > & extensionFilters, std::filesystem::path & filepath) noexcept
		{
			Tracer::debug(SaveFile::ClassId, "[LEGACY] Using Win32 save file dialog (GetSaveFileNameW).");

			const auto wideTitle = convertUTF8ToWide(title);
			auto filterBuffer = buildLegacyFilter(extensionFilters);
			const auto defaultExtension = extractDefaultExtension(extensionFilters);

			/* Pre-fill the filename buffer. Keep it large enough for a custom path. */
			std::vector< wchar_t > fileBuffer(32768, L'\0');

			if ( !defaultFilename.empty() )
			{
				const auto wideFilename = convertUTF8ToWide(defaultFilename);
				const auto copyCount = std::min(wideFilename.size(), fileBuffer.size() - 1);
				std::copy_n(wideFilename.begin(), copyCount, fileBuffer.begin());
			}

			std::wstring initialDir;

			if ( !defaultDirectory.empty() )
			{
				initialDir = defaultDirectory.wstring();
			}

			OPENFILENAMEW ofn{};
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = parentWindow;
			ofn.lpstrFilter = filterBuffer.data();
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = fileBuffer.data();
			ofn.nMaxFile = static_cast< DWORD >(fileBuffer.size());
			ofn.lpstrTitle = wideTitle.c_str();
			ofn.lpstrInitialDir = initialDir.empty() ? nullptr : initialDir.c_str();
			ofn.lpstrDefExt = defaultExtension.empty() ? nullptr : defaultExtension.c_str();
			ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN;

			if ( !GetSaveFileNameW(&ofn) )
			{
				/* User cancelled or dialog failed. */
				return true;
			}

			filepath = ofn.lpstrFile;

			return true;
		}
	}

	bool
	SaveFile::execute (Window & window, bool parentToWindow) noexcept
	{
		HWND parentWindow = parentToWindow ? window.getWin32Window() : nullptr;

		/* NOTE: Branch to the Win32 legacy path when the compatibility setting is enabled.
		 * Useful on Windows 11 when the modern COM dialog misbehaves with accessibility tools. */
		if ( window.primaryServices().settings().getOrSetDefault< bool >(CompatibilityWindowsUseLegacyFileDialogsKey, DefaultCompatibilityWindowsUseLegacyFileDialogs) )
		{
			return executeLegacySaveFile(this->title(), parentWindow, m_defaultDirectory, m_defaultFilename, m_extensionFilters, m_filepath);
		}

		Tracer::debug(ClassId, "[COM] Using modern save file dialog (IFileSaveDialog).");

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

		/* NOTE: Set default extension from the first filter's first extension. */
		if ( !m_extensionFilters.empty() && !m_extensionFilters[0].second.empty() )
		{
			const auto defaultExt = convertUTF8ToWide(m_extensionFilters[0].second[0]);

			dialogHandle->SetDefaultExtension(defaultExt.data());
		}

		/* NOTE: Set default directory. */
		if ( !m_defaultDirectory.empty() )
		{
			IShellItem * folderItem = nullptr;
			const auto dirWide = m_defaultDirectory.wstring();
			HRESULT hr2 = SHCreateItemFromParsingName(dirWide.c_str(), nullptr, IID_IShellItem, reinterpret_cast< void * * >(&folderItem));

			if ( SUCCEEDED(hr2) && folderItem != nullptr )
			{
				dialogHandle->SetFolder(folderItem);

				folderItem->Release();
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
		hr = dialogHandle->Show(parentWindow);
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
