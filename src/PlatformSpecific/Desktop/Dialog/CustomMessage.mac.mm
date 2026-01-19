/*
 * src/PlatformSpecific/Desktop/Dialog/CustomMessage.mac.mm
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

/* Third-party inclusions. */
#import <AppKit/AppKit.h>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	CustomMessage::execute (Window * /*window*/) noexcept
	{
		if ( m_buttons.empty() )
		{
			m_clickedIndex = -1;

			return false;
		}

		@autoreleasepool
		{
			[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

			NSAlert * alert = [[NSAlert alloc] init];

			/* Set alert style based on message type. */
			switch ( m_messageType )
			{
#ifdef __MAC_10_12
				case MessageType::Info:
				case MessageType::Question:
					[alert setAlertStyle:NSAlertStyleInformational];
					break;

				case MessageType::Warning:
					[alert setAlertStyle:NSAlertStyleWarning];
					break;

				case MessageType::Error:
					[alert setAlertStyle:NSAlertStyleCritical];
					break;
#else
				case MessageType::Info:
				case MessageType::Question:
					[alert setAlertStyle:NSInformationalAlertStyle];
					break;

				case MessageType::Warning:
					[alert setAlertStyle:NSWarningAlertStyle];
					break;

				case MessageType::Error:
					[alert setAlertStyle:NSCriticalAlertStyle];
					break;
#endif
				default:
					break;
			}

			/* Add custom buttons.
			 * NSAlert returns 1000, 1001, 1002... for buttons in the order they were added.
			 * The first button added is the default (highlighted). */
			for ( const auto & label : m_buttons )
			{
				NSString * buttonTitle = [NSString stringWithUTF8String:label.c_str()];
				[alert addButtonWithTitle:buttonTitle];
			}

			/* Set message text. */
			NSString * messageString = [NSString stringWithUTF8String:m_message.c_str()];
			[alert setMessageText:messageString];

			/* Set title as informative text. */
			NSString * titleString = [NSString stringWithUTF8String:this->title().c_str()];
			[alert setInformativeText:titleString];

			[alert.window setLevel:CGShieldingWindowLevel()];
			NSInteger button = [alert runModal];

			/* Convert NSAlert button code to 0-based index.
			 * NSAlertFirstButtonReturn = 1000, second = 1001, etc. */
			m_clickedIndex = static_cast< int >(button - 1000);

			/* Validate index is within bounds. */
			if ( m_clickedIndex < 0 || m_clickedIndex >= static_cast< int >(m_buttons.size()) )
			{
				m_clickedIndex = -1;
			}
		}

		return true;
	}
}
