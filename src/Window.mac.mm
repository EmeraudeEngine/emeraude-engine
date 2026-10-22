/*
 * src/Window.mac.cpp
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

/* Emeraude-Engine configuration. */

#include "Window.hpp"

/* NOTE: Written for ARC (no retain / release anywhere): the build passes -fobjc-arc to every engine .mm (triad 14). */
#if !__has_feature(objc_arc)
#error "This file must be compiled with ARC (-fobjc-arc)."
#endif

/* STL inclusions. */
#include <functional>
#include <utility>

/* Third-party inclusions. */
#import <AppKit/AppKit.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3native.h"

/* Local inclusions. */
#include "Vulkan/Utility.hpp"
#include "Vulkan/Instance.hpp"
#include "Tracer.hpp"

/**
 * @brief Receives the "quit" Apple Event in place of AppKit's default handler.
 * @details The default one called [NSApp terminate:], GLFW's delegate answered NSTerminateCancel and the engine then
 * stopped on its own: the sender got "User canceled" (-128: an AppleScript `quit`, and the end of the user session,
 * which macOS may take for a refusal — macOS peer, 2026-10-11). Here the event is SUSPENDED, the engine starts a
 * system stop (its JavaScript handshake included), and the reply ("done") is sent when the window is released at
 * shutdown.
 */
@interface EmEnQuitAppleEventReceiver : NSObject

- (void) handleQuitEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)reply;

@end

@implementation EmEnQuitAppleEventReceiver
{
	/* NOTE: Read and written by QuitAppleEventHandler (this file only), on the main thread. */
	@public
	std::function< void (bool sessionEnds) > onQuit;
	NSAppleEventManagerSuspensionID suspensionID;
	BOOL suspended;
}

- (void) handleQuitEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)reply
{
	(void) reply;

	/* NOTE: A second quit while the first one waits gets its reply at once: the stop is already under way. */
	if ( suspended )
	{
		return;
	}

	/* NOTE: The end of the session (log out, restart, shut down) carries its reason (kAEQuitReason); a "Quit" from the
	 * Dock or an AppleScript `quit` carries none: that one is the user's own quit, stopped like Cmd+Q (owner decision
	 * 2026-10-22: a system stop's 3 s deadline cut the application's cleanup short after a long "save?" dialog). */
	const BOOL sessionEnds = [event attributeDescriptorForKeyword:kAEQuitReason] != nil;

	suspensionID = [[NSAppleEventManager sharedAppleEventManager] suspendCurrentAppleEvent];
	suspended = YES;

	if ( onQuit )
	{
		onQuit(sessionEnds == YES);
	}
}

@end

namespace EmEn
{
	using namespace Vulkan;

	/** @brief Owns the receiver of the "quit" Apple Event; restores the default handling and answers on destruction. */
	class QuitAppleEventHandler final
	{
		public:

			explicit
			QuitAppleEventHandler (std::function< void (bool sessionEnds) > onQuit) noexcept
				: m_receiver{[[EmEnQuitAppleEventReceiver alloc] init]}
			{
				m_receiver->onQuit = std::move(onQuit);
				m_receiver->suspended = NO;

				[[NSAppleEventManager sharedAppleEventManager]
					setEventHandler:m_receiver
					andSelector:@selector(handleQuitEvent:withReplyEvent:)
					forEventClass:kCoreEventClass
					andEventID:kAEQuitApplication];
			}

			QuitAppleEventHandler (const QuitAppleEventHandler &) = delete;
			QuitAppleEventHandler (QuitAppleEventHandler &&) = delete;
			QuitAppleEventHandler & operator= (const QuitAppleEventHandler &) = delete;
			QuitAppleEventHandler & operator= (QuitAppleEventHandler &&) = delete;

			~QuitAppleEventHandler ()
			{
				/* NOTE: No callback after this point: the owner (Window) is being released. */
				m_receiver->onQuit = nullptr;

				[[NSAppleEventManager sharedAppleEventManager] removeEventHandlerForEventClass:kCoreEventClass andEventID:kAEQuitApplication];

				/* NOTE: The engine is stopping: the sender (AppleScript, the session end) is told the quit is done. */
				if ( m_receiver->suspended )
				{
					[[NSAppleEventManager sharedAppleEventManager] resumeWithSuspensionID:m_receiver->suspensionID];

					m_receiver->suspended = NO;
				}
			}

			/**
			 * @brief Answers the suspended "quit" Apple Event as refused ("user canceled", -128): the application runs on.
			 * @return void
			 */
			void
			cancel () noexcept
			{
				if ( !m_receiver->suspended )
				{
					return;
				}

				NSAppleEventDescriptor * reply = [[NSAppleEventManager sharedAppleEventManager] replyAppleEventForSuspensionID:m_receiver->suspensionID];

				[reply setParamDescriptor:[NSAppleEventDescriptor descriptorWithInt32:userCanceledErr] forKeyword:keyErrorNumber];

				[[NSAppleEventManager sharedAppleEventManager] resumeWithSuspensionID:m_receiver->suspensionID];

				m_receiver->suspended = NO;
			}

		private:

			EmEnQuitAppleEventReceiver * m_receiver;
	};

	void
	QuitAppleEventHandlerDeleter::operator() (QuitAppleEventHandler * handler) const noexcept
	{
		delete handler;
	}

	void
	Window::disableTitleBar () noexcept
	{

	}

	void
	Window::applyTitleBarTheme () noexcept
	{
		// Nothing to do
	}

	bool
	Window::isDisplayConnectionLost () const noexcept
	{
		/* NOTE: Wayland only (Window.linux.cpp). */
		return false;
	}

	void
	Window::cancelQuitRequest () noexcept
	{
		if ( m_quitAppleEventHandler != nullptr )
		{
			m_quitAppleEventHandler->cancel();
		}
	}

	bool
	Window::initializeNativeWindow () noexcept
	{
		/* NOTE: The callback captures this window, which owns the handler: releaseNativeWindow() destroys it first. */
		m_quitAppleEventHandler.reset(new QuitAppleEventHandler{[this] (bool sessionEnds) {
			if ( sessionEnds )
			{
				Tracer::info(ClassId, "macOS asks the application to quit: the session ends (Apple Event).");

				this->notify(OSRequestsToEndSession);
			}
			else
			{
				Tracer::info(ClassId, "macOS asks the application to quit: Dock or AppleScript (Apple Event).");

				this->notify(OSRequestsToTerminate);
			}
		}});

		return true;
	}

	void
	Window::releaseNativeWindow () noexcept
	{
		/* NOTE: Answers a suspended "quit" Apple Event (the engine is stopping) and restores the default handling. */
		m_quitAppleEventHandler.reset();
	}

	void
	Window::drainDisplayConnection () const noexcept
	{
		/* NOTE: The Cocoa event queue does not drop its window when it is not pumped for a while. */
	}

	id
	Window::getCocoaWindow () const noexcept
	{
		return glfwGetCocoaWindow(m_handle.get());
	}

	id
	Window::getCocoaView () const noexcept
	{
		return glfwGetCocoaView(m_handle.get());
	}
}
