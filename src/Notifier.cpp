/*
 * src/Notifier.cpp
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

#include "Notifier.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Graphics/FontResource.hpp"
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;

	bool
	Notifier::onInitialize () noexcept
	{
		if ( !m_overlayManager.usable() )
		{
			Tracer::error(ClassId, "The overlay manager is not usable to build the notifier !");

			return false;
		}

		this->observe(&m_overlayManager);

		m_screen = m_overlayManager.createScreen(ScreenName, false, false);

		if ( m_screen == nullptr )
		{
			Tracer::error(ClassId, "Unable to create a screen !");

			return false;
		}

		m_surface = m_screen->createSurface< Overlay::Surface >("Notifier", Space2D::AARectangle{0.0F, 0.9F, 1.0F, 0.1F}, 0.0F);

		if ( m_surface == nullptr )
		{
			Tracer::error(ClassId, "Unable to create a surface !");

			return false;
		}

		m_surface->activePixmap().fill(Transparent);

		{
			//m_font = m_resourceManager.container< Graphics::FontResource >()->getResource("old", false);
			m_font = m_resourceManager.container< Graphics::FontResource >()->getDefaultResource();

			m_processor.setPixmap(m_surface->activePixmap());
			m_processor.setFont(m_font->font(), 16U);
			m_processor.setFontColor(White);
		}

		/* NOTE: Create a timer to update the notification lifetime. */
		m_timerID = this->createTimer([&] (Time::TimerID /*timerID*/)
		{
			if ( m_notifications.empty() )
			{
				return false;
			}

			auto notificationIt = m_notifications.begin();

			while ( notificationIt != m_notifications.end() )
			{
				if ( notificationIt->second <= 0 )
				{
					notificationIt = m_notifications.erase(notificationIt);
				}
				else
				{
					notificationIt->second -= 500;

					++notificationIt;
				}
			}

			if ( m_notifications.empty() )
			{
				m_surface->hide();
			}
			else
			{
				this->renderNotifications();

				m_surface->show();
			}

			/* Do not stop the timer. */
			return false;
		}, 500, false, true);

		return m_overlayManager.enableScreen("NotifierScreen");
	}

	bool
	Notifier::onTerminate () noexcept
	{
		this->destroyTimer(m_timerID);

		m_font.reset();

		m_surface.reset();
		m_screen.reset();

		this->forget(&m_overlayManager);

		return true;
	}

	void
	Notifier::renderNotifications () noexcept
	{
		/* [VULKAN-CPU-SYNC] Lock the front framebuffer during write operations. */
		const std::lock_guard< std::mutex > lock{m_surface->activeBufferMutex()};

		/* NOTE: Skip rendering if the surface is being resized (pixmap dimensions may be invalid). */
		if ( !m_surface->isVideoMemorySizeValid() )
		{
			return;
		}

		auto & pixmap = m_surface->activePixmap();

		/* NOTE: Skip rendering if the pixmap has no valid dimensions (during resize transition). */
		if ( pixmap.width() == 0 || pixmap.height() == 0 )
		{
			return;
		}

		/* NOTE: Update the processor pixmap reference in case of resize. */
		m_processor.setPixmap(pixmap);

		if ( pixmap.fill(m_clearColor) )
		{
			if ( !m_notifications.empty() )
			{
				std::stringstream buffer;

				{
					const std::lock_guard< std::mutex > lockNotifications{m_notificationAccess};

					for ( const auto & [message, delay] : std::ranges::reverse_view(m_notifications))
					{
						buffer << message << '\n';
					}
				}

				m_processor.write(buffer.str());
			}

			m_surface->setVideoMemoryOutdated();
		}
	}

	void
	Notifier::clearDisplay () const noexcept
	{
		/* [VULKAN-CPU-SYNC] Lock the front framebuffer during write operations. */
		const std::lock_guard< std::mutex > lock{m_surface->activeBufferMutex()};

		if ( m_surface->activePixmap().fill(m_clearColor) )
		{
			m_surface->setVideoMemoryOutdated();
		}
	}

	void
	Notifier::clear () noexcept
	{
		/* NOTE: Removes all notifications. */
		{
			const std::lock_guard< std::mutex > lock{m_notificationAccess};

			m_notifications.clear();
		}

		/* NOTE: Wipe remaining notifications visible on the pixmap. */
		this->clearDisplay();
	}

	bool
	Notifier::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(Overlay::Manager::getClassUID()) )
		{
			if ( notificationCode == Overlay::Manager::OverlayResized )
			{
				this->renderNotifications();
			}

			return true;
		}

		return false;
	}
}
