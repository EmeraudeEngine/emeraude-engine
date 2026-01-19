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

/* Third-party inclusions. */
#import <AppKit/AppKit.h>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	Message::execute (Window * /*window*/) noexcept
	{
        @autoreleasepool
        {
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

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

            NSString * messageString = [NSString stringWithUTF8String:m_message.c_str()];
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
