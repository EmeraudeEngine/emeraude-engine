/*
 * src/PlatformSpecific/Desktop/Dialog/OpenFile.windows.cpp
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

#include "OpenFile.hpp"

/* STL inclusions. */
#include <filesystem>
#include <map>
#include <vector>

/* Third-party inclusions. */
#include <shobjidl.h>
#include <shlobj.h>
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

		bool
		executeLegacyFolderPicker (const std::string & title, HWND parentWindow, const std::filesystem::path & defaultDirectory, std::vector< std::filesystem::path > & filepaths) noexcept
		{
			Tracer::debug(OpenFile::ClassId, "[LEGACY] Using Win32 folder picker (SHBrowseForFolderW).");

			const auto wideTitle = convertUTF8ToWide(title);

			BROWSEINFOW info{};
			info.hwndOwner = parentWindow;
			info.lpszTitle = wideTitle.c_str();
			info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;

			std::wstring defaultDir;

			if ( !defaultDirectory.empty() )
			{
				defaultDir = defaultDirectory.wstring();
				info.lParam = reinterpret_cast< LPARAM >(defaultDir.c_str());
				info.lpfn = [] (HWND hwnd, UINT msg, LPARAM /*lp*/, LPARAM data) -> int {
					if ( msg == BFFM_INITIALIZED && data != 0 )
					{
						SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, data);
					}
					return 0;
				};
			}

			LPITEMIDLIST idList = SHBrowseForFolderW(&info);

			if ( idList == nullptr )
			{
				/* User cancelled. */
				return true;
			}

			wchar_t pathBuffer[MAX_PATH]{};

			if ( SHGetPathFromIDListW(idList, pathBuffer) )
			{
				filepaths.emplace_back(pathBuffer);
			}

			CoTaskMemFree(idList);

			return true;
		}

		bool
		executeLegacyOpenFile (const std::string & title, HWND parentWindow, bool multiSelect, const std::filesystem::path & defaultDirectory, const std::vector< std::pair< std::string, std::vector< std::string > > > & extensionFilters, std::vector< std::filesystem::path > & filepaths) noexcept
		{
			Tracer::debug(OpenFile::ClassId, "[LEGACY] Using Win32 open file dialog (GetOpenFileNameW).");

			const auto wideTitle = convertUTF8ToWide(title);
			auto filterBuffer = buildLegacyFilter(extensionFilters);

			/* Large buffer to bypass MAX_PATH, especially required for multi-select. */
			std::vector< wchar_t > fileBuffer(32768, L'\0');

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
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

			if ( multiSelect )
			{
				ofn.Flags |= OFN_ALLOWMULTISELECT;
			}

			if ( !GetOpenFileNameW(&ofn) )
			{
				/* User cancelled or dialog failed. CommDlgExtendedError() would detail the cause. */
				return true;
			}

			if ( !multiSelect )
			{
				filepaths.emplace_back(ofn.lpstrFile);

				return true;
			}

			/* Multi-select: buffer holds directory, null, filename, null, ..., filename, null, null.
			 * If only one file was selected, the first field already contains the full path. */
			const wchar_t * cursor = ofn.lpstrFile;
			const std::wstring directory{cursor};
			cursor += directory.size() + 1;

			if ( *cursor == L'\0' )
			{
				filepaths.emplace_back(directory);

				return true;
			}

			const std::filesystem::path directoryPath{directory};

			while ( *cursor != L'\0' )
			{
				const std::wstring filename{cursor};
				filepaths.emplace_back(directoryPath / filename);
				cursor += filename.size() + 1;
			}

			return true;
		}
	}

	bool
	OpenFile::execute (Window & window, bool parentToWindow) noexcept
	{
		HWND parentWindow = parentToWindow ? window.getWin32Window() : nullptr;

		/* NOTE: Branch to the Win32 legacy path when the compatibility setting is enabled.
		 * Useful on Windows 11 when the modern COM dialog misbehaves with accessibility tools. */
		if ( window.primaryServices().settings().getOrSetDefault< bool >(CompatibilityWindowsUseLegacyFileDialogsKey, DefaultCompatibilityWindowsUseLegacyFileDialogs) )
		{
			if ( m_selectFolder )
			{
				return executeLegacyFolderPicker(this->title(), parentWindow, m_defaultDirectory, m_filepaths);
			}

			return executeLegacyOpenFile(this->title(), parentWindow, m_multiSelect, m_defaultDirectory, m_extensionFilters, m_filepaths);
		}

		Tracer::debug(ClassId, "[COM] Using modern open file dialog (IFileOpenDialog).");

		IFileOpenDialog * dialogHandle = nullptr;

		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast< void * * >(&dialogHandle));
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

		/* NOTE: Set dialog options. */
		if ( m_selectFolder || m_multiSelect )
		{
			FILEOPENDIALOGOPTIONS options = 0;

			hr = dialogHandle->GetOptions(&options);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to get initial options from the dialog instance !");

				return false;
			}

			if ( m_selectFolder )
			{
				options |= FOS_PICKFOLDERS;
			}

			if ( m_multiSelect )
			{
				options |= FOS_ALLOWMULTISELECT;
			}

			hr = dialogHandle->SetOptions(options);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to set an option to the dialog instance !");

				return false;
			}
		}

		/* NOTE: Set the dialog file extension filter.
		 * Filters are only applicable when selecting files, not folders. */
		std::map< std::wstring, std::wstring > dataHolder;

		if ( !m_selectFolder )
		{
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

		/* Get the filepaths selected. */
		{
			IShellItemArray * items = nullptr;
			
			hr = dialogHandle->GetResults(&items);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to get the items selected !");

				return false;
			}
			
			DWORD filepathCount = 0;
			
			hr = items->GetCount(&filepathCount);
			if ( FAILED(hr) )
			{
				Tracer::error(ClassId, "Unable to get the number of file selected !");

				return false;
			}

			for ( int filepathIndex = 0; filepathIndex < static_cast< int >(filepathCount); ++filepathIndex )
			{
				IShellItem * item = nullptr;

				items->GetItemAt(filepathIndex, &item);
				if ( SUCCEEDED(hr) )
				{
					PWSTR path;

					hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
					if ( SUCCEEDED(hr) )
					{
						m_filepaths.emplace_back(path);

						CoTaskMemFree(path);
					}

					item->Release();
				}
			}

			items->Release();
		}

		return true;
	}
}
