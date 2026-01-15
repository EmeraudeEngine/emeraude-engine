/*
 * src/PlatformSpecific/Desktop/Dialog/CustomMessage.windows.cpp
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

#include "CustomMessage.hpp"

#if IS_WINDOWS

/* STL inclusions. */
#include <vector>

/* Local inclusions. */
#include "Window.hpp"

/* Windows inclusions. */
#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

/* Manifest dependency for Common Controls v6 (required for TaskDialogIndirect).
 * This pragma injects the dependency into any executable linking with this library. */
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	namespace
	{
		PCWSTR
		getTaskDialogIcon (MessageType messageType) noexcept
		{
			switch ( messageType )
			{
				case MessageType::Info:
					return TD_INFORMATION_ICON;

				case MessageType::Warning:
					return TD_WARNING_ICON;

				case MessageType::Error:
					return TD_ERROR_ICON;

				case MessageType::Question:
				default:
					return nullptr;
			}
		}

		std::wstring
		toWideString (const std::string & str) noexcept
		{
			if ( str.empty() )
			{
				return {};
			}

			const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast< int >(str.size()), nullptr, 0);

			if ( sizeNeeded <= 0 )
			{
				return {};
			}

			std::wstring result(static_cast< size_t >(sizeNeeded), 0);
			MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast< int >(str.size()), result.data(), sizeNeeded);

			return result;
		}
	}

	bool
	CustomMessage::execute (Window * window) noexcept
	{
		if ( m_buttons.empty() )
		{
			m_clickedIndex = -1;

			return false;
		}

		/* Convert strings to wide strings. */
		const std::wstring wideTitle = toWideString(this->title());
		const std::wstring wideMessage = toWideString(m_message);

		/* Convert button labels to wide strings and create TASKDIALOG_BUTTON array.
		 * We store wide strings separately to keep them alive during the dialog. */
		std::vector< std::wstring > wideLabels;
		wideLabels.reserve(m_buttons.size());

		for ( const auto & label : m_buttons )
		{
			wideLabels.push_back(toWideString(label));
		}

		/* Create button array for TaskDialogIndirect.
		 * Button IDs start at 100 to avoid conflicts with common button IDs. */
		constexpr int ButtonIdBase = 100;
		std::vector< TASKDIALOG_BUTTON > buttons;
		buttons.reserve(m_buttons.size());

		for ( size_t i = 0; i < wideLabels.size(); ++i )
		{
			TASKDIALOG_BUTTON btn{};
			btn.nButtonID = ButtonIdBase + static_cast< int >(i);
			btn.pszButtonText = wideLabels[i].c_str();
			buttons.push_back(btn);
		}

		/* Configure the task dialog. */
		TASKDIALOGCONFIG config{};
		config.cbSize = sizeof(TASKDIALOGCONFIG);
		config.hwndParent = window != nullptr ? window->getWin32Window() : nullptr;
		config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
		config.pszWindowTitle = wideTitle.c_str();
		config.pszMainInstruction = wideMessage.c_str();
		config.pButtons = buttons.data();
		config.cButtons = static_cast< UINT >(buttons.size());
		config.nDefaultButton = ButtonIdBase; /* First button is default. */
		config.pszMainIcon = getTaskDialogIcon(m_messageType);

		/* Show the dialog. */
		int clickedButtonId = 0;
		HRESULT hr = TaskDialogIndirect(&config, &clickedButtonId, nullptr, nullptr);

		if ( FAILED(hr) )
		{
			m_clickedIndex = -1;

			return false;
		}

		/* Convert button ID back to 0-based index. */
		if ( clickedButtonId >= ButtonIdBase && clickedButtonId < ButtonIdBase + static_cast< int >(m_buttons.size()) )
		{
			m_clickedIndex = clickedButtonId - ButtonIdBase;
		}
		else
		{
			/* Dialog was cancelled (closed via X or Escape). */
			m_clickedIndex = -1;
		}

		return true;
	}
}

#endif
