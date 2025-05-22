/*
 * src/PlatformSpecific/Desktop/Commands.mac.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "Commands.hpp"

#if IS_MACOS

/* STL inclusions. */
#include <cstdlib>
#include <sstream>

/* Third-party inclusions. */
#import <AppKit/AppKit.h>
#include "reproc++/run.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific::Desktop
{
	constexpr auto TracerTag{"Commands"};

	bool
	runDesktopApplication (const std::string & argument) noexcept
	{
		if ( argument.empty() )
		{
			Tracer::error(TracerTag, "No argument to open with desktop terminal.");

			return false;
		}

		/*const std::array< const char *, 3 > args{"open", argument.data(), nullptr};

		const auto [exitCode, errorCode] = reproc::run(args.data());

		if ( exitCode != 0 )
		{
			TraceError{TracerTag} << "Failed to run a subprocess : " << errorCode.message();

			return false;
		}*/

		std::stringstream commandStream;
		commandStream << "open \"" << argument << "\"";

		system(commandStream.str().c_str());

		return true;
	}

	void
	flashTaskbarIcon (bool state) noexcept
	{
		if ( !state )
		{
			return;
		}

		@autoreleasepool
		{
			if ( state )
			{
				[NSApp requestUserAttention:NSCriticalRequest];
			}
			else
			{
				[NSApp requestUserAttention:NSInformationalRequest];
			}
		}
	}

	void
	setTaskbarIconProgression (float /*progress*/, ProgressMode /*mode*/) noexcept
	{

	}
}

#endif
