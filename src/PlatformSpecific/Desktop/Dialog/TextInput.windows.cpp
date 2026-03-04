/*
 * src/PlatformSpecific/Desktop/Dialog/TextInput.windows.cpp
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

#include "TextInput.hpp"

/* STL inclusions. */
#include <cstring>
#include <vector>

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	namespace
	{
		/* Control IDs for the runtime dialog. */
		static constexpr WORD IDC_MESSAGE_LABEL = 100;
		static constexpr WORD IDC_TEXT_EDIT = 101;
		static constexpr WORD IDC_TEXT_MULTILINE = 102;

		/* Dialog dimensions (in dialog units). */
		static constexpr WORD DLG_WIDTH = 260;
		static constexpr WORD DLG_HEIGHT_SINGLE = 90;
		static constexpr WORD DLG_HEIGHT_MULTI = 170;
		static constexpr WORD MARGIN = 7;
		static constexpr WORD LABEL_HEIGHT = 16;
		static constexpr WORD EDIT_HEIGHT = 14;
		static constexpr WORD EDIT_HEIGHT_MULTI = 80;
		static constexpr WORD BUTTON_WIDTH = 50;
		static constexpr WORD BUTTON_HEIGHT = 14;

		struct DialogData
		{
			const std::wstring * message;
			const std::wstring * defaultText;
			std::wstring * result;
			InputMode inputMode;
			bool canceled;
		};

		/**
		 * @brief Appends a DLGITEMTEMPLATE to the dialog template buffer.
		 */
		void
		appendDlgItem (std::vector< BYTE > & buffer, DWORD style, WORD x, WORD y, WORD cx, WORD cy, WORD id, WORD classAtom, const std::wstring & text)
		{
			/* Align to DWORD boundary. */
			while ( buffer.size() % 4 != 0 )
			{
				buffer.push_back(0);
			}

			DLGITEMTEMPLATE item{};
			item.style = style | WS_CHILD | WS_VISIBLE;
			item.dwExtendedStyle = 0;
			item.x = x;
			item.y = y;
			item.cx = cx;
			item.cy = cy;
			item.id = id;

			const auto * raw = reinterpret_cast< const BYTE * >(&item);
			buffer.insert(buffer.end(), raw, raw + sizeof(DLGITEMTEMPLATE));

			/* Class atom (0xFFFF followed by atom). */
			const WORD classPrefix = 0xFFFF;
			const auto * classRaw = reinterpret_cast< const BYTE * >(&classPrefix);
			buffer.insert(buffer.end(), classRaw, classRaw + sizeof(WORD));

			const auto * atomRaw = reinterpret_cast< const BYTE * >(&classAtom);
			buffer.insert(buffer.end(), atomRaw, atomRaw + sizeof(WORD));

			/* Title string. */
			const auto * textRaw = reinterpret_cast< const BYTE * >(text.c_str());
			buffer.insert(buffer.end(), textRaw, textRaw + (text.size() + 1) * sizeof(wchar_t));

			/* Extra data (0 bytes). */
			const WORD extraCount = 0;
			const auto * extraRaw = reinterpret_cast< const BYTE * >(&extraCount);
			buffer.insert(buffer.end(), extraRaw, extraRaw + sizeof(WORD));
		}

		INT_PTR CALLBACK
		textInputDialogProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch ( msg )
			{
				case WM_INITDIALOG:
				{
					auto * data = reinterpret_cast< DialogData * >(lParam);
					SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast< LONG_PTR >(data));

					/* Set message label text. */
					SetDlgItemTextW(hDlg, IDC_MESSAGE_LABEL, data->message->c_str());

					/* Set default text in the edit control. */
					WORD editId = (data->inputMode == InputMode::MultiLine) ? IDC_TEXT_MULTILINE : IDC_TEXT_EDIT;
					SetDlgItemTextW(hDlg, editId, data->defaultText->c_str());

					/* Focus the edit control. */
					HWND hEdit = GetDlgItem(hDlg, editId);
					SetFocus(hEdit);

					return FALSE;
				}

				case WM_COMMAND:
				{
					auto * data = reinterpret_cast< DialogData * >(GetWindowLongPtr(hDlg, GWLP_USERDATA));

					switch ( LOWORD(wParam) )
					{
						case IDOK:
						{
							WORD editId = (data->inputMode == InputMode::MultiLine) ? IDC_TEXT_MULTILINE : IDC_TEXT_EDIT;
							int length = GetWindowTextLengthW(GetDlgItem(hDlg, editId));
							std::wstring text(static_cast< size_t >(length), L'\0');

							GetDlgItemTextW(hDlg, editId, text.data(), length + 1);
							*data->result = std::move(text);
							data->canceled = false;

							EndDialog(hDlg, IDOK);
							return TRUE;
						}

						case IDCANCEL:
							data->canceled = true;
							EndDialog(hDlg, IDCANCEL);
							return TRUE;
					}
					break;
				}

				case WM_CLOSE:
				{
					auto * data = reinterpret_cast< DialogData * >(GetWindowLongPtr(hDlg, GWLP_USERDATA));
					data->canceled = true;
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
				}
			}

			return FALSE;
		}

		/**
		 * @brief Builds a runtime dialog template for text input.
		 */
		std::vector< BYTE >
		buildDialogTemplate (const std::wstring & title, InputMode inputMode)
		{
			const bool isMultiLine = (inputMode == InputMode::MultiLine);
			const WORD dlgHeight = isMultiLine ? DLG_HEIGHT_MULTI : DLG_HEIGHT_SINGLE;
			const WORD contentWidth = DLG_WIDTH - MARGIN * 2;

			std::vector< BYTE > buffer;
			buffer.reserve(512);

			/* DLGTEMPLATE header. */
			DLGTEMPLATE dlg{};
			dlg.style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
			dlg.dwExtendedStyle = 0;
			dlg.cdit = 4; /* label + edit + OK + Cancel */
			dlg.x = 0;
			dlg.y = 0;
			dlg.cx = DLG_WIDTH;
			dlg.cy = dlgHeight;

			const auto * raw = reinterpret_cast< const BYTE * >(&dlg);
			buffer.insert(buffer.end(), raw, raw + sizeof(DLGTEMPLATE));

			/* Menu (none). */
			const WORD zero = 0;
			const auto * zeroRaw = reinterpret_cast< const BYTE * >(&zero);
			buffer.insert(buffer.end(), zeroRaw, zeroRaw + sizeof(WORD));

			/* Class (default). */
			buffer.insert(buffer.end(), zeroRaw, zeroRaw + sizeof(WORD));

			/* Title. */
			const auto * titleRaw = reinterpret_cast< const BYTE * >(title.c_str());
			buffer.insert(buffer.end(), titleRaw, titleRaw + (title.size() + 1) * sizeof(wchar_t));

			/* Static text label (class atom 0x0082). */
			appendDlgItem(buffer, SS_LEFT, MARGIN, MARGIN, contentWidth, LABEL_HEIGHT, IDC_MESSAGE_LABEL, 0x0082, L"");

			/* Edit control (class atom 0x0081). */
			WORD editY = MARGIN + LABEL_HEIGHT + 4;

			if ( isMultiLine )
			{
				DWORD editStyle = ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | WS_BORDER | WS_TABSTOP;
				appendDlgItem(buffer, editStyle, MARGIN, editY, contentWidth, EDIT_HEIGHT_MULTI, IDC_TEXT_MULTILINE, 0x0081, L"");
			}
			else
			{
				DWORD editStyle = ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP;

				if ( inputMode == InputMode::Password )
				{
					editStyle |= ES_PASSWORD;
				}

				appendDlgItem(buffer, editStyle, MARGIN, editY, contentWidth, EDIT_HEIGHT, IDC_TEXT_EDIT, 0x0081, L"");
			}

			/* Button Y position. */
			WORD buttonY = dlgHeight - MARGIN - BUTTON_HEIGHT;

			/* OK button (class atom 0x0080). */
			appendDlgItem(buffer, BS_DEFPUSHBUTTON | WS_TABSTOP, DLG_WIDTH - MARGIN - BUTTON_WIDTH * 2 - 4, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT, IDOK, 0x0080, L"OK");

			/* Cancel button. */
			appendDlgItem(buffer, BS_PUSHBUTTON | WS_TABSTOP, DLG_WIDTH - MARGIN - BUTTON_WIDTH, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT, IDCANCEL, 0x0080, L"Cancel");

			return buffer;
		}
	}

	bool
	TextInput::execute (Window * window) noexcept
	{
		const std::wstring wsTitle = convertUTF8ToWide(this->title());
		const std::wstring wsMessage = convertUTF8ToWide(m_message);
		const std::wstring wsDefault = convertUTF8ToWide(m_defaultText);
		std::wstring wsResult;

		DialogData data{};
		data.message = &wsMessage;
		data.defaultText = &wsDefault;
		data.result = &wsResult;
		data.inputMode = m_inputMode;
		data.canceled = true;

		auto templateBuffer = buildDialogTemplate(wsTitle, m_inputMode);

		HWND parentWindow = window != nullptr ? window->getWin32Window() : nullptr;

		INT_PTR result = DialogBoxIndirectParamW(
			GetModuleHandle(nullptr),
			reinterpret_cast< LPCDLGTEMPLATEW >(templateBuffer.data()),
			parentWindow,
			textInputDialogProc,
			reinterpret_cast< LPARAM >(&data)
		);

		if ( result == -1 || data.canceled )
		{
			m_canceled = true;
		}
		else
		{
			m_text = convertWideToUTF8(wsResult);
		}

		return result != -1;
	}
}
