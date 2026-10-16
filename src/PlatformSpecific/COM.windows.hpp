/*
 * src/PlatformSpecific/COM.windows.hpp
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

#pragma once

/* NOTE: Windows sources only (the engine's *.windows.cpp files). A private header: it is not in the installed header
 * set (cmake/PrepareEngineSourceFiles.cmake), like StringConversion.mac.hpp. */
#ifndef _WIN32
#error "COM.windows.hpp is for Windows sources (*.windows.cpp) only."
#endif

/* STL inclusions. */
#include <memory>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief The COM initialization of the calling thread (CoInitializeEx), given back by the destructor (CoUninitialize).
	 * @note S_OK and S_FALSE both take a reference that CoUninitialize() must give back; a failure, RPC_E_CHANGED_MODE
	 * included (the thread already lives in the other apartment model), takes none. The caller judges result() itself:
	 * some callers carry on under RPC_E_CHANGED_MODE, others refuse every failure.
	 * @note Thread-affine: destroyed on the thread that constructed it, never copied nor moved.
	 */
	class ComInitialization final
	{
		public:

			/**
			 * @brief Initializes COM on the calling thread.
			 * @param concurrencyModel The CoInitializeEx() flags (COINIT_APARTMENTTHREADED, COINIT_MULTITHREADED...).
			 */
			explicit
			ComInitialization (DWORD concurrencyModel) noexcept
				: m_result{CoInitializeEx(nullptr, concurrencyModel)}
			{

			}

			/** @brief Deleted copy constructor. */
			ComInitialization (const ComInitialization & copy) noexcept = delete;

			/** @brief Deleted move constructor. */
			ComInitialization (ComInitialization && copy) noexcept = delete;

			/** @brief Deleted copy assignment. */
			ComInitialization & operator= (const ComInitialization & copy) noexcept = delete;

			/** @brief Deleted move assignment. */
			ComInitialization & operator= (ComInitialization && copy) noexcept = delete;

			/**
			 * @brief Gives the COM reference back when the constructor took one.
			 */
			~ComInitialization ()
			{
				if ( SUCCEEDED(m_result) )
				{
					CoUninitialize();
				}
			}

			/**
			 * @brief Returns the result of CoInitializeEx().
			 * @return HRESULT
			 */
			[[nodiscard]]
			HRESULT
			result () const noexcept
			{
				return m_result;
			}

		private:

			HRESULT m_result;
	};

	/**
	 * @brief The deleter of a block from COM's task allocator (CoTaskMemAlloc), for std::unique_ptr.
	 */
	struct CoTaskMemDeleter final
	{
		/**
		 * @brief Frees the block (CoTaskMemFree(nullptr) is a no-op).
		 * @param block The block.
		 */
		void
		operator() (void * block) const noexcept
		{
			CoTaskMemFree(block);
		}
	};

	/**
	 * @brief Owns a block that a COM API allocated for the caller to free: a string (PWSTR), an ID list, an array.
	 * @tparam data_t The pointed type (wchar_t for a PWSTR).
	 */
	template< typename data_t >
	using CoTaskMemPointer = std::unique_ptr< data_t, CoTaskMemDeleter >;

	/**
	 * @brief Owns a BSTR (SysAllocString), freed by the destructor (SysFreeString).
	 * @note It replaces _bstr_t, which reports an allocation failure by throwing _com_error (no exceptions in the
	 * engine: that throw would terminate the process). Here a failed allocation leaves get() null: check it before use.
	 */
	class BasicString final
	{
		public:

			/**
			 * @brief Allocates the BSTR copy of a string.
			 * @param string A NUL-terminated wide string.
			 */
			explicit
			BasicString (const wchar_t * string) noexcept
				: m_string{SysAllocString(string)}
			{

			}

			/** @brief Deleted copy constructor. */
			BasicString (const BasicString & copy) noexcept = delete;

			/** @brief Deleted move constructor. */
			BasicString (BasicString && copy) noexcept = delete;

			/** @brief Deleted copy assignment. */
			BasicString & operator= (const BasicString & copy) noexcept = delete;

			/** @brief Deleted move assignment. */
			BasicString & operator= (BasicString && copy) noexcept = delete;

			/**
			 * @brief Frees the BSTR (SysFreeString(nullptr) is a no-op).
			 */
			~BasicString ()
			{
				SysFreeString(m_string);
			}

			/**
			 * @brief Returns the BSTR, null when the allocation failed.
			 * @return BSTR
			 */
			[[nodiscard]]
			BSTR
			get () const noexcept
			{
				return m_string;
			}

		private:

			BSTR m_string;
	};
}
