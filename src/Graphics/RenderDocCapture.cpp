/*
 * src/Graphics/RenderDocCapture.cpp
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

#include "RenderDocCapture.hpp"

#ifdef EMERAUDE_ENABLE_RENDERDOC

/* Local inclusions. */
#include "Tracer.hpp"

/* Platform-specific inclusions for dynamic library loading. */
#if IS_LINUX || IS_MACOS
#include <dlfcn.h>
#elif IS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace EmEn::Graphics
{
	static constexpr auto ClassId{"RenderDocCapture"};

	bool
	RenderDocCapture::initialize () noexcept
	{
		if ( m_api != nullptr )
		{
			TraceWarning{ClassId} << "RenderDoc API already initialized.";
			return true;
		}

#if IS_LINUX
		void * module = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
#elif IS_MACOS
		void * module = dlopen("librenderdoc.dylib", RTLD_NOW | RTLD_NOLOAD);
#elif IS_WINDOWS
		HMODULE module = GetModuleHandleA("renderdoc.dll");
#endif

#if IS_LINUX || IS_MACOS
		if ( module == nullptr )
		{
			TraceInfo{ClassId} << "RenderDoc not detected (library not injected). Capture API disabled.";
			return false;
		}

		auto getRenderDocAPI = reinterpret_cast< pRENDERDOC_GetAPI >(dlsym(module, "RENDERDOC_GetAPI"));
#elif IS_WINDOWS
		if ( module == nullptr )
		{
			TraceInfo{ClassId} << "RenderDoc not detected (library not injected). Capture API disabled.";
			return false;
		}

		auto getRenderDocAPI = reinterpret_cast< pRENDERDOC_GetAPI >(GetProcAddress(module, "RENDERDOC_GetAPI"));
#endif

		if ( getRenderDocAPI == nullptr )
		{
			TraceWarning{ClassId} << "RenderDoc library found but RENDERDOC_GetAPI symbol not available.";
			return false;
		}

		const int result = getRenderDocAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast< void ** >(&m_api));

		if ( result != 1 || m_api == nullptr )
		{
			TraceWarning{ClassId} << "Failed to retrieve RenderDoc API version 1.6.0.";
			m_api = nullptr;
			return false;
		}

		/* Configure capture options. */
		m_api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 0);
		m_api->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 0);
		m_api->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials, 0);
		m_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

		int major = 0;
		int minor = 0;
		int patch = 0;
		m_api->GetAPIVersion(&major, &minor, &patch);

		TraceSuccess{ClassId} << "RenderDoc API " << major << "." << minor << "." << patch << " initialized successfully.";

		return true;
	}

	void
	RenderDocCapture::setDevice (void * nativeInstance) noexcept
	{
		m_device = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(nativeInstance);

		if ( m_api != nullptr )
		{
			TraceInfo{ClassId} << "Vulkan instance registered for capture operations.";
		}
	}

	void
	RenderDocCapture::shutdown () noexcept
	{
		m_device = nullptr;
		m_api = nullptr;

		TraceInfo{ClassId} << "RenderDoc capture API shut down.";
	}

	bool
	RenderDocCapture::isAvailable () const noexcept
	{
		return m_api != nullptr;
	}

	void
	RenderDocCapture::startCapture () noexcept
	{
		if ( m_api != nullptr )
		{
			m_api->StartFrameCapture(m_device, nullptr);

			TraceInfo{ClassId} << "Frame capture started.";
		}
	}

	bool
	RenderDocCapture::endCapture () noexcept
	{
		if ( m_api == nullptr )
		{
			return false;
		}

		const uint32_t result = m_api->EndFrameCapture(m_device, nullptr);

		if ( result != 1 )
		{
			TraceWarning{ClassId} << "EndFrameCapture returned failure — no capture was saved.";
			return false;
		}

		const uint32_t numCaptures = m_api->GetNumCaptures();

		TraceSuccess{ClassId} << "Frame capture ended successfully. Total captures: " << numCaptures;

		/* Log the path of the last capture. */
		if ( numCaptures > 0 )
		{
			uint32_t pathLength = 0;
			m_api->GetCapture(numCaptures - 1, nullptr, &pathLength, nullptr);

			std::string capturePath(pathLength, '\0');
			m_api->GetCapture(numCaptures - 1, capturePath.data(), &pathLength, nullptr);

			TraceInfo{ClassId} << "Capture saved to: " << capturePath;
		}

		return true;
	}

	void
	RenderDocCapture::triggerCapture () noexcept
	{
		if ( m_api != nullptr )
		{
			m_api->TriggerCapture();

			TraceInfo{ClassId} << "Triggered capture of next presented frame.";
		}
	}

	void
	RenderDocCapture::triggerMultiFrameCapture (uint32_t count) noexcept
	{
		if ( m_api != nullptr )
		{
			m_api->TriggerMultiFrameCapture(count);

			TraceInfo{ClassId} << "Triggered capture of next " << count << " presented frame(s).";
		}
	}

	void
	RenderDocCapture::setCaptureFilePath (const std::string & path) noexcept
	{
		if ( m_api != nullptr )
		{
			m_api->SetCaptureFilePathTemplate(path.c_str());

			TraceInfo{ClassId} << "Capture file path template set to: " << path;
		}
	}

	void
	RenderDocCapture::setCaptureTitle (const std::string & title) noexcept
	{
		if ( m_api != nullptr )
		{
			m_api->SetCaptureTitle(title.c_str());
		}
	}

	bool
	RenderDocCapture::isCapturing () const noexcept
	{
		if ( m_api != nullptr )
		{
			return m_api->IsFrameCapturing() != 0;
		}

		return false;
	}

	bool
	RenderDocCapture::launchReplayUI () noexcept
	{
		if ( m_api != nullptr )
		{
			const uint32_t pid = m_api->LaunchReplayUI(1, nullptr);

			if ( pid != 0 )
			{
				TraceSuccess{ClassId} << "RenderDoc replay UI launched (PID: " << pid << ").";
				return true;
			}

			TraceWarning{ClassId} << "Failed to launch RenderDoc replay UI.";
		}

		return false;
	}
}

#endif /* EMERAUDE_ENABLE_RENDERDOC */
