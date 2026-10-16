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

/* NOTE: Written for ARC (no retain / release anywhere): the build passes -fobjc-arc to every engine .mm (triad 14). */
#if !__has_feature(objc_arc)
#error "This file must be compiled with ARC (-fobjc-arc)."
#endif

/* Third-party inclusions. */
#import <Cocoa/Cocoa.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

namespace EmEn::Input
{
	void
	Manager::installMacOSGestureHandlers (GLFWwindow * window) noexcept
	{
		/* Install local event monitor for magnification (pinch-to-zoom) gestures. */
		/* NOTE: The block captures this manager: removeMacOSGestureHandlers() runs before it ends (unlinkWindowCallbacks(),
		 * and the destructor). A second install replaces this manager's first monitor; another manager keeps its own. */
		this->removeMacOSGestureHandlers();

		const id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
			handler:^NSEvent * (NSEvent * event)
			{
				/* Get the magnification delta (positive = zoom in, negative = zoom out). */
				const auto magnification = [event magnification];

				/* Convert magnification to scroll offset.
				 * Magnification is typically in range [-1, 1], multiply to get reasonable scroll values. */
				const auto yOffset = magnification * 50.0;  /* Scale factor to match scroll sensitivity. */

				/* Convert to GLFW coordinates (origin at top-left) */
				const auto position = this->pointerLocation(window);

				/* Dispatch as a scroll with CTRL, to simulate pinch-to-zoom. */
				this->dispatchMouseWheel(position, 0.0F, static_cast< float >(yOffset), GLFW_MOD_CONTROL);

				/* Return event to allow normal processing. */
				return event;
			}
		];

		/* NOTE: Owned by this manager until removeMacOSGestureHandlers() hands it back to ARC. */
		m_magnificationEventMonitor = (__bridge_retained void *)monitor;
	}

	void
	Manager::removeMacOSGestureHandlers () noexcept
	{
		if ( m_magnificationEventMonitor != nullptr )
		{
			const id monitor = (__bridge_transfer id)m_magnificationEventMonitor;

			m_magnificationEventMonitor = nullptr;

			[NSEvent removeMonitor:monitor];
		}
	}
}
