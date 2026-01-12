/*
 * src/PlatformSpecific/Desktop/Commands.windows.cpp
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

#include "Commands.hpp"

#if IS_WINDOWS

/* STL inclusions. */
#include <vector>

/* Third-party inclusions. */
#include <windows.h>
#include <shobjidl.h>
#include "reproc++/run.hpp"

/* Local inclusions. */
#include "Window.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific::Desktop
{
	constexpr auto TracerTag{"Commands"};

	bool
	runDesktopApplication (const std::string & executable, const std::string & argument) noexcept
	{
		if ( executable.empty() )
		{
			Tracer::error(TracerTag, "No executable to run!");

			return false;
		}

		std::vector< const char * > args;
		args.reserve(7);
		args.push_back("cmd.exe");
		args.push_back("/c");
		args.push_back("start");
		args.push_back(""); /* Requested by "start" */
		args.push_back(executable.data());
		if ( !argument.empty() )
		{
			args.push_back(argument.data());
		}
		args.push_back(nullptr);

		const auto [exitCode, errorCode] = reproc::run(args.data());

		if ( exitCode != 0 )
		{
			TraceError{TracerTag} << "Failed to run a subprocess : " << errorCode.message();

			return false;
		}

		return true;
	}

	bool
	runDefaultDesktopApplication (const std::string & argument) noexcept
	{
		if ( argument.empty() )
		{
			Tracer::error(TracerTag, "No argument to open with desktop terminal.");

			return false;
		}

		std::vector< const char * > args;
		args.reserve(6);
		args.push_back("cmd.exe");
		args.push_back("/c");
		args.push_back("start");
		args.push_back(""); /* Requested by "start" */
		args.push_back(argument.data());
		args.push_back(nullptr);

		const auto [exitCode, errorCode] = reproc::run(args.data());

		if ( exitCode != 0 )
		{
			TraceError{TracerTag} << "Failed to run a subprocess : " << errorCode.message();

			return false;
		}

		return true;
	}

	void
	flashTaskbarIcon (const Window & window, bool state) noexcept
	{
		FLASHWINFO fwi;
		fwi.cbSize = sizeof(fwi);
		fwi.hwnd = window.getWin32Window();
		fwi.dwFlags = state ? FLASHW_ALL : FLASHW_STOP;
		fwi.uCount = 0;
		FlashWindowEx(&fwi);
	}

	void
	setTaskbarIconProgression (const Window & window, float progress, ProgressMode mode) noexcept
	{
		ITaskbarList3 * taskbar = nullptr;

		/* Initialize COM if not already done. */
		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		if ( FAILED(hr) && hr != RPC_E_CHANGED_MODE )
		{
			Tracer::error(TracerTag, "Failed to initialize COM for taskbar progress.");

			return;
		}

		/* Create ITaskbarList3 instance. */
		hr = CoCreateInstance(
			CLSID_TaskbarList,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_ITaskbarList3,
			reinterpret_cast< void ** >(&taskbar)
		);

		if ( FAILED(hr) || taskbar == nullptr )
		{
			Tracer::error(TracerTag, "Failed to create ITaskbarList3 instance.");

			return;
		}

		/* Initialize the taskbar list. */
		hr = taskbar->HrInit();

		if ( FAILED(hr) )
		{
			taskbar->Release();

			Tracer::error(TracerTag, "Failed to initialize ITaskbarList3.");

			return;
		}

		HWND hwnd = window.getWin32Window();

		/* Handle progress disable (negative value). */
		if ( progress < 0.0F )
		{
			taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
			taskbar->Release();

			return;
		}

		/* Convert ProgressMode to Windows TBPFLAG. */
		TBPFLAG state = TBPF_NORMAL;

		switch ( mode )
		{
			case ProgressMode::None:
				state = TBPF_NOPROGRESS;
				break;

			case ProgressMode::Normal:
				state = TBPF_NORMAL;
				break;

			case ProgressMode::Indeterminate:
				state = TBPF_INDETERMINATE;
				break;

			case ProgressMode::Error:
				state = TBPF_ERROR;
				break;

			case ProgressMode::Paused:
				state = TBPF_PAUSED;
				break;
		}

		/* Set the progress state. */
		taskbar->SetProgressState(hwnd, state);

		/* Set the progress value (0-100 range). */
		if ( mode != ProgressMode::None && mode != ProgressMode::Indeterminate )
		{
			const auto progressValue = static_cast< ULONGLONG >(progress * 100.0F);
			taskbar->SetProgressValue(hwnd, progressValue, 100);
		}

		taskbar->Release();
	}
}

#endif
