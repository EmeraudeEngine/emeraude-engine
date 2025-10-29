/*
 * src/Input/Manager.mac.mm
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

#include "Manager.hpp"

/* Third-party inclusions. */
#import <Cocoa/Cocoa.h>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace EmEn::Input
{
	static id g_magnificationEventMonitor = nil;

	void
	Manager::installMacOSGestureHandlers (GLFWwindow * window) noexcept
	{
		/* Install local event monitor for magnification (pinch-to-zoom) gestures. */
		g_magnificationEventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
			handler:^NSEvent * (NSEvent * event)
			{
				if ( s_instance == nullptr )
				{
					return event;
				}

				/* Get the magnification delta (positive = zoom in, negative = zoom out). */
				const auto magnification = [event magnification];

				/* Convert magnification to scroll offset.
				 * Magnification is typically in range [-1, 1], multiply to get reasonable scroll values. */
				const auto yOffset = magnification * 50.0;  /* Scale factor to match scroll sensitivity. */

				/* Get cursor position */
				NSWindow * nsWindow = glfwGetCocoaWindow(window);
				NSPoint mouseLocation = [nsWindow mouseLocationOutsideOfEventStream];
				NSRect contentRect = [[nsWindow contentView] frame];

				/* Convert to GLFW coordinates (origin at top-left) */
				const auto position = getPointerLocation(window);

				/* Trigger scroll callback with CTRL modifier to simulate pinch-to-zoom */
				const auto xOffsetF = 0.0F;
				const auto yOffsetF = static_cast< float >(yOffset);
				const auto modifiers = GLFW_MOD_CONTROL;  // Mark as Ctrl to indicate zoom gesture

				/* Dispatch to scroll listeners with CTRL modifier. */
				if ( s_instance->m_moveEventsTracking != nullptr )
				{
					s_instance->m_moveEventsTracking->onMouseWheel(
						position[0], position[1], xOffsetF, yOffsetF, modifiers
					);
				}
				else
				{
					for ( const auto & listener : s_instance->m_pointerListeners )
					{
						if ( !listener->isListeningPointer() )
						{
							continue;
						}

						const auto eventProcessed = listener->onMouseWheel(
							position[0], position[1], xOffsetF, yOffsetF, modifiers
						);

						if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
						{
							break;
						}
					}
				}

				/* Return event to allow normal processing. */
				return event;
			}
		];
	}

	void
	Manager::removeMacOSGestureHandlers () noexcept
	{
		if ( g_magnificationEventMonitor != nil )
		{
			[NSEvent removeMonitor:g_magnificationEventMonitor];
			g_magnificationEventMonitor = nil;
		}
	}
}
