/*
 * src/PlatformSpecific/Desktop/Notification.mac.mm
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

#include "Notification.hpp"

/* NOTE: Written for ARC (no retain / release anywhere): the build passes -fobjc-arc to every engine .mm (triad 14). */
#if !__has_feature(objc_arc)
#error "This file must be compiled with ARC (-fobjc-arc)."
#endif

/* Local inclusions for the string conversions. */
#include "PlatformSpecific/StringConversion.mac.hpp"

/* macOS inclusions. */
#import <Foundation/Foundation.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace EmEn::PlatformSpecific::Desktop
{
	bool
	Notification::show () noexcept
	{
		@autoreleasepool
		{
			/*
			 * Use NSUserNotificationCenter for native macOS notifications.
			 * This API is deprecated since macOS 10.14 but still functional,
			 * and doesn't require explicit notification permissions unlike
			 * UNUserNotificationCenter.
			 */
			NSUserNotificationCenter * center = [NSUserNotificationCenter defaultUserNotificationCenter];

			/* NOTE: There is no notification center outside an application bundle: nothing was delivered while true
			 * was returned. */
			if ( center == nil )
			{
				return false;
			}

			NSUserNotification * notification = [[NSUserNotification alloc] init];

			notification.title = toNSString(m_title);
			notification.informativeText = toNSString(m_message);
			notification.soundName = NSUserNotificationDefaultSoundName;

			/* Deliver the notification. */
			[center deliverNotification:notification];

			return true;
		}
	}
}

#pragma clang diagnostic pop
