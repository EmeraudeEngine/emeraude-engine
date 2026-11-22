/*
 * src/PlatformSpecific/ProcessActivity.mac.mm
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

#include "Helpers.hpp"

/* Third-party inclusions. */
#import <Foundation/Foundation.h>

namespace EmEn::PlatformSpecific
{
	ProcessActivity::~ProcessActivity ()
	{
		this->end();
	}

	bool
	ProcessActivity::begin () noexcept
	{
		if ( m_token != nullptr )
		{
			return true;
		}

		@autoreleasepool
		{
			/* NOTE: "User initiated" keeps App Nap away, "latency critical" asks for the highest timer precision (the
			 * logic thread's deadline); "allowing idle system sleep" leaves the machine's own sleep to the user's
			 * settings. */
			const NSActivityOptions options = NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical;

			const id< NSObject > activity = [[NSProcessInfo processInfo] beginActivityWithOptions:options reason:@"Emeraude engine: real-time simulation and rendering"];

			if ( activity == nil )
			{
				return false;
			}

			/* NOTE: ARC hands its reference over to this object, which gives it back in end(). */
			m_token = (__bridge_retained void *)activity;
		}

		return true;
	}

	void
	ProcessActivity::end () noexcept
	{
		if ( m_token == nullptr )
		{
			return;
		}

		@autoreleasepool
		{
			/* NOTE: ARC takes the reference back and drops it at the end of this scope. */
			const id< NSObject > activity = (__bridge_transfer id< NSObject >)m_token;

			m_token = nullptr;

			[[NSProcessInfo processInfo] endActivity:activity];
		}
	}
}
