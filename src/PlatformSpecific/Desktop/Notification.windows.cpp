/*
 * src/PlatformSpecific/Desktop/Notification.windows.cpp
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

#include "Notification.hpp"

/* Local inclusions. */
#include "Window.hpp"

/* Windows inclusions. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>

/* STL inclusions. */
#include <string>
#include <thread>

namespace EmEn::PlatformSpecific::Desktop
{
	namespace
	{
		std::wstring
		toWideString (const std::string & str) noexcept
		{
			if ( str.empty() )
			{
				return {};
			}

			const int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast< int >(str.size()), nullptr, 0);

			std::wstring result(size, 0);

			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast< int >(str.size()), result.data(), size);

			return result;
		}

		DWORD
		toNiifIcon (NotificationIcon icon) noexcept
		{
			switch ( icon )
			{
				case NotificationIcon::Info:
					return NIIF_INFO;

				case NotificationIcon::Warning:
					return NIIF_WARNING;

				case NotificationIcon::Error:
					return NIIF_ERROR;

				default:
					return NIIF_NONE;
			}
		}
	}

	bool
	Notification::show () noexcept
	{
		/*
		 * Use Shell_NotifyIconW for balloon notifications.
		 * If a window is provided, use its HWND. Otherwise, create a temporary hidden window.
		 */
		HWND hwnd = nullptr;
		bool ownsWindow = false;

		if ( m_window != nullptr )
		{
			hwnd = m_window->getWin32Window();
		}

		if ( hwnd == nullptr )
		{
			/* Create a temporary hidden message-only window. */
			static const wchar_t * className = L"EmEnNotificationWindow";
			static bool classRegistered = false;

			if ( !classRegistered )
			{
				WNDCLASSEXW wc = {};
				wc.cbSize = sizeof(wc);
				wc.lpfnWndProc = DefWindowProcW;
				wc.hInstance = GetModuleHandleW(nullptr);
				wc.lpszClassName = className;

				if ( RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
				{
					return false;
				}

				classRegistered = true;
			}

			/* Create a message-only window (HWND_MESSAGE parent). */
			hwnd = CreateWindowExW(
				0,
				className,
				L"",
				0,
				0, 0, 0, 0,
				HWND_MESSAGE,
				nullptr,
				GetModuleHandleW(nullptr),
				nullptr
			);

			if ( hwnd == nullptr )
			{
				return false;
			}

			ownsWindow = true;
		}

		NOTIFYICONDATAW nid = {};
		nid.cbSize = sizeof(nid);
		nid.hWnd = hwnd;
		nid.uID = 1;
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_INFO | NIF_SHOWTIP;

		/* Load a system icon for the tray. */
		nid.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512)); /* 32512 = IDI_APPLICATION */

		/* Set balloon icon type. */
		if ( m_icon.has_value() )
		{
			nid.dwInfoFlags = toNiifIcon(m_icon.value());
		}
		else
		{
			nid.dwInfoFlags = NIIF_INFO;
		}

		/* Copy title and message. */
		const std::wstring wideTitle = toWideString(m_title);
		const std::wstring wideMessage = toWideString(m_message);

		wcsncpy_s(nid.szInfoTitle, wideTitle.c_str(), _TRUNCATE);
		wcsncpy_s(nid.szInfo, wideMessage.c_str(), _TRUNCATE);
		wcsncpy_s(nid.szTip, wideTitle.c_str(), _TRUNCATE);

		/* Add the notification icon and show balloon. */
		if ( !Shell_NotifyIconW(NIM_ADD, &nid) )
		{
			if ( ownsWindow )
			{
				DestroyWindow(hwnd);
			}

			return false;
		}

		/* Set version for modern behavior. */
		nid.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIconW(NIM_SETVERSION, &nid);

		/*
		 * Schedule cleanup after a delay to allow the notification to display.
		 * Use a separate thread to avoid blocking.
		 */
		std::thread([hwnd, uID = nid.uID, ownsWindow]() {
			/* Wait for notification to be visible (Windows displays for ~5 seconds by default). */
			Sleep(6000);

			/* Remove the notification icon. */
			NOTIFYICONDATAW nidCleanup = {};
			nidCleanup.cbSize = sizeof(nidCleanup);
			nidCleanup.hWnd = hwnd;
			nidCleanup.uID = uID;
			Shell_NotifyIconW(NIM_DELETE, &nidCleanup);

			/* Destroy the temporary window only if we created it. */
			if ( ownsWindow )
			{
				DestroyWindow(hwnd);
			}
		}).detach();

		return true;
	}
}
