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
	}

	bool
	runFileDialogOnDedicatedThread (Window & window, bool parentToWindow, const std::function< bool (HWND) > & dialogBody) noexcept
	{
		/* Reentrancy guard. While the dialog is up, the caller pumps its message queue (below), which
		 * re-enters the main window's WndProc; a dispatched message can call back into this function to
		 * open a SECOND native file dialog on the same thread. That is refused outright: two stacked
		 * native file choosers make no UX sense, and — more importantly — a nested dialog owned by the
		 * same main window would EnableWindow(owner, TRUE) on close (Win32 owner enable/disable is NOT
		 * ref-counted), re-enabling the main window while the outer dialog is still modal. Refusing at
		 * the top prevents the nested Show() entirely, which is the only robust fix. A refused call
		 * returns false — the caller treats it as a cancellation. The flag is thread_local (file dialogs
		 * are shown from the UI thread) and RAII-scoped over the whole operation, including the inline
		 * fallback path. */
		static thread_local bool s_fileDialogActive = false;

		if ( s_fileDialogActive )
		{
			TraceWarning{FileDialogClassId} << "Reentrant native file dialog ignored: one is already open on this thread.";

			return false;
		}

		struct ActiveGuard final
		{
			ActiveGuard () noexcept { s_fileDialogActive = true; }
			~ActiveGuard () { s_fileDialogActive = false; }
			ActiveGuard (const ActiveGuard &) = delete;
			ActiveGuard (ActiveGuard &&) = delete;
			ActiveGuard & operator= (const ActiveGuard &) = delete;
			ActiveGuard & operator= (ActiveGuard &&) = delete;
		} activeGuard;

		/* The dialog is OWNED by the real main window so the OS provides native modality (it disables
		 * the owner), native centering (on the owner), and native Z-order (the dialog stays above the
		 * owner and resurfaces with it on activation — e.g. after an alt-tab / taskbar click). Read
		 * the owner + its DPI context on the CALLER thread; the worker only receives handles/ids by
		 * value. When parentToWindow == false the owner is null: the dialog is shown ownerless —
		 * no modality, no centering. The DPI context is replicated whenever a main window exists
		 * (independent of parentToWindow) so even an ownerless dialog renders at the right scale. */
		const HWND mainWindow = window.getWin32Window();
		const HWND ownerWindow = parentToWindow ? mainWindow : nullptr;
		const DWORD ownerThreadId = ownerWindow != nullptr ? GetWindowThreadProcessId(ownerWindow, nullptr) : 0;
		const DPI_AWARENESS_CONTEXT dpiContext = mainWindow != nullptr ? GetWindowDpiAwarenessContext(mainWindow) : nullptr;

		/* The actual dialog run: STA COM init + body, on whichever thread invokes it (the dedicated
		 * worker normally, the caller thread on the fallback path).
		 *
		 * The owner is CROSS-THREAD (dialog on the worker, owner on the main thread). That is safe
		 * here because the caller pumps messages while waiting (below), so the owner's implicit
		 * enable/disable/activation sends complete. AttachThreadInput merges the two threads' input
		 * state for the dialog's lifetime so activation and focus return behave as if same-thread. */
		const auto runDialog = [&dialogBody, ownerWindow, ownerThreadId] () -> bool {
			const HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

			const bool inputAttached = ownerThreadId != 0 && AttachThreadInput(GetCurrentThreadId(), ownerThreadId, TRUE) != 0;

			const bool dialogResult = dialogBody(ownerWindow);

			if ( inputAttached )
			{
				AttachThreadInput(GetCurrentThreadId(), ownerThreadId, FALSE);
			}

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

		/* Wait for the dialog WITHOUT hard-blocking the main thread — drain the caller's own message
		 * queue, then block, repeat. Pumping is required for two reasons:
		 *   1. The dialog owns the main window CROSS-THREAD, so the OS sends it WM_ENABLE / activation
		 *      messages originating from the worker; a bare join() would never service them, deadlocking
		 *      the native modal handshake.
		 *   2. A thread that stops pumping is marked "Not Responding" by Windows after ~5 s (DWM ghost:
		 *      greyed snapshot, spinning cursor), even though the dialog is alive on the worker.
		 * The main window is disabled by the native modal (it is the owner), so the pumped input is
		 * ignored — responsive but non-interactive, the correct modal behavior. A reentrant open from a
		 * dispatched message is refused by the guard at the top of this function.
		 *
		 * This does NOT reintroduce the shell-pane perf storm: that was the DIALOG's modal loop pumping
		 * heavy traffic. The dialog's loop runs on the worker; this pump only services the main thread's
		 * own messages (paint, CEF, DWM pings). */
		const HANDLE workerHandle = static_cast< HANDLE >(worker->native_handle());

		for ( ;; )
		{
			/* Drain FIRST, then wait. Dispatch everything currently queued for THIS thread's own
			 * windows (paint, posted CEF work, timers, WM_QUIT), then block. Draining before the wait
			 * catches a WM_QUIT already sitting in the queue immediately, and removes the need for
			 * MWMO_INPUTAVAILABLE — which, combined with the shared input queue (see the wait below),
			 * is what caused the busy-spin. */
			MSG message;
			bool quitReceived = false;

			while ( PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != 0 )
			{
				if ( message.message == WM_QUIT )
				{
					/* App is quitting. Stop pumping, wait the dialog out with a plain block (brief,
					 * and only on this rare path), then re-post WM_QUIT so the engine's main loop
					 * sees it after we return — we never return while the dialog is still up. */
					WaitForSingleObject(workerHandle, INFINITE);
					PostQuitMessage(static_cast< int >(message.wParam));
					quitReceived = true;
					break;
				}

				TranslateMessage(&message);
				DispatchMessageW(&message);
			}

			if ( quitReceived )
			{
				break;
			}

			/* Wait for the worker to finish OR for work THIS thread must service. The wake mask
			 * deliberately EXCLUDES QS_INPUT: AttachThreadInput (in runDialog) shares the worker's
			 * input queue, so mouse-moves / keystrokes aimed at the DIALOG would otherwise wake this
			 * wait continuously while the PeekMessage drain above — which only retrieves messages for
			 * THIS thread's own windows — removes nothing, spinning at 100% CPU whenever the user
			 * moves the mouse over the dialog. The main window is disabled by the modal and has no
			 * input to process anyway; it only needs paint, posted (CEF), timer and sent messages.
			 * MWMO_INPUTAVAILABLE is intentionally omitted (we drained first), so already-seen shared
			 * input never forces an immediate return. Sent messages (the owner's WM_ENABLE /
			 * activation handshake) are serviced regardless of the mask. */
			const DWORD waitStatus = MsgWaitForMultipleObjectsEx(1, &workerHandle, INFINITE, QS_PAINT | QS_TIMER | QS_POSTMESSAGE | QS_SENDMESSAGE, 0);

			/* WAIT_OBJECT_0 → worker finished; anything other than "a message is available" (or an
			 * unexpected wait failure) → stop and let join() reap the worker. */
			if ( waitStatus != WAIT_OBJECT_0 + 1 )
			{
				break;
			}
		}

		worker->join();

		return result;
	}
}
