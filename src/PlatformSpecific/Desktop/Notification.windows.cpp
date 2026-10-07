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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Notification.hpp"

/* STL inclusions. */
#include <atomic>
#include <string>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop
{
	namespace
	{
		/** @brief How long the notification icon stays after the balloon shows (Windows displays it ~5 s). */
		constexpr UINT NotificationLifetimeMS{6000};

		/** @brief Marks, in a timer id, a notification that created its own message-only window. */
		constexpr UINT_PTR OwnsWindowFlag{0x40000000};

		/**
		 * @brief Returns a fresh notification id: the icon's uID and its removal timer's id. A fixed id made a second
		 * notification within 6 s fail NIM_ADD (the first icon was still there).
		 * @return UINT In [1, OwnsWindowFlag).
		 */
		[[nodiscard]]
		UINT
		nextNotificationID () noexcept
		{
			static std::atomic< UINT > counter{0};

			return (counter.fetch_add(1) % static_cast< UINT >(OwnsWindowFlag - 1)) + 1;
		}

		/**
		 * @brief Removes a notification icon (and its own window), on the thread that owns the window: SetTimer() calls
		 * it from that thread's message dispatch. DestroyWindow() fails from any other thread — the former detached
		 * cleanup thread leaked every message-only window.
		 * @param window The window of the icon.
		 * @param message Unused (WM_TIMER).
		 * @param timerID The notification id, with OwnsWindowFlag when the window is the notification's own.
		 * @param time Unused.
		 * @return void
		 */
		VOID CALLBACK
		removeNotification (HWND window, UINT /*message*/, UINT_PTR timerID, DWORD /*time*/) noexcept
		{
			KillTimer(window, timerID);

			NOTIFYICONDATAW nidCleanup = {};
			nidCleanup.cbSize = sizeof(nidCleanup);
			nidCleanup.hWnd = window;
			nidCleanup.uID = static_cast< UINT >(timerID & ~OwnsWindowFlag);

			Shell_NotifyIconW(NIM_DELETE, &nidCleanup);

			if ( (timerID & OwnsWindowFlag) != 0 )
			{
				DestroyWindow(window);
			}
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
		nid.uID = nextNotificationID();
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
		const std::wstring wideTitle = convertUTF8ToWide(m_title);
		const std::wstring wideMessage = convertUTF8ToWide(m_message);

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

		/* The icon is removed later, on the thread that owns the window (no thread of its own: owner decision,
		 * 2026-10-07). ⚠️ That thread must dispatch messages — the main thread does (the window's event pump). */
		const UINT_PTR timerID = static_cast< UINT_PTR >(nid.uID) | (ownsWindow ? OwnsWindowFlag : 0);

		if ( SetTimer(hwnd, timerID, NotificationLifetimeMS, &removeNotification) == 0 )
		{
			/* Not this thread's window, or no timer left: remove it now rather than leave an orphan icon. */
			removeNotification(hwnd, WM_TIMER, timerID, 0);

			return false;
		}

		return true;
	}
}
