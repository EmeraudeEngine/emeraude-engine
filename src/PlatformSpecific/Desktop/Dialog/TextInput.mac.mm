/*
 * src/PlatformSpecific/Desktop/Dialog/TextInput.mac.mm
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

/* Third-party inclusions. */
#import <AppKit/AppKit.h>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	TextInput::execute (Window & /*window*/, bool /*parentToWindow*/) noexcept
	{
		@autoreleasepool
		{
			[NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

			NSAlert * alert = [[NSAlert alloc] init];
			[alert setAlertStyle:NSAlertStyleInformational];
			[alert addButtonWithTitle:@"OK"];
			[alert addButtonWithTitle:@"Cancel"];

			NSString * messageString = [NSString stringWithUTF8String:m_message.c_str()];
			[alert setMessageText:messageString];

			NSString * defaultString = [NSString stringWithUTF8String:m_defaultText.c_str()];

			switch ( m_inputMode )
			{
				case InputMode::Password:
				{
					NSSecureTextField * input = [[NSSecureTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
					[input setStringValue:defaultString];
					[alert setAccessoryView:input];
					[alert.window setLevel:CGShieldingWindowLevel()];
					[alert.window setInitialFirstResponder:input];

					NSInteger button = [alert runModal];

					if ( button == NSAlertFirstButtonReturn )
					{
						m_text = [[input stringValue] UTF8String];
					}
					else
					{
						m_canceled = true;
					}
					break;
				}

				case InputMode::MultiLine:
				{
					NSScrollView * scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 400, 200)];
					[scrollView setHasVerticalScroller:YES];
					[scrollView setBorderType:NSBezelBorder];

					NSTextView * textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 394, 200)];
					[textView setMinSize:NSMakeSize(0, 200)];
					[textView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
					[textView setVerticallyResizable:YES];
					[textView setHorizontallyResizable:NO];
					[textView setAutoresizingMask:NSViewWidthSizable];
					[[textView textContainer] setWidthTracksTextView:YES];
					[textView setString:defaultString];
					[textView setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];

					[scrollView setDocumentView:textView];
					[alert setAccessoryView:scrollView];
					[alert.window setLevel:CGShieldingWindowLevel()];
					[alert.window setInitialFirstResponder:textView];

					NSInteger button = [alert runModal];

					if ( button == NSAlertFirstButtonReturn )
					{
						m_text = [[textView string] UTF8String];
					}
					else
					{
						m_canceled = true;
					}
					break;
				}

				case InputMode::SingleLine:
				default:
				{
					NSTextField * input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
					[input setStringValue:defaultString];
					[alert setAccessoryView:input];
					[alert.window setLevel:CGShieldingWindowLevel()];
					[alert.window setInitialFirstResponder:input];

					NSInteger button = [alert runModal];

					if ( button == NSAlertFirstButtonReturn )
					{
						m_text = [[input stringValue] UTF8String];
					}
					else
					{
						m_canceled = true;
					}
					break;
				}
			}
		}  // end of autorelease pool.

		return true;
	}
}
