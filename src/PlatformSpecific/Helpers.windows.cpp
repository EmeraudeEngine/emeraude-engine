/*
 * src/PlatformSpecific/Helpers.windows.cpp
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

#include "Helpers.hpp"

/* STL inclusions. */
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <objbase.h>
#include <psapi.h>
#include <tlhelp32.h>

/* Local inclusions. */
#include "Tracer.hpp"
#include "Window.hpp"

namespace EmEn::PlatformSpecific
{
	constexpr auto TracerTag{"Helpers"};

	std::wstring
	getStringValueFromHKLM (const std::wstring & regSubKey, const std::wstring & regValue)
	{
		size_t bufferSize = 0xFFF; // If too small, will be resized down below.
		std::wstring valueBuf;
		valueBuf.resize(bufferSize);

		auto cbData = static_cast< DWORD >(bufferSize * sizeof(wchar_t));
		auto rc = RegGetValueW(
			HKEY_LOCAL_MACHINE,
			regSubKey.c_str(),
			regValue.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			valueBuf.data(),
			&cbData
		);

		while ( rc == ERROR_MORE_DATA )
		{
			// Get a buffer that is big enough.
			cbData /= sizeof(wchar_t);

			if ( cbData > static_cast< DWORD >(bufferSize) )
			{
				bufferSize = static_cast<size_t>(cbData);
			}
			else
			{
				bufferSize *= 2;
				cbData = static_cast< DWORD >(bufferSize * sizeof(wchar_t));
			}

			valueBuf.resize(bufferSize);

			rc = RegGetValueW(
				HKEY_LOCAL_MACHINE,
				regSubKey.c_str(),
				regValue.c_str(),
				RRF_RT_REG_SZ,
				nullptr,
				static_cast<void*>(valueBuf.data()),
				&cbData
			);
		}

		if ( rc != ERROR_SUCCESS )
		{
			throw std::runtime_error("Windows system error code: " + std::to_string(rc));
		}

		cbData /= sizeof(wchar_t);
		valueBuf.resize(static_cast< size_t >(cbData - 1)); // remove end null character

		return valueBuf;
	}

	std::string
	convertWideToANSI (const std::wstring & input)
	{
		const int count = WideCharToMultiByte(
			CP_ACP,
			0,
			input.data(),
			static_cast< int >(input.length()),
			nullptr,
			0,
			nullptr,
			nullptr
		);

		std::string output{};
		output.resize(count);

		WideCharToMultiByte(
			CP_ACP,
			0,
			input.data(),
			-1,
			output.data(),
			count,
			nullptr,
			nullptr
		);

		return output;
	}

	std::wstring
	convertANSIToWide (const std::string & input)
	{
		const int count = MultiByteToWideChar(
			CP_ACP,
			0,
			input.data(),
			static_cast< int >(input.length()),
			nullptr,
			0
		);

		std::wstring output{};
		output.resize(count);

		MultiByteToWideChar(
			CP_ACP,
			0,
			input.data(),
			static_cast< int >(input.length()),
			output.data(),
			count
		);

		return output;
	}

	std::string
	convertWideToUTF8 (const std::wstring & input)
	{
		const int count = WideCharToMultiByte(
			CP_UTF8,
			0,
			input.data(),
			static_cast< int >(input.length()),
			nullptr,
			0,
			nullptr,
			nullptr
		);

		std::string output{};
		output.resize(count);

		WideCharToMultiByte(
			CP_UTF8,
			0,
			input.data(),
			-1,
			output.data(),
			count,
			nullptr,
			nullptr
		);

		return output;
	}

	std::wstring
	convertUTF8ToWide (const std::string & input)
	{
		const int count = MultiByteToWideChar(
			CP_UTF8,
			0,
			input.data(),
			static_cast< int >(input.length()),
			nullptr,
			0
		);

		std::wstring output{};
		output.resize(count);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			input.data(),
			static_cast< int >(input.length()),
			output.data(),
			count
		);

		return output;
	}

	bool
	createConsole (const std::string & title)
	{
		if ( !AllocConsole() )
		{
			return false;
		}

		SetConsoleTitleA(title.c_str());

		// std::cout, std::clog, std::cerr, std::cin
		FILE * fDummy = nullptr;

		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);

		std::cout.clear();
		std::clog.clear();
		std::cerr.clear();
		std::cin.clear();

		auto hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleMode(hOutput, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

		auto hError = GetStdHandle(STD_ERROR_HANDLE);
		SetConsoleMode(hError, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

		return true;
	}

	bool
	attachToParentConsole ()
	{
		/* Try to attach to parent process console (e.g., Visual Studio terminal). */
		if ( !AttachConsole(ATTACH_PARENT_PROCESS) )
		{
			return false;
		}

		/* Redirect std streams to the attached console. */
		FILE * fDummy = nullptr;
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONOUT$", "w", stderr);
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		std::cout.clear();
		std::clog.clear();
		std::cerr.clear();
		std::cin.clear();

		enableConsoleANSI();

		return true;
	}

	void
	enableConsoleANSI ()
	{
		auto hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleMode(hOutput, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

		auto hError = GetStdHandle(STD_ERROR_HANDLE);
		SetConsoleMode(hError, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}

	void
	waitBeforeConsoleClose ()
	{
		std::cout << "\nPress any key to close this window..." << std::flush;

		HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);

		/* Clear any pending input. */
		FlushConsoleInputBuffer(hInput);

		/* Wait for a key press using Windows API. */
		INPUT_RECORD inputRecord{};
		DWORD eventsRead = 0;

		while ( true )
		{
			ReadConsoleInput(hInput, &inputRecord, 1, &eventsRead);

			if ( inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown )
			{
				break;
			}
		}
	}

	int
	getParentProcessId (DWORD pid) noexcept
	{
		DWORD ppid = 0;

		HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		PROCESSENTRY32 pe{0};
		pe.dwSize = sizeof(PROCESSENTRY32);

		if ( Process32First(h, &pe) )
		{
			do
			{
				if ( pe.th32ProcessID == pid )
				{
					ppid = pe.th32ParentProcessID;
					break;
				}

			} while ( Process32Next(h, &pe) );
		}

		CloseHandle(h);

		return static_cast< int >(ppid);
	}

	std::vector< COMDLG_FILTERSPEC >
	createExtensionFilter (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters, std::map< std::wstring, std::wstring > & dataHolder)
	{
		std::vector< COMDLG_FILTERSPEC > filterPointers;
		filterPointers.reserve(filters.size());

		for ( const auto &[name, extensions] : filters )
		{
			std::wstringstream extensionsRule;

			for ( const auto & extension : extensions )
			{
				extensionsRule << "*." << convertUTF8ToWide(extension) << ";";
			}

			auto [item, success] = dataHolder.emplace(convertUTF8ToWide(name), extensionsRule.str());

			COMDLG_FILTERSPEC type;
			type.pszName = item->first.data();
			type.pszSpec = item->second.data();

			filterPointers.emplace_back(type);
		}

		return filterPointers;
	}

	namespace
	{
		constexpr auto FileDialogClassId{"FileDialog"};

		/**
		 * @brief Hidden top-level owner window positioned over a target screen rectangle, created
		 * on the calling (STA worker) thread so the shell centers the modal dialog on it WITHOUT a
		 * cross-thread owner.
		 *
		 * Uses the built-in "STATIC" class (no RegisterClass, no class-atom lifetime concern) and
		 * is never shown (no WS_VISIBLE) — the shell only needs its geometry. Created and destroyed
		 * (RAII) on the same thread as the dialog's modal loop, so the implicit owner enable/disable
		 * sends stay same-thread and cannot deadlock against the blocked caller thread.
		 */
		class CenteringOwner final
		{
			public:

				explicit
				CenteringOwner (const RECT * targetRect) noexcept
				{
					if ( targetRect == nullptr )
					{
						return;
					}

					const LONG rawWidth = targetRect->right - targetRect->left;
					const LONG rawHeight = targetRect->bottom - targetRect->top;
					const int width = rawWidth > 1 ? static_cast< int >(rawWidth) : 1;
					const int height = rawHeight > 1 ? static_cast< int >(rawHeight) : 1;

					m_handle = CreateWindowExW(
						WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
						L"STATIC", L"",
						WS_POPUP,
						static_cast< int >(targetRect->left), static_cast< int >(targetRect->top),
						width, height,
						nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
				}

				CenteringOwner (const CenteringOwner &) = delete;

				CenteringOwner (CenteringOwner &&) = delete;

				CenteringOwner & operator= (const CenteringOwner &) = delete;

				CenteringOwner & operator= (CenteringOwner &&) = delete;

				~CenteringOwner ()
				{
					if ( m_handle != nullptr )
					{
						DestroyWindow(m_handle);
					}
				}

				[[nodiscard]]
				HWND
				handle () const noexcept
				{
					return m_handle;
				}

			private:

				HWND m_handle{nullptr};
		};
	}

	bool
	runFileDialogOnDedicatedThread (Window & window, bool parentToWindow, const std::function< bool (HWND) > & dialogBody) noexcept
	{
		/* Read the main window geometry + DPI context on the CALLER thread; never deref the main
		 * window HWND from the worker.
		 *
		 * Skip the centering owner when the window is minimized: GetWindowRect then returns the
		 * iconic ghost coordinates (~{-32000, -32000}), which would center the dialog off-screen.
		 * With no owner rect we fall back to the shell's default (visible) placement. We test
		 * IsIconic rather than the coordinate sign — a window on a left secondary monitor has a
		 * legitimately negative X. */
		const HWND mainWindow = window.getWin32Window();
		RECT ownerRect{};
		const bool haveOwnerRect = parentToWindow && mainWindow != nullptr && IsIconic(mainWindow) == 0 && GetWindowRect(mainWindow, &ownerRect) != 0;
		const DPI_AWARENESS_CONTEXT dpiContext = mainWindow != nullptr ? GetWindowDpiAwarenessContext(mainWindow) : nullptr;

		/* The actual dialog run: STA COM init + hidden centering owner + body, on whichever thread
		 * invokes it (the dedicated worker normally, the caller thread on the fallback path). */
		const auto runDialog = [&dialogBody, haveOwnerRect, &ownerRect] () -> bool {
			const HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

			/* RAII hidden owner, created and destroyed on the running thread. */
			const CenteringOwner owner{haveOwnerRect ? &ownerRect : nullptr};

			const bool dialogResult = dialogBody(owner.handle());

			if ( SUCCEEDED(comInit) )
			{
				CoUninitialize();
			}

			return dialogResult;
		};

		bool result = false;

		/* The dialog runs on a DEDICATED STA thread (empty message queue) — see the header for the
		 * full perf rationale. The DPI awareness is replicated on the worker ONLY; setting it on the
		 * caller thread would permanently mutate the main thread's context.
		 *
		 * Only the thread CONSTRUCTION is guarded: it is the sole realistic failure (std::thread
		 * throws std::system_error on OS resource exhaustion). join() on a freshly joinable thread
		 * does not throw, so it stays outside the try. */
		std::optional< std::thread > worker;

		try
		{
			worker.emplace([&runDialog, &result, dpiContext] () {
				if ( dpiContext != nullptr )
				{
					SetThreadDpiAwarenessContext(dpiContext);
				}

				result = runDialog();
			});
		}
		catch ( const std::exception & error )
		{
			/* Spawning the dedicated thread failed. Fall back to running the dialog on the caller
			 * thread: slower on cloud-redirected-known-folder machines (the original symptom), but
			 * functional. No DPI mutation here — the caller thread already owns the window. */
			TraceWarning{FileDialogClassId} << "Unable to spawn the dedicated dialog thread (" << error.what() << "); running the dialog inline.";

			return runDialog();
		}

		worker->join();

		return result;
	}
}
