/*
 * src/PlatformSpecific/Desktop/Dialog/Message.mac.mm
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

#include "Message.hpp"

/* NOTE: Written for ARC (no retain / release anywhere): the build passes -fobjc-arc to every engine .mm (triad 14). */
#if !__has_feature(objc_arc)
#error "This file must be compiled with ARC (-fobjc-arc)."
#endif

/* Local inclusions for the string conversions. */
#include "PlatformSpecific/StringConversion.mac.hpp"

/* Third-party inclusions. */
#import <AppKit/AppKit.h>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	Message::execute (Window & /*window*/, bool /*parentToWindow*/) noexcept
	{
        @autoreleasepool
        {
            NSAlert * alert = [[NSAlert alloc] init];

            switch ( m_messageType )
            {
#ifdef __MAC_10_12
                case MessageType::Info :
                case MessageType::Question :
                    [alert setAlertStyle:NSAlertStyleInformational];
                    break;

                case MessageType::Warning :
                    [alert setAlertStyle:NSAlertStyleWarning];
                    break;

                case MessageType::Error :
                    [alert setAlertStyle:NSAlertStyleCritical];
                    break;
#else
                case MessageType::Info :
                case MessageType::Question :
                    [alert setAlertStyle:NSInformationalAlertStyle];
                    break;

                case MessageType::Warning :
                    [alert setAlertStyle:NSWarningAlertStyle];
                    break;

                case MessageType::Error :
                    [alert setAlertStyle:NSCriticalAlertStyle];
                    break;
#endif
                default:
                    break;
            }

            switch ( m_buttonLayout )
            {
                case ButtonLayout::OK :
                    [alert addButtonWithTitle:@"OK"];
                    break;

                case ButtonLayout::OKCancel :
                    [alert addButtonWithTitle:@"OK"];
                    [alert addButtonWithTitle:@"Cancel"];
                    break;

                case ButtonLayout::YesNo :
                    [alert addButtonWithTitle:@"Yes"];
                    [alert addButtonWithTitle:@"No"];
                    break;

                case ButtonLayout::Quit :
                    [alert addButtonWithTitle:@"Quit"];
                    break;

                default:
                    break;
            }

            /* NOTE: The second button (No / Cancel) answers Return: the first one is NSAlert's default otherwise, and a
             * stray key press answered Yes. The button order (and so the 1000 / 1001 answers) is unchanged. */
            if ( (m_buttonLayout == ButtonLayout::YesNo || m_buttonLayout == ButtonLayout::OKCancel) && (m_defaultAnswer == Answer::No || m_defaultAnswer == Answer::Cancel) && alert.buttons.count == 2 )
            {
                [[alert.buttons objectAtIndex:0] setKeyEquivalent:@""];
                [[alert.buttons objectAtIndex:1] setKeyEquivalent:@"\r"];
            }

            NSString * messageString = toNSString(m_message);
            [alert setMessageText:messageString];

            [alert.window setLevel:CGShieldingWindowLevel()];
            NSInteger button = [alert runModal];

            switch ( m_buttonLayout )
            {
                case ButtonLayout::OK :
                    m_userAnswer = Answer::OK;
                    break;

                case ButtonLayout::OKCancel :
                    if ( button == 1000 )
                    {
                        m_userAnswer = Answer::OK;
                    }
                    else
                    {
                        m_userAnswer = Answer::Cancel;
                    }
                    break;

                case ButtonLayout::YesNo :
                    if ( button == 1000 )
                    {
                        m_userAnswer = Answer::Yes;
                    }
                    else
                    {
                        m_userAnswer = Answer::No;
                    }
                    break;

                case ButtonLayout::Quit:
                    m_userAnswer = Answer::Cancel;
                    break;

                default:
                    break;
            }
        }  // end of autorelease pool.

	    return true;
	}
}
