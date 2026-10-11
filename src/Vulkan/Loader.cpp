/*
 * src/Vulkan/Loader.cpp
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

#include "Loader.hpp"

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <array>

/* Local inclusions. */
#if IS_WINDOWS
	#include <Windows.h>
#else
	#include <dlfcn.h>
#endif
#include "IO/IO.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief The places the shipped loader may live in, in order.
		 * @param binaryDirectory The directory of the running executable.
		 * @return std::array< std::filesystem::path, 2 >
		 */
		[[nodiscard]]
		std::array< std::filesystem::path, 2 >
		loaderCandidates (const std::filesystem::path & binaryDirectory) noexcept
		{
			if constexpr ( IsWindows )
			{
				return {binaryDirectory / "vulkan" / "vulkan-1.dll", std::filesystem::path{}};
			}
			else if constexpr ( IsMacOS )
			{
				/* NOTE: A bundle: Contents/MacOS/<binary> -> Contents/Frameworks/libvulkan.1.dylib. */
				return {binaryDirectory.parent_path() / "Frameworks" / "libvulkan.1.dylib", binaryDirectory / "vulkan" / "libvulkan.1.dylib"};
			}
			else
			{
				return {binaryDirectory / "vulkan" / "libvulkan.so.1", std::filesystem::path{}};
			}
		}
	}

	bool
	Loader::open (const std::filesystem::path & binaryDirectory) noexcept
	{
		if ( this->isOpen() )
		{
			return true;
		}

		for ( const auto & candidate : loaderCandidates(binaryDirectory) )
		{
			if ( candidate.empty() || !IO::fileExists(candidate) )
			{
				continue;
			}

#if IS_WINDOWS
			/* NOTE: A full path loads THIS file even when CEF's vulkan-1.dll is already in the process. */
			const auto module = LoadLibraryExW(candidate.wstring().c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

			if ( module == nullptr )
			{
				TraceError{ClassId} << "Unable to load the Vulkan loader '" << candidate.string() << "' (error " << GetLastError() << ") !";

				return false;
			}

			/* NOTE: GetProcAddress answers a generic FARPROC; the symbol is the loader's vkGetInstanceProcAddr. */
			const auto entry = reinterpret_cast< PFN_vkGetInstanceProcAddr >(reinterpret_cast< void * >(GetProcAddress(module, "vkGetInstanceProcAddr")));

			if ( entry == nullptr )
			{
				FreeLibrary(module);
			}
			else
			{
				m_handle = module;
			}
#else
			/* NOTE: An explicit path with RTLD_LOCAL never shares CEF's copy (a different file); on Linux RTLD_DEEPBIND
			 * also keeps the loader's own symbols bound to itself if another loader exported the same names globally. */
	#if IS_LINUX
			constexpr int Flags{RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND};
	#else
			constexpr int Flags{RTLD_NOW | RTLD_LOCAL};
	#endif
			auto * library = dlopen(candidate.c_str(), Flags);

			if ( library == nullptr )
			{
				const char * reason = dlerror();

				TraceError{ClassId} << "Unable to load the Vulkan loader '" << candidate.string() << "' : " << ( reason != nullptr ? reason : "unknown reason" ) << " !";

				return false;
			}

			/* NOTE: POSIX guarantees a dlsym() result converts to a function pointer. */
			const auto entry = reinterpret_cast< PFN_vkGetInstanceProcAddr >(dlsym(library, "vkGetInstanceProcAddr"));

			if ( entry == nullptr )
			{
				dlclose(library);
			}
			else
			{
				m_handle = library;
			}
#endif

			if ( entry == nullptr )
			{
				TraceError{ClassId} << "The Vulkan loader '" << candidate.string() << "' exports no vkGetInstanceProcAddr !";

				return false;
			}

			m_getInstanceProcAddr = entry;
			m_path = candidate;

			volkInitializeCustom(m_getInstanceProcAddr);

			TraceInfo{ClassId} << "Vulkan loader '" << m_path.string() << "' opened (instance version " <<
				VK_API_VERSION_MAJOR(volkGetInstanceVersion()) << '.' << VK_API_VERSION_MINOR(volkGetInstanceVersion()) << '.' << VK_API_VERSION_PATCH(volkGetInstanceVersion()) << ").";

			return true;
		}

		TraceFatal{ClassId} << "No Vulkan loader shipped with the application (looked for '" << loaderCandidates(binaryDirectory)[0].string() << "') : the build did not copy the external dependencies' vulkan-sdk loader.";

		return false;
	}

	void
	Loader::close () noexcept
	{
		if ( m_handle == nullptr )
		{
			return;
		}

		volkFinalize();

#if IS_WINDOWS
		FreeLibrary(static_cast< HMODULE >(m_handle));
#else
		dlclose(m_handle);
#endif

		m_handle = nullptr;
		m_getInstanceProcAddr = nullptr;
		m_path.clear();
	}
}
