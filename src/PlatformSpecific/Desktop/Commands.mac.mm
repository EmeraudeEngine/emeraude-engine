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
	runDesktopApplication (const std::string & executable, const std::string & argument) noexcept
	{
		if ( executable.empty() )
		{
			Tracer::error(TracerTag, "No executable to run!");

			return false;
		}

		std::vector< const char * > args;
		args.reserve(5);
		args.push_back("open");
		args.push_back("-a");
		args.push_back(executable.data());
		if ( !argument.empty() )
		{
			args.push_back(argument.data());
		}
		args.push_back(nullptr);

		const auto [exitCode, errorCode] = reproc::run(args.data());

		if ( exitCode != 0 )
		{
			TraceError{TracerTag} << "Failed to run a subprocess : " << errorCode.message();

			return false;
		}

		return true;
	}

	bool
	runDefaultDesktopApplication (const std::string & argument) noexcept
	{
		if ( argument.empty() )
		{
			Tracer::error(TracerTag, "No argument to open with desktop terminal.");

			return false;
		}

		std::stringstream commandStream;
		commandStream << "open \"" << argument << "\"";

		system(commandStream.str().c_str());

		return true;
	}

	void
	flashTaskbarIcon (const Window & window, bool state) noexcept
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
	setTaskbarIconProgression (const Window & /*window*/, float progress, ProgressMode /*mode*/) noexcept
	{
		@autoreleasepool
		{
			NSDockTile * dockTile = [NSApp dockTile];

			/* Handle progress disable (negative value). */
			if ( progress < 0.0F )
			{
				/* Remove the progress indicator. */
				[dockTile setContentView:nil];
				[dockTile display];

				return;
			}

			/* Get or create the content view. */
			NSView * contentView = [dockTile contentView];
			NSProgressIndicator * progressIndicator = nil;

			if ( contentView == nil )
			{
				/* Create a new image view with the app icon as background. */
				NSImageView * imageView = [[NSImageView alloc] init];
				[imageView setImage:[NSApp applicationIconImage]];

				/* Create a progress indicator. */
				progressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0, 0, dockTile.size.width, 15)];
				[progressIndicator setStyle:NSProgressIndicatorStyleBar];
				[progressIndicator setIndeterminate:NO];
				[progressIndicator setMinValue:0.0];
				[progressIndicator setMaxValue:1.0];

				/* Add the progress indicator to the image view. */
				[imageView addSubview:progressIndicator];

				/* Set the content view. */
				[dockTile setContentView:imageView];
			}
			else
			{
				/* Find the existing progress indicator. */
				for ( NSView * subview in [contentView subviews] )
				{
					if ( [subview isKindOfClass:[NSProgressIndicator class]] )
					{
						progressIndicator = (NSProgressIndicator *)subview;

						break;
					}
				}
			}

			/* Update the progress value. */
			if ( progressIndicator != nil )
			{
				[progressIndicator setDoubleValue:static_cast< double >(progress)];
			}

			/* Refresh the dock tile. */
			[dockTile display];
		}
	}
}

#endif
